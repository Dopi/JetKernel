/*
 * zfcp device driver
 *
 * Fibre Channel related functions for the zfcp device driver.
 *
 * Copyright IBM Corporation 2008, 2009
 */

#define KMSG_COMPONENT "zfcp"
#define pr_fmt(fmt) KMSG_COMPONENT ": " fmt

#include "zfcp_ext.h"

enum rscn_address_format {
	RSCN_PORT_ADDRESS	= 0x0,
	RSCN_AREA_ADDRESS	= 0x1,
	RSCN_DOMAIN_ADDRESS	= 0x2,
	RSCN_FABRIC_ADDRESS	= 0x3,
};

static u32 rscn_range_mask[] = {
	[RSCN_PORT_ADDRESS]		= 0xFFFFFF,
	[RSCN_AREA_ADDRESS]		= 0xFFFF00,
	[RSCN_DOMAIN_ADDRESS]		= 0xFF0000,
	[RSCN_FABRIC_ADDRESS]		= 0x000000,
};

struct ct_iu_gpn_ft_req {
	struct ct_hdr header;
	u8 flags;
	u8 domain_id_scope;
	u8 area_id_scope;
	u8 fc4_type;
} __attribute__ ((packed));

struct gpn_ft_resp_acc {
	u8 control;
	u8 port_id[3];
	u8 reserved[4];
	u64 wwpn;
} __attribute__ ((packed));

#define ZFCP_CT_SIZE_ONE_PAGE	(PAGE_SIZE - sizeof(struct ct_hdr))
#define ZFCP_GPN_FT_ENTRIES	(ZFCP_CT_SIZE_ONE_PAGE \
					/ sizeof(struct gpn_ft_resp_acc))
#define ZFCP_GPN_FT_BUFFERS 4
#define ZFCP_GPN_FT_MAX_SIZE (ZFCP_GPN_FT_BUFFERS * PAGE_SIZE \
				- sizeof(struct ct_hdr))
#define ZFCP_GPN_FT_MAX_ENTRIES ZFCP_GPN_FT_BUFFERS * (ZFCP_GPN_FT_ENTRIES + 1)

struct ct_iu_gpn_ft_resp {
	struct ct_hdr header;
	struct gpn_ft_resp_acc accept[ZFCP_GPN_FT_ENTRIES];
} __attribute__ ((packed));

struct zfcp_gpn_ft {
	struct zfcp_send_ct ct;
	struct scatterlist sg_req;
	struct scatterlist sg_resp[ZFCP_GPN_FT_BUFFERS];
};

struct zfcp_fc_ns_handler_data {
	struct completion done;
	void (*handler)(unsigned long);
	unsigned long handler_data;
};

static int zfcp_wka_port_get(struct zfcp_wka_port *wka_port)
{
	if (mutex_lock_interruptible(&wka_port->mutex))
		return -ERESTARTSYS;

	if (wka_port->status == ZFCP_WKA_PORT_OFFLINE ||
	    wka_port->status == ZFCP_WKA_PORT_CLOSING) {
		wka_port->status = ZFCP_WKA_PORT_OPENING;
		if (zfcp_fsf_open_wka_port(wka_port))
			wka_port->status = ZFCP_WKA_PORT_OFFLINE;
	}

	mutex_unlock(&wka_port->mutex);

	wait_event(wka_port->completion_wq,
		   wka_port->status == ZFCP_WKA_PORT_ONLINE ||
		   wka_port->status == ZFCP_WKA_PORT_OFFLINE);

	if (wka_port->status == ZFCP_WKA_PORT_ONLINE) {
		atomic_inc(&wka_port->refcount);
		return 0;
	}
	return -EIO;
}

static void zfcp_wka_port_offline(struct work_struct *work)
{
	struct delayed_work *dw = to_delayed_work(work);
	struct zfcp_wka_port *wka_port =
			container_of(dw, struct zfcp_wka_port, work);

	mutex_lock(&wka_port->mutex);
	if ((atomic_read(&wka_port->refcount) != 0) ||
	    (wka_port->status != ZFCP_WKA_PORT_ONLINE))
		goto out;

	wka_port->status = ZFCP_WKA_PORT_CLOSING;
	if (zfcp_fsf_close_wka_port(wka_port)) {
		wka_port->status = ZFCP_WKA_PORT_OFFLINE;
		wake_up(&wka_port->completion_wq);
	}
out:
	mutex_unlock(&wka_port->mutex);
}

