// Maxim 8906 Command Module: Interface Window
// Firmware Group
// 1/2/2008   initialize
// (C) 2004 Maxim Integrated Products
//---------------------------------------------------------------------------

#include <linux/kernel.h>
#include <linux/i2c/maximi2c.h>
#include <linux/i2c/pmic.h>

#ifdef PREFIX
#undef PREFIX
#endif
#define PREFIX "MAX8906: "

#define MSG_HIGH(a,b,c,d)		{}

max8906_register_type  max8906reg[ENDOFREG] =
{
    //========================================================
    //  B A T T E R Y   C H A R G E R S
    //========================================================
    /* Slave addr   Reg addr */
    {  0x78,        0x7C }, // REG_CHG_CNTL1,
    {  0x78,        0x7D }, // REG_CHG_CNTL2,
    {  0x78,        0x7E }, // REG_CHG_IRQ1,
    {  0x78,        0x7F }, // REG_CHG_IRQ2,
    {  0x78,        0x80 }, // REG_CHG_IRQ1_MASK,
    {  0x78,        0x81 }, // REG_CHG_IRQ2_MASK,
    {  0x78,        0x82 }, // REG_CHG_STAT,
    {  0x78,        0x78 }, // REG_BBATTCNFG,

    //========================================================
    //  S T E P - D O W N    R E G U L A T O R S
    //========================================================
    /* Slave addr   Reg addr */
    {  0x78,        0x5C }, // REG_WBBCOREEN,
    {  0x78,        0x5D }, // REG_WBBCOREFSEQ,
    {  0x78,        0x5E }, // REG_WBBCORETV,
    {  0x78,        0x9C }, // REG_WBBRFEN,
    {  0x78,        0x9D }, // REG_WBBRFFSEQ,
    {  0x78,        0x9E }, // REG_WBBRFTV,
    {  0x78,        0x04 }, // REG_APPSEN,
    {  0x68,        0x10 }, // REG_OVER1,
    {  0x78,        0x05 }, // REG_APPSFSEQ,
    {  0x68,        0x23 }, // REG_ADTV1,
    {  0x68,        0x24 }, // REG_ADTV2,
    {  0x78,        0x06 }, // REG_APPSCLG,
    {  0x68,        0x20 }, // REG_VCC1,
    {  0x78,        0x08 }, // REG_IOEN,
    {  0x78,        0x09 }, // REG_IOFSEQ,
    {  0x78,        0x0C }, // REG_MEMEN,
    {  0x78,        0x0D }, // REG_MEMFSEQ,

    //========================================================
    //  L I N E A R   R E G U L A T O R S
    //========================================================
    /* Slave addr   Reg addr */
    {  0x78,        0x10 }, // REG_WBBMEMEN,
    {  0x78,        0x11 }, // REG_WBBMEMFSEQ,
    {  0x78,        0x12 }, // REG_WBBMEMTV,
    {  0x78,        0x14 }, // REG_WBBIOEN,
    {  0x78,        0x15 }, // REG_WBBIOFSEQ,
    {  0x78,        0x16 }, // REG_WBBIOTV,
    {  0x78,        0x18 }, // REG_WBBANAEN,
    {  0x78,        0x19 }, // REG_WBBANAFSEQ,
    {  0x78,        0x1A }, // REG_WBBANATV,
    {  0x78,        0x1C }, // REG_RFRXLEN,
    {  0x78,        0x1D }, // REG_RFRXLFSEQ,
    {  0x78,        0x1E }, // REG_RFRXLTV,
    {  0x78,        0x20 }, // REG_RFTXLEN,
    {  0x78,        0x21 }, // REG_RFTXLFSEQ,
    {  0x78,        0x22 }, // REG_RFTXLTV,
    {  0x78,        0x24 }, // REG_RFRXHEN,
    {  0x78,        0x25 }, // REG_RFRXHFSEQ,
    {  0x78,        0x26 }, // REG_RFRXHTV,
    {  0x78,        0x28 }, // REG_RFTCXOEN,
    {  0x78,        0x29 }, // REG_RFTCXOFSEQ,
    {  0x78,        0x2A }, // REG_RFTCXOTV,
    {  0x78,        0x2C }, // REG_LDOAEN,
    {  0x78,        0x2D }, // REG_LDOAFSEQ,
    {  0x78,        0x2E }, // REG_LDOATV,
    {  0x78,        0x30 }, // REG_LDOBEN,
    {  0x78,        0x31 }, // REG_LDOBFSEQ,
    {  0x78,        0x32 }, // REG_LDOBTV,
    {  0x78,        0x34 }, // REG_LDOCEN,
    {  0x78,        0x35 }, // REG_LDOCFSEQ,
    {  0x78,        0x36 }, // REG_LDOCTV,
    {  0x78,        0x38 }, // REG_LDODEN,
    {  0x78,        0x39 }, // REG_LDODFSEQ,
    {  0x78,        0x3A }, // REG_LDODTV,
    {  0x78,        0x3C }, // REG_SIMLTEN,
    {  0x78,        0x3D }, // REG_SIMLTFSEQ,
    {  0x78,        0x3E }, // REG_SIMLTTV,
    {  0x78,        0x40 }, // REG_SRAMEN,
    {  0x78,        0x41 }, // REG_SRAMFSEQ,
    {  0x68,        0x29 }, // REG_SDTV1,
    {  0x68,        0x2A }, // REG_SDTV2,
    {  0x78,        0x42 }, // REG_SRAMCLG,
    {  0x78,        0x44 }, // REG_CARD1EN,
    {  0x78,        0x45 }, // REG_CARD1FSEQ,
    {  0x78,        0x46 }, // REG_CARD1TV,
    {  0x78,        0x48 }, // REG_CARD2EN,
    {  0x78,        0x49 }, // REG_CARD2FSEQ,
    {  0x78,        0x4A }, // REG_CARD2TV,
    {  0x78,        0x4C }, // REG_MVTENEN,
    {  0x78,        0x4D }, // REG_MVTFSEQ,
    {  0x68,        0x32 }, // REG_MDTV1,
    {  0x68,        0x33 }, // REG_MDTV2,
    {  0x78,        0x50 }, // REG_BIASEN,
    {  0x78,        0x51 }, // REG_BIASFSEQ,
    {  0x78,        0x52 }, // REG_BIASTV,
    {  0x78,        0x54 }, // REG_VBUSEN,
    {  0x78,        0x55 }, // REG_VBUSFSEQ,
    {  0x78,        0x58 }, // REG_USBTXRXEN,
    {  0x78,        0x59 }, // REG_USBTXRXFSEQ,

    //========================================================
    //  M A I N - B A T T E R Y   F A U L T   D E T E C T O R
    //========================================================
    /* Slave addr   Reg addr */
    {  0x78,        0x60 }, // REG_LBCNFG,

    //========================================================
    //  O N / O F F   C O N T R O L L E R
    //========================================================
    /* Slave addr   Reg addr */
    {  0x78,        0x00 }, // REG_EXTWKSEL,
    {  0x78,        0x01 }, // REG_ON_OFF_IRQ,
    {  0x78,        0x02 }, // REG_ON_OFF_IRQ_MASK,
    {  0x78,        0x03 }, // REG_ON_OFF_STAT,

    //========================================================
    //  F L E X I B L E   P O W E R   S E Q U E N C E R
    //========================================================
    /* Slave addr   Reg addr */
    {  0x78,        0x64 }, // REG_SEQ1CNFG,
    {  0x78,        0x65 }, // REG_SEQ2CNFG,
    {  0x78,        0x66 }, // REG_SEQ3CNFG,
    {  0x78,        0x67 }, // REG_SEQ4CNFG,
    {  0x78,        0x68 }, // REG_SEQ5CNFG,
    {  0x78,        0x69 }, // REG_SEQ6CNFG,
    {  0x78,        0x6A }, // REG_SEQ7CNFG,

    //========================================================
    //  U S B   T R A N S C E I V E R
    //========================================================
    /* Slave addr   Reg addr */
    {  0x78,        0x6C }, // REG_USBCNFG,

    //========================================================
    //  T C X O   B U F F E R
    //========================================================
    /* Slave addr   Reg addr */
    {  0x78,        0x74 }, // REG_TCXOCNFG,

    //========================================================
    //  R E F E R E N C E   O U T P U T (R E F O U T)
    //========================================================
    /* Slave addr   Reg addr */
    {  0x78,        0x70 }, // REG_REFOUTCNFG,

    //========================================================
    //  R E A L   T I M E   C L O C K (R T C)
    //========================================================
    /* Slave addr   Reg addr */
    {  0xD0,        0x00 }, // REG_RTC_SEC,
    {  0xD0,        0x01 }, // REG_RTC_MIN,
    {  0xD0,        0x02 }, // REG_RTC_HOURS,
    {  0xD0,        0x03 }, // REG_RTC_WEEKDAY,
    {  0xD0,        0x04 }, // REG_RTC_DATE,
    {  0xD0,        0x05 }, // REG_RTC_MONTH,
    {  0xD0,        0x06 }, // REG_RTC_YEAR1,
    {  0xD0,        0x07 }, // REG_RTC_YEAR2,
    {  0xD0,        0x08 }, // REG_ALARM0_SEC,
    {  0xD0,        0x09 }, // REG_ALARM0_MIN,
    {  0xD0,        0x0A }, // REG_ALARM0_HOURS,
    {  0xD0,        0x0B }, // REG_ALARM0_WEEKDAY,
    {  0xD0,        0x0C }, // REG_ALARM0_DATE,
    {  0xD0,        0x0D }, // REG_ALARM0_MONTH,
    {  0xD0,        0x0E }, // REG_ALARM0_YEAR1,
    {  0xD0,        0x0F }, // REG_ALARM0_YEAR2,
    {  0xD0,        0x10 }, // REG_ALARM1_SEC,
    {  0xD0,        0x11 }, // REG_ALARM1_MIN,
    {  0xD0,        0x12 }, // REG_ALARM1_HOURS,
    {  0xD0,        0x13 }, // REG_ALARM1_WEEKDAY,
    {  0xD0,        0x14 }, // REG_ALARM1_DATE,
    {  0xD0,        0x15 }, // REG_ALARM1_MONTH,
    {  0xD0,        0x16 }, // REG_ALARM1_YEAR1,
    {  0xD0,        0x17 }, // REG_ALARM1_YEAR2,
    {  0xD0,        0x18 }, // REG_ALARM0_CNTL,
    {  0xD0,        0x19 }, // REG_ALARM1_CNTL,
    {  0xD0,        0x1A }, // REG_RTC_STATUS,
    {  0xD0,        0x1B }, // REG_RTC_CNTL,
    {  0xD0,        0x1C }, // REG_RTC_IRQ,
    {  0xD0,        0x1D }, // REG_RTC_IRQ_MASK,
    {  0xD0,        0x1E }, // REG_MPL_CNTL,

    //========================================================
    //  T O U C H - S C R E E N / A D C   C O N T R O L L E R
    //========================================================
    /* Slave addr   Reg addr */
    {  0x8E,        0x00 }, // REG_TSC_STA_INT,
    {  0x8E,        0x01 }, // REG_TSC_INT_MASK,
    {  0x8E,        0x02 }, // REG_TSC_CNFG1,
    {  0x8E,        0x03 }, // REG_TSC_CNFG2,
    {  0x8E,        0x04 }, // REG_TSC_CNFG3,
    {  0x8E,        0x05 }, // REG_TSC_CNFG4,
    {  0x8E,        0x06 }, // REG_TSC_RES_CNFG1,
    {  0x8E,        0x07 }, // REG_TSC_AVG_CNFG1,
    {  0x8E,        0x08 }, // REG_TSC_ACQ_CNFG1,
    {  0x8E,        0x09 }, // REG_TSC_ACQ_CNFG2,
    {  0x8E,        0x0A }, // REG_TSC_ACQ_CNFG3,
    //========== ADC_RESULTS registers
    {  0x8E,        0x50 }, // REG_ADC_X_MSB,
    {  0x8E,        0x51 }, // REG_ADC_X_LSB,
    {  0x8E,        0x52 }, // REG_ADC_Y_MSB,
    {  0x8E,        0x53 }, // REG_ADC_Y_LSB,
    {  0x8E,        0x54 }, // REG_ADC_Z1_MSB,
    {  0x8E,        0x55 }, // REG_ADC_Z1_LSB,
    {  0x8E,        0x56 }, // REG_ADC_Z2_MSB,
    {  0x8E,        0x57 }, // REG_ADC_Z2_LSB,
    {  0x8E,        0x60 }, // REG_ADC_AUX1_MSB,
    {  0x8E,        0x61 }, // REG_ADC_AUX1_LSB,
    {  0x8E,        0x62 }, // REG_ADC_VBUS_MSB,
    {  0x8E,        0x63 }, // REG_ADC_VBUS_LSB,
    {  0x8E,        0x64 }, // REG_ADC_VAC_MSB,
    {  0x8E,        0x65 }, // REG_ADC_VAC_LSB,
    {  0x8E,        0x66 }, // REG_ADC_MBATT_MSB,
    {  0x8E,        0x67 }, // REG_ADC_MBATT_LSB,
    {  0x8E,        0x68 }, // REG_ADC_BBATT_MSB,
    {  0x8E,        0x69 }, // REG_ADC_BBATT_LSB,
    {  0x8E,        0x6A }, // REG_ADC_ICHG_MSB,
    {  0x8E,        0x6B }, // REG_ADC_ICHG_LSB,
    {  0x8E,        0x6C }, // REG_ADC_TDIE_MSB,
    {  0x8E,        0x6D }, // REG_ADC_TDIE_LSB,
    {  0x8E,        0x6E }, // REG_ADC_AUX2_MSB,
    {  0x8E,        0x6F }, // REG_ADC_AUX2_LSB,

    // TOUCH-SCREEN CONVERSION COMMAND registers
    {  0x8E,        0x80 }, // REG_TSC_X_Drive,
    {  0x8E,        0x88 }, // REG_TSC_X_Measurement,
    {  0x8E,        0x90 }, // REG_TSC_Y_Drive,
    {  0x8E,        0x98 }, // REG_TSC_Y_Measurement,
    {  0x8E,        0xA0 }, // REG_TSC_Z1_Drive,
    {  0x8E,        0xA8 }, // REG_TSC_Z1_Measurement,
    {  0x8E,        0xB0 }, // REG_TSC_Z2_Drive,
    {  0x8E,        0xB8 }, // REG_TSC_Z2_Measurement,
    {  0x8E,        0xC0 }, // REG_TSC_AUX1_Measurement,
    {  0x8E,        0xC8 }, // REG_TSC_VBUS_Measurement,      
    {  0x8E,        0xD0 }, // REG_TSC_VAC_Measurement,
    {  0x8E,        0xD8 }, // REG_TSC_MBATT_Measurement,
    {  0x8E,        0xE0 }, // REG_TSC_BBATT_Measurement,
    {  0x8E,        0xE8 }, // REG_TSC_ICHG_Measurement,
    {  0x8E,        0xF0 }, // REG_TSC_TDIE_Measurement,
    {  0x8E,        0xF8 }, // REG_TSC_AUX2_Measurement,

    //========================================================
    //  A U D I O   S U B S Y S T E M
    //========================================================
    /* Slave addr   Reg addr */
    {  0x78,        0x84 }, // REG_PGA_CNTL1,
    {  0x78,        0x85 }, // REG_PGA_CNTL2,
    {  0x78,        0x86 }, // REG_LMIX_CNTL,
    {  0x78,        0x87 }, // REG_RMIX_CNTL,
    {  0x78,        0x88 }, // REG_MMIX_CNTL,
    {  0x78,        0x89 }, // REG_HS_RIGHT_GAIN_CNTL,
    {  0x78,        0x8A }, // REG_HS_LEFT_GAIN_CNTL,
    {  0x78,        0x8B }, // REG_LINE_OUT_GAIN_CNTL,
    {  0x78,        0x8C }, // REG_LS_GAIN_CNTL,
    {  0x78,        0x8D }, // REG_AUDIO_CNTL,
    {  0x78,        0x8E }, // REG_AUDIO_ENABLE1,

    //========================================================
    //  C H I P   I D E N T I F I C A T I O N
    //========================================================
    /* Slave addr   Reg addr */
    {  0x68,        0x8E }, // REG_II1RR,
    {  0x68,        0x8F }, // REG_II2RR,
    {  0x78,        0x98 } // REG_IRQ_STAT,
};


