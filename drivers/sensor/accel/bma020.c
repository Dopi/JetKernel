
#include "bma020.h"


bma020_t *p_bma020;				/**< pointer to BMA020 device structure  */

bma020acc_t cal_data;

int bma020_init(bma020_t *bma020) 
{
	int comres=0;
	unsigned char data;

	p_bma020 = bma020;															/* assign bma020 ptr */
	p_bma020->dev_addr = BMA020_I2C_ADDR;										/* preset SM380 I2C_addr */
	comres += p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, CHIP_ID__REG, &data, 1);	/* read Chip Id */
	
	p_bma020->chip_id = BMA020_GET_BITSLICE(data, CHIP_ID);			/* get bitslice */
		
	comres += p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, ML_VERSION__REG, &data, 1); /* read Version reg */
	p_bma020->ml_version = BMA020_GET_BITSLICE(data, ML_VERSION);	/* get ML Version */
	p_bma020->al_version = BMA020_GET_BITSLICE(data, AL_VERSION);	/* get AL Version */

	// make sure the default value of the interrupt enable register
	data = 0x00; // All interrupt disable
	p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, 0x0b, &data, 1 );

	return comres;

}

int bma020_soft_reset() 
{
	int comres;
	unsigned char data=0;
	
	if (p_bma020==0) 
		return E_BMA020_NULL_PTR;
	
	data = BMA020_SET_BITSLICE(data, SOFT_RESET, 1);
    comres = p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, SOFT_RESET__REG, &data,1); 
	
	return comres;
}

#if 0
int bma020_update_image() 
{
	int comres;
	unsigned char data=0;
	
	if (p_bma020==0) 
		return E_BMA020_NULL_PTR;
	
	data = BMA020_SET_BITSLICE(data, UPDATE_IMAGE, 1);
    comres = p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, UPDATE_IMAGE__REG, &data,1); 
	
	return comres;
}


int bma020_set_image (bma020regs_t *bma020Image) 
{
	int comres;
	unsigned char data;
	
	if (p_bma020==0)
		return E_BMA020_NULL_PTR;
    
	comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, EE_W__REG,&data, 1);
	data = BMA020_SET_BITSLICE(data, EE_W, BMA020_EE_W_ON);
	comres = p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, EE_W__REG, &data, 1);
	comres = p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, BMA020_IMAGE_BASE, (unsigned char*)bma020Image, BMA020_IMAGE_LEN);
	comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, EE_W__REG,&data, 1);
	data = BMA020_SET_BITSLICE(data, EE_W, BMA020_EE_W_OFF);
	comres = p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, EE_W__REG, &data, 1);
	
	return comres;
}



int bma020_get_image(bma020regs_t *bma020Image)
{
	int comres;
	unsigned char data;
	
	if (p_bma020==0)
		return E_BMA020_NULL_PTR;
    
	comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, EE_W__REG,&data, 1);
	data = BMA020_SET_BITSLICE(data, EE_W, BMA020_EE_W_ON);
	comres = p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, EE_W__REG, &data, 1);
	comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, BMA020_IMAGE_BASE, (unsigned char *)bma020Image, BMA020_IMAGE_LEN);
	data = BMA020_SET_BITSLICE(data, EE_W, BMA020_EE_W_OFF);
	comres = p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, EE_W__REG, &data, 1);
	
	return comres;
}

int bma020_get_offset(unsigned char xyz, unsigned short *offset) 
{
   int comres;
   unsigned char data;
   
   if (p_bma020==0)
   		return E_BMA020_NULL_PTR;
   
   comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, (OFFSET_X_LSB__REG+xyz), &data, 1);
   data = BMA020_GET_BITSLICE(data, OFFSET_X_LSB);
   *offset = data;
   comres += p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, (OFFSET_X_MSB__REG+xyz), &data, 1);
   *offset |= (data<<2);
   
   return comres;
}


int bma020_set_offset(unsigned char xyz, unsigned short offset) 
{
   int comres;
   unsigned char data;
   
   if (p_bma020==0)
   		return E_BMA020_NULL_PTR;
   
   comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, (OFFSET_X_LSB__REG+xyz), &data, 1);
   data = BMA020_SET_BITSLICE(data, OFFSET_X_LSB, offset);
   comres += p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, (OFFSET_X_LSB__REG+xyz), &data, 1);
   data = (offset&0x3ff)>>2;
   comres += p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, (OFFSET_X_MSB__REG+xyz), &data, 1);
   
   return comres;
}