static void zfcp_wka_port_put(struct zfcp_wka_port *wka_port)
{
	if (atomic_dec_return(&wka_port->refcount) != 0)
		return;
	/* wait 10 milliseconds, other reqs might pop in */
	schedule_delayed_work(&wka_port->work, HZ / 100);
}

static void zfcp_fc_wka_port_init(struct zfcp_wka_port *wka_port, u32 d_id,
				  struct zfcp_adapter *adapter)
{
	init_waitqueue_head(&wka_port->completion_wq);

	wka_port->adapter = adapter;
	wka_port->d_id = d_id;

	wka_port->status = ZFCP_WKA_PORT_OFFLINE;
	atomic_set(&wka_port->refcount, 0);
	mutex_init(&wka_port->mutex);
	INIT_DELAYED_WORK(&wka_port->work, zfcp_wka_port_offline);
}

void zfcp_fc_wka_port_force_offline(struct zfcp_wka_port *wka)
{
	cancel_delayed_work_sync(&wka->work);
	mutex_lock(&wka->mutex);
	wka->status = ZFCP_WKA_PORT_OFFLINE;
	mutex_unlock(&wka->mutex);
}

void zfcp_fc_wka_ports_init(struct zfcp_adapter *adapter)
{
	struct zfcp_wka_ports *gs = adapter->gs;

	zfcp_fc_wka_port_init(&gs->ms, FC_FID_MGMT_SERV, adapter);
	zfcp_fc_wka_port_init(&gs->ts, FC_FID_TIME_SERV, adapter);
	zfcp_fc_wka_port_init(&gs->ds, FC_FID_DIR_SERV, adapter);
	zfcp_fc_wka_port_init(&gs->as, FC_FID_ALIASES, adapter);
	zfcp_fc_wka_port_init(&gs->ks, FC_FID_SEC_KEY, adapter);
}

static void _zfcp_fc_incoming_rscn(struct zfcp_fsf_req *fsf_req, u32 range,
				   struct fcp_rscn_element *elem)
{
	unsigned long flags;
	struct zfcp_port *port;

	read_lock_irqsave(&zfcp_data.config_lock, flags);
	list_for_each_entry(port, &fsf_req->adapter->port_list_head, list) {
		if ((port->d_id & range) == (elem->nport_did & range))
			zfcp_test_link(port);
		if (!port->d_id)
			zfcp_erp_port_reopen(port,
					     ZFCP_STATUS_COMMON_ERP_FAILED,
					     "fcrscn1", NULL);
	}

	read_unlock_irqrestore(&zfcp_data.config_lock, flags);
}

static void zfcp_fc_incoming_rscn(struct zfcp_fsf_req *fsf_req)
{
	struct fsf_status_read_buffer *status_buffer = (void *)fsf_req->data;
	struct fcp_rscn_head *fcp_rscn_head;
	struct fcp_rscn_element *fcp_rscn_element;
	u16 i;
	u16 no_entries;
	u32 range_mask;

	fcp_rscn_head = (struct fcp_rscn_head *) status_buffer->payload.data;
	fcp_rscn_element = (struct fcp_rscn_element *) fcp_rscn_head;

	/* see FC-FS */
	no_entries = fcp_rscn_head->payload_len /
			sizeof(struct fcp_rscn_element);

	for (i = 1; i < no_entries; i++) {
		/* skip head and start with 1st element */
		fcp_rscn_element++;
		range_mask = rscn_range_mask[fcp_rscn_element->addr_format];
		_zfcp_fc_incoming_rscn(fsf_req, range_mask, fcp_rscn_element);
	}
	schedule_work(&fsf_req->adapter->scan_work);
}

static void zfcp_fc_incoming_wwpn(struct zfcp_fsf_req *req, u64 wwpn)
{
	struct zfcp_adapter *adapter = req->adapter;
	struct zfcp_port *port;
	unsigned long flags;

	read_lock_irqsave(&zfcp_data.config_lock, flags);
	list_for_each_entry(port, &adapter->port_list_head, list)
		if (port->wwpn == wwpn)
			break;
	read_unlock_irqrestore(&zfcp_data.config_lock, flags);

	if (port && (port->wwpn == wwpn))
		zfcp_erp_port_forced_reopen(port, 0, "fciwwp1", req);
}