max8906_function_type  max8906pm[ENDOFPM] =
{
    //========================================================
    //  B A T T E R Y   C H A R G E R S
    //========================================================
    // CHG_CNTL1 register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x7C,       0x80,       0x7F,       0x07 }, // nCHGEN
    {  0x78,       0x7C,       0x60,       0x9F,       0x05 }, // CHG_TOPOFF_TH
    {  0x78,       0x7C,       0x18,       0xE7,       0x03 }, // CHG_RST_HYS
    {  0x78,       0x7C,       0x07,       0xF8,       0x00 }, // AC_FCHG

    // CHG_CNTL2 register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x7D,       0xC0,       0x3F,       0x06 }, // VBUS_FCHG
    {  0x78,       0x7D,       0x30,       0xCF,       0x04 }, // FCHG_TMR
    {  0x78,       0x7D,       0x08,       0xF7,       0x03 }, // MBAT_REG_TH
    {  0x78,       0x7D,       0x07,       0xF8,       0x00 }, // MBATT_THERM_REG

    // CHG_IRQ1 register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x7E,       0x80,       0x7F,       0x07 }, // VAC_R
    {  0x78,       0x7E,       0x40,       0xBF,       0x06 }, // VAC_F
    {  0x78,       0x7E,       0x20,       0xDF,       0x05 }, // VAC_OVP
    {  0x78,       0x7E,       0x10,       0xEF,       0x04 }, // VBUS_R
    {  0x78,       0x7E,       0x08,       0xF7,       0x03 }, // VBUS_F
    {  0x78,       0x7E,       0x04,       0xFB,       0x02 }, // VBUS_OVP

    // CHG_IRQ2 register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x7F,       0x80,       0x7F,       0x07 }, // CHG_TMR_FAULT
    {  0x78,       0x7F,       0x40,       0xBF,       0x06 }, // CHG_TOPOFF
    {  0x78,       0x7F,       0x20,       0xDF,       0x05 }, // CHG_DONE
    {  0x78,       0x7F,       0x10,       0xEF,       0x04 }, // CHG_RST
    {  0x78,       0x7F,       0x08,       0xF7,       0x03 }, // MBATTLOWR
    {  0x78,       0x7F,       0x04,       0xFB,       0x02 }, // MBATTLOWF

    // CHG_IRQ1_MASK register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x80,       0x80,       0x7F,       0x07 }, // VAC_R_MASK
    {  0x78,       0x80,       0x40,       0xBF,       0x06 }, // VAC_F_MASK
    {  0x78,       0x80,       0x20,       0xDF,       0x05 }, // VAC_OVP_MASK
    {  0x78,       0x80,       0x10,       0xEF,       0x04 }, // VBUS_R_MASK
    {  0x78,       0x80,       0x08,       0xF7,       0x03 }, // VBUS_F_MASK
    {  0x78,       0x80,       0x04,       0xFB,       0x02 }, // VBUS_OVP_MASK

    // CHG_IRQ2_MASK register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x81,       0x80,       0x7F,       0x07 }, // CHG_TMR_FAULT_MASK
    {  0x78,       0x81,       0x40,       0xBF,       0x06 }, // CHG_TOPOFF_MASK
    {  0x78,       0x81,       0x20,       0xDF,       0x05 }, // CHG_DONE_MASK
    {  0x78,       0x81,       0x10,       0xEF,       0x04 }, // CHG_RST_MASK
    {  0x78,       0x81,       0x08,       0xF7,       0x03 }, // MBATTLOWR_MASK
    {  0x78,       0x81,       0x04,       0xFB,       0x02 }, // MBATTLOWF_MASK

    // CHG_STAT register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x82,       0x80,       0x7F,       0x07 }, // VAC_OK
    {  0x78,       0x82,       0x40,       0xBF,       0x06 }, // VBUS_OK
    {  0x78,       0x82,       0x20,       0xDF,       0x05 }, // CHG_TMR
    {  0x78,       0x82,       0x10,       0xEF,       0x04 }, // CHG_EN_STAT
    {  0x78,       0x82,       0x0C,       0xF3,       0x02 }, // CHG_MODE
    {  0x78,       0x82,       0x02,       0xFD,       0x01 }, // MBATT_DET
    {  0x78,       0x82,       0x01,       0xFE,       0x00 }, // MBATTLOW

    // BBATTCNFG register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x78,       0x80,       0x7F,       0x07 }, // APPALLOFF
    {  0x78,       0x78,       0x03,       0xFC,       0x00 }, // VBBATTCV

    //========================================================
    //  S T E P - D O W N    R E G U L A T O R S
    //========================================================
    // WBBCOREEN register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x5C,       0x80,       0x7F,       0x07 }, // nWCRADE
    {  0x78,       0x5C,       0x0E,       0xF1,       0x01 }, // WCRENSRC
    {  0x78,       0x5C,       0x01,       0xFE,       0x00 }, // WCREN

    // WBBCOREFSEQ register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x5D,       0xF0,       0x0F,       0x04 }, // WCRFSEQPU
    {  0x78,       0x5D,       0x0F,       0xF0,       0x00 }, // WCRFSEQPD

    // WBBCORETV register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x5E,       0x3F,       0xC0,       0x00 }, // WCRTV

    // WBBRFEN register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x9C,       0x80,       0x7F,       0x07 }, // nWRFADE
    {  0x78,       0x9C,       0x0E,       0xF1,       0x01 }, // WRFENSRC
    {  0x78,       0x9C,       0x01,       0xFE,       0x00 }, // WRFEN

    // WBBRFFSEQ register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x9D,       0xF0,       0x0F,       0x04 }, // WRFFSEQPU
    {  0x78,       0x9D,       0x0F,       0xF0,       0x00 }, // WRFFSEQPD

    // WBBRFTV register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x9E,       0x3F,       0xC0,       0x00 }, // WRFTV

    // APPSEN register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x04,       0x80,       0x7F,       0x07 }, // nAPPSADE
    {  0x78,       0x04,       0x10,       0xEF,       0x04 }, // nOVER1ENAPPS
    {  0x78,       0x04,       0x0E,       0xF1,       0x01 }, // APPSENSRC
    {  0x78,       0x04,       0x01,       0xFE,       0x00 }, // APPSEN

    // OVER1 register
    /* slave addr  addr        mask        clear       shift */
    {  0x68,       0x10,       0x04,       0xFB,       0x02 }, // ENSRAM
    {  0x68,       0x10,       0x01,       0xFE,       0x00 }, // ENAPPS

    // APPSFSEQ register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x05,       0xF0,       0x0F,       0x04 }, // APPSFSEQPU
    {  0x78,       0x05,       0x0F,       0xF0,       0x00 }, // APPSFSEQPD

    // ADTV1 register
    /* slave addr  addr        mask        clear       shift */
    {  0x68,       0x23,       0x80,       0x7F,       0x07 }, // T1AOST
    {  0x68,       0x23,       0x3F,       0xC0,       0x00 }, // T1APPS

    // ADTV2 register
    /* slave addr  addr        mask        clear       shift */
    {  0x68,       0x24,       0x80,       0x7F,       0x07 }, // T2AOST
    {  0x68,       0x24,       0x3F,       0xC0,       0x00 }, // T2APPS

    // APPSCLG register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x06,       0x3F,       0xC0,       0x00 }, // CLGAPPS

    // VCC1 register
    /* slave addr  addr        mask        clear       shift */
    {  0x68,       0x20,       0x80,       0x7F,       0x07 }, // MVS
    {  0x68,       0x20,       0x40,       0xBF,       0x06 }, // MGO
    {  0x68,       0x20,       0x20,       0xDF,       0x05 }, // SVS
    {  0x68,       0x20,       0x10,       0xEF,       0x04 }, // SGO
    {  0x68,       0x20,       0x02,       0xFD,       0x01 }, // AVS
    {  0x68,       0x20,       0x01,       0xFE,       0x00 }, // AGO

    // IOEN register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x08,       0x80,       0x7F,       0x07 }, // nIOADE
    {  0x78,       0x08,       0x0E,       0xF1,       0x01 }, // IOENSRC
    {  0x78,       0x08,       0x01,       0xFE,       0x00 }, // IOEN

    // IOFSEQ register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x09,       0xF0,       0x0F,       0x04 }, // IOFSEQPU
    {  0x78,       0x09,       0x0F,       0xF0,       0x00 }, // IOFSEQPD

    // MEMEN register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x0C,       0x80,       0x7F,       0x07 }, // nMEMADE
    {  0x78, 	  0x0C, 	 0x30,	  0xCF,	  0x04 }, // MEMDVM
    {  0x78,       0x0C,       0x0E,       0xF1,       0x01 }, // MEMENSRC
    {  0x78,       0x0C,       0x01,       0xFE,       0x00 }, // MEMEN

    // MEMFSEQ register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x0D,       0xF0,       0x0F,       0x04 }, // MEMFSEQPU
    {  0x78,       0x0D,       0x0F,       0xF0,       0x00 }, // MEMFSEQPD

    //========================================================
    //  L I N E A R   R E G U L A T O R S
    //========================================================
    // WBBMEMEN register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x10,       0x80,       0x7F,       0x07 }, // nWMEMADE
    {  0x78,       0x10,       0x0E,       0xF1,       0x01 }, // WMEMENSRC
    {  0x78,       0x10,       0x01,       0xFE,       0x00 }, // WMEMEN

    // WBBMEMFSEQ register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x11,       0xF0,       0x0F,       0x04 }, // WMEMFSEQPU
    {  0x78,       0x11,       0x0F,       0xF0,       0x00 }, // WMEMFSEQPD

    // WBBMEMTV register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x12,       0x3F,       0xC0,       0x00 }, // WMEMTV

    // WBBIOEN register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x14,       0x80,       0x7F,       0x07 }, // nWIOADE
    {  0x78,       0x14,       0x10,       0xEF,       0x04 }, // SFTRSTWBB
    {  0x78,       0x14,       0x0E,       0xF1,       0x01 }, // WIOENSRC
    {  0x78,       0x14,       0x01,       0xFE,       0x00 }, // WIOEN

    // WBBIOFSEQ register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x15,       0xF0,       0x0F,       0x04 }, // WIOFSEQPU
    {  0x78,       0x15,       0x0F,       0xF0,       0x00 }, // WIOFSEQPD

    // WBBIOTV register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x16,       0x3F,       0xC0,       0x00 }, // WIOTV

    // WBBANAEN register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x18,       0x80,       0x7F,       0x07 }, // nWANAADE
    {  0x78,       0x18,       0x0E,       0xF1,       0x01 }, // WANAENSRC
    {  0x78,       0x18,       0x01,       0xFE,       0x00 }, // WANAEN

    // WBBANAFSEQ register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x19,       0xF0,       0x0F,       0x04 }, // WANAFSEQPU
    {  0x78,       0x19,       0x0F,       0xF0,       0x00 }, // WANAFSEQPD

    // WBBANATV register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x1A,       0x3F,       0xC0,       0x00 }, // WANATV

    // RFRXLEN register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x1C,       0x80,       0x7F,       0x07 }, // nRFRXLADE
    {  0x78,       0x1C,       0x0E,       0xF1,       0x01 }, // RFRXLENSRC
    {  0x78,       0x1C,       0x01,       0xFE,       0x00 }, // RFRXLEN

    // RFRXLFSEQ register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x1D,       0xF0,       0x0F,       0x04 }, // RFRXLFSEQPU
    {  0x78,       0x1D,       0x0F,       0xF0,       0x00 }, // RFRXLFSEQPD

    // RFRXLTV register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x1E,       0x3F,       0xC0,       0x00 }, // RFRXLTV

    // RFTXLEN register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x20,       0x80,       0x7F,       0x07 }, // nRFTXLADE
    {  0x78,       0x20,       0x0E,       0xF1,       0x01 }, // RFTXLENSRC
    {  0x78,       0x20,       0x01,       0xFE,       0x00 }, // RFTXLEN

    // RFTXLFSEQ register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x21,       0xF0,       0x0F,       0x04 }, // RFTXLFSEQPU
    {  0x78,       0x21,       0x0F,       0xF0,       0x00 }, // RFTXLFSEQPD

    // RFTXLTV register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x22,       0x3F,       0xC0,       0x00 }, // RFTXLTV

    // RFRXHEN register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x24,       0x80,       0x7F,       0x07 }, // nRFRXHADE
    {  0x78,       0x24,       0x0E,       0xF1,       0x01 }, // RFRXHENSRC
    {  0x78,       0x24,       0x01,       0xFE,       0x00 }, // RFRXHEN

    // RFRXHFSEQ register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x25,       0xF0,       0x0F,       0x04 }, // RFRXHFSEQPU
    {  0x78,       0x25,       0x0F,       0xF0,       0x00 }, // RFRXHFSEQPD

    // RFRXHTV register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x26,       0x3F,       0xC0,       0x00 }, // RFRXHTV

    // RFTCXOEN register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x28,       0x80,       0x7F,       0x07 }, // nRFTCXOADE
    {  0x78,       0x28,       0x0E,       0xF1,       0x01 }, // RFTCXOENSRC
    {  0x78,       0x28,       0x01,       0xFE,       0x00 }, // RFTCXOEN

    // RFTCXOFSEQ register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x29,       0xF0,       0x0F,       0x04 }, // RFTCXOFSEQPU
    {  0x78,       0x29,       0x0F,       0xF0,       0x00 }, // RFTCXOFSEQPD

    // RFTCXOTV register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x2A,       0x3F,       0xC0,       0x00 }, // RFTCXOLTV

    // LDOAEN register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x2C,       0x80,       0x7F,       0x07 }, // nLDOAADE
    {  0x78,       0x2C,       0x0E,       0xF1,       0x01 }, // LDOAENSRC
    {  0x78,       0x2C,       0x01,       0xFE,       0x00 }, // LDOAEN

    // LDOAFSEQ register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x2D,       0xF0,       0x0F,       0x04 }, // LDOAFSEQPU
    {  0x78,       0x2D,       0x0F,       0xF0,       0x00 }, // LDOAFSEQPD

    // LDOATV register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x2E,       0x3F,       0xC0,       0x00 }, // LDOATV

    // LDOBEN register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x30,       0x80,       0x7F,       0x07 }, // nLDOBADE
    {  0x78,       0x30,       0x0E,       0xF1,       0x01 }, // LDOBENSRC
    {  0x78,       0x30,       0x01,       0xFE,       0x00 }, // LDOBEN

    // LDOBFSEQ register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x31,       0xF0,       0x0F,       0x04 }, // LDOBFSEQPU
    {  0x78,       0x31,       0x0F,       0xF0,       0x00 }, // LDOBFSEQPD

    // LDOBTV register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x32,       0x3F,       0xC0,       0x00 }, // LDOBTV

    // LDOCEN register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x34,       0x80,       0x7F,       0x07 }, // nLDOCADE
    {  0x78,       0x34,       0x0E,       0xF1,       0x01 }, // LDOCENSRC
    {  0x78,       0x34,       0x01,       0xFE,       0x00 }, // LDOCEN

    // LDOCFSEQ register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x35,       0xF0,       0x0F,       0x04 }, // LDOCFSEQPU
    {  0x78,       0x35,       0x0F,       0xF0,       0x00 }, // LDOCFSEQPD

    // LDOCTV register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x36,       0x3F,       0xC0,       0x00 }, // LDOCTV

    // LDODEN register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x38,       0x80,       0x7F,       0x07 }, // nLDODADE
    {  0x78,       0x38,       0x0E,       0xF1,       0x01 }, // LDODENSRC
    {  0x78,       0x38,       0x01,       0xFE,       0x00 }, // LDODEN

    // LDODFSEQ register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x39,       0xF0,       0x0F,       0x04 }, // LDODFSEQPU
    {  0x78,       0x39,       0x0F,       0xF0,       0x00 }, // LDODFSEQPD

    // LDODTV register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x3A,       0x3F,       0xC0,       0x00 }, // LDODTV

    // SIMLTEN register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x3C,       0x80,       0x7F,       0x07 }, // nSIMLTADE
    {  0x78,       0x3C,       0x0E,       0xF1,       0x01 }, // SIMLTENSRC
    {  0x78,       0x3C,       0x01,       0xFE,       0x00 }, // SIMLTEN

    // SIMLTFSEQ register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x3D,       0xF0,       0x0F,       0x04 }, // SIMLTFSEQPU
    {  0x78,       0x3D,       0x0F,       0xF0,       0x00 }, // SIMLTFSEQPD

    // SIMLTTV register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x3E,       0x3F,       0xC0,       0x00 }, // SIMLTTV

    // SRAMEN register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x40,       0x80,       0x7F,       0x07 }, // nSRAMADE
    {  0x78,       0x40,       0x10,       0xEF,       0x04 }, // nOVER1ENSRAM
    {  0x78,       0x40,       0x0E,       0xF1,       0x01 }, // SRAMENSRC
    {  0x78,       0x40,       0x01,       0xFE,       0x00 }, // SRAMEN

    // SRAMFSEQ register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x41,       0xF0,       0x0F,       0x04 }, // SRAMFSEQPU
    {  0x78,       0x41,       0x0F,       0xF0,       0x00 }, // SRAMFSEQPD

    // SDTV1 register
    /* slave addr  addr        mask        clear       shift */
    {  0x68,       0x29,       0x80,       0x7F,       0x07 }, // T1SOST
    {  0x68,       0x29,       0x3F,       0xC0,       0x00 }, // T1SRAM

    // SDTV2 register
    /* slave addr  addr        mask        clear       shift */
    {  0x68,       0x2A,       0x80,       0x7F,       0x07 }, // T2SOST
    {  0x68,       0x2A,       0x3F,       0xC0,       0x00 }, // T2SRAM

    // SRAMCLG register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x42,       0x3F,       0xC0,       0x00 }, // CLGSRAM

    // CARD1EN register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x44,       0x80,       0x7F,       0x07 }, // nCARD1ADE
    {  0x78,       0x44,       0x0E,       0xF1,       0x01 }, // CARD1ENSRC
    {  0x78,       0x44,       0x01,       0xFE,       0x00 }, // CARD1EN

    // CARD1FSEQ register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x45,       0xF0,       0x0F,       0x04 }, // CARD1FSEQPU
    {  0x78,       0x45,       0x0F,       0xF0,       0x00 }, // CARD1FSEQPD

    // CARD1TV register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x46,       0x3F,       0xC0,       0x00 }, // CARD1TV

    // CARD2EN register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x48,       0x80,       0x7F,       0x07 }, // nCARD2ADE
    {  0x78,       0x48,       0x0E,       0xF1,       0x01 }, // CARD2ENSRC
    {  0x78,       0x48,       0x01,       0xFE,       0x00 }, // CARD2EN

    // CARD2FSEQ register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x49,       0xF0,       0x0F,       0x04 }, // CARD2FSEQPU
    {  0x78,       0x49,       0x0F,       0xF0,       0x00 }, // CARD2FSEQPD

    // CARD2TV register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x4A,       0x3F,       0xC0,       0x00 }, // CARD2TV

    // MVTENEN register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x4C,       0x80,       0x7F,       0x07 }, // nMVTADE
    {  0x78,       0x4C,       0x0E,       0xF1,       0x01 }, // MVTENSRC
    {  0x78,       0x4C,       0x01,       0xFE,       0x00 }, // MVTEN

    // MVTFSEQ register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x4D,       0xF0,       0x0F,       0x04 }, // MVTFSEQPU
    {  0x78,       0x4D,       0x0F,       0xF0,       0x00 }, // MVTFSEQPD

    // MDTV1 register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x32,       0x0F,       0xF0,       0x00 }, // T1MVT

    // MDTV2 register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x33,       0x0F,       0xF0,       0x00 }, // T2MVT

    // BIASEN register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x50,       0x80,       0x7F,       0x07 }, // nBIASADE
    {  0x78,       0x50,       0x0E,       0xF1,       0x01 }, // BIASENSRC
    {  0x78,       0x50,       0x01,       0xFE,       0x00 }, // BIASEN

    // BIASFSEQ register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x51,       0xF0,       0x0F,       0x04 }, // BIASFSEQPU
    {  0x78,       0x51,       0x0F,       0xF0,       0x00 }, // BIASFSEQPD

    // BIASTV register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x52,       0x3F,       0xC0,       0x00 }, // BIASTV

    // VBUSEN register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x54,       0x80,       0x7F,       0x07 }, // nVBUSADE
    {  0x78,       0x54,       0x10,       0xEF,       0x04 }, // VBUSVINEN
    {  0x78,       0x54,       0x0E,       0xF1,       0x01 }, // VBUSENSRC
    {  0x78,       0x54,       0x01,       0xFE,       0x00 }, // VBUSEN

    // VBUSFSEQ register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x55,       0xF0,       0x0F,       0x04 }, // VBUSFSEQPU
    {  0x78,       0x55,       0x0F,       0xF0,       0x00 }, // VBUSFSEQPD

    // USBTXRXEN register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x58,       0x80,       0x7F,       0x07 }, // nUSBTXRXADE
    {  0x78,       0x58,       0x10,       0xEF,       0x04 }, // USBTXRXVINEN
    {  0x78,       0x58,       0x0E,       0xF1,       0x01 }, // USBTXRXENSRC
    {  0x78,       0x58,       0x01,       0xFE,       0x00 }, // USBTXRXEN

    // USBTXRXFSEQ register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x59,       0xF0,       0x0F,       0x04 }, // USBTXRXFSEQPU
    {  0x78,       0x59,       0x0F,       0xF0,       0x00 }, // USBTXRXFSEQPD

    //========================================================
    //  M A I N - B A T T E R Y   F A U L T   D E T E C T O R
    //========================================================
    // LBCNFG register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x60,       0x30,       0xCF,       0x04 }, // LHYST
    {  0x78,       0x60,       0x0E,       0xF1,       0x01 }, // LBDAC
    {  0x78,       0x60,       0x01,       0xFE,       0x00 }, // LBEN

    //========================================================
    //  O N / O F F   C O N T R O L L E R
    //========================================================
    // EXTWKSEL register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x00,       0x80,       0x7F,       0x07 }, // HRDRSTEN
    {  0x78,       0x00,       0x10,       0xEF,       0x04 }, // WKVBUS
    {  0x78,       0x00,       0x08,       0xF7,       0x03 }, // WKVAC
    {  0x78,       0x00,       0x04,       0xFB,       0x02 }, // WKALRM1R
    {  0x78,       0x00,       0x02,       0xFD,       0x01 }, // WKALRM0R
    {  0x78,       0x00,       0x01,       0xFE,       0x00 }, // WKSW

    // ON_OFF_IRQ register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x01,       0x80,       0x7F,       0x07 }, // SW_R
    {  0x78,       0x01,       0x40,       0xBF,       0x06 }, // SW_F
    {  0x78,       0x01,       0x20,       0xDF,       0x05 }, // SW_1SEC
    {  0x78,       0x01,       0x10,       0xEF,       0x04 }, // JIG_R
    {  0x78,       0x01,       0x08,       0xF7,       0x03 }, // JIG_F
    {  0x78,       0x01,       0x04,       0xFB,       0x02 }, // SW_3SEC
    {  0x78,       0x01,       0x02,       0xFD,       0x01 }, // MPL_EVENT

    // ON_OFF_IRQ_MASK register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x02,       0x80,       0x7F,       0x07 }, // SW_R_MASK
    {  0x78,       0x02,       0x40,       0xBF,       0x06 }, // SW_F_MASK
    {  0x78,       0x02,       0x20,       0xDF,       0x05 }, // SW_1SEC_MASK
    {  0x78,       0x02,       0x10,       0xEF,       0x04 }, // JIG_R_MASK
    {  0x78,       0x02,       0x08,       0xF7,       0x03 }, // JIG_F_MASK
    {  0x78,       0x02,       0x04,       0xFB,       0x02 }, // SW_3SEC_MASK
    {  0x78,       0x02,       0x02,       0xFD,       0x01 }, // MPL_EVENT_MASK

    // ON_OFF_STAT register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x03,       0x80,       0x7F,       0x07 }, // SW
    {  0x78,       0x03,       0x40,       0xBF,       0x06 }, // SW_1SEC
    {  0x78,       0x03,       0x20,       0xDF,       0x05 }, // JIG
    {  0x78,       0x03,       0x10,       0xEF,       0x04 }, // SW_3SEC

    //========================================================
    //  F L E X I B L E   P O W E R   S E Q U E N C E R
    //========================================================
    // SEQ1CNFG register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x64,       0x38,       0xC7,       0x03 }, // SEQ1T
    {  0x78,       0x64,       0x06,       0xF9,       0x01 }, // SEQ1SRC
    {  0x78,       0x64,       0x01,       0xFE,       0x00 }, // SEQ1EN

    // SEQ2CNFG register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x65,       0x38,       0xC7,       0x03 }, // SEQ2T
    {  0x78,       0x65,       0x06,       0xF9,       0x01 }, // SEQ2SRC
    {  0x78,       0x65,       0x01,       0xFE,       0x00 }, // SEQ2EN

    // SEQ3CNFG register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x66,       0x38,       0xC7,       0x03 }, // SEQ3T
    {  0x78,       0x66,       0x06,       0xF9,       0x01 }, // SEQ3SRC
    {  0x78,       0x66,       0x01,       0xFE,       0x00 }, // SEQ3EN

    // SEQ4CNFG register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x67,       0x38,       0xC7,       0x03 }, // SEQ4T
    {  0x78,       0x67,       0x06,       0xF9,       0x01 }, // SEQ4SRC
    {  0x78,       0x67,       0x01,       0xFE,       0x00 }, // SEQ4EN

    // SEQ5CNFG register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x68,       0x38,       0xC7,       0x03 }, // SEQ5T
    {  0x78,       0x68,       0x06,       0xF9,       0x01 }, // SEQ5SRC
    {  0x78,       0x68,       0x01,       0xFE,       0x00 }, // SEQ5EN

    // SEQ6CNFG register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x69,       0x38,       0xC7,       0x03 }, // SEQ6T
    {  0x78,       0x69,       0x06,       0xF9,       0x01 }, // SEQ6SRC
    {  0x78,       0x69,       0x01,       0xFE,       0x00 }, // SEQ6EN

    // SEQ7CNFG register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x6A,       0x38,       0xC7,       0x03 }, // SEQ7T
    {  0x78,       0x6A,       0x06,       0xF9,       0x01 }, // SEQ7SRC
    {  0x78,       0x6A,       0x01,       0xFE,       0x00 }, // SEQ7EN

    //========================================================
    //  U S B   T R A N S C E I V E R
    //========================================================
    // USBCNFG register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x6C,       0x30,       0xCF,       0x04 }, // USB_PU_EN
    {  0x78,       0x6C,       0x08,       0xF7,       0x03 }, // USB_SUSP
    {  0x78,       0x6C,       0x06,       0xF9,       0x01 }, // USB_EN

    //========================================================
    //  T C X O   B U F F E R
    //========================================================
    // TCXOCNFG register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x74,       0x01,       0xFE,       0x00 }, // TCXOEN

    //========================================================
    //  R E F E R E N C E   O U T P U T (R E F O U T)
    //========================================================
    // REFOUTCNFG register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x70,       0x01,       0xFE,       0x00 }, // REFOUTEN

    //========================================================
    //  R E A L   T I M E   C L O C K (R T C)
    //========================================================
    // R T C registers
    /* slave addr  addr        mask        clear       shift */
    {  0xD0,       0x00,       0x0F,       0xF0,       0x00 }, // RTC_SEC,
    {  0xD0,       0x00,       0x70,       0x8F,       0x04 }, // RTC_10SEC,
    {  0xD0,       0x01,       0x0F,       0xF0,       0x00 }, // RTC_MIN,
    {  0xD0,       0x01,       0x70,       0x8F,       0x04 }, // RTC_10MIN,
    {  0xD0,       0x02,       0x0F,       0xF0,       0x00 }, // RTC_HOURS,
    {  0xD0,       0x02,       0x30,       0xCF,       0x04 }, // RTC_10HOURS,
    {  0xD0,       0x02,       0x80,       0x7F,       0x07 }, // RTC_12_n24,
    {  0xD0,       0x03,       0x07,       0xF8,       0x00 }, // RTC_WEEKDAY,
    {  0xD0,       0x04,       0x0F,       0xF0,       0x00 }, // RTC_DATE,
    {  0xD0,       0x04,       0x30,       0xCF,       0x04 }, // RTC_10DATE,
    {  0xD0,       0x05,       0x0F,       0xF0,       0x00 }, // RTC_MONTH,
    {  0xD0,       0x05,       0x10,       0xEF,       0x04 }, // RTC_10MONTH,
    {  0xD0,       0x06,       0x0F,       0xF0,       0x00 }, // RTC_YEAR,
    {  0xD0,       0x06,       0xF0,       0x0F,       0x04 }, // RTC_10YEAR,
    {  0xD0,       0x07,       0x0F,       0xF0,       0x00 }, // RTC_100YEAR,
    {  0xD0,       0x07,       0xF0,       0x0F,       0x04 }, // RTC_1000YEAR,

    // ALARM0 registers
    /* slave addr  addr        mask        clear       shift */
    {  0xD0,       0x08,       0x0F,       0xF0,       0x00 }, // ALARM0_SEC,
    {  0xD0,       0x08,       0x70,       0x8F,       0x04 }, // ALARM0_10SEC,
    {  0xD0,       0x09,       0x0F,       0xF0,       0x00 }, // ALARM0_MIN,
    {  0xD0,       0x09,       0x70,       0x8F,       0x04 }, // ALARM0_10MIN,
    {  0xD0,       0x0A,       0x0F,       0xF0,       0x00 }, // ALARM0_HOURS,
    {  0xD0,       0x0A,       0x30,       0xCF,       0x04 }, // ALARM0_10HOURS,
    {  0xD0,       0x0A,       0x80,       0x7F,       0x07 }, // ALARM0_12_n24,
    {  0xD0,       0x0B,       0x07,       0xF8,       0x00 }, // ALARM0_WEEKDAY,
    {  0xD0,       0x0C,       0x0F,       0xF0,       0x00 }, // ALARM0_DATE,
    {  0xD0,       0x0C,       0x30,       0xCF,       0x04 }, // ALARM0_10DATE,
    {  0xD0,       0x0D,       0x0F,       0xF0,       0x00 }, // ALARM0_MONTH,
    {  0xD0,       0x0D,       0x10,       0xEF,       0x04 }, // ALARM0_10MONTH,
    {  0xD0,       0x0E,       0x0F,       0xF0,       0x00 }, // ALARM0_YEAR,
    {  0xD0,       0x0E,       0xF0,       0x0F,       0x04 }, // ALARM0_10YEAR,
    {  0xD0,       0x0F,       0x0F,       0xF0,       0x00 }, // ALARM0_100YEAR,
    {  0xD0,       0x0F,       0xF0,       0x0F,       0x04 }, // ALARM0_1000YEAR,

    // ALARM1 registers
    /* slave addr  addr        mask        clear       shift */
    {  0xD0,       0x10,       0x0F,       0xF0,       0x00 }, // ALARM1_SEC,
    {  0xD0,       0x10,       0x70,       0x8F,       0x04 }, // ALARM1_10SEC,
    {  0xD0,       0x11,       0x0F,       0xF0,       0x00 }, // ALARM1_MIN,
    {  0xD0,       0x11,       0x70,       0x8F,       0x04 }, // ALARM1_10MIN,
    {  0xD0,       0x12,       0x0F,       0xF0,       0x00 }, // ALARM1_HOURS,
    {  0xD0,       0x12,       0x30,       0xCF,       0x04 }, // ALARM1_10HOURS,
    {  0xD0,       0x12,       0x80,       0x7F,       0x07 }, // ALARM1_12_n24,
    {  0xD0,       0x13,       0x07,       0xF8,       0x00 }, // ALARM1_WEEKDAY,
    {  0xD0,       0x14,       0x0F,       0xF0,       0x00 }, // ALARM1_DATE,
    {  0xD0,       0x14,       0x30,       0xCF,       0x04 }, // ALARM1_10DATE,
    {  0xD0,       0x15,       0x0F,       0xF0,       0x00 }, // ALARM1_MONTH,
    {  0xD0,       0x15,       0x10,       0xEF,       0x04 }, // ALARM1_10MONTH,
    {  0xD0,       0x16,       0x0F,       0xF0,       0x00 }, // ALARM1_YEAR,
    {  0xD0,       0x16,       0xF0,       0x0F,       0x04 }, // ALARM1_10YEAR,
    {  0xD0,       0x17,       0x0F,       0xF0,       0x00 }, // ALARM1_100YEAR,
    {  0xD0,       0x17,       0xF0,       0x0F,       0x04 }, // ALARM1_1000YEAR,

    // ALARM0_CNTL register
    /* slave addr  addr        mask        clear       shift */
    {  0xD0,       0x18,       0xFF,       0x00,       0x00 }, // ALARM0_CNTL,

    // ALARM1_CNTL register
    /* slave addr  addr        mask        clear       shift */
    {  0xD0,       0x19,       0xFF,       0x00,       0x00 }, // ALARM1_CNTL,

    // RTC_STATUS register
    {  0xD0,       0x1A,       0x80,       0x7F,       0x07 }, // RTC_STATUS_DIV_OK,       
    {  0xD0,       0x1A,       0x40,       0xBF,       0x06 }, // RTC_STATUS_LEAP_OK,      
    {  0xD0,       0x1A,       0x20,       0xDF,       0x05 }, // RTC_STATUS_MON_OK,       
    {  0xD0,       0x1A,       0x10,       0xEF,       0x04 }, // RTC_STATUS_CARY_OK,      
    {  0xD0,       0x1A,       0x08,       0xF7,       0x03 }, // RTC_STATUS_REG_OK,       
    {  0xD0,       0x1A,       0x04,       0xFB,       0x02 }, // RTC_STATUS_ALARM0,       
    {  0xD0,       0x1A,       0x02,       0xFD,       0x01 }, // RTC_STATUS_ALARM1,       
    {  0xD0,       0x1A,       0x01,       0xFE,       0x00 }, // RTC_STATUS_XTAL_FLT,     

    // RTC_CNTL register
    {  0xD0,       0x1B,       0x20,       0xDF,       0x05 }, // RTC_CNTL_ALARM_WP,       
    {  0xD0,       0x1B,       0x10,       0xEF,       0x04 }, // RTC_CNTL_RTC_WP,      
    {  0xD0,       0x1B,       0x08,       0xF7,       0x03 }, // RTC_CNTL_nTCLKWBB_EN,       
    {  0xD0,       0x1B,       0x02,       0xFD,       0x01 }, // RTC_CNTL_nTCLKAP_EN,       
    {  0xD0,       0x1B,       0x01,       0xFE,       0x00 }, // RTC_CNTL_nRTC_EN,     

    // RTC_IRQ register
    {  0xD0,       0x1C,       0x08,       0xF7,       0x03 }, // ALARM0_R,       
    {  0xD0,       0x1C,       0x04,       0xFB,       0x02 }, // ALARM1_R,       

    // RTC_IRQ_MASK register
    {  0xD0,       0x1D,       0x08,       0xF7,       0x03 }, // ALARM0_R_MASK,       
    {  0xD0,       0x1D,       0x04,       0xFB,       0x02 }, // ALARM1_R_MASK,       

    // MPL_CNTL register
    {  0xD0,       0x1E,       0x10,       0xEF,       0x04 }, // EN_MPL,       
    {  0xD0,       0x1E,       0x0C,       0xF3,       0x02 }, // TIME_MPL,       

    {  0x1E,      0x40,       0xBF,       0x06 }, // WTSR_SMPL_CNTL_EN_WTSR,  
    {  0x1E,      0x10,       0xEF,       0x04 }, // WTSR_SMPL_CNTL_EN_SMPL,  
    {  0x1E,      0x0C,       0xF3,       0x02 }, // WTSR_SMPL_CNTL_TIME_SMPL,
    {  0x1E,      0x03,       0xFC,       0x00 }, // WTSR_SMPL_CNTL_TIME_WTSR,

    //========================================================
    //  T O U C H - S C R E E N / A D C   C O N T R O L L E R
    //========================================================
    // TSC_STA_INT register
    /* slave addr  addr        mask        clear       shift */
    {  0x8E,       0x00,       0x10,       0xEF,       0x04 }, // nREF_OK
    {  0x8E,       0x00,       0x08,       0xF7,       0x03 }, // nCONV_NS
    {  0x8E,       0x00,       0x04,       0xFB,       0x02 }, // CONV_S
    {  0x8E,       0x00,       0x02,       0xFD,       0x01 }, // nTS_NS
    {  0x8E,       0x00,       0x01,       0xFE,       0x00 }, // nTS_S

    // TSC_INT_MASK register
    /* slave addr  addr        mask        clear       shift */
    {  0x8E,       0x01,       0x08,       0xF7,       0x03 }, // nCONV_NS_M
    {  0x8E,       0x01,       0x04,       0xFB,       0x02 }, // CONV_S_M
    {  0x8E,       0x01,       0x02,       0xFD,       0x01 }, // nTS_NS_M
    {  0x8E,       0x01,       0x01,       0xFE,       0x00 }, // nTS_S_M

    // TSC_CNFG1 register
    /* slave addr  addr        mask        clear       shift */
    {  0x8E,       0x02,       0x80,       0x7F,       0x07 }, // PU_100_50
    {  0x8E,       0x02,       0x10,       0xEF,       0x04 }, // Four_Wire_CNFG
    {  0x8E,       0x02,       0x03,       0xFC,       0x00 }, // REF_CNFG

    // TSC_CNFG2 register
    /* slave addr  addr        mask        clear       shift */
    {  0x8E,       0x03,       0x80,       0x7F,       0x07 }, // RES_X
    {  0x8E,       0x03,       0x40,       0xBF,       0x06 }, // RES_Y
    {  0x8E,       0x03,       0x20,       0xDF,       0x05 }, // RES_Z1
    {  0x8E,       0x03,       0x10,       0xEF,       0x04 }, // RES_Z2
    {  0x8E,       0x03,       0x08,       0xF7,       0x03 }, // AVG-X
    {  0x8E,       0x03,       0x04,       0xFB,       0x02 }, // AVG_Y
    {  0x8E,       0x03,       0x02,       0xFD,       0x01 }, // AVG_Z1
    {  0x8E,       0x03,       0x01,       0xFE,       0x00 }, // AVG_Z2

    // TSC_CNFG3 register
    /* slave addr  addr        mask        clear       shift */
    {  0x8E,       0x04,       0xC0,       0x3F,       0x06 }, // T_ACQ_X
    {  0x8E,       0x04,       0x30,       0xCF,       0x04 }, // T_ACQ_Y
    {  0x8E,       0x04,       0x0C,       0xF3,       0x02 }, // T_ACQ_Z1
    {  0x8E,       0x04,       0x03,       0xFC,       0x00 }, // T_ACQ_Z2

    // TSC_CNFG4 register
    /* slave addr  addr        mask        clear       shift */
    {  0x8E,       0x05,       0x08,       0xF7,       0x03 }, // D_CV_X
    {  0x8E,       0x05,       0x04,       0xFB,       0x02 }, // D_CV_Y
    {  0x8E,       0x05,       0x02,       0xFD,       0x01 }, // D_CV_Z1
    {  0x8E,       0x05,       0x01,       0xFE,       0x00 }, // D_CV-Z2

    // TSC_RES_CNFG1 register
    /* slave addr  addr        mask        clear       shift */
    {  0x8E,       0x06,       0x80,       0x7F,       0x07 }, // RES_AUX1
    {  0x8E,       0x06,       0x40,       0xBF,       0x06 }, // RES_VBUS
    {  0x8E,       0x06,       0x20,       0xDF,       0x05 }, // RES_VAC
    {  0x8E,       0x06,       0x10,       0xEF,       0x04 }, // RES_MBATT
    {  0x8E,       0x06,       0x08,       0xF7,       0x03 }, // RES_BBATT
    {  0x8E,       0x06,       0x04,       0xFB,       0x02 }, // RES_ICHG
    {  0x8E,       0x06,       0x02,       0xFD,       0x01 }, // RES_TDIE
    {  0x8E,       0x06,       0x01,       0xFE,       0x00 }, // RES_AUX2

    // TSC_AVG_CNFG1 register
    /* slave addr  addr        mask        clear       shift */
    {  0x8E,       0x07,       0x80,       0x7F,       0x07 }, // AVG_AUX1
    {  0x8E,       0x07,       0x40,       0xBF,       0x06 }, // AVG_VBUS
    {  0x8E,       0x07,       0x20,       0xDF,       0x05 }, // AVG_VAC
    {  0x8E,       0x07,       0x10,       0xEF,       0x04 }, // AVG_MBATT
    {  0x8E,       0x07,       0x08,       0xF7,       0x03 }, // AVG_BBATT
    {  0x8E,       0x07,       0x04,       0xFB,       0x02 }, // AVG_ICHG
    {  0x8E,       0x07,       0x02,       0xFD,       0x01 }, // AVG_TDIE
    {  0x8E,       0x07,       0x01,       0xFE,       0x00 }, // AVG_AUX2

    // TSC_ACQ_CNFG1 register
    /* slave addr  addr        mask        clear       shift */
    {  0x8E,       0x08,       0xC0,       0x3F,       0x06 }, // T_ACQ_AUX1
    {  0x8E,       0x08,       0x30,       0xCF,       0x04 }, // T_ACQ_VBUS
    {  0x8E,       0x08,       0x0C,       0xF3,       0x02 }, // T_ACQ_VAC
    {  0x8E,       0x08,       0x03,       0xFC,       0x00 }, // T_ACQ_MBATT

    // TSC_ACQ_CNFG2 register
    /* slave addr  addr        mask        clear       shift */
    {  0x8E,       0x09,       0xC0,       0x3F,       0x06 }, // T_ACQ_BBATT
    {  0x8E,       0x09,       0x30,       0xCF,       0x04 }, // T_ACQ_ICHG
    {  0x8E,       0x09,       0x0C,       0xF3,       0x02 }, // T_ACQ_TDIE
    {  0x8E,       0x09,       0x03,       0xFC,       0x00 }, // T_ACQ_AUX2

    // TSC_ACQ_CNFG3 register
    /* slave addr  addr        mask        clear       shift */
    {  0x8E,       0x0A,       0x80,       0x7F,       0x07 }, // D_CV_AUX1
    {  0x8E,       0x0A,       0x40,       0xBF,       0x06 }, // D_CV_VBUS
    {  0x8E,       0x0A,       0x20,       0xDF,       0x05 }, // D_CV_VAC
    {  0x8E,       0x0A,       0x10,       0xEF,       0x04 }, // D_CV_MBATT
    {  0x8E,       0x0A,       0x08,       0xF7,       0x03 }, // D_CV_BBATT
    {  0x8E,       0x0A,       0x04,       0xFB,       0x02 }, // D_CV_ICHG
    {  0x8E,       0x0A,       0x02,       0xFD,       0x01 }, // D_CV_TDIE
    {  0x8E,       0x0A,       0x01,       0xFE,       0x00 }, // D_CV_AUX2

    // ADC_RESULTS register
    /* slave addr  addr        mask        clear       shift */
    {  0x8E,       0x50,       0xFF,       0x00,       0x00 }, // X_MSB
    {  0x8E,       0x51,       0xF0,       0x0F,       0x04 }, // X_LSB
    {  0x8E,       0x52,       0xFF,       0x00,       0x00 }, // Y_MSB
    {  0x8E,       0x53,       0xF0,       0x0F,       0x04 }, // Y_LSB
    {  0x8E,       0x54,       0xFF,       0x00,       0x00 }, // Z1_MSB
    {  0x8E,       0x55,       0xF0,       0x0F,       0x04 }, // Z1_LSB
    {  0x8E,       0x56,       0xFF,       0x00,       0x00 }, // Z2_MSB
    {  0x8E,       0x57,       0xF0,       0x0F,       0x04 }, // Z2_LSB
    {  0x8E,       0x60,       0xFF,       0x00,       0x00 }, // AUX1_MSB
    {  0x8E,       0x61,       0xF0,       0x0F,       0x04 }, // AUX1_LSB
    {  0x8E,       0x62,       0xFF,       0x00,       0x00 }, // VBUS_MSB
    {  0x8E,       0x63,       0xF0,       0x0F,       0x04 }, // VBUS_LSB
    {  0x8E,       0x64,       0xFF,       0x00,       0x00 }, // VAC_MSB
    {  0x8E,       0x65,       0xF0,       0x0F,       0x04 }, // VAC_LSB
    {  0x8E,       0x66,       0xFF,       0x00,       0x00 }, // MBATT_MSB
    {  0x8E,       0x67,       0xF0,       0x0F,       0x04 }, // MBATT_LSB
    {  0x8E,       0x68,       0xFF,       0x00,       0x00 }, // BBATT_MSB
    {  0x8E,       0x69,       0xF0,       0x0F,       0x04 }, // BBATT_LSB
    {  0x8E,       0x6A,       0xFF,       0x00,       0x00 }, // ICHG_MSB
    {  0x8E,       0x6B,       0xF0,       0x0F,       0x04 }, // ICHG_LSB
    {  0x8E,       0x6C,       0xFF,       0x00,       0x00 }, // TDIE_MSB
    {  0x8E,       0x6D,       0xF0,       0x0F,       0x04 }, // TDIE_LSB
    {  0x8E,       0x6E,       0xFF,       0x00,       0x00 }, // AUX2_MSB
    {  0x8E,       0x6F,       0xF0,       0x0F,       0x04 }, // AUX2_LSB

    // TOUCH-SCREEN CONVERSION COMMAND register
    /* slave addr  addr        mask        clear       shift */
    {  0x8E,       0x80,       0x07,       0xF8,       0x00 }, // X_Drive
    {  0x8E,       0x88,       0x07,       0xF8,       0x00 }, // X_Measurement
    {  0x8E,       0x90,       0x07,       0xF8,       0x00 }, // Y_Drive      
    {  0x8E,       0x98,       0x07,       0xF8,       0x00 }, // Y_Measurement
    {  0x8E,       0xA0,       0x07,       0xF8,       0x00 }, // Z1_Drive      
    {  0x8E,       0xA8,       0x07,       0xF8,       0x00 }, // Z1_Measurement
    {  0x8E,       0xB0,       0x07,       0xF8,       0x00 }, // Z2_Drive      
    {  0x8E,       0xB8,       0x07,       0xF8,       0x00 }, // Z2_Measurement
    {  0x8E,       0xC0,       0x07,       0xF8,       0x00 }, // AUX1_Measurement      
    {  0x8E,       0xC8,       0x07,       0xF8,       0x00 }, // VBUS_Measurement
    {  0x8E,       0xD0,       0x07,       0xF8,       0x00 }, // VAC_Measurement      
    {  0x8E,       0xD8,       0x07,       0xF8,       0x00 }, // MBATT_Measurement
    {  0x8E,       0xE0,       0x07,       0xF8,       0x00 }, // BBATT_Measurement      
    {  0x8E,       0xE8,       0x07,       0xF8,       0x00 }, // ICHG_Measurement
    {  0x8E,       0xF0,       0x07,       0xF8,       0x00 }, // TDIE_Measurement      
    {  0x8E,       0xF8,       0x07,       0xF8,       0x00 }, // AUX2_Measurement

    //========================================================
    //  A U D I O   S U B S Y S T E M
    //========================================================
    // PGA_CNTL1 register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x84,       0xC0,       0x3F,       0x06 }, // VOICE_PGA_CNTL_P
    {  0x78,       0x84,       0x30,       0xCF,       0x04 }, // VOICE_PGA_CNTL_N
    {  0x78,       0x84,       0x08,       0xF7,       0x03 }, // VOICE_IN_CONFIG
    {  0x78,       0x84,       0x03,       0xFC,       0x00 }, // IN1_PGA_CNTL

    // PGA_CNTL2 register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x85,       0xC0,       0x3F,       0x06 }, // IN2_PGA_CNTL
    {  0x78,       0x85,       0x30,       0xCF,       0x04 }, // IN3_PGA_CNTL
    {  0x78,       0x85,       0x0A,       0xF5,       0x01 }, // IN4_PGA_CNTL
    {  0x78,       0x85,       0x01,       0xFE,       0x00 }, // ZDC

    // LMIX_CNTL register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x86,       0x20,       0xDF,       0x05 }, // VOICE_IN_P_LMIX
    {  0x78,       0x86,       0x10,       0xEF,       0x04 }, // VOICE_IN_N_LMIX
    {  0x78,       0x86,       0x08,       0xF7,       0x03 }, // IN1_LMIX
    {  0x78,       0x86,       0x04,       0xFB,       0x02 }, // IN2_LMIX
    {  0x78,       0x86,       0x02,       0xFD,       0x01 }, // IN3_LMIX
    {  0x78,       0x86,       0x01,       0xFE,       0x00 }, // IN4_LMIX

    // RMIX_CNTL register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x87,       0x20,       0xDF,       0x05 }, // VOICE_IN_P_RMIX
    {  0x78,       0x87,       0x10,       0xEF,       0x04 }, // VOICE_IN_N_RMIX
    {  0x78,       0x87,       0x08,       0xF7,       0x03 }, // IN1_RMIX
    {  0x78,       0x87,       0x04,       0xFB,       0x02 }, // IN2_RMIX
    {  0x78,       0x87,       0x02,       0xFD,       0x01 }, // IN3_RMIX
    {  0x78,       0x87,       0x01,       0xFE,       0x00 }, // IN4_RMIX

    // MMIX_CNTL register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x88,       0x03,       0xFC,       0x00 }, // MONO_MIX_CNTL

    // HS_RIGHT_GAIN_CNTL register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x89,       0x1F,       0xE0,       0x00 }, // RIGHT_HS_GAIN

    // HS_LEFT_GAIN_CNTL register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x8A,       0x1F,       0xE0,       0x00 }, // LEFT_HS_GAIN

    // LINE_OUT_GAIN_CNTL register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x8B,       0x1F,       0xE0,       0x00 }, // LINE_OUT_GAIN

    // LS_GAIN_CNTL register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x8C,       0x1F,       0xE0,       0x00 }, // LS_GAIN

    // AUDIO_CNTL register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x8D,       0x80,       0x7F,       0x07 }, // MUTE
    {  0x78,       0x8D,       0x40,       0xBF,       0x06 }, // AUDIO_SHDN
    //{  0x78,       0x8D,       0x20,       0xDF,       0x05 }, // LS_BP_EN
    {  0x78,       0x8D,       0x10,       0xEF,       0x04 }, // AMP_EN_CNTL
    {  0x78,       0x8D,       0x0C,       0xF3,       0x02 }, // CLASS_D_OSC_CNTL
    {  0x78,       0x8D,       0x02,       0xFD,       0x01 }, // HS_MONO_SW

    // AUDIO_ENABLE1 register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x8E,       0x20,       0xDF,       0x05 }, // LS_BP_EN
    {  0x78,       0x8E,       0x08,       0xF7,       0x03 }, // LS_AMP_EN
    {  0x78,       0x8E,       0x04,       0xFB,       0x02 }, // LS_LINEOUT_EN
    {  0x78,       0x8E,       0x03,       0xFC,       0x00 }, // HS_EN

    //========================================================
    //  C H I P   I D E N T I F I C A T I O N
    //========================================================
    // II1RR register
    /* slave addr  addr        mask        clear       shift */
    {  0x68,       0x8E,       0xFF,       0x00,       0x00 }, // IIR1

    // II2RR register
    /* slave addr  addr        mask        clear       shift */
    {  0x68,       0x8F,       0xFF,       0x00,       0x00 }, // IIR2

    //========================================================
    //  I N T E R R U P T   &   S T A T U S
    //========================================================
    // IRQ_STAT register
    /* slave addr  addr        mask        clear       shift */
    {  0x78,       0x98,       0x02,       0xFD,       0x01 }, // SFT_nTIRQ
    {  0x78,       0x98,       0x01,       0xFE,       0x00 } // SFT_nIRQ

};