int bma020_set_offset_eeprom(unsigned char xyz, unsigned short offset) 
{
   int comres;
   unsigned char data;
   
   if (p_bma020==0)
   		return E_BMA020_NULL_PTR;   
   
   comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, (OFFSET_X_LSB__REG+xyz), &data, 1);
   data = BMA020_SET_BITSLICE(data, OFFSET_X_LSB, offset);
   comres += p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, (BMA020_EEP_OFFSET+OFFSET_X_LSB__REG + xyz), &data, 1);   
   p_bma020->delay_msec(34);
   data = (offset&0x3ff)>>2;
   comres += p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, (BMA020_EEP_OFFSET+ OFFSET_X_MSB__REG+xyz), &data, 1);
   p_bma020->delay_msec(34);
   
   return comres;
}




int bma020_set_ee_w(unsigned char eew)
{
    unsigned char data;
	int comres;
	
	if (p_bma020==0)
		return E_BMA020_NULL_PTR;
    
	comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, EE_W__REG,&data, 1);
	data = BMA020_SET_BITSLICE(data, EE_W, eew);
	comres = p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, EE_W__REG, &data, 1);
	
	return comres;
}



int bma020_write_ee(unsigned char addr, unsigned char data) 
{	
	int comres;
	
	if (p_bma020==0) 			/* check pointers */
		return E_BMA020_NULL_PTR;
    
	if (p_bma020->delay_msec == 0)
	    return E_BMA020_NULL_PTR;
    
	comres = bma020_set_ee_w( BMA020_EE_W_ON );
	addr|=0x20;   /* add eeprom address offset to image address if not applied */
	comres += bma020_write_reg(addr, &data, 1 );
	p_bma020->delay_msec( BMA020_EE_W_DELAY );
	comres += bma020_set_ee_w( BMA020_EE_W_OFF);
	
	return comres;
}
#endif


int bma020_selftest(unsigned char st)
{
	int comres;
	unsigned char data;
	
	comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, SELF_TEST__REG, &data, 1);
	data = BMA020_SET_BITSLICE(data, SELF_TEST, st);
	comres += p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, SELF_TEST__REG, &data, 1);  
	
	return comres;  

}


int bma020_set_range(char range) 
{			
   int comres = 0;
   unsigned char data;

   if (p_bma020==0)
	    return E_BMA020_NULL_PTR;

   if (range<3) 
   {	
	 	comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, RANGE__REG, &data, 1 );
	 	data = BMA020_SET_BITSLICE(data, RANGE, range);		  	
		comres += p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, RANGE__REG, &data, 1);
   }
   
   return comres;

}


int bma020_get_range(unsigned char *range) 
{
	int comres = 0;
	

	if (p_bma020==0)
		return E_BMA020_NULL_PTR;
	
	comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, RANGE__REG, range, 1 );

	*range = BMA020_GET_BITSLICE(*range, RANGE);
	
	return comres;

}


int bma020_set_mode(unsigned char mode) {
	
	int comres=0;
	unsigned char data1, data2;

	if (p_bma020==0)
		return E_BMA020_NULL_PTR;

	if (mode<4 || mode!=1) 
	{
		comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, WAKE_UP__REG, &data1, 1 );
		data1  = BMA020_SET_BITSLICE(data1, WAKE_UP, mode);		  
        comres += p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, SLEEP__REG, &data2, 1 );
		data2  = BMA020_SET_BITSLICE(data2, SLEEP, (mode>>1));
    	comres += p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, WAKE_UP__REG, &data1, 1);
	  	comres += p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, SLEEP__REG, &data2, 1);
	  	p_bma020->mode = mode;
	} 
	
	return comres;
	
}



unsigned char bma020_get_mode(void) 
{
    if (p_bma020==0)
    	return E_BMA020_NULL_PTR;	
	
	return p_bma020->mode;
	
}

int bma020_set_bandwidth(char bw) 
{
	int comres = 0;
	unsigned char data;

	if (p_bma020==0)
		return E_BMA020_NULL_PTR;

	if (bw<8) 
	{
  	  comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, RANGE__REG, &data, 1 );
	  data = BMA020_SET_BITSLICE(data, BANDWIDTH, bw);
	  comres += p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, RANGE__REG, &data, 1 );
	}

	return comres;


}