static void zfcp_fc_incoming_plogi(struct zfcp_fsf_req *req)
{
	struct fsf_status_read_buffer *status_buffer =
		(struct fsf_status_read_buffer *)req->data;
	struct fsf_plogi *els_plogi =
		(struct fsf_plogi *) status_buffer->payload.data;

	zfcp_fc_incoming_wwpn(req, els_plogi->serv_param.wwpn);
}

static void zfcp_fc_incoming_logo(struct zfcp_fsf_req *req)
{
	struct fsf_status_read_buffer *status_buffer =
		(struct fsf_status_read_buffer *)req->data;
	struct fcp_logo *els_logo =
		(struct fcp_logo *) status_buffer->payload.data;

	zfcp_fc_incoming_wwpn(req, els_logo->nport_wwpn);
}

/**
 * zfcp_fc_incoming_els - handle incoming ELS
 * @fsf_req - request which contains incoming ELS
 */
void zfcp_fc_incoming_els(struct zfcp_fsf_req *fsf_req)
{
	struct fsf_status_read_buffer *status_buffer =
		(struct fsf_status_read_buffer *) fsf_req->data;
	unsigned int els_type = status_buffer->payload.data[0];

	zfcp_san_dbf_event_incoming_els(fsf_req);
	if (els_type == LS_PLOGI)
		zfcp_fc_incoming_plogi(fsf_req);
	else if (els_type == LS_LOGO)
		zfcp_fc_incoming_logo(fsf_req);
	else if (els_type == LS_RSCN)
		zfcp_fc_incoming_rscn(fsf_req);
}

static void zfcp_fc_ns_handler(unsigned long data)
{
	struct zfcp_fc_ns_handler_data *compl_rec =
			(struct zfcp_fc_ns_handler_data *) data;

	if (compl_rec->handler)
		compl_rec->handler(compl_rec->handler_data);

	complete(&compl_rec->done);
}

static void zfcp_fc_ns_gid_pn_eval(unsigned long data)
{
	struct zfcp_gid_pn_data *gid_pn = (struct zfcp_gid_pn_data *) data;
	struct zfcp_send_ct *ct = &gid_pn->ct;
	struct ct_iu_gid_pn_req *ct_iu_req = sg_virt(ct->req);
	struct ct_iu_gid_pn_resp *ct_iu_resp = sg_virt(ct->resp);
	struct zfcp_port *port = gid_pn->port;

	if (ct->status)
		return;
	if (ct_iu_resp->header.cmd_rsp_code != ZFCP_CT_ACCEPT)
		return;

	/* paranoia */
	if (ct_iu_req->wwpn != port->wwpn)
		return;
	/* looks like a valid d_id */
	port->d_id = ct_iu_resp->d_id & ZFCP_DID_MASK;
}

int static zfcp_fc_ns_gid_pn_request(struct zfcp_erp_action *erp_action,
				     struct zfcp_gid_pn_data *gid_pn)
{
	struct zfcp_adapter *adapter = erp_action->adapter;
	struct zfcp_fc_ns_handler_data compl_rec;
	int ret;

	/* setup parameters for send generic command */
	gid_pn->port = erp_action->port;
	gid_pn->ct.wka_port = &adapter->gs->ds;
	gid_pn->ct.handler = zfcp_fc_ns_handler;
	gid_pn->ct.handler_data = (unsigned long) &compl_rec;
	gid_pn->ct.timeout = ZFCP_NS_GID_PN_TIMEOUT;
	gid_pn->ct.req = &gid_pn->req;
	gid_pn->ct.resp = &gid_pn->resp;
	sg_init_one(&gid_pn->req, &gid_pn->ct_iu_req,
		    sizeof(struct ct_iu_gid_pn_req));
	sg_init_one(&gid_pn->resp, &gid_pn->ct_iu_resp,
		    sizeof(struct ct_iu_gid_pn_resp));