max8906_regulator_name_type regulator_name[NUMOFREG] =
{
    /* reg_name  active_discharge   ena_src_item    sw_ena_dis */
    {  WBBCORE,  nWCRADE,           WCRENSRC,       WCREN      },
    {  WBBRF,    nWRFADE,           WRFENSRC,       WRFEN      },
    {  APPS,     nAPPSADE,          APPSENSRC,      APPSEN     },
	{  IO,       nIOADE,            IOENSRC,        IOEN       },
    {  MEM,      nMEMADE,           MEMENSRC,       MEMEN      },
    {  WBBMEM,   nWMEMADE,          WMEMENSRC,      WMEMEN     },
    {  WBBIO,    nWIOADE,           WIOENSRC,       WIOEN      },
    {  WBBANA,   nWANAADE,          WANAENSRC,      WANAEN     },
    {  RFRXL,    nRFRXLADE,         RFRXLENSRC,     RFRXLEN    },
    {  RFTXL,    nRFTXLADE,         RFTXLENSRC,     RFTXLEN    },
    {  RFRXH,    nRFRXHADE,         RFRXHENSRC,     RFRXHEN    },
    {  RFTCXO,   nRFTCXOADE,        RFTCXOENSRC,    RFTCXOEN   },
    {  LDOA,     nLDOAADE,          LDOAENSRC,      LDOAEN     },
    {  LDOB,     nLDOBADE,          LDOBENSRC,      LDOBEN     },
    {  LDOC,     nLDOCADE,          LDOCENSRC,      LDOCEN     },
    {  LDOD,     nLDODADE,          LDODENSRC,      LDODEN     },
    {  SIMLT,    nSIMLTADE,         SIMLTENSRC,     SIMLTEN    },
    {  SRAM,     nSRAMADE,          SRAMENSRC,      SRAMEN     },
    {  CARD1,    nCARD1ADE,         CARD1ENSRC,     CARD1EN    },
    {  CARD2,    nCARD2ADE,         CARD2ENSRC,     CARD2EN    },
    {  MVT,      nMVTADE,           MVTENSRC,       MVTEN      },
    {  BIAS,     nBIASADE,          BIASENSRC,      BIASEN     },
    {  VBUS,     nVBUSADE,          VBUSENSRC,      VBUSEN     },
    {  USBTXRX,  nUSBTXRXADE,       USBTXRXENSRC,   USBTXRXEN  }
};


