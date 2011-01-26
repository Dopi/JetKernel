/* mfc/FrameBufMgr.c
 *
 * Copyright (c) 2008 Samsung Electronics
 *
 * Samsung S3C MFC driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "MfcMemory.h"
#include "FramBufMgr.h"
#include "MfcTypes.h"
#include "LogMsg.h"


// The size in bytes of the BUF_SEGMENT.
// The buffers are fragemented into the segment unit of this size.
#define BUF_SEGMENT_SIZE	1024


typedef struct
{
	unsigned char *pBaseAddr;
	int            idx_commit;
} SEGMENT_INFO;


typedef struct
{
	int index_base_seg;
	int num_segs;
} COMMIT_INFO;


static SEGMENT_INFO  *_p_segment_info = NULL;
static COMMIT_INFO   *_p_commit_info  = NULL;


static unsigned char *_pBufferBase  = NULL;
static int            _nBufferSize  = 0;
static int            _nNumSegs		= 0;


//
// int FramBufMgrInit(unsigned char *pBufBase, int nBufSize)
//
// Description
//		This function initializes the MfcFramBufMgr(Buffer Segment Manager)
// Parameters
//		pBufBase [IN]: pointer to the buffer which will be managed by this MfcFramBufMgr functions.
//		nBufSize [IN]: buffer size in bytes
// Return Value
//		1 : Success
//		0 : Fail
//
BOOL FramBufMgrInit(unsigned char *pBufBase, int nBufSize)
{
	int   i;

	// check parameters
	if (pBufBase == NULL || nBufSize == 0)
		return FALSE;


	// �̹� �ʱ�ȭ�� �Ǿ� �ְ�
	// (1) �ʱ�ȭ ����� �Է� �Ķ���� ���� ���ٸ�
	//     �Ǽ��� �� �Լ��� �� �� ȣ��� ���̹Ƿ� �׳� �ٷ� 1�� ����
	// (2) ���� �ʴٸ�, 
	//     Finalize ��Ų ��, �ٽ� �� �ʱ�ȭ �Ѵ�. 
	if ((_pBufferBase != NULL) && (_nBufferSize != 0)) {
		if ((pBufBase == _pBufferBase) && (nBufSize == _nBufferSize))
			return TRUE;

		FramBufMgrFinal();
	}


	_pBufferBase = pBufBase;
	_nBufferSize = nBufSize;
	_nNumSegs = nBufSize / BUF_SEGMENT_SIZE;

	_p_segment_info = (SEGMENT_INFO *) Mem_Alloc(_nNumSegs * sizeof(SEGMENT_INFO));
	for (i=0; i<_nNumSegs; i++) {
		_p_segment_info[i].pBaseAddr   = pBufBase  +  (i * BUF_SEGMENT_SIZE);
		_p_segment_info[i].idx_commit  = 0;
	}

	_p_commit_info  = (COMMIT_INFO *) Mem_Alloc(_nNumSegs * sizeof(COMMIT_INFO));
	for (i=0; i<_nNumSegs; i++) {
		_p_commit_info[i].index_base_seg  = -1;
		_p_commit_info[i].num_segs        = 0;
	}


	return TRUE;
}


//
// void FramBufMgrFinal()
//
// Description
//		This function finalizes the MfcFramBufMgr(Buffer Segment Manager)
// Parameters
//		None
// Return Value
//		None
//
void FramBufMgrFinal()
{
	if (_p_segment_info != NULL) {
		Mem_Free(_p_segment_info);
		_p_segment_info = NULL;
	}

	if (_p_commit_info != NULL) {
		Mem_Free(_p_commit_info);
		_p_commit_info = NULL;
	}


	_pBufferBase  = NULL;
	_nBufferSize  = 0;
	_nNumSegs       = 0;
}


//
// unsigned char *FramBufMgrCommit(int idx_commit, int commit_size)
//
// Description
//		This function requests the commit for commit_size buffer to be reserved.
// Parameters
//		idx_commit  [IN]: pointer to the buffer which will be managed by this MfcFramBufMgr functions.
//		commit_size [IN]: commit size in bytes
// Return Value
//		NULL : Failed to commit (Wrong parameters, commit_size too big, and so on.)
//		Otherwise it returns the pointer which was committed.
//
unsigned char *FramBufMgrCommit(int idx_commit, int commit_size)
{
	int  i, j;

	int  num_fram_buf_seg;		// �ʿ��� buffer�� ���� SEGMENT ���� 


	// �ʱ�ȭ ���� üũ 
	if (_p_segment_info == NULL || _p_commit_info == NULL) {
		return NULL;
	}

	// check parameters
	if (idx_commit < 0 || idx_commit >= _nNumSegs)
		return NULL;
	if (commit_size <= 0 || commit_size > _nBufferSize)
		return NULL;

	// COMMIT_INFO �迭���� idx_commit ��° ����
	// free���� �̹� commit �Ǿ����� Ȯ���Ѵ�. 
	if (_p_commit_info[idx_commit].index_base_seg != -1)
		return NULL;


	// �ʿ��� FRAM_BUF_SEGMENT ������ ���Ѵ�. 
	// Instance FRAM_BUF�� �ʿ��� ũ�Ⱑ FRAM_BUF_SEGMENT_SIZE ������ �����̶� �Ѿ��
	// FRAM_BUF_SEGMENT�� ��°�� �ϳ� �� �ʿ��ϰ� �ȴ�.
	if ((commit_size % BUF_SEGMENT_SIZE) == 0)
		num_fram_buf_seg = commit_size / BUF_SEGMENT_SIZE;
	else
		num_fram_buf_seg = (commit_size / BUF_SEGMENT_SIZE)  +  1;

	for (i=0; i<(_nNumSegs - num_fram_buf_seg); i++) {
		// SEGMENT �߿� commit �ȵ� ���� ���� ������ �� �˻� 
		if (_p_segment_info[i].idx_commit != 0)
			continue;

		for (j=0; j<num_fram_buf_seg; j++) {
			if (_p_segment_info[i+j].idx_commit != 0)
				break;
		}

		// j=0 ~ num_fram_buf_seg ���� commit�� SEGMENT�� ���ٸ� 
		// �� �κ��� commit�Ѵ�. 
		if (j == num_fram_buf_seg) {

			for (j=0; j<num_fram_buf_seg; j++) {
				_p_segment_info[i+j].idx_commit = 1;
			}

			_p_commit_info[idx_commit].index_base_seg  = i;
			_p_commit_info[idx_commit].num_segs        = num_fram_buf_seg;

			return _p_segment_info[i].pBaseAddr;
		}
		else
		{
			// instance buffer�� ���̿� ������� �߻�������, �Ǵٸ� instance�� ���� ���۸� �Ҵ���
			// ũ�Ⱑ ���� ������ �� ������� �ǳʶٰ�, �� ���۸� ã�´�. 
			i = i + j - 1;
		}
	}

	return NULL;
}


//
// void FramBufMgrFree(int idx_commit)
//
// Description
//		This function frees the committed region of buffer.
// Parameters
//		idx_commit  [IN]: pointer to the buffer which will be managed by this MfcFramBufMgr functions.
// Return Value
//		None
//
void FramBufMgrFree(int idx_commit)
{
	int  i;

	int  index_base_seg;		// �ش� commmit �κ��� base segment�� index
	int  num_fram_buf_seg;		// �ʿ��� buffer�� ���� SEGMENT�� ���� 


	// �ʱ�ȭ ���� üũ 
	if (_p_segment_info == NULL || _p_commit_info == NULL)
		return;

	// check parameters
	if (idx_commit < 0 || idx_commit >= _nNumSegs)
		return;

	// COMMIT_INFO �迭���� idx_commit ��° ���� 
	// free���� �̹� commit �Ǿ����� Ȯ���Ѵ� 
	if (_p_commit_info[idx_commit].index_base_seg == -1)
		return;


	index_base_seg    =  _p_commit_info[idx_commit].index_base_seg;
	num_fram_buf_seg  =  _p_commit_info[idx_commit].num_segs;

	for (i=0; i<num_fram_buf_seg; i++) {
		_p_segment_info[index_base_seg + i].idx_commit = 0;
	}


	_p_commit_info[idx_commit].index_base_seg  =  -1;
	_p_commit_info[idx_commit].num_segs        =  0;

}




//
// unsigned char *FramBufMgrGetBuf(int idx_commit)
//
// Description
//		This function obtains the committed buffer of 'idx_commit'.
// Parameters
//		idx_commit  [IN]: commit index of the buffer which will be obtained
// Return Value
//		NULL : Failed to get the indicated buffer (Wrong parameters, not committed, and so on.)
//		Otherwise it returns the pointer which was committed.
//
unsigned char *FramBufMgrGetBuf(int idx_commit)
{
	int index_base_seg;

	// �ʱ�ȭ ���� üũ 
	if (_p_segment_info == NULL || _p_commit_info == NULL)
		return NULL;

	// check parameters
	if (idx_commit < 0 || idx_commit >= _nNumSegs)
		return NULL;

	// COMMIT_INFO �迭���� idx_commit ��° ���� 
	// free���� �̹� commit �Ǿ����� Ȯ���Ѵ�. 
	if (_p_commit_info[idx_commit].index_base_seg == -1)
		return NULL;


	index_base_seg  =  _p_commit_info[idx_commit].index_base_seg;

	return _p_segment_info[index_base_seg].pBaseAddr;
}

//
// int FramBufMgrGetBufSize(int idx_commit)
//
// Description
//		This function obtains the size of the committed buffer of 'idx_commit'.
// Parameters
//		idx_commit  [IN]: commit index of the buffer which will be obtained
// Return Value
//		0 : Failed to get the size of indicated buffer (Wrong parameters, not committed, and so on.)
//		Otherwise it returns the size of the buffer.
//		Note that the size is multiples of the BUF_SEGMENT_SIZE.
//
int FramBufMgrGetBufSize(int idx_commit)
{
	// �ʱ�ȭ ���� üũ 
	if (_p_segment_info == NULL || _p_commit_info == NULL)
		return 0;

	// check parameters
	if (idx_commit < 0 || idx_commit >= _nNumSegs)
		return 0;

	// COMMIT_INFO �迭���� idx_commit ��° ���� 
	// free���� �̹� commit �Ǿ����� Ȯ���Ѵ�. 
	if (_p_commit_info[idx_commit].index_base_seg == -1)
		return 0;


	return (_p_commit_info[idx_commit].num_segs * BUF_SEGMENT_SIZE);
}


//
// void FramBufMgrPrintCommitInfo()
//
// Description
//		This function prints the commited information on the console screen.
// Parameters
//		None
// Return Value
//		None
//
void FramBufMgrPrintCommitInfo()
{
	int  i;

	// �ʱ�ȭ ���� üũ 
	if (_p_segment_info == NULL || _p_commit_info == NULL) {
		LOG_MSG(LOG_TRACE, "FramBufMgrPrintCommitInfo", "\n The FramBufMgr is not initialized.\n");
		return;
	}


	for (i=0; i<_nNumSegs; i++) {
		if (_p_commit_info[i].index_base_seg != -1)  {
			LOG_MSG(LOG_TRACE, "FramBufMgrPrintCommitInfo", "\n\nCOMMIT INDEX = [%03d], BASE_SEG_IDX = %d", i, _p_commit_info[i].index_base_seg);
			LOG_MSG(LOG_TRACE, "FramBufMgrPrintCommitInfo", "\nCOMMIT INDEX = [%03d], NUM OF SEGS  = %d", i, _p_commit_info[i].num_segs);
		}
	}
}