	/* setup nameserver request */
	gid_pn->ct_iu_req.header.revision = ZFCP_CT_REVISION;
	gid_pn->ct_iu_req.header.gs_type = ZFCP_CT_DIRECTORY_SERVICE;
	gid_pn->ct_iu_req.header.gs_subtype = ZFCP_CT_NAME_SERVER;
	gid_pn->ct_iu_req.header.options = ZFCP_CT_SYNCHRONOUS;
	gid_pn->ct_iu_req.header.cmd_rsp_code = ZFCP_CT_GID_PN;
	gid_pn->ct_iu_req.header.max_res_size = ZFCP_CT_SIZE_ONE_PAGE / 4;
	gid_pn->ct_iu_req.wwpn = erp_action->port->wwpn;

	init_completion(&compl_rec.done);
	compl_rec.handler = zfcp_fc_ns_gid_pn_eval;
	compl_rec.handler_data = (unsigned long) gid_pn;
	ret = zfcp_fsf_send_ct(&gid_pn->ct, adapter->pool.fsf_req_erp,
			       erp_action);
	if (!ret)
		wait_for_completion(&compl_rec.done);
	return ret;
}

/**
 * zfcp_fc_ns_gid_pn_request - initiate GID_PN nameserver request
 * @erp_action: pointer to zfcp_erp_action where GID_PN request is needed
 * return: -ENOMEM on error, 0 otherwise
 */
int zfcp_fc_ns_gid_pn(struct zfcp_erp_action *erp_action)
{
	int ret;
	struct zfcp_gid_pn_data *gid_pn;
	struct zfcp_adapter *adapter = erp_action->adapter;

	gid_pn = mempool_alloc(adapter->pool.data_gid_pn, GFP_ATOMIC);
	if (!gid_pn)
		return -ENOMEM;

	memset(gid_pn, 0, sizeof(*gid_pn));

	ret = zfcp_wka_port_get(&adapter->gs->ds);
	if (ret)
		goto out;

	ret = zfcp_fc_ns_gid_pn_request(erp_action, gid_pn);

	zfcp_wka_port_put(&adapter->gs->ds);
out:
	mempool_free(gid_pn, adapter->pool.data_gid_pn);
	return ret;
}

/**
 * zfcp_fc_plogi_evaluate - evaluate PLOGI playload
 * @port: zfcp_port structure
 * @plogi: plogi payload
 *
 * Evaluate PLOGI playload and copy important fields into zfcp_port structure
 */
void zfcp_fc_plogi_evaluate(struct zfcp_port *port, struct fsf_plogi *plogi)
{
	port->maxframe_size = plogi->serv_param.common_serv_param[7] |
		((plogi->serv_param.common_serv_param[6] & 0x0F) << 8);
	if (plogi->serv_param.class1_serv_param[0] & 0x80)
		port->supported_classes |= FC_COS_CLASS1;
	if (plogi->serv_param.class2_serv_param[0] & 0x80)
		port->supported_classes |= FC_COS_CLASS2;
	if (plogi->serv_param.class3_serv_param[0] & 0x80)
		port->supported_classes |= FC_COS_CLASS3;
	if (plogi->serv_param.class4_serv_param[0] & 0x80)
		port->supported_classes |= FC_COS_CLASS4;
}

struct zfcp_els_adisc {
	struct zfcp_send_els els;
	struct scatterlist req;
	struct scatterlist resp;
	struct zfcp_ls_adisc ls_adisc;
	struct zfcp_ls_adisc ls_adisc_acc;
};

static void zfcp_fc_adisc_handler(unsigned long data)
{
	struct zfcp_els_adisc *adisc = (struct zfcp_els_adisc *) data;
	struct zfcp_port *port = adisc->els.port;
	struct zfcp_ls_adisc *ls_adisc = &adisc->ls_adisc_acc;

	if (adisc->els.status) {
		/* request rejected or timed out */
		zfcp_erp_port_forced_reopen(port, ZFCP_STATUS_COMMON_ERP_FAILED,
					    "fcadh_1", NULL);
		goto out;
	}

	if (!port->wwnn)
		port->wwnn = ls_adisc->wwnn;

	if ((port->wwpn != ls_adisc->wwpn) ||
	    !(atomic_read(&port->status) & ZFCP_STATUS_COMMON_OPEN)) {
		zfcp_erp_port_reopen(port, ZFCP_STATUS_COMMON_ERP_FAILED,
				     "fcadh_2", NULL);
		goto out;
	}

	/* port is good, unblock rport without going through erp */
	zfcp_scsi_schedule_rport_register(port);
 out:
	zfcp_port_put(port);
	kfree(adisc);
}