max8906_irq_mask_type max8906_irq_init_array[NUMOFIRQ] = {
    { REG_ON_OFF_IRQ_MASK, ON_OFF_IRQ_M},
    { REG_CHG_IRQ1_MASK,   CHG_IRQ1_M  },
    { REG_CHG_IRQ2_MASK,   CHG_IRQ2_M  },
    { REG_RTC_IRQ_MASK,    RTC_IRQ_M   },
    { REG_TSC_INT_MASK,    TSC_INT_M   }
};

max8906_irq_table_type max8906_irq_table[ENDOFTIRQ+1] = {
    /* ON_OFF_IRQ */
	{ 0, SW_R     , NULL }, // IRQ_SW_R
    { 0, SW_F     , NULL }, // IRQ_SW_F
    { 0, SW_1SEC  , NULL }, // IRQ_SW_1SEC
    { 0, JIG_R    , NULL }, // IRQ_JIG_R
    { 0, JIG_F    , NULL }, // IRQ_JIG_F
    { 0, SW_3SEC  , NULL }, // IRQ_SW_3SEC
    { 0, MPL_EVENT, NULL }, // IRQ_MPL_EVENT
    /* CHG_IRQ1 */
    { 1, VAC_R   , NULL }, // IRQ_VAC_R
    { 1, VAC_F   , NULL }, // IRQ_VAC_F
    { 1, VAC_OVP , NULL }, // IRQ_VAC_OVP
    { 1, VBUS_F  , NULL }, // IRQ_VBUS_F
    { 1, VBUS_R  , NULL }, // IRQ_VBUS_R
    { 1, VBUS_OVP, NULL }, // IRQ_VBUS_OVP
    /* CHG_IRQ2 */
    { 2, CHG_TMR_FAULT, NULL }, // IRQ_CHG_TMR_FAULT
    { 2, CHG_TOPOFF   , NULL }, // IRQ_CHG_TOPOFF
    { 2, CHG_DONE     , NULL }, // IRQ_CHG_DONE
    { 2, CHG_RST      , NULL }, // IRQ_CHG_RST
    { 2, MBATTLOWR    , NULL }, // IRQ_MBATTLOWR
    { 2, MBATTLOWF    , NULL }, // IRQ_MBATTLOWF
    /* RTC_IRQ */
    { 3, ALARM0_R, NULL }, // IRQ_ALARM0_R
    { 3, ALARM1_R, NULL }, // IRQ_ALARM1_R
    /* TSC_STA_INT */
    { 4, nREF_OK , NULL }, // IRQ_nREF_OK
    { 4, nCONV_NS, NULL }, // IRQ_nCONV_NS
    { 4, CONV_S  , NULL }, // IRQ_CONV_S
    { 4, nTS_NS  , NULL }, // IRQ_nTS_NS
    { 4, nTS_S   , NULL }  // IRQ_nTS_S
};