int bma020_get_bandwidth(unsigned char *bw) 
{
	int comres = 1;

	if (p_bma020==0)
		return E_BMA020_NULL_PTR;

	comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, BANDWIDTH__REG, bw, 1 );		

	*bw = BMA020_GET_BITSLICE(*bw, BANDWIDTH);
	
	return comres;
}

int bma020_set_wake_up_pause(unsigned char wup)
{
	int comres=0;
	unsigned char data;

	if (p_bma020==0)
		return E_BMA020_NULL_PTR;

	comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, WAKE_UP_PAUSE__REG, &data, 1 );
	data = BMA020_SET_BITSLICE(data, WAKE_UP_PAUSE, wup);
	comres += p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, WAKE_UP_PAUSE__REG, &data, 1 );
	
	return comres;
}

int bma020_get_wake_up_pause(unsigned char *wup)
{
    int comres = 1;
	unsigned char data;
	
	if (p_bma020==0)
		return E_BMA020_NULL_PTR;

	comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, WAKE_UP_PAUSE__REG, &data,  1 );		
	
	*wup = BMA020_GET_BITSLICE(data, WAKE_UP_PAUSE);
	
	return comres;

}


int bma020_set_low_g_threshold(unsigned char th) 
{
	int comres;	

	if (p_bma020==0)
		return E_BMA020_NULL_PTR;		

	comres = p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, LG_THRES__REG, &th, 1);

	return comres;
}


int bma020_get_low_g_threshold(unsigned char *th)
{

	int comres=1;	
	
	if (p_bma020==0)
		return E_BMA020_NULL_PTR;	

		comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, LG_THRES__REG, th, 1);		

	return comres;

}


int bma020_set_low_g_countdown(unsigned char cnt)
{
	int comres=0;
	unsigned char data;

	if (p_bma020==0)
		return E_BMA020_NULL_PTR;
	
	comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, COUNTER_LG__REG, &data, 1 );
	data = BMA020_SET_BITSLICE(data, COUNTER_LG, cnt);
	comres += p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, COUNTER_LG__REG, &data, 1 );
	
	return comres;
}


int bma020_get_low_g_countdown(unsigned char *cnt)
{
    int comres = 1;
	unsigned char data;
	
	if (p_bma020==0)
		return E_BMA020_NULL_PTR;

	comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, COUNTER_LG__REG, &data,  1 );		
	*cnt = BMA020_GET_BITSLICE(data, COUNTER_LG);
	
	return comres;
}


int bma020_set_high_g_countdown(unsigned char cnt)
{
	int comres=1;
	unsigned char data;

	if (p_bma020==0)
		return E_BMA020_NULL_PTR;

    comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, COUNTER_HG__REG, &data, 1 );
	data = BMA020_SET_BITSLICE(data, COUNTER_HG, cnt);
	comres += p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, COUNTER_HG__REG, &data, 1 );
	
	return comres;
}


int bma020_get_high_g_countdown(unsigned char *cnt)
{
    int comres = 0;
	unsigned char data;
	
	if (p_bma020==0)
		return E_BMA020_NULL_PTR;
	
	comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, COUNTER_HG__REG, &data,  1 );		
	
	*cnt = BMA020_GET_BITSLICE(data, COUNTER_HG);
	
	return comres;

}


int bma020_set_low_g_duration(unsigned char dur) 
{
	int comres=0;	
	
	if (p_bma020==0)
		return E_BMA020_NULL_PTR;
		
	comres = p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, LG_DUR__REG, &dur, 1);

	return comres;
}


int bma020_get_low_g_duration(unsigned char *dur) {
	
	int comres=0;	
	
	if (p_bma020==0)
		return E_BMA020_NULL_PTR;

	comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, LG_DUR__REG, dur, 1);				  
	
	return comres;
}


int bma020_set_high_g_threshold(unsigned char th) 
{
	int comres=0;	

	if (p_bma020==0)
		return E_BMA020_NULL_PTR;

	comres = p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, HG_THRES__REG, &th, 1);
	
	return comres;
}

int bma020_get_high_g_threshold(unsigned char *th)
{
	int comres=0;
	
	if (p_bma020==0)
		return E_BMA020_NULL_PTR;

	comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, HG_THRES__REG, th, 1);		

	return comres;
}



int bma020_set_high_g_duration(unsigned char dur) 
{
	int comres=0;	

	if (p_bma020==0)
		return E_BMA020_NULL_PTR;

	comres = p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, HG_DUR__REG, &dur, 1);

	return comres;
}