static int zfcp_fc_adisc(struct zfcp_port *port)
{
	struct zfcp_els_adisc *adisc;
	struct zfcp_adapter *adapter = port->adapter;

	adisc = kzalloc(sizeof(struct zfcp_els_adisc), GFP_ATOMIC);
	if (!adisc)
		return -ENOMEM;

	adisc->els.req = &adisc->req;
	adisc->els.resp = &adisc->resp;
	sg_init_one(adisc->els.req, &adisc->ls_adisc,
		    sizeof(struct zfcp_ls_adisc));
	sg_init_one(adisc->els.resp, &adisc->ls_adisc_acc,
		    sizeof(struct zfcp_ls_adisc));

	adisc->els.adapter = adapter;
	adisc->els.port = port;
	adisc->els.d_id = port->d_id;
	adisc->els.handler = zfcp_fc_adisc_handler;
	adisc->els.handler_data = (unsigned long) adisc;
	adisc->els.ls_code = adisc->ls_adisc.code = ZFCP_LS_ADISC;

	/* acc. to FC-FS, hard_nport_id in ADISC should not be set for ports
	   without FC-AL-2 capability, so we don't set it */
	adisc->ls_adisc.wwpn = fc_host_port_name(adapter->scsi_host);
	adisc->ls_adisc.wwnn = fc_host_node_name(adapter->scsi_host);
	adisc->ls_adisc.nport_id = fc_host_port_id(adapter->scsi_host);

	return zfcp_fsf_send_els(&adisc->els);
}

void zfcp_fc_link_test_work(struct work_struct *work)
{
	struct zfcp_port *port =
		container_of(work, struct zfcp_port, test_link_work);
	int retval;

	zfcp_port_get(port);
	port->rport_task = RPORT_DEL;
	zfcp_scsi_rport_work(&port->rport_work);

	retval = zfcp_fc_adisc(port);
	if (retval == 0)
		return;

	/* send of ADISC was not possible */
	zfcp_erp_port_forced_reopen(port, 0, "fcltwk1", NULL);

	zfcp_port_put(port);
}

/**
 * zfcp_test_link - lightweight link test procedure
 * @port: port to be tested
 *
 * Test status of a link to a remote port using the ELS command ADISC.
 * If there is a problem with the remote port, error recovery steps
 * will be triggered.
 */
void zfcp_test_link(struct zfcp_port *port)
{
	zfcp_port_get(port);
	if (!queue_work(zfcp_data.work_queue, &port->test_link_work))
		zfcp_port_put(port);
}

static void zfcp_free_sg_env(struct zfcp_gpn_ft *gpn_ft, int buf_num)
{
	struct scatterlist *sg = &gpn_ft->sg_req;

	kfree(sg_virt(sg)); /* free request buffer */
	zfcp_sg_free_table(gpn_ft->sg_resp, buf_num);

	kfree(gpn_ft);
}

static struct zfcp_gpn_ft *zfcp_alloc_sg_env(int buf_num)
{
	struct zfcp_gpn_ft *gpn_ft;
	struct ct_iu_gpn_ft_req *req;

	gpn_ft = kzalloc(sizeof(*gpn_ft), GFP_KERNEL);
	if (!gpn_ft)
		return NULL;

	req = kzalloc(sizeof(struct ct_iu_gpn_ft_req), GFP_KERNEL);
	if (!req) {
		kfree(gpn_ft);
		gpn_ft = NULL;
		goto out;
	}
	sg_init_one(&gpn_ft->sg_req, req, sizeof(*req));

	if (zfcp_sg_setup_table(gpn_ft->sg_resp, buf_num)) {
		zfcp_free_sg_env(gpn_ft, buf_num);
		gpn_ft = NULL;
	}
out:
	return gpn_ft;
}