/* MAX8906 voltage table */
static const unsigned int arm_voltage_table[61] = {
	725, 750, 775, 800, 825, 850, 875, 900,		/* 0 ~ 7 */
	925, 950, 975, 1000, 1025, 1050, 1075, 1100,	/* 8 ~ 15 */
	1125, 1150, 1175, 1200, 1225, 1250, 1275, 1300,	/* 16 ~ 23 */
	1325, 1350, 1375, 1400, 1425, 1450, 1475, 1500,	/* 24 ~ 31 */
	1525, 1550, 1575, 1600, 1625, 1650, 1675, 1700,	/* 32 ~ 39 */
	1725, 1750, 1775, 1800, 1825, 1850, 1875, 1900,	/* 40 ~ 47 */
	1925, 1950, 1975, 2000, 2025, 2050, 2075, 2100,	/* 48 ~ 55 */
	2125, 2150, 2175, 2200, 2225,				/* 56 ~ 60 */
};

static const unsigned int int_voltage_table[3] = {
	1050, 1200, 1300,
};

/*===========================================================================

      P O W E R     M A N A G E M E N T     S E C T I O N

===========================================================================*/

/*===========================================================================

FUNCTION Set_MAX8906_PM_REG                                

DESCRIPTION
    This function write the value at the selected register in the PM section.

INPUT PARAMETERS
    reg_num :   selected register in the register address.
    value   :   the value for reg_num.
                This is aligned to the right side of the return value

RETURN VALUE
    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 
    Set_MAX8906_PM_REG(CHGENB, onoff);

===========================================================================*/
boolean Set_MAX8906_PM_REG(max8906_pm_function_type reg_num, byte value)
{
    byte reg_buff;

    if(pmic_read(max8906pm[reg_num].slave_addr, max8906pm[reg_num].addr, &reg_buff, (byte)1) != PMIC_PASS)
    {
        // Register Read command failed
        return FALSE;
    }

    reg_buff = (reg_buff & max8906pm[reg_num].clear) | (value << max8906pm[reg_num].shift);
    if(pmic_write(max8906pm[reg_num].slave_addr, max8906pm[reg_num].addr, &reg_buff, (byte)1) != PMIC_PASS)
    {
        // Register Write command failed
        return FALSE;
    }
    return TRUE;
}