int bma020_get_high_g_duration(unsigned char *dur) {	
	
	int comres=0;
	
	if (p_bma020==0)
		return E_BMA020_NULL_PTR;
			
    comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, HG_DUR__REG, dur, 1);		

	return comres;
}


int bma020_set_any_motion_threshold(unsigned char th) 
{
	int comres=0;	

	if (p_bma020==0)
		return E_BMA020_NULL_PTR;

	comres = p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, ANY_MOTION_THRES__REG, &th, 1);

	return comres;
}


int bma020_get_any_motion_threshold(unsigned char *th)
{
	int comres=0;

	if (p_bma020==0)
		return E_BMA020_NULL_PTR;
	
	comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, ANY_MOTION_THRES__REG, th, 1);		

	return comres;

}


int bma020_set_any_motion_count(unsigned char amc)
{
	int comres=0;	
	unsigned char data;
	
	if (p_bma020==0)
		return E_BMA020_NULL_PTR;

 	comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, ANY_MOTION_DUR__REG, &data, 1 );
	data = BMA020_SET_BITSLICE(data, ANY_MOTION_DUR, amc);
	comres = p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, ANY_MOTION_DUR__REG, &data, 1 );
	
	return comres;
}


int bma020_get_any_motion_count(unsigned char *amc)
{
    int comres = 0;
	unsigned char data;
	
	if (p_bma020==0)
		return E_BMA020_NULL_PTR;

	comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, ANY_MOTION_DUR__REG, &data,  1 );		
	
	*amc = BMA020_GET_BITSLICE(data, ANY_MOTION_DUR);
	
	return comres;
}



int bma020_set_interrupt_mask(unsigned char mask) 
{
	int comres=0;
	unsigned char data[4];

	if (p_bma020==0)
		return E_BMA020_NULL_PTR;
	
	data[0] = mask & BMA020_CONF1_INT_MSK;
	data[2] = ((mask<<1) & BMA020_CONF2_INT_MSK);		

	comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, BMA020_CONF1_REG, &data[1], 1);
	comres += p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, BMA020_CONF2_REG, &data[3], 1);		
	
	data[1] &= (~BMA020_CONF1_INT_MSK);
	data[1] |= data[0];
	data[3] &=(~(BMA020_CONF2_INT_MSK));
	data[3] |= data[2];

	comres += p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, BMA020_CONF1_REG, &data[1], 1);
	comres += p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, BMA020_CONF2_REG, &data[3], 1);

	return comres;	
}


int bma020_get_interrupt_mask(unsigned char *mask) 
{
	int comres=0;
	unsigned char data;

	if (p_bma020==0)
		return E_BMA020_NULL_PTR;

	p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, BMA020_CONF1_REG, &data,1);
	*mask = data & BMA020_CONF1_INT_MSK;
	p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, BMA020_CONF2_REG, &data,1);
	*mask = *mask | ((data & BMA020_CONF2_INT_MSK)>>1);

	return comres;
}


int bma020_reset_interrupt(void) 
{	
	int comres=0;
	unsigned char data=(1<<RESET_INT__POS);	
	
	if (p_bma020==0)
		return E_BMA020_NULL_PTR;

	comres = p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, RESET_INT__REG, &data, 1);
	return comres;

}


int bma020_read_accel_x(short *a_x) 
{
	int comres;
	unsigned char data[2];

	if (p_bma020==0)
		return E_BMA020_NULL_PTR;

	comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, ACC_X_LSB__REG, data, 2);
	
	*a_x = BMA020_GET_BITSLICE(data[0],ACC_X_LSB) | BMA020_GET_BITSLICE(data[1],ACC_X_MSB)<<ACC_X_LSB__LEN;
	*a_x = *a_x << (sizeof(short)*8-(ACC_X_LSB__LEN+ACC_X_MSB__LEN));
	*a_x = *a_x >> (sizeof(short)*8-(ACC_X_LSB__LEN+ACC_X_MSB__LEN));
	
	return comres;
	
}