static int zfcp_scan_issue_gpn_ft(struct zfcp_gpn_ft *gpn_ft,
				  struct zfcp_adapter *adapter,
				  int max_bytes)
{
	struct zfcp_send_ct *ct = &gpn_ft->ct;
	struct ct_iu_gpn_ft_req *req = sg_virt(&gpn_ft->sg_req);
	struct zfcp_fc_ns_handler_data compl_rec;
	int ret;

	/* prepare CT IU for GPN_FT */
	req->header.revision = ZFCP_CT_REVISION;
	req->header.gs_type = ZFCP_CT_DIRECTORY_SERVICE;
	req->header.gs_subtype = ZFCP_CT_NAME_SERVER;
	req->header.options = ZFCP_CT_SYNCHRONOUS;
	req->header.cmd_rsp_code = ZFCP_CT_GPN_FT;
	req->header.max_res_size = max_bytes / 4;
	req->flags = 0;
	req->domain_id_scope = 0;
	req->area_id_scope = 0;
	req->fc4_type = ZFCP_CT_SCSI_FCP;

	/* prepare zfcp_send_ct */
	ct->wka_port = &adapter->gs->ds;
	ct->handler = zfcp_fc_ns_handler;
	ct->handler_data = (unsigned long)&compl_rec;
	ct->timeout = 10;
	ct->req = &gpn_ft->sg_req;
	ct->resp = gpn_ft->sg_resp;

	init_completion(&compl_rec.done);
	compl_rec.handler = NULL;
	ret = zfcp_fsf_send_ct(ct, NULL, NULL);
	if (!ret)
		wait_for_completion(&compl_rec.done);
	return ret;
}

static void zfcp_validate_port(struct zfcp_port *port)
{
	struct zfcp_adapter *adapter = port->adapter;

	if (!(atomic_read(&port->status) & ZFCP_STATUS_COMMON_NOESC))
		return;

	atomic_clear_mask(ZFCP_STATUS_COMMON_NOESC, &port->status);

	if ((port->supported_classes != 0) ||
	    !list_empty(&port->unit_list_head)) {
		zfcp_port_put(port);
		return;
	}
	zfcp_erp_port_shutdown(port, 0, "fcpval1", NULL);
	zfcp_erp_wait(adapter);
	zfcp_port_put(port);
	zfcp_port_dequeue(port);
}

static int zfcp_scan_eval_gpn_ft(struct zfcp_gpn_ft *gpn_ft, int max_entries)
{
	struct zfcp_send_ct *ct = &gpn_ft->ct;
	struct scatterlist *sg = gpn_ft->sg_resp;
	struct ct_hdr *hdr = sg_virt(sg);
	struct gpn_ft_resp_acc *acc = sg_virt(sg);
	struct zfcp_adapter *adapter = ct->wka_port->adapter;
	struct zfcp_port *port, *tmp;
	u32 d_id;
	int ret = 0, x, last = 0;

	if (ct->status)
		return -EIO;

	if (hdr->cmd_rsp_code != ZFCP_CT_ACCEPT) {
		if (hdr->reason_code == ZFCP_CT_UNABLE_TO_PERFORM_CMD)
			return -EAGAIN; /* might be a temporary condition */
		return -EIO;
	}

	if (hdr->max_res_size) {
		dev_warn(&adapter->ccw_device->dev,
			 "The name server reported %d words residual data\n",
			 hdr->max_res_size);
		return -E2BIG;
	}

	down(&zfcp_data.config_sema);

	/* first entry is the header */
	for (x = 1; x < max_entries && !last; x++) {
		if (x % (ZFCP_GPN_FT_ENTRIES + 1))
			acc++;
		else
			acc = sg_virt(++sg);

		last = acc->control & 0x80;
		d_id = acc->port_id[0] << 16 | acc->port_id[1] << 8 |
		       acc->port_id[2];

		/* don't attach ports with a well known address */
		if ((d_id & ZFCP_DID_WKA) == ZFCP_DID_WKA)
			continue;
		/* skip the adapter's port and known remote ports */
		if (acc->wwpn == fc_host_port_name(adapter->scsi_host))
			continue;
		port = zfcp_get_port_by_wwpn(adapter, acc->wwpn);
		if (port)
			continue;

		port = zfcp_port_enqueue(adapter, acc->wwpn,
					 ZFCP_STATUS_COMMON_NOESC, d_id);
		if (IS_ERR(port))
			ret = PTR_ERR(port);
		else
			zfcp_erp_port_reopen(port, 0, "fcegpf1", NULL);
	}

	zfcp_erp_wait(adapter);
	list_for_each_entry_safe(port, tmp, &adapter->port_list_head, list)
		zfcp_validate_port(port);
	up(&zfcp_data.config_sema);
	return ret;
}