/*===========================================================================

FUNCTION Get_MAX8906_PM_REG                                

DESCRIPTION
    This function read the value at the selected register in the PM section.

INPUT PARAMETERS
    reg_num :   selected register in the register address.

RETURN VALUE
    boolean : 0 = FALSE
              1 = TRUE
    reg_buff :  the value of selected register.
                reg_buff is aligned to the right side of the return value.

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
boolean Get_MAX8906_PM_REG(max8906_pm_function_type reg_num, byte *reg_buff)
{
    byte temp_buff;

    if(pmic_read(max8906pm[reg_num].slave_addr, max8906pm[reg_num].addr, &temp_buff, (byte)1) != PMIC_PASS)
    {
        // Register Read Command failed
        return FALSE;
    }

    *reg_buff = (temp_buff & max8906pm[reg_num].mask) >> max8906pm[reg_num].shift;

    return TRUE;
}


/*===========================================================================

FUNCTION Set_MAX8906_PM_ADDR                                

DESCRIPTION
    This function write the value at the selected register address
    in the PM section.

INPUT PARAMETERS
    max8906_pm_register_type reg_addr    : the register address.
    byte *reg_buff   : the array for data of register to write.
 	byte length      : the number of the register 

RETURN VALUE
    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
boolean Set_MAX8906_PM_ADDR(max8906_pm_register_type reg_addr, byte *reg_buff, byte length)
{

    if(pmic_write(max8906reg[reg_addr].slave_addr, max8906reg[reg_addr].addr, reg_buff, length) != PMIC_PASS)
    {
        // Register Write command failed
        return FALSE;
    }
    return TRUE;

}


/*===========================================================================

FUNCTION Get_MAX8906_PM_ADDR                                

DESCRIPTION
    This function read the value at the selected register address
    in the PM section.

INPUT PARAMETERS
    max8906_pm_register_type reg_addr   : the register address.
    byte *reg_buff  : the array for data of register to write.
 	byte length     : the number of the register 

RETURN VALUE
    byte *reg_buff : the pointer parameter for data of sequential registers
    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
boolean Get_MAX8906_PM_ADDR(max8906_pm_register_type reg_addr, byte *reg_buff, byte length)
{
    if(reg_addr > ENDOFREG)
    {
        // Invalid Read Register
        return FALSE; // return error;
    }
    if(pmic_read(max8906reg[reg_addr].slave_addr, max8906reg[reg_addr].addr, reg_buff, length) != PMIC_PASS)
    {
        // Register Read command failed
        return FALSE;
    }
    return TRUE;
}


/*===========================================================================

FUNCTION Set_MAX8906_TSC_CONV_REG                                

DESCRIPTION
    This function write the value at the selected register for Touch-Screen
    Conversion Command.

INPUT PARAMETERS
    reg_num :   selected register in the register address.
    byte cmd   : a data(bit0~2) of register to write.

RETURN VALUE
    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
boolean Set_MAX8906_TSC_CONV_REG(max8906_pm_function_type reg_num, byte cmd)
{
    byte tsc_buff;

    if((reg_num < X_Drive) || (reg_num > AUX2_Measurement))
    {
        // Incorrect register for TSC
        return FALSE;
    }

	tsc_buff = (max8906pm[reg_num].addr | (max8906pm[reg_num].mask & (byte)(cmd)));

    if(pmic_tsc_write(max8906pm[reg_num].slave_addr, &tsc_buff) != PMIC_PASS)
    {
        // Register Write command failed
        return FALSE;
    }
    return TRUE;
}


/*===========================================================================

FUNCTION Get_MAX8906_TSC_CONV_REG

DESCRIPTION
    This function read the value at the selected register for Touch-Screen
    Conversion Command.

INPUT PARAMETERS
    reg_num :   selected register in the register address.
    byte cmd   : a data(bit0~2) of register to write.

RETURN VALUE
    byte *reg_buff : the pointer parameter for data of sequential registers
    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
boolean Get_MAX8906_TSC_CONV_REG(max8906_pm_function_type reg_num, byte *cmd)
{
    byte tsc_buff = 0;

    if((reg_num < X_Drive) || (reg_num > AUX2_Measurement))
    {
        // Incorrect register for TSC
        return FALSE;
    }
    if(pmic_tsc_read(max8906pm[reg_num].slave_addr, &tsc_buff) != PMIC_PASS)
    {
        // Register Read Command failed
        return FALSE;
    }

    *cmd = (tsc_buff & max8906pm[reg_num].mask);

    return TRUE;
}


/*===========================================================================

FUNCTION Set_MAX8906_PM_Regulator_Active_Discharge

DESCRIPTION
    Enable/Disable Active Discharge for regulators.

INPUT PARAMETERS
    byte onoff : 0 = Active Discharge Enabled
                 1 = Active Discharge Disabled

    dword  regulators      : multiple regulators using "OR"
                             WBBCORE    WBBRF   APPS    IO      MEM     WBBMEM  
                             WBBIO      WBBANA  RFRXL   RFTXL   RFRXH   RFTCXO  
                             LDOA       LDOB    LDOC    LDOD    SIMLT   SRAM    
                             CARD1      CARD2   MVT     BIAS    VBUS    USBTXRX 

RETURN VALUE
    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 
    Set_MAX8906_PM_Regulator_Active_Discharge( 1, WBBCORE | APPS | MEM | WBBMEM);
    If you want to select one or a few regulators, please use Set_MAX8906_PM_REG().
    That is, Set_MAX8906_PM_REG(nWCRADE, 0); // APPS uses SEQ7

===========================================================================*/
boolean Set_MAX8906_PM_Regulator_Active_Discharge(byte onoff, dword regulators)
{
    boolean status;
    int i;

    status = TRUE;

    for(i=0; i < NUMOFREG; i++)
    {
        if(regulator_name[i].reg_name | regulators)
        {
            if(Set_MAX8906_PM_REG(regulator_name[i].active_discharge, onoff) != TRUE)
            {
                status = FALSE;
            }
        }
    }

    return status;
}


/*===========================================================================

FUNCTION Set_MAX8906_PM_Regulator_ENA_SRC

DESCRIPTION
    Control Enable source for regulators from Flexible Power Sequence between
    SEQ1 ~ SEQ7 and software enable.

INPUT PARAMETERS
    flex_power_seq_type sequencer : selected Sequence number for each regulator
                                         SEQ1 ~ SEQ7 or SW_CNTL

    dword  regulators      : multiple regulators using "OR"
                             WBBCORE    WBBRF   APPS    IO      MEM     WBBMEM  
                             WBBIO      WBBANA  RFRXL   RFTXL   RFRXH   RFTCXO  
                             LDOA       LDOB    LDOC    LDOD    SIMLT   SRAM    
                             CARD1      CARD2   MVT     BIAS    VBUS    USBTXRX 

RETURN VALUE
    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 
    Set_MAX8906_PM_Regulator_ENA_SRC( SEQ1, WBBCORE | APPS | MEM | WBBMEM);
    If you want to select one or a few regulators, please use Set_MAX8906_PM_REG().
    That is, Set_MAX8906_PM_REG(APPSENSRC, 0x06); // APPS uses SEQ7

===========================================================================*/
boolean Set_MAX8906_PM_Regulator_ENA_SRC(flex_power_seq_type sequencer, dword regulators)
{
    boolean status;
    int i;

    status = TRUE;

    for(i=0; i < NUMOFREG; i++)
    {
        if(regulator_name[i].reg_name | regulators)
        {
            if(Set_MAX8906_PM_REG(regulator_name[i].ena_src_item, sequencer) != TRUE)
            {
                status = FALSE;
            }
        }
    }

    return status;
}


/*===========================================================================

FUNCTION Set_MAX8906_PM_Regulator_SW_Enable

DESCRIPTION
    Enable/Disable Active Discharge for regulators.

INPUT PARAMETERS
    byte onoff : 
                 0 = Disabled
                 1 = Enabled

    dword  regulators      : multiple regulators using "OR"
                             WBBCORE    WBBRF   APPS    IO      MEM     WBBMEM  
                             WBBIO      WBBANA  RFRXL   RFTXL   RFRXH   RFTCXO  
                             LDOA       LDOB    LDOC    LDOD    SIMLT   SRAM    
                             CARD1      CARD2   MVT     BIAS    VBUS    USBTXRX 

RETURN VALUE
    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 
    Set_MAX8906_PM_Regulator_Active_Discharge( 1, WBBCORE | APPS | MEM | WBBMEM);
    If you want to select one or a few regulators, please use Set_MAX8906_PM_REG().
    That is, Set_MAX8906_PM_REG(nWCRADE, 0); // APPS uses SEQ7

===========================================================================*/
boolean Set_MAX8906_PM_Regulator_SW_Enable(byte onoff, dword regulators)
{
    boolean status;
    int i;

    status = TRUE;

    for(i=0; i < NUMOFREG; i++)
    {
        if(regulator_name[i].reg_name | regulators)
        {
            if(Set_MAX8906_PM_REG(regulator_name[i].sw_ena_dis, onoff) != TRUE)
            {
                status = FALSE;
            }
        }
    }

    return status;
}


/*===========================================================================

FUNCTION Set_MAX8906_PM_PWR_SEQ_Timer_Period

DESCRIPTION
    Control the Timer Period between each sequencer event for Flexible Power
    Sequencer.

INPUT PARAMETERS
    max8906_pm_function_type cntl_item : selected Sequence number for Timer Period
                                         SEQ1T ~ SEQ7T

    timer_period_type  value           : T_Period_20uS
                                         T_Period_40uS
                                         T_Period_80uS
                                         T_Period_160uS
                                         T_Period_320uS
                                         T_Period_640uS
                                         T_Period_1280uS
                                         T_Period_2560uS

RETURN VALUE
    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
boolean Set_MAX8906_PM_PWR_SEQ_Timer_Period(max8906_pm_function_type cntl_item, timer_period_type value)
{
    boolean status;

    status = TRUE;
    switch(cntl_item)
    {
    case SEQ1T:    case SEQ2T:
    case SEQ3T:    case SEQ4T:
    case SEQ5T:    case SEQ6T:
    case SEQ7T:
        if(Set_MAX8906_PM_REG(cntl_item, value) != TRUE)
        {
            status =FALSE;
        }
        break;
    default:
        status = FALSE;
    }

    return status;
}


/*===========================================================================

FUNCTION Get_MAX8906_PM_PWR_SEQ_Timer_Period

DESCRIPTION
    Read the Timer Period between each sequencer event for Flexible Power
    Sequencer.

INPUT PARAMETERS
    max8906_pm_function_type cntl_item : selected Sequence number for Timer Period
                                         SEQ1T, SEQ2T, SEQ3T, SEQ4T,
                                         SEQ5T, SEQ6T, SEQ7T

RETURN VALUE
    timer_period_type  value           : T_Period_20uS
                                         T_Period_40uS
                                         T_Period_80uS
                                         T_Period_160uS
                                         T_Period_320uS
                                         T_Period_640uS
                                         T_Period_1280uS
                                         T_Period_2560uS

    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
boolean Get_MAX8906_PM_PWR_SEQ_Timer_Period(max8906_pm_function_type cntl_item, timer_period_type *value)
{
    boolean status;

    status = TRUE;
    switch(cntl_item)
    {
    case SEQ1T:    case SEQ2T:
	case SEQ3T:    case SEQ4T:
    case SEQ5T:    case SEQ6T:
	case SEQ7T:
		if(Get_MAX8906_PM_REG(cntl_item, (byte *)value) != TRUE)
        {
            status =FALSE;
        }
        break;
    default:
        status = FALSE;
    }

    return status;
}


/*===========================================================================

FUNCTION Set_MAX8906_PM_PWR_SEQ_Ena_Src

DESCRIPTION
    Control the enable source for Flexible Power Sequencer.

INPUT PARAMETERS
    max8906_pm_function_type cntl_item : selected Sequence number for enable source
                                         SEQ1SRC, SEQ2SRC, SEQ3SRC, SEQ4SRC, 
                                         SEQ5SRC, SEQ6SRC, SEQ7SRC

    byte value                         : 
                            SEQ1SRC = 0 : SYSEN hardware input
                                      1 : PWREN hardware input
                                      2 : SEQ1EN software bit
                                         
                            SEQ2SRC = 0 : PWREN hardware input
                                      1 : SYSEN hardware input
                                      2 : SEQ2EN software bit
                                         
                            SEQ3SRC = 0 : WBBEN hardware input
                                      1 : reserved
                                      2 : SEQ3EN software bit
                                         
                            SEQ4SRC = 0 : TCXOEN hardware input
                                      1 : reserved
                                      2 : SEQ4EN software bit
                                         
                            SEQ5SRC = 0 : RFRXEN hardware input
                                      1 : reserved
                                      2 : SEQ5EN software bit
                                         
                            SEQ6SRC = 0 : RFTXEN hardware input
                                      1 : reserved
                                      2 : SEQ6EN software bit
                                         
                            SEQ7SRC = 0 : ENA hardware input
                                      1 : reserved
                                      2 : SEQ7EN software bit
                                         

RETURN VALUE
    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
boolean Set_MAX8906_PM_PWR_SEQ_Ena_Src(max8906_pm_function_type cntl_item, byte value)
{
    boolean status;

    status = TRUE;
    switch(cntl_item)
    {
    case SEQ1SRC:    case SEQ2SRC:
    case SEQ3SRC:    case SEQ4SRC:
    case SEQ5SRC:    case SEQ6SRC:
    case SEQ7SRC:
        if(Set_MAX8906_PM_REG(cntl_item, value) != TRUE)
        {
            status =FALSE;
        }
        break;
    default:
        status = FALSE;
    }

    return status;
}


/*===========================================================================

FUNCTION Get_MAX8906_PM_PWR_SEQ_Ena_Src

DESCRIPTION
    Read the enable source for Flexible Power Sequencer.

INPUT PARAMETERS
    max8906_pm_function_type cntl_item : selected Sequence number for enable source
                                         SEQ1SRC, SEQ2SRC, SEQ3SRC, SEQ4SRC, 
                                         SEQ5SRC, SEQ6SRC, SEQ7SRC

RETURN VALUE
    byte value                         : 
                            SEQ1SRC = 0 : SYSEN hardware input
                                      1 : PWREN hardware input
                                      2 : SEQ1EN software bit
                                         
                            SEQ2SRC = 0 : PWREN hardware input
                                      1 : SYSEN hardware input
                                      2 : SEQ2EN software bit
                                         
                            SEQ3SRC = 0 : WBBEN hardware input
                                      1 : reserved
                                      2 : SEQ3EN software bit
                                         
                            SEQ4SRC = 0 : TCXOEN hardware input
                                      1 : reserved
                                      2 : SEQ4EN software bit
                                         
                            SEQ5SRC = 0 : RFRXEN hardware input
                                      1 : reserved
                                      2 : SEQ5EN software bit
                                         
                            SEQ6SRC = 0 : RFTXEN hardware input
                                      1 : reserved
                                      2 : SEQ6EN software bit
                                         
                            SEQ7SRC = 0 : ENA hardware input
                                      1 : reserved
                                      2 : SEQ7EN software bit

    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
boolean Get_MAX8906_PM_PWR_SEQ_Ena_Src(max8906_pm_function_type cntl_item, byte *value)
{
    boolean status;

    status = TRUE;
    switch(cntl_item)
    {
    case SEQ1SRC:    case SEQ2SRC:
    case SEQ3SRC:    case SEQ4SRC:
    case SEQ5SRC:    case SEQ6SRC:
    case SEQ7SRC:
        if(Get_MAX8906_PM_REG(cntl_item, value) != TRUE)
        {
            status =FALSE;
        }
        break;
    default:
        status = FALSE;
    }

    return status;
}


/*===========================================================================

FUNCTION Set_MAX8906_PM_PWR_SEQ_SW_Enable

DESCRIPTION
    Control the enable source for Flexible Power Sequencer.

INPUT PARAMETERS
    max8906_pm_function_type cntl_item : selected Sequence number for enable source
                                         SEQ1EN, SEQ2EN, SEQ3EN, SEQ4EN, 
                                         SEQ5EN, SEQ6EN, SEQ7EN

    byte value :       0 = Disable regulators
                       1 = Enable regulators

RETURN VALUE
    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
boolean Set_MAX8906_PM_PWR_SEQ_SW_Enable(max8906_pm_function_type cntl_item, byte value)
{
    boolean status;

    status = TRUE;
    switch(cntl_item)
    {
    case SEQ1EN:    case SEQ2EN:
    case SEQ3EN:    case SEQ4EN:
    case SEQ5EN:    case SEQ6EN:
    case SEQ7EN:
        if(Set_MAX8906_PM_REG(cntl_item, value) != TRUE)
        {
            status =FALSE;
        }
        break;
    default:
        status = FALSE;
    }

    return status;
}


/*===========================================================================

FUNCTION Get_MAX8906_PM_PWR_SEQ_SW_Enable

DESCRIPTION
    Read the enable source for Flexible Power Sequencer.

INPUT PARAMETERS
    max8906_pm_function_type cntl_item : selected Sequence number for enable source
                                         SEQ1EN, SEQ2EN, SEQ3EN, SEQ4EN, 
                                         SEQ5EN, SEQ6EN, SEQ7EN


RETURN VALUE
    byte value :       0 = Disable regulators
                       1 = Enable regulators

    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 


===========================================================================*/
boolean Get_MAX8906_PM_PWR_SEQ_SW_Enable(max8906_pm_function_type cntl_item, byte *value)
{
    boolean status;

    status = TRUE;
    switch(cntl_item)
    {
    case SEQ1EN:    case SEQ2EN:
    case SEQ3EN:    case SEQ4EN:
    case SEQ5EN:    case SEQ6EN:
    case SEQ7EN:
        if(Get_MAX8906_PM_REG(cntl_item, value) != TRUE)
        {
            status =FALSE;
        }
        break;
    default:
        status = FALSE;
    }

    return status;
}