int bma020_read_accel_y(short *a_y) 
{
	int comres;
	unsigned char data[2];	

	if (p_bma020==0)
		return E_BMA020_NULL_PTR;

	comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, ACC_Y_LSB__REG, data, 2);
	
	*a_y = BMA020_GET_BITSLICE(data[0],ACC_Y_LSB) | BMA020_GET_BITSLICE(data[1],ACC_Y_MSB)<<ACC_Y_LSB__LEN;
	*a_y = *a_y << (sizeof(short)*8-(ACC_Y_LSB__LEN+ACC_Y_MSB__LEN));
	*a_y = *a_y >> (sizeof(short)*8-(ACC_Y_LSB__LEN+ACC_Y_MSB__LEN));
	
	return comres;
}


/** Z-axis acceleration data readout 
	\param *a_z pointer for 16 bit 2's complement data output (LSB aligned)
*/
int bma020_read_accel_z(short *a_z)
{
	int comres;
	unsigned char data[2];	

	if (p_bma020==0)
		return E_BMA020_NULL_PTR;

	comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, ACC_Z_LSB__REG, data, 2);
	
	*a_z = BMA020_GET_BITSLICE(data[0],ACC_Z_LSB) | BMA020_GET_BITSLICE(data[1],ACC_Z_MSB)<<ACC_Z_LSB__LEN;
	*a_z = *a_z << (sizeof(short)*8-(ACC_Z_LSB__LEN+ACC_Z_MSB__LEN));
	*a_z = *a_z >> (sizeof(short)*8-(ACC_Z_LSB__LEN+ACC_Z_MSB__LEN));
	
	return comres;
}

#if 0
int bma020_read_temperature(unsigned char * temp) 
{
	int comres;	

	if (p_bma020==0)
		return E_BMA020_NULL_PTR;

	comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, TEMPERATURE__REG, temp, 1);
	
	return comres;

}
#endif


int bma020_read_accel_xyz(bma020acc_t * acc)
{
	int comres;
	unsigned char data[6];

	if (p_bma020==0)
		return E_BMA020_NULL_PTR;
	
	comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, ACC_X_LSB__REG, &data[0],6);
	
	acc->x = BMA020_GET_BITSLICE(data[0],ACC_X_LSB) | (BMA020_GET_BITSLICE(data[1],ACC_X_MSB)<<ACC_X_LSB__LEN);
	acc->x = acc->x << (sizeof(short)*8-(ACC_X_LSB__LEN+ACC_X_MSB__LEN));
	acc->x = acc->x >> (sizeof(short)*8-(ACC_X_LSB__LEN+ACC_X_MSB__LEN));

	acc->y = BMA020_GET_BITSLICE(data[2],ACC_Y_LSB) | (BMA020_GET_BITSLICE(data[3],ACC_Y_MSB)<<ACC_Y_LSB__LEN);
	acc->y = acc->y << (sizeof(short)*8-(ACC_Y_LSB__LEN + ACC_Y_MSB__LEN));
	acc->y = acc->y >> (sizeof(short)*8-(ACC_Y_LSB__LEN + ACC_Y_MSB__LEN));
	
/*	
	acc->z = BMA020_GET_BITSLICE(data[4],ACC_Z_LSB); 
	acc->z |= (BMA020_GET_BITSLICE(data[5],ACC_Z_MSB)<<ACC_Z_LSB__LEN);
	acc->z = acc->z << (sizeof(short)*8-(ACC_Z_LSB__LEN+ACC_Z_MSB__LEN));
	acc->z = acc->z >> (sizeof(short)*8-(ACC_Z_LSB__LEN+ACC_Z_MSB__LEN));
*/	
	acc->z = BMA020_GET_BITSLICE(data[4],ACC_Z_LSB) | (BMA020_GET_BITSLICE(data[5],ACC_Z_MSB)<<ACC_Z_LSB__LEN);
	acc->z = acc->z << (sizeof(short)*8-(ACC_Z_LSB__LEN + ACC_Z_MSB__LEN));
	acc->z = acc->z >> (sizeof(short)*8-(ACC_Z_LSB__LEN + ACC_Z_MSB__LEN));

	acc->x -= cal_data.x;
	acc->y -= cal_data.y;
	acc->z -= cal_data.z;
	
	return comres;
}


int bma020_get_interrupt_status(unsigned char * ist) 
{

	int comres=0;	

	if (p_bma020==0)
		return E_BMA020_NULL_PTR;
	
	comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, BMA020_STATUS_REG, ist, 1);
	
	return comres;
}