/**
 * zfcp_scan_ports - scan remote ports and attach new ports
 * @adapter: pointer to struct zfcp_adapter
 */
int zfcp_scan_ports(struct zfcp_adapter *adapter)
{
	int ret, i;
	struct zfcp_gpn_ft *gpn_ft;
	int chain, max_entries, buf_num, max_bytes;

	chain = adapter->adapter_features & FSF_FEATURE_ELS_CT_CHAINED_SBALS;
	buf_num = chain ? ZFCP_GPN_FT_BUFFERS : 1;
	max_entries = chain ? ZFCP_GPN_FT_MAX_ENTRIES : ZFCP_GPN_FT_ENTRIES;
	max_bytes = chain ? ZFCP_GPN_FT_MAX_SIZE : ZFCP_CT_SIZE_ONE_PAGE;

	if (fc_host_port_type(adapter->scsi_host) != FC_PORTTYPE_NPORT &&
	    fc_host_port_type(adapter->scsi_host) != FC_PORTTYPE_NPIV)
		return 0;

	ret = zfcp_wka_port_get(&adapter->gs->ds);
	if (ret)
		return ret;

	gpn_ft = zfcp_alloc_sg_env(buf_num);
	if (!gpn_ft) {
		ret = -ENOMEM;
		goto out;
	}

	for (i = 0; i < 3; i++) {
		ret = zfcp_scan_issue_gpn_ft(gpn_ft, adapter, max_bytes);
		if (!ret) {
			ret = zfcp_scan_eval_gpn_ft(gpn_ft, max_entries);
			if (ret == -EAGAIN)
				ssleep(1);
			else
				break;
		}
	}
	zfcp_free_sg_env(gpn_ft, buf_num);
out:
	zfcp_wka_port_put(&adapter->gs->ds);
	return ret;
}


void _zfcp_scan_ports_later(struct work_struct *work)
{
	zfcp_scan_ports(container_of(work, struct zfcp_adapter, scan_work));
}

struct zfcp_els_fc_job {
	struct zfcp_send_els els;
	struct fc_bsg_job *job;
};

static void zfcp_fc_generic_els_handler(unsigned long data)
{
	struct zfcp_els_fc_job *els_fc_job = (struct zfcp_els_fc_job *) data;
	struct fc_bsg_job *job = els_fc_job->job;
	struct fc_bsg_reply *reply = job->reply;

	if (els_fc_job->els.status) {
		/* request rejected or timed out */
		reply->reply_data.ctels_reply.status = FC_CTELS_STATUS_REJECT;
		goto out;
	}

	reply->reply_data.ctels_reply.status = FC_CTELS_STATUS_OK;
	reply->reply_payload_rcv_len = job->reply_payload.payload_len;

out:
	job->state_flags = FC_RQST_STATE_DONE;
	job->job_done(job);
	kfree(els_fc_job);
}