/*===========================================================================
 T O U C H   S C R E E N
===========================================================================*/
void Set_MAX8906_PM_TSC_init()
{
    // set interrupt & config

    // set averaging for average check

    // 
}

void Set_MAX8906_PM_TSC_detect_isr()
{
    // send TSC detect event
}


/*===========================================================================

FUNCTION Set_MAX8906_TSC_measurement

DESCRIPTION
    Read x, y, z1, and z2 coordinates for Touch_Screen.
    (z1 and z2 is used for 5-wire mode.)

INPUT PARAMETERS

RETURN VALUE
    tsc_coordinates : return value for inserting x, y, z1, and z2 coordinates

    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 


===========================================================================*/
boolean Set_MAX8906_TSC_measurement(max8906_coordinates_type *tsc_coordinates)
{
    boolean status;
    byte tsc_buff;
    byte result[2];
    
    // set x drive & measurement
	Set_MAX8906_TSC_CONV_REG(X_Drive, CONT);
	Set_MAX8906_TSC_CONV_REG(X_Measurement, CONT);

    // X Measurement done?
    do {
        status = Get_MAX8906_PM_REG(nCONV_NS, &tsc_buff);
    } while ( !(status && (tsc_buff & nCONV_NS_M)) );

    // set y drive & measurement
	Set_MAX8906_TSC_CONV_REG(Y_Drive, CONT);
	Set_MAX8906_TSC_CONV_REG(Y_Measurement, CONT);

    // Y Measurement done?
    do {
        status = Get_MAX8906_PM_REG(nCONV_NS, &tsc_buff);
    } while ( !(status && (tsc_buff & nCONV_NS_M)) );

#ifdef TSC_5_WIRE_MODE
    // set z1 drive & measurement
    Set_MAX8906_TSC_CONV_REG(Z1_Drive, CONT);
    Set_MAX8906_TSC_CONV_REG(Z1_Measurement, CONT);

    // Y Measurement done?
    do {
        status = Get_MAX8906_PM_REG(nCONV_NS, &tsc_buff);
    } while ( !(status && (tsc_buff & nCONV_NS_M)) );

    // set z2 drive & measurement
    Set_MAX8906_TSC_CONV_REG(Z2_Drive, CONT);
    Set_MAX8906_TSC_CONV_REG(Z2_Measurement, CONT);

    // Y Measurement done?
    do {
        status = Get_MAX8906_PM_REG(nCONV_NS, &tsc_buff);
    } while ( !(status && (tsc_buff & nCONV_NS_M)) );

#endif

    // read irq reg for clearing interrupt

    // read x & y coordinates if nTIRQ is low
    status = Get_MAX8906_PM_REG(nCONV_NS, &tsc_buff);
	if(tsc_buff & nTS_NS_MASK)
	{
        // No touch event detected on sensor now
        return FALSE;
    }

    if(Get_MAX8906_PM_ADDR(REG_ADC_X_MSB, result, 2) != TRUE)
    {
        if(Get_MAX8906_PM_ADDR(REG_ADC_X_MSB, result, 2) != TRUE)
        {
            // X can't be read
            return FALSE;
        }
    }
    tsc_coordinates->x = (word)(result[0]<<4) | (word)(result[1]>>4);

    if(Get_MAX8906_PM_ADDR(REG_ADC_Y_MSB, result, 2) != TRUE)
    {
        if(Get_MAX8906_PM_ADDR(REG_ADC_Y_MSB, result, 2) != TRUE)
        {
            // Y can't be read
            return FALSE;
        }
    }
    tsc_coordinates->y = (word)(result[0]<<4) | (word)(result[1]>>4);

#ifdef TSC_5_WIRE_MODE
    if(Get_MAX8906_PM_ADDR(REG_ADC_Z1_MSB, result, 2) != TRUE)
    {
        if(Get_MAX8906_PM_ADDR(REG_ADC_Z1_MSB, result, 2) != TRUE)
        {
            // Z1 can't be read
            return FALSE;
        }
    }
    tsc_coordinates->z1 = (word)(result[0]<<4) | (word)(result[1]>>4);

    if(Get_MAX8906_PM_ADDR(REG_ADC_Z2_MSB, result, 2) != TRUE)
    {
        if(Get_MAX8906_PM_ADDR(REG_ADC_Z2_MSB, result, 2) != TRUE)
        {
            // Z2 can't be read
            return FALSE;
        }
    }
    tsc_coordinates->z2 = (word)(result[0]<<4) | (word)(result[1]>>4);
#endif

    // set x drive & measurement
    Set_MAX8906_TSC_CONV_REG(X_Drive, NON_EN_REF_CONT);
    Set_MAX8906_TSC_CONV_REG(X_Measurement, NON_EN_REF_CONT);

    // set y drive & measurement
    Set_MAX8906_TSC_CONV_REG(Y_Drive, NON_EN_REF_CONT);
    Set_MAX8906_TSC_CONV_REG(Y_Measurement, NON_EN_REF_CONT);

#ifdef TSC_5_WIRE_MODE
    // set z1 drive & measurement
    Set_MAX8906_TSC_CONV_REG(Z1_Drive, CONT);
    Set_MAX8906_TSC_CONV_REG(Z1_Measurement, CONT);

    // set z2 drive & measurement
    Set_MAX8906_TSC_CONV_REG(Z2_Drive, CONT);
    Set_MAX8906_TSC_CONV_REG(Z2_Measurement, CONT);
#endif

    return TRUE;
}


/*===========================================================================*/
boolean change_vcc_arm(int voltage)
{
	byte reg_value = 0;

	pr_info(PREFIX "%s:I: voltage: %d\n", __func__, voltage);

	if(voltage < arm_voltage_table[0] || voltage > arm_voltage_table[60]) {
		pr_err(PREFIX "%s:E: invalid voltage: %d\n", __func__, voltage);
		return FALSE;
	}

	if (voltage % 25) {
		pr_err(PREFIX "%s:E: invalid voltage: %d\n", __func__, voltage);
		return FALSE;
	}

	if(voltage < arm_voltage_table[16]) { //725 ~ 1100 mV
		for(reg_value = 0; reg_value <= 15; reg_value++) {
			if(arm_voltage_table[reg_value] == voltage) break;
		}
	}
	else if(voltage < arm_voltage_table[32]) {	//1125 ~ 1500 mV
		for(reg_value = 16; reg_value <= 31; reg_value++) {
			if(arm_voltage_table[reg_value] == voltage) break;
		}
	}
	else if(voltage < arm_voltage_table[48]) {	//1525 ~ 1900 mV
		for(reg_value = 32; reg_value <= 47; reg_value++) {
			if(arm_voltage_table[reg_value] == voltage) break;
		}
	}
	else if(voltage <= arm_voltage_table[60]) {	//1925 ~ 2225 mV
		for(reg_value = 48; reg_value <= 60; reg_value++) {
			if(arm_voltage_table[reg_value] == voltage) break;
		}
	}
	else {
		pr_err(PREFIX "%s:E: invalid voltage: %d\n", __func__, voltage);
		return FALSE;
	}

 	Set_MAX8906_PM_REG(T1AOST, 0x0);

	Set_MAX8906_PM_REG(T1APPS, reg_value);

	/* Start Voltage Change */
	Set_MAX8906_PM_REG(AGO, 0x01);

	return TRUE;
}

boolean change_vcc_internal(int voltage)
{	
	byte reg_value = 0;

	pr_info(PREFIX "%s:I: voltage: %d\n", __func__, voltage);

	if(voltage < int_voltage_table[0] || voltage > int_voltage_table[2]) {
		pr_err(PREFIX "%s:E: invalid voltage: %d\n", __func__, voltage);
		return FALSE;
	}

	for(reg_value = 0; reg_value <= 2; reg_value++) {
		if(int_voltage_table[reg_value] == voltage) break;
	}

	if (reg_value > 2) {
		pr_err(PREFIX "%s:E: invalid voltage: %d\n", __func__, voltage);
		return FALSE;
	}
	if (!Set_MAX8906_PM_REG(MEMDVM, reg_value)) {
		pr_err(PREFIX "%s:E: set pmic reg fail(%d)\n", __func__, reg_value);
		return FALSE;
	}

	return TRUE;
}

boolean set_pmic(pmic_pm_type pm_type, int value)
{
	boolean rc = FALSE;
	switch (pm_type) {
	case VCC_ARM:
		rc = change_vcc_arm(value);
		break;
	case VCC_INT:
		rc = change_vcc_internal(value);
		break;
	default:
		pr_err(PREFIX "%s:E: invalid pm_type: %d\n", __func__, pm_type);
		rc = FALSE;
		break;
	}
	return rc;
}