int bma020_set_low_g_int(unsigned char onoff) {
	int comres;
	unsigned char data;
	
	if(p_bma020==0) 
		return E_BMA020_NULL_PTR;

	comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, ENABLE_LG__REG, &data, 1);				
	
	data = BMA020_SET_BITSLICE(data, ENABLE_LG, onoff);
	
	comres += p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, ENABLE_LG__REG, &data, 1);
	
	return comres;
}


int bma020_set_high_g_int(unsigned char onoff) 
{
	int comres;
	
	unsigned char data;
	
	if(p_bma020==0) 
		return E_BMA020_NULL_PTR;

	comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, ENABLE_HG__REG, &data, 1);				
	
	data = BMA020_SET_BITSLICE(data, ENABLE_HG, onoff);
	
	comres += p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, ENABLE_HG__REG, &data, 1);
	
	return comres;
}


int bma020_set_any_motion_int(unsigned char onoff) {
	int comres;
	
	unsigned char data;
	
	if(p_bma020==0) 
		return E_BMA020_NULL_PTR;
	
	comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, EN_ANY_MOTION__REG, &data, 1);				
	data = BMA020_SET_BITSLICE(data, EN_ANY_MOTION, onoff);
	comres += p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, EN_ANY_MOTION__REG, &data, 1);
	
	return comres;

}


int bma020_set_alert_int(unsigned char onoff) 
{
	int comres;
	unsigned char data;
	
	if(p_bma020==0) 
		return E_BMA020_NULL_PTR;

	comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, ALERT__REG, &data, 1);				
	data = BMA020_SET_BITSLICE(data, ALERT, onoff);
	
	comres += p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, ALERT__REG, &data, 1);
	
	return comres;

}


int bma020_set_advanced_int(unsigned char onoff) 
{
	int comres;
	unsigned char data;

	if(p_bma020==0) 
		return E_BMA020_NULL_PTR;
	
	comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, ENABLE_ADV_INT__REG, &data, 1);				
	data = BMA020_SET_BITSLICE(data, EN_ANY_MOTION, onoff);

	comres += p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, ENABLE_ADV_INT__REG, &data, 1);
	
	return comres;
}


int bma020_latch_int(unsigned char latched) 
{
	int comres;
	unsigned char data;
	
	if(p_bma020==0) 
		return E_BMA020_NULL_PTR;
	
	comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, LATCH_INT__REG, &data, 1);				
	data = BMA020_SET_BITSLICE(data, LATCH_INT, latched);

	comres += p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, LATCH_INT__REG, &data, 1);
	
	return comres;
}


int bma020_set_new_data_int(unsigned char onoff) 
{
	int comres;
	unsigned char data;

	if(p_bma020==0) 
		return E_BMA020_NULL_PTR;

	comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, NEW_DATA_INT__REG, &data, 1);				
	data = BMA020_SET_BITSLICE(data, NEW_DATA_INT, onoff);
	comres += p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, NEW_DATA_INT__REG, &data, 1);
	
	return comres;
}


int bma020_pause(int msec) 
{
	if (p_bma020==0)
		return E_BMA020_NULL_PTR;
	else
		p_bma020->delay_msec(msec);	

	return msec;
}


int bma020_read_reg(unsigned char addr, unsigned char *data, unsigned char len)
{
	int comres;
	
	if (p_bma020==0)
		return E_BMA020_NULL_PTR;

	comres = p_bma020->BMA020_BUS_READ_FUNC(p_bma020->dev_addr, addr, data, len);
	
	return comres;
}


int bma020_write_reg(unsigned char addr, unsigned char *data, unsigned char len) 
{
	int comres;

	if (p_bma020==0)
		return E_BMA020_NULL_PTR;

	comres = p_bma020->BMA020_BUS_WRITE_FUNC(p_bma020->dev_addr, addr, data, len);

	return comres;
}

bma020acc_t bma020_calibrate()
{
	int sum_x = 0;
	int sum_y = 0;
	int sum_z = 0;
	bma020acc_t cur_val;
 	int i = 0;

	cal_data.x = cal_data.y = cal_data.z = 0;
		
	for(i = 0; i < 20; i++)
		{
		bma020_read_accel_xyz(&cur_val);
		sum_x += cur_val.x;
		sum_y += cur_val.y;
		sum_z += cur_val.z;
		}

	cal_data.x = (sum_x / 20) - 0;
	cal_data.y = (sum_y / 20) - 0;
	cal_data.z = (sum_z / 20) - 256;

	return cal_data;
}