int zfcp_fc_execute_els_fc_job(struct fc_bsg_job *job)
{
	struct zfcp_els_fc_job *els_fc_job;
	struct fc_rport *rport = job->rport;
	struct Scsi_Host *shost;
	struct zfcp_adapter *adapter;
	struct zfcp_port *port;
	u8 *port_did;

	shost = rport ? rport_to_shost(rport) : job->shost;
	adapter = (struct zfcp_adapter *)shost->hostdata[0];

	if (!(atomic_read(&adapter->status) & ZFCP_STATUS_COMMON_OPEN))
		return -EINVAL;

	els_fc_job = kzalloc(sizeof(struct zfcp_els_fc_job), GFP_KERNEL);
	if (!els_fc_job)
		return -ENOMEM;

	els_fc_job->els.adapter = adapter;
	if (rport) {
		read_lock_irq(&zfcp_data.config_lock);
		port = rport->dd_data;
		if (port)
			els_fc_job->els.d_id = port->d_id;
		read_unlock_irq(&zfcp_data.config_lock);
		if (!port) {
			kfree(els_fc_job);
			return -EINVAL;
		}
	} else {
		port_did = job->request->rqst_data.h_els.port_id;
		els_fc_job->els.d_id = (port_did[0] << 16) +
					(port_did[1] << 8) + port_did[2];
	}

	els_fc_job->els.req = job->request_payload.sg_list;
	els_fc_job->els.resp = job->reply_payload.sg_list;
	els_fc_job->els.handler = zfcp_fc_generic_els_handler;
	els_fc_job->els.handler_data = (unsigned long) els_fc_job;
	els_fc_job->job = job;

	return zfcp_fsf_send_els(&els_fc_job->els);
}

struct zfcp_ct_fc_job {
	struct zfcp_send_ct ct;
	struct fc_bsg_job *job;
};

static void zfcp_fc_generic_ct_handler(unsigned long data)
{
	struct zfcp_ct_fc_job *ct_fc_job = (struct zfcp_ct_fc_job *) data;
	struct fc_bsg_job *job = ct_fc_job->job;

	job->reply->reply_data.ctels_reply.status = ct_fc_job->ct.status ?
				FC_CTELS_STATUS_REJECT : FC_CTELS_STATUS_OK;
	job->reply->reply_payload_rcv_len = job->reply_payload.payload_len;
	job->state_flags = FC_RQST_STATE_DONE;
	job->job_done(job);

	zfcp_wka_port_put(ct_fc_job->ct.wka_port);

	kfree(ct_fc_job);
}

int zfcp_fc_execute_ct_fc_job(struct fc_bsg_job *job)
{
	int ret;
	u8 gs_type;
	struct fc_rport *rport = job->rport;
	struct Scsi_Host *shost;
	struct zfcp_adapter *adapter;
	struct zfcp_ct_fc_job *ct_fc_job;
	u32 preamble_word1;

	shost = rport ? rport_to_shost(rport) : job->shost;

	adapter = (struct zfcp_adapter *)shost->hostdata[0];
	if (!(atomic_read(&adapter->status) & ZFCP_STATUS_COMMON_OPEN))
		return -EINVAL;

	ct_fc_job = kzalloc(sizeof(struct zfcp_ct_fc_job), GFP_KERNEL);
	if (!ct_fc_job)
		return -ENOMEM;

	preamble_word1 = job->request->rqst_data.r_ct.preamble_word1;
	gs_type = (preamble_word1 & 0xff000000) >> 24;

	switch (gs_type) {
	case FC_FST_ALIAS:
		ct_fc_job->ct.wka_port = &adapter->gs->as;
		break;
	case FC_FST_MGMT:
		ct_fc_job->ct.wka_port = &adapter->gs->ms;
		break;
	case FC_FST_TIME:
		ct_fc_job->ct.wka_port = &adapter->gs->ts;
		break;
	case FC_FST_DIR:
		ct_fc_job->ct.wka_port = &adapter->gs->ds;
		break;
	default:
		kfree(ct_fc_job);
		return -EINVAL; /* no such service */
	}

	ret = zfcp_wka_port_get(ct_fc_job->ct.wka_port);
	if (ret) {
		kfree(ct_fc_job);
		return ret;
	}

	ct_fc_job->ct.req = job->request_payload.sg_list;
	ct_fc_job->ct.resp = job->reply_payload.sg_list;
	ct_fc_job->ct.timeout = ZFCP_FSF_REQUEST_TIMEOUT;
	ct_fc_job->ct.handler = zfcp_fc_generic_ct_handler;
	ct_fc_job->ct.handler_data = (unsigned long) ct_fc_job;
	ct_fc_job->ct.completion = NULL;
	ct_fc_job->job = job;

	ret = zfcp_fsf_send_ct(&ct_fc_job->ct, NULL, NULL);
	if (ret) {
		kfree(ct_fc_job);
		zfcp_wka_port_put(ct_fc_job->ct.wka_port);
	}
	return ret;
}