/*===========================================================================

      R T C     S E C T I O N

===========================================================================*/
/*===========================================================================

FUNCTION Set_MAX8906_RTC                                

DESCRIPTION
    This function write the value at the selected register address
    in the RTC section.

INPUT PARAMETERS
    max8906_rtc_cmd_type :     TIMEKEEPER = timekeeper register 0x0~0x7
                               ALARM0     = alarm0 register 0x8~0xF
                               ALARM1     = alarm1 register 0x10~0x18

    byte* max8906_rtc_ptr : the write value for registers.

RETURN VALUE
    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
boolean Set_MAX8906_RTC(max8906_rtc_cmd_type rtc_cmd,byte *max8906_rtc_ptr)
{
    byte reg;

    reg = (byte)rtc_cmd * 8;
#if 0 
	if(pmic_rtc_write(reg, max8906_rtc_ptr, (byte)8) != PMIC_PASS)
    {
        //Write RTC failed
        return FALSE;
    }
#endif    
	return TRUE;
}

/*===========================================================================

FUNCTION Get_MAX8906_RTC                                

DESCRIPTION
    This function read the value at the selected register address
    in the RTC section.

INPUT PARAMETERS
    max8906_rtc_cmd_type :     TIMEKEEPER = timekeeper register 0x0~0x7
                               ALARM0     = alarm0 register 0x8~0xF
                               ALARM1     = alarm1 register 0x10~0x18

    byte* max8906_rtc_ptr : the return value for registers.

RETURN VALUE
    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
boolean Get_MAX8906_RTC(max8906_rtc_cmd_type rtc_cmd, byte *max8906_rtc_ptr)
{
    byte reg;

    reg = (byte)rtc_cmd * 8;
#if 0  
	if(pmic_rtc_read(reg, max8906_rtc_ptr, (byte)8) != PMIC_PASS)
    {
        // Read RTC failed
        return FALSE;
    }
#endif    
	return TRUE;
}



/*===========================================================================

      I R Q    R O U T I N E

===========================================================================*/
/*===========================================================================

FUNCTION MAX8906_IRQ_init                                

DESCRIPTION
    Initialize the IRQ Mask register for the IRQ handler.

INPUT PARAMETERS

RETURN VALUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
void MAX8906_IRQ_init()
{
    int i;

    for(i=0; i<NUMOFIRQ; i++)
    {
        if(pmic_write(max8906reg[max8906_irq_init_array[i].irq_reg].slave_addr, 
                   max8906reg[max8906_irq_init_array[i].irq_reg].addr, &max8906_irq_init_array[i].irq_mask, (byte)1) != PMIC_PASS)
        {
            // Write IRQ Mask failed
        }
    }
}


/*===========================================================================

FUNCTION Set_MAX8906_PM_IRQ                                

DESCRIPTION
    When some irq mask is changed, this function can be used.
    If you send max8906_isr as null(0) point, it means that the irq is masked.
    If max8906_isr points some functions, it means that the irq is unmasked.

INPUT PARAMETERS
    irq_name                   : IRQ Mask register number
    void (*max8906_isr)(void) : If irq is happened, then this routine is running.

RETURN VALUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
boolean Set_MAX8906_PM_IRQ(max8906_irq_type irq_name, void (*max8906_isr)(void))
{
    byte reg_buff;
    byte value;

    if(pmic_read(max8906pm[max8906_irq_table[irq_name].irq_reg].slave_addr, 
                 max8906pm[max8906_irq_table[irq_name].irq_reg].addr, &reg_buff, (byte)1) != PMIC_PASS)
    {
        // Read IRQ register failed
        return FALSE;
    }

    if(max8906_isr == 0)
    {   // if max8906_isr is a null pointer
        value = 1;
        reg_buff = (reg_buff | max8906pm[max8906_irq_table[irq_name].irq_reg].mask);
        max8906_irq_table[irq_name].irq_ptr = NULL;
    }
    else
    {
        value = 0;
        reg_buff = (reg_buff & max8906pm[max8906_irq_table[irq_name].irq_reg].clear);
        max8906_irq_table[irq_name].irq_ptr = max8906_isr;
    }

    if(pmic_write(max8906pm[max8906_irq_table[irq_name].irq_reg].slave_addr, 
                  max8906pm[max8906_irq_table[irq_name].irq_reg].addr, &reg_buff, (byte)1) != PMIC_PASS)
    {
        // Write IRQ register failed
        return FALSE;
    }

    return TRUE;
}


/*===========================================================================

FUNCTION MAX8906_PM_IRQ_isr                                

DESCRIPTION
    When nIRQ pin is asserted, this isr routine check the irq bit and then
    proper function is called.
    Irq register can be set although irq is masked.
    So, the isr routine shoud check the irq mask bit, too.

INPUT PARAMETERS

RETURN VALUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
void MAX8906_PM_IRQ_isr(void)
{
    byte pm_irq_reg[4];
    byte pm_irq_mask[4];
    byte irq_name;
    if(pmic_read(max8906reg[REG_ON_OFF_IRQ].slave_addr, max8906reg[REG_ON_OFF_IRQ].addr, &pm_irq_reg[0], (byte)1) != PMIC_PASS)
    {
        MSG_HIGH("IRQ register isn't read", 0, 0, 0);
        return; // return error
    }
    if(pmic_read(max8906reg[REG_CHG_IRQ1].slave_addr, max8906reg[REG_CHG_IRQ1].addr, &pm_irq_reg[1], (byte)2) != PMIC_PASS)
    {
        MSG_HIGH("IRQ register isn't read", 0, 0, 0);
        return; // return error
    }
    if(pmic_read(max8906reg[REG_RTC_IRQ].slave_addr, max8906reg[REG_RTC_IRQ].addr, &pm_irq_reg[3], (byte)1) != PMIC_PASS)
    {
        MSG_HIGH("IRQ register isn't read", 0, 0, 0);
        return; // return error
    }

    if(pmic_read(max8906reg[REG_ON_OFF_IRQ_MASK].slave_addr, max8906reg[REG_ON_OFF_IRQ].addr, &pm_irq_reg[0], (byte)1) != PMIC_PASS)
    {
        MSG_HIGH("IRQ register isn't read", 0, 0, 0);
        return; // return error
    }
    if(pmic_read(max8906reg[REG_CHG_IRQ1_MASK].slave_addr, max8906reg[REG_CHG_IRQ1].addr, &pm_irq_reg[1], (byte)2) != PMIC_PASS)
    {
        MSG_HIGH("IRQ register isn't read", 0, 0, 0);
        return; // return error
    }
    if(pmic_read(max8906reg[REG_RTC_IRQ_MASK].slave_addr, max8906reg[REG_RTC_IRQ].addr, &pm_irq_reg[3], (byte)1) != PMIC_PASS)
    {
        MSG_HIGH("IRQ register isn't read", 0, 0, 0);
        return; // return error
    }

    for(irq_name = START_IRQ; irq_name <= ENDOFIRQ; irq_name++)
    {
        if( (pm_irq_reg[max8906_irq_table[irq_name].item_num] & max8906pm[max8906_irq_table[irq_name].irq_reg].mask)
            && ( (pm_irq_mask[max8906_irq_table[irq_name].item_num] & max8906pm[max8906_irq_table[irq_name].irq_reg].mask) == 0)
            && (max8906_irq_table[irq_name].irq_ptr != 0) )
        {
            (max8906_irq_table[irq_name].irq_ptr)();
        }
    }
}


/*===========================================================================

FUNCTION MAX8906_PM_TIRQ_isr                                

DESCRIPTION
    When nTIRQ pin is asserted for Touch-Screen, this isr routine check the irq bit and then
    proper function is called.
    Irq register can be set although irq is masked.
    So, the isr routine shoud check the irq mask bit, too.

INPUT PARAMETERS

RETURN VALUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
void MAX8906_PM_TIRQ_isr(void)
{
    byte pm_irq_reg[4];
    byte pm_irq_mask[4];
    byte irq_name;

    if(pmic_read(max8906reg[REG_TSC_STA_INT].slave_addr, max8906reg[REG_TSC_STA_INT].addr, &pm_irq_reg[0], (byte)1) != PMIC_PASS)
    {
        MSG_HIGH("IRQ register isn't read", 0, 0, 0);
	 	return; // return error
    }

    if(pmic_read(max8906reg[REG_TSC_INT_MASK].slave_addr, max8906reg[REG_TSC_INT_MASK].addr, &pm_irq_reg[0], (byte)1) != PMIC_PASS)
    {
	 	MSG_HIGH("IRQ register isn't read", 0, 0, 0);
	 	return; // return error
    }


    for(irq_name = START_TIRQ; irq_name <= ENDOFTIRQ; irq_name++)
    {
        if( (pm_irq_reg[max8906_irq_table[irq_name].item_num] & max8906pm[max8906_irq_table[irq_name].irq_reg].mask)
            && ( (pm_irq_mask[max8906_irq_table[irq_name].item_num] & max8906pm[max8906_irq_table[irq_name].irq_reg].mask) == 0)
            && (max8906_irq_table[irq_name].irq_ptr != 0) )
        {
            (max8906_irq_table[irq_name].irq_ptr)();
        }
    }
}



/*===========================================================================

      I N I T    R O U T I N E

===========================================================================*/
/*===========================================================================

FUNCTION MAX8906_PM_init

DESCRIPTION
    When power up, MAX8906_PM_init will initialize the MAX8906 for each part

INPUT PARAMETERS

RETURN VALUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
void MAX8906_PM_init(void)
{
    // Set the LDO voltage
    // When PWRSL is connected to GND, the default voltage of MAX8906 is like this.
    // LDO1 = 2.6V    LDO2  = 2.6V   LDO3 = 2.85V   LDO4 = 2.85V
    // LDO5 = 2.85V    LDO6  = 2.85V  LDO7 = 2.85V   LDO8 = 2.85V
    // LDO9 = 2.85V   LDO10 = 2.85V  LDO11 = 1.8V   LDO14 = 2.80V

    // Set the LDO on/off function
    // the default value for MAX8906
    // SD1   = On   SD2   = On   SD3  = Off  LDO1  = On   LDO2   = On
    // LDO3  = Off  LDO4  = Off  LDO5 = Off  LDO6  = Off  LDO7   = Off
    // LDO8  = Off  LDO9 = Off  LDO10 = Off  LDO11 = Off  LDO12  = Off
    // LDO11 = Off  LDO12 = On  LDO13 = Off  LDO14 = Off  REFOUT = On 

     // if you use USB transceiver, Connect internal 1.5k pullup resistor.
     //Set_MAX8906_PM_USB_CNTL(USB_PU_EN,1);
     // enable SMPL function
     //Set_MAX8906_RTC_REG(WTSR_SMPL_CNTL_EN_SMPL, (byte)1);
     // enable WTSR function for soft reset
     //Set_MAX8906_RTC_REG(WTSR_SMPL_CNTL_EN_WTSR, (byte)1);
}

/*===================================================================================================================*/
/* MAX8906 I2C Interface                                                                                             */
/*===================================================================================================================*/

#include <linux/i2c.h>

#define MAX8906_RTC_ID	0xD0	/* Read Time Clock */
#define MAX8906_ADC_ID	0x8E	/* ADC/Touchscreen */
#define MAX8906_GPM_ID	0x78	/* General Power Management */
#define MAX8906_APM_ID	0x68	/* APP Specific Power Management */

static struct i2c_driver max8906_driver;

static struct i2c_client *max8906_rtc_i2c_client = NULL;
static struct i2c_client *max8906_adc_i2c_client = NULL;
static struct i2c_client *max8906_gpm_i2c_client = NULL;
static struct i2c_client *max8906_apm_i2c_client = NULL;

static unsigned short max8906_normal_i2c[] = { I2C_CLIENT_END };
static unsigned short max8906_ignore[] = { I2C_CLIENT_END };
static unsigned short max8906_probe[] = { 3, (MAX8906_RTC_ID >> 1), 3, (MAX8906_ADC_ID >> 1),
											3, (MAX8906_GPM_ID >> 1), 3, (MAX8906_APM_ID >> 1), I2C_CLIENT_END };

static struct i2c_client_address_data max8906_addr_data = {
	.normal_i2c = max8906_normal_i2c,
	.ignore		= max8906_ignore,
	.probe		= max8906_probe,
};

static int max8906_read(struct i2c_client *client, u8 reg, u8 *data)
{
	int ret;
	u8 buf[1];
	struct i2c_msg msg[2];

	buf[0] = reg; 

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = buf;

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 1;
	msg[1].buf = buf;

	ret = i2c_transfer(client->adapter, msg, 2);
	if (ret != 2) 
		return -EIO;

	*data = buf[0];
	
	return 0;
}

static int max8906_write(struct i2c_client *client, u8 reg, u8 data)
{
	int ret;
	u8 buf[2];
	struct i2c_msg msg[1];

	buf[0] = reg;
	buf[1] = data;

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = 2;
	msg[0].buf = buf;

	ret = i2c_transfer(client->adapter, msg, 1);
	if (ret != 1) 
		return -EIO;

	return 0;
}

unsigned int pmic_read(u8 slaveaddr, u8 reg, u8 *data, u8 length)
{
	struct i2c_client *client;
#if 0	
	printk("%s -> slaveaddr 0x%02x, reg 0x%02x, data 0x%02x\n",	__FUNCTION__, slaveaddr, reg, *data);
#endif	
	if (slaveaddr == MAX8906_GPM_ID)
		client = max8906_gpm_i2c_client;
	else if (slaveaddr == MAX8906_APM_ID)
		client = max8906_apm_i2c_client;
	else 
		return PMIC_FAIL;

	if (max8906_read(client, reg, data) < 0) { 
		printk(KERN_ERR "%s -> Failed! (slaveaddr 0x%02x, reg 0x%02x, data 0x%02x)\n",
					__FUNCTION__, slaveaddr, reg, *data);
		return PMIC_FAIL;
	}	

	return PMIC_PASS;
}

unsigned int pmic_write(u8 slaveaddr, u8 reg, u8 *data, u8 length)
{
	struct i2c_client *client;
#if 0	
	printk("%s -> slaveaddr 0x%02x, reg 0x%02x, data 0x%02x\n",	__FUNCTION__, slaveaddr, reg, *data);
#endif	
	if (slaveaddr == MAX8906_GPM_ID)
		client = max8906_gpm_i2c_client;
	else if (slaveaddr == MAX8906_APM_ID)
		client = max8906_apm_i2c_client;
	else 
		return PMIC_FAIL;

	if (max8906_write(client, reg, *data) < 0) { 
		printk(KERN_ERR "%s -> Failed! (slaveaddr 0x%02x, reg 0x%02x, data 0x%02x)\n",
					__FUNCTION__, slaveaddr, reg, *data);
		return PMIC_FAIL;
	}	

	return PMIC_PASS;
}

unsigned int pmic_tsc_write(u8 slaveaddr, u8 *cmd)
{
	return 0;
}

unsigned int pmic_tsc_read(u8 slaveaddr, u8 *cmd)
{
	return 0;
}

static int max8906_attach(struct i2c_adapter *adap, int addr, int kind)
{
	struct i2c_client *c;
	int ret;

	c = kmalloc(sizeof(*c), GFP_KERNEL);
	if (!c)
		return -ENOMEM;

	memset(c, 0, sizeof(struct i2c_client));	

	strcpy(c->name, max8906_driver.driver.name);
	c->addr = addr;
	c->adapter = adap;
	c->driver = &max8906_driver;

	if ((ret = i2c_attach_client(c)))
		goto error;

	if ((addr << 1) == MAX8906_RTC_ID)
		max8906_rtc_i2c_client = c;
	else if ((addr << 1) == MAX8906_ADC_ID)
		max8906_adc_i2c_client = c;
	else if ((addr << 1) == MAX8906_GPM_ID)
		max8906_gpm_i2c_client = c;
	else /* (addr << 1) == MAX8906_APM_ID */
		max8906_apm_i2c_client = c;

error:
	return ret;
}

static int max8906_attach_adapter(struct i2c_adapter *adap)
{
	return i2c_probe(adap, &max8906_addr_data, max8906_attach);
}

static int max8906_detach_client(struct i2c_client *client)
{
	i2c_detach_client(client);
	return 0;
}

static int max8906_command(struct i2c_client *client, unsigned int cmd, void *arg)
{
	return 0;
}

static struct i2c_driver max8906_driver = {
	.driver = {
		.name = "max8906",
	},
	.attach_adapter = max8906_attach_adapter,
	.detach_client = max8906_detach_client,
	.command = max8906_command
};

static int __init max8906_init(void)
{
	return i2c_add_driver(&max8906_driver);
}

static void __exit max8906_exit(void)
{
	i2c_del_driver(&max8906_driver);
}

MODULE_AUTHOR("Jeonghwan Min <jh78.min@samsung.com>");
MODULE_DESCRIPTION("MAX8906 Driver");
MODULE_LICENSE("GPL");

module_init(max8906_init);
module_exit(max8906_exit);
