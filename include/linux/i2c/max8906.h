// Maxim 8906 Command Module: Interface Window
// Mark Underwood
// 04/04/2006
// (C) 2004 Maxim Integrated Products
//---------------------------------------------------------------------------
#ifndef MAX8906
#define MAX8906

#define TRUE   1   /* Boolean true value. */
#define FALSE  0   /* Boolean false value. */
#ifndef NULL
#define NULL   0
#endif

typedef  unsigned char      boolean;     /* Boolean value type. */

typedef  unsigned long int  uint32;      /* Unsigned 32 bit value */
typedef  unsigned short     uint16;      /* Unsigned 16 bit value */
typedef  unsigned char      uint8;       /* Unsigned 8  bit value */

typedef  signed long int    int32;       /* Signed 32 bit value */
typedef  signed short       int16;       /* Signed 16 bit value */
typedef  signed char        int8;        /* Signed 8  bit value */

/* This group are the deprecated types.  Their use should be
** discontinued and new code should use the types above
*/
typedef  unsigned char     byte;         /* Unsigned 8  bit value type. */
typedef  unsigned short    word;         /* Unsinged 16 bit value type. */
typedef  unsigned long     dword;        /* Unsigned 32 bit value type. */

typedef  unsigned char     uint1;        /* Unsigned 8  bit value type. */
typedef  unsigned short    uint2;        /* Unsigned 16 bit value type. */
typedef  unsigned long     uint4;        /* Unsigned 32 bit value type. */

typedef  signed char       int1;         /* Signed 8  bit value type. */
typedef  signed short      int2;         /* Signed 16 bit value type. */
typedef  long int          int4;         /* Signed 32 bit value type. */

typedef  signed long       sint31;       /* Signed 32 bit value */
typedef  signed short      sint15;       /* Signed 16 bit value */
typedef  signed char       sint7;        /* Signed 8  bit value */


//===========================================================================
// 
//  MAX8906 Power Management Section
// 
//===========================================================================
// Regulator Name
#define NUMOFREG    24 // Number of regulators

#define WBBCORE      0x01
#define WBBRF        0x02
#define APPS         0x04
#define IO           0x08
#define MEM          0x10
#define WBBMEM       0x20
#define WBBIO        0x40
#define WBBANA       0x80
#define RFRXL      0x0100
#define RFTXL      0x0200
#define RFRXH      0x0400
#define RFTCXO     0x0800
#define LDOA       0x1000
#define LDOB       0x2000
#define LDOC       0x4000
#define LDOD       0x8000
#define SIMLT    0x010000
#define SRAM     0x020000
#define CARD1    0x040000
#define CARD2    0x080000
#define MVT      0x100000
#define BIAS     0x200000
#define VBUS     0x400000
#define USBTXRX  0x800000

// Flexible Power Sequencer Number
typedef enum {
    SEQ1 = 0,
    SEQ2,
    SEQ3,
    SEQ4,
    SEQ5,
    SEQ6,
    SEQ7,
    SW_CNTL
} flex_power_seq_type;

// IRQ Mask register setting
/* ON_OFF_IRQ_MASK */
#define SW_R_M      0x80
#define SW_F_M      0x40
#define SW_1SEC_M   0x20
#define JIG_R_M     0x10
#define JIG_F_M     0x08
#define SW_3SEC_M   0x04
#define MPL_EVENT_M 0x02

/* CHG_IRQ1_MASK */
#define VAC_R_M     0x80
#define VAC_F_M     0x40
#define VAC_OVP_M   0x20
#define VBUS_F_M    0x10
#define VBUS_R_M    0x08
#define VBUS_OVP_M  0x04

/* CHG_IRQ2_M */
#define CHG_TMR_FAULT_M 0x80
#define CHG_TOPOFF_M    0x40
#define CHG_DONE_M      0x20
#define CHG_RST_M       0x10
#define MBATTLOWR_M     0x08
#define MBATTLOWF_M     0x04

/* RTC_IRQ_M */
#define ALARM0_R_M  0x08
#define ALARM1_R_M  0x04

/* TSC_INT_M */
#define nCONV_NS_M  0x08
#define CONV_S_M    0x04
#define nTS_NS_M    0x02
#define nTS_S_M     0x01

// initialize the mask register as belows.
// If MASK bit is set, then the rising edge detection interrupt for the MASK bit is masked.
// So, IRQ\ pin is not asserted.
// If you want to clear some bit, please check the max8906_irq_table[] for interrupt service routine.
#define NUMOFIRQ    0x05
#define ON_OFF_IRQ_M    (SW_R_M | SW_F_M | SW_1SEC_M | JIG_R_M | JIG_F_M | SW_3SEC_M | MPL_EVENT_M)
#define CHG_IRQ1_M      (VAC_R_M | VAC_F_M | VAC_OVP_M | VBUS_F_M | VBUS_R_M | VBUS_OVP_M)
#define CHG_IRQ2_M      (CHG_TMR_FAULT_M | CHG_TOPOFF_M | CHG_DONE_M | CHG_RST_M | MBATTLOWR_M | MBATTLOWF_M)
#define RTC_IRQ_M       (ALARM0_R_M | ALARM1_R_M)
#define TSC_INT_M       (nCONV_NS_M | CONV_S_M | nTS_NS_M | nTS_S_M)

typedef enum {
    START_IRQ = 0,
    /* ON_OFF_IRQ */
    IRQ_SW_R = START_IRQ,
    IRQ_SW_F,
    IRQ_SW_1SEC,
    IRQ_JIG_R,
    IRQ_JIG_F,
    IRQ_SW_3SEC,
    IRQ_MPL_EVENT,
    /* CHG_IRQ1 */
    IRQ_VAC_R,
    IRQ_VAC_F,
    IRQ_VAC_OVP,
    IRQ_VBUS_F,
    IRQ_VBUS_R,
    IRQ_VBUS_OVP,
    /* CHG_IRQ2 */
    IRQ_CHG_TMR_FAULT,
    IRQ_CHG_TOPOFF,
    IRQ_CHG_DONE,
    IRQ_CHG_RST,
    IRQ_MBATTLOWR,
    IRQ_MBATTLOWF,
    /* RTC_IRQ */
    IRQ_ALARM0_R,
    IRQ_ALARM1_R,
    ENDOFIRQ = IRQ_ALARM1_R,
    /* TSC_STA_INT */
    START_TIRQ,
    IRQ_nREF_OK = START_TIRQ,
    IRQ_nCONV_NS,
    IRQ_CONV_S,
    IRQ_nTS_NS,
    IRQ_nTS_S,
    ENDOFTIRQ = IRQ_nTS_S
} max8906_irq_type;

typedef enum {
    //========================================================
    //  B A T T E R Y   C H A R G E R S
    //========================================================
    REG_CHG_CNTL1,
    REG_CHG_CNTL2,
    REG_CHG_IRQ1,
    REG_CHG_IRQ2,
    REG_CHG_IRQ1_MASK,
    REG_CHG_IRQ2_MASK,
    REG_CHG_STAT,
    REG_BBATTCNFG,
    
    //========================================================
    //  S T E P - D O W N    R E G U L A T O R S
    //========================================================
    REG_WBBCOREEN,
    REG_WBBCOREFSEQ,
    REG_WBBCORETV,
    REG_WBBRFEN,
    REG_WBBRFFSEQ,
    REG_WBBRFTV,
    REG_APPSEN,
    REG_OVER1,
    REG_APPSFSEQ,
    REG_ADTV1,
    REG_ADTV2,
    REG_APPSCLG,
    REG_VCC1,
    REG_IOEN,
    REG_IOFSEQ,
    REG_MEMEN,
    REG_MEMFSEQ,
    
    //========================================================
    //  L I N E A R   R E G U L A T O R S
    //========================================================
    REG_WBBMEMEN,
    REG_WBBMEMFSEQ,
    REG_WBBMEMTV,
    REG_WBBIOEN,
    REG_WBBIOFSEQ,
    REG_WBBIOTV,
    REG_WBBANAEN,
    REG_WBBANAFSEQ,
    REG_WBBANATV,
    REG_RFRXLEN,
    REG_RFRXLFSEQ,
    REG_RFRXLTV,
    REG_RFTXLEN,
    REG_RFTXLFSEQ,
    REG_RFTXLTV,
    REG_RFRXHEN,
    REG_RFRXHFSEQ,
    REG_RFRXHTV,
    REG_RFTCXOEN,
    REG_RFTCXOFSEQ,
    REG_RFTCXOTV,
    REG_LDOAEN,
    REG_LDOAFSEQ,
    REG_LDOATV,
    REG_LDOBEN,
    REG_LDOBFSEQ,
    REG_LDOBTV,
    REG_LDOCEN,
    REG_LDOCFSEQ,
    REG_LDOCTV,
    REG_LDODEN,
    REG_LDODFSEQ,
    REG_LDODTV,
    REG_SIMLTEN,
    REG_SIMLTFSEQ,
    REG_SIMLTTV,
    REG_SRAMEN,
    REG_SRAMFSEQ,
    REG_SDTV1,
    REG_SDTV2,
    REG_SRAMCLG,
    REG_CARD1EN,
    REG_CARD1FSEQ,
    REG_CARD1TV,
    REG_CARD2EN,
    REG_CARD2FSEQ,
    REG_CARD2TV,
    REG_MVTENEN,
    REG_MVTFSEQ,
    REG_MDTV1,
    REG_MDTV2,
    REG_BIASEN,
    REG_BIASFSEQ,
    REG_BIASTV,
    REG_VBUSEN,
    REG_VBUSFSEQ,
    REG_USBTXRXEN,
    REG_USBTXRXFSEQ,
    
    //========================================================
    //  M A I N - B A T T E R Y   F A U L T   D E T E C T O R
    //========================================================
    REG_LBCNFG,
    
    //========================================================
    //  O N / O F F   C O N T R O L L E R
    //========================================================
    REG_EXTWKSEL,
    REG_ON_OFF_IRQ,
    REG_ON_OFF_IRQ_MASK,
    REG_ON_OFF_STAT,
    
    //========================================================
    //  F L E X I B L E   P O W E R   S E Q U E N C E R
    //========================================================
    REG_SEQ1CNFG,
    REG_SEQ2CNFG,
    REG_SEQ3CNFG,
    REG_SEQ4CNFG,
    REG_SEQ5CNFG,
    REG_SEQ6CNFG,
    REG_SEQ7CNFG,
    
    //========================================================
    //  U S B   T R A N S C E I V E R
    //========================================================
    REG_USBCNFG,
    
    //========================================================
    //  T C X O   B U F F E R
    //========================================================
    REG_TCXOCNFG,
    
    //========================================================
    //  R E F E R E N C E   O U T P U T (R E F O U T)
    //========================================================
    REG_REFOUTCNFG,
    
    //========================================================
    //  R E A L   T I M E   C L O C K (R T C)
    //========================================================
    REG_RTC_SEC,       
    REG_RTC_MIN,       
    REG_RTC_HOURS,     
    REG_RTC_WEEKDAY,   
    REG_RTC_DATE,      
    REG_RTC_MONTH,     
    REG_RTC_YEAR1,     
    REG_RTC_YEAR2,     
    REG_ALARM0_SEC,    
    REG_ALARM0_MIN,    
    REG_ALARM0_HOURS,  
    REG_ALARM0_WEEKDAY,
    REG_ALARM0_DATE,   
    REG_ALARM0_MONTH,  
    REG_ALARM0_YEAR1,  
    REG_ALARM0_YEAR2,  
    REG_ALARM1_SEC,    
    REG_ALARM1_MIN,    
    REG_ALARM1_HOURS,  
    REG_ALARM1_WEEKDAY,
    REG_ALARM1_DATE,   
    REG_ALARM1_MONTH,  
    REG_ALARM1_YEAR1,  
    REG_ALARM1_YEAR2,  
    REG_ALARM0_CNTL,
    REG_ALARM1_CNTL,
    REG_RTC_STATUS,
    REG_RTC_CNTL,
    REG_RTC_IRQ,
    REG_RTC_IRQ_MASK,
    REG_MPL_CNTL,
    
    //========================================================
    //  T O U C H - S C R E E N / A D C   C O N T R O L L E R
    //========================================================
    REG_TSC_STA_INT,
    REG_TSC_INT_MASK,
    REG_TSC_CNFG1,
    REG_TSC_CNFG2,
    REG_TSC_CNFG3,
    REG_TSC_CNFG4,
    REG_TSC_RES_CNFG1,
    REG_TSC_AVG_CNFG1,
    REG_TSC_ACQ_CNFG1,
    REG_TSC_ACQ_CNFG2,
    REG_TSC_ACQ_CNFG3,
    //========== ADC_RESULTS registers
    REG_ADC_X_MSB,
    REG_ADC_X_LSB,
    REG_ADC_Y_MSB,
    REG_ADC_Y_LSB,
    REG_ADC_Z1_MSB,
    REG_ADC_Z1_LSB,
    REG_ADC_Z2_MSB,
    REG_ADC_Z2_LSB,
    REG_ADC_AUX1_MSB,
    REG_ADC_AUX1_LSB,
    REG_ADC_VBUS_MSB,
    REG_ADC_VBUS_LSB,
    REG_ADC_VAC_MSB,
    REG_ADC_VAC_LSB,
    REG_ADC_MBATT_MSB,
    REG_ADC_MBATT_LSB,
    REG_ADC_BBATT_MSB,
    REG_ADC_BBATT_LSB,
    REG_ADC_ICHG_MSB,
    REG_ADC_ICHG_LSB,
    REG_ADC_TDIE_MSB,
    REG_ADC_TDIE_LSB,
    REG_ADC_AUX2_MSB,
    REG_ADC_AUX2_LSB,
    
    // TOUCH-SCREEN CONVERSION COMMAND registers
    REG_TSC_X_Drive,
    REG_TSC_X_Measurement,
    REG_TSC_Y_Drive,
    REG_TSC_Y_Measurement,
    REG_TSC_Z1_Drive,
    REG_TSC_Z1_Measurement,
    REG_TSC_Z2_Drive,
    REG_TSC_Z2_Measurement,
    REG_TSC_AUX1_Measurement,
    REG_TSC_VBUS_Measurement,      
    REG_TSC_VAC_Measurement,
    REG_TSC_MBATT_Measurement,
    REG_TSC_BBATT_Measurement,
    REG_TSC_ICHG_Measurement,
    REG_TSC_TDIE_Measurement,
    REG_TSC_AUX2_Measurement,
    
    //========================================================
    //  A U D I O   S U B S Y S T E M
    //========================================================
    REG_PGA_CNTL1,
    REG_PGA_CNTL2,
    REG_LMIX_CNTL,
    REG_RMIX_CNTL,
    REG_MMIX_CNTL,
    REG_HS_RIGHT_GAIN_CNTL,
    REG_HS_LEFT_GAIN_CNTL,
    REG_LINE_OUT_GAIN_CNTL,
    REG_LS_GAIN_CNTL,
    REG_AUDIO_CNTL,
    REG_AUDIO_ENABLE1,
    
    //========================================================
    //  C H I P   I D E N T I F I C A T I O N
    //========================================================
    REG_II1RR,
    REG_II2RR,
    REG_IRQ_STAT,
        
    ENDOFREG

} max8906_pm_register_type;

typedef enum {
    //========================================================
    //  B A T T E R Y   C H A R G E R S
    //========================================================
    // CHG_CNTL1 register
    nCHGEN,
    CHG_TOPOFF_TH,
    CHG_RST_HYS,
    AC_FCHG,
    
    // CHG_CNTL2 register
    VBUS_FCHG,
    FCHG_TMR,
    MBAT_REG_TH,
    MBATT_THERM_REG,
    
    // CHG_IRQ1 register
    VAC_R,
    VAC_F,
    VAC_OVP,
    VBUS_R,
    VBUS_F,
    VBUS_OVP,
    
    // CHG_IRQ2 register
    CHG_TMR_FAULT,
    CHG_TOPOFF,
    CHG_DONE,
    CHG_RST,
    MBATTLOWR,
    MBATTLOWF,
    
    // CHG_IRQ1_MASK register
    VAC_R_MASK,
    VAC_F_MASK,
    VAC_OVP_MASK,
    VBUS_R_MASK,
    VBUS_F_MASK,
    VBUS_OVP_MASK,
    
    // CHG_IRQ2_MASK register
    CHG_TMR_FAULT_MASK,
    CHG_TOPOFF_MASK,
    CHG_DONE_MASK,
    CHG_RST_MASK,
    MBATTLOWR_MASK,
    MBATTLOWF_MASK,
    
    // CHG_STAT register
    VAC_OK,
    VBUS_OK,
    CHG_TMR,
    CHG_EN_STAT,
    CHG_MODE,
    MBATT_DET,
    MBATTLOW,
    
    // BBATTCNGF register
    APPALLOFF,
    VBBATTCV,
    
    //========================================================
    //  S T E P - D O W N    R E G U L A T O R S
    //========================================================
    // WBBCOREEN register
    nWCRADE,
    WCRENSRC,
    WCREN,
    
    // WBBCOREFSEQ register
    WCRFSEQPU,
    WCRFSEQPD,
    
    // WBBCORETV register
    WCRTV,
    
    // WBBRFEN register
    nWRFADE,
    WRFENSRC,
    WRFEN,
    
    // WBBRFFSEQ register
    WRFFSEQPU,
    WRFFSEQPD,
    
    // WBBRFTV register
    WRFTV,
    
    // APPSEN register
    nAPPSADE,
    nOVER1ENAPPS,
    APPSENSRC,
    APPSEN,
    
    // OVER1 register
    ENSRAM,
    ENAPPS,
    
    // APPSFSEQ register
    APPSFSEQPU,
    APPSFSEQPD,
    
    // ADTV1 register
    T1AOST,
    T1APPS,
    
    // ADTV2 register
    T2AOST,
    T2APPS,
    
    // APPSCLG register
    CLGAPPS,
    
    // VCC1 register
    MVS,
    MGO,
    SVS,
    SGO,
    AVS,
    AGO,
    
    // IOEN register
    nIOADE,
    IOENSRC,
    IOEN,
    
    // IOFSEQ register
    IOFSEQPU,
    IOFSEQPD,
    
    // MEMEN register
    nMEMADE,
    MEMDVM,
    MEMENSRC,
    MEMEN,
    
    // MEMFSEQ register
    MEMFSEQPU,
    MEMFSEQPD,
    
    //========================================================
    //  L I N E A R   R E G U L A T O R S
    //========================================================
    // WBBMEMEN register
    nWMEMADE,
    WMEMENSRC,
    WMEMEN,
    
    // WBBMEMFSEQ register
    WMEMFSEQPU,
    WMEMFSEQPD,
    
    // WBBMEMTV register
    WMEMTV,
    
    // WBBIOEN register
    nWIOADE,
    SFTRSTWBB,
    WIOENSRC,
    WIOEN,
    
    // WBBIOFSEQ register
    WIOFSEQPU,
    WIOFSEQPD,
    
    // WBBIOTV register
    WIOTV,
    
    // WBBANAEN register
    nWANAADE,
    WANAENSRC,
    WANAEN,
    
    // WBBANAFSEQ register
    WANAFSEQPU,
    WANAFSEQPD,
    
    // WBBANATV register
    WANATV,
    
    // RFRXLEN register
    nRFRXLADE,
    RFRXLENSRC,
    RFRXLEN,
    
    // RFRXLFSEQ register
    RFRXLFSEQPU,
    RFRXLFSEQPD,
    
    // RFRXLTV register
    RFRXLTV,
    
    // RFTXLEN register
    nRFTXLADE,
    RFTXLENSRC,
    RFTXLEN,
    
    // RFTXLFSEQ register
    RFTXLFSEQPU,
    RFTXLFSEQPD,
    
    // RFTXLTV register
    RFTXLTV,
    
    // RFRXHEN register
    nRFRXHADE,
    RFRXHENSRC,
    RFRXHEN,
    
    // RFRXHFSEQ register
    RFRXHFSEQPU,
    RFRXHFSEQPD,
    
    // RFRXHTV register
    RFRXHTV,
    
    // RFTCXOEN register
    nRFTCXOADE,
    RFTCXOENSRC,
    RFTCXOEN,
    
    // RFTCXOFSEQ register
    RFTCXOFSEQPU,
    RFTCXOFSEQPD,
    
    // RFTCXOTV register
    RFTCXOLTV,
    
    // LDOAEN register
    nLDOAADE,
    LDOAENSRC,
    LDOAEN,
    
    // LDOAFSEQ register
    LDOAFSEQPU,
    LDOAFSEQPD,
    
    // LDOATV register
    LDOATV,
    
    // LDOBEN register
    nLDOBADE,
    LDOBENSRC,
    LDOBEN,
    
    // LDOBFSEQ register
    LDOBFSEQPU,
    LDOBFSEQPD,
    
    // LDOBTV register
    LDOBTV,
    
    // LDOCEN register
    nLDOCADE,
    LDOCENSRC,
    LDOCEN,
    
    // LDOCFSEQ register
    LDOCFSEQPU,
    LDOCFSEQPD,
    
    // LDOCTV register
    LDOCTV,
    
    // LDODEN register
    nLDODADE,
    LDODENSRC,
    LDODEN,
    
    // LDODFSEQ register
    LDODFSEQPU,
    LDODFSEQPD,
    
    // LDODTV register
    LDODTV,
    
    // SIMLTEN register
    nSIMLTADE,
    SIMLTENSRC,
    SIMLTEN,
    
    // SIMLTFSEQ register
    SIMLTFSEQPU,
    SIMLTFSEQPD,
    
    // SIMLTTV register
    SIMLTTV,
    
    // SRAMEN register
    nSRAMADE,
    nOVER1ENSRAM,
    SRAMENSRC,
    SRAMEN,
    
    // SRAMFSEQ register
    SRAMFSEQPU,
    SRAMFSEQPD,
    
    // SDTV1 register
    T1SOST,
    T1SRAM,
    
    // SDTV2 register
    T2SOST,
    T2SRAM,
    
    // SRAMCLG register
    CLGSRAM,
    
    // CARD1EN register
    nCARD1ADE,
    CARD1ENSRC,
    CARD1EN,
    
    // CARD1FSEQ register
    CARD1FSEQPU,
    CARD1FSEQPD,
    
    // CARD1TV register
    CARD1TV,
    
    // CARD2EN register
    nCARD2ADE,
    CARD2ENSRC,
    CARD2EN,
    
    // CARD2FSEQ register
    CARD2FSEQPU,
    CARD2FSEQPD,
    
    // CARD2TV register
    CARD2TV,
    
    // MVTENEN register
    nMVTADE,
    MVTENSRC,
    MVTEN,
    
    // MVTFSEQ register
    MVTFSEQPU,
    MVTFSEQPD,
    
    // MDTV1 register
    T1MVT,
    
    // MDTV2 register
    T2MVT,
    
    // BIASEN register
    nBIASADE,
    BIASENSRC,
    BIASEN,
    
    // BIASFSEQ register
    BIASFSEQPU,
    BIASFSEQPD,
    
    // BIASTV register
    BIASTV,
    
    // VBUSEN register
    nVBUSADE,
    VBUSVINEN,
    VBUSENSRC,
    VBUSEN,
    
    // VBUSFSEQ register
    VBUSFSEQPU,
    VBUSFSEQPD,
    
    // USBTXRXEN register
    nUSBTXRXADE,
    USBTXRXVINEN,
    USBTXRXENSRC,
    USBTXRXEN,
    
    // USBTXRXFSEQ register
    USBTXRXFSEQPU,
    USBTXRXFSEQPD,
    
    //========================================================
    //  M A I N - B A T T E R Y   F A U L T   D E T E C T O R
    //========================================================
    // LBCNFG register
    LHYST,
    LBDAC,
    LBEN,
    
    //========================================================
    //  O N / O F F   C O N T R O L L E R
    //========================================================
    // EXTWKSEL register
    HRDRSTEN,
    WKVBUS,
    WKVAC,
    WKALRM1R,
    WKALRM0R,
    WKSW,
    
    // ON_OFF_IRQ register
    SW_R,
    SW_F,
    SW_1SEC,
    JIG_R,
    JIG_F,
    SW_3SEC,
    MPL_EVENT,
    
    // ON_OFF_IRQ_MASK register
    SW_R_MASK,
    SW_F_MASK,
    SW_1SEC_MASK,
    JIG_R_MASK,
    JIG_F_MASK,
    SW_3SEC_MASK,
    MPL_EVENT_MASK,
    
    // ON_OFF_STAT register
    STAT_SW,
    STAT_SW_1SEC,
    STAT_JIG,
    STAT_SW_3SEC,
    
    //========================================================
    //  F L E X I B L E   P O W E R   S E Q U E N C E R
    //========================================================
    // SEQ1CNFG register
    SEQ1T,
    SEQ1SRC,
    SEQ1EN,
    
    // SEQ2CNFG register
    SEQ2T,
    SEQ2SRC,
    SEQ2EN,
    
    // SEQ3CNFG register
    SEQ3T,
    SEQ3SRC,
    SEQ3EN,
    
    // SEQ4CNFG register
    SEQ4T,
    SEQ4SRC,
    SEQ4EN,
    
    // SEQ5CNFG register
    SEQ5T,
    SEQ5SRC,
    SEQ5EN,
    
    // SEQ6CNFG register
    SEQ6T,
    SEQ6SRC,
    SEQ6EN,
    
    // SEQ7CNFG register
    SEQ7T,
    SEQ7SRC,
    SEQ7EN,
    
    //========================================================
    //  U S B   T R A N S C E I V E R
    //========================================================
    // USBCNFG register
    USB_PU_EN,
    USB_SUSP,
    USB_EN,
    
    //========================================================
    //  T C X O   B U F F E R
    //========================================================
    // TCXOCNFG register
    TCXOEN,
    
    //========================================================
    //  R E F E R E N C E   O U T P U T (R E F O U T)
    //========================================================
    // REFOUTCNFG register
    REFOUTEN,
    
    //========================================================
    //  R E A L   T I M E   C L O C K (R T C)
    //========================================================
    // R T C registers
    RTC_SEC,
    RTC_10SEC,
    RTC_MIN,
    RTC_10MIN,
    RTC_HOURS,
    RTC_10HOURS,
    RTC_12_n24,
    RTC_WEEKDAY,
    RTC_DATE,
    RTC_10DATE,
    RTC_MONTH,
    RTC_10MONTH,
    RTC_YEAR,
    RTC_10YEAR,
    RTC_100YEAR,
    RTC_1000YEAR,
    
    // ALARM0 registers
    ALARM0_SEC,
    ALARM0_10SEC,
    ALARM0_MIN,
    ALARM0_10MIN,
    ALARM0_HOURS,
    ALARM0_10HOURS,
    ALARM0_12_n24,
    ALARM0_WEEKDAY,
    ALARM0_DATE,
    ALARM0_10DATE,
    ALARM0_MONTH,
    ALARM0_10MONTH,
    ALARM0_YEAR,
    ALARM0_10YEAR,
    ALARM0_100YEAR,
    ALARM0_1000YEAR,
    
    // ALARM1 registers
    ALARM1_SEC,
    ALARM1_10SEC,
    ALARM1_MIN,
    ALARM1_10MIN,
    ALARM1_HOURS,
    ALARM1_10HOURS,
    ALARM1_12_n24,
    ALARM1_WEEKDAY,
    ALARM1_DATE,
    ALARM1_10DATE,
    ALARM1_MONTH,
    ALARM1_10MONTH,
    ALARM1_YEAR,
    ALARM1_10YEAR,
    ALARM1_100YEAR,
    ALARM1_1000YEAR,
    
    // ALARM0_CNTL register
    ALARM0_CNTL,
    
    // ALARM1_CNTL register
    ALARM1_CNTL,
    
    // RTC_STATUS register
    RTC_STATUS_DIV_OK,       
    RTC_STATUS_LEAP_OK,      
    RTC_STATUS_MON_OK,       
    RTC_STATUS_CARY_OK,      
    RTC_STATUS_REG_OK,       
    RTC_STATUS_ALARM0,       
    RTC_STATUS_ALARM1,       
    RTC_STATUS_XTAL_FLT,     
    
    // RTC_CNTL register
    RTC_CNTL_ALARM_WP,       
    RTC_CNTL_RTC_WP,      
    RTC_CNTL_nTCLKWBB_EN,       
    RTC_CNTL_nTCLKAP_EN,       
    RTC_CNTL_nRTC_EN,     
    
    // RTC_IRQ register
    ALARM0_R,       
    ALARM1_R,       
    
    // RTC_IRQ_MASK register
    ALARM0_R_MASK,       
    ALARM1_R_MASK,       
    
    // MPL_CNTL register
    EN_MPL,       
    TIME_MPL, 

    WTSR_SMPL_CNTL_EN_WTSR,
    WTSR_SMPL_CNTL_EN_SMPL,
    WTSR_SMPL_CNTL_TIME_SMPL,
    WTSR_SMPL_CNTL_TIME_WTSR,
    
    //========================================================
    //  T O U C H - S C R E E N / A D C   C O N T R O L L E R
    //========================================================
    // TSC_STA_INT register
    nREF_OK,
    nCONV_NS,
    CONV_S,
    nTS_NS,
    nTS_S,
    
    // TSC_INT_MASK register
    nCONV_NS_MASK,
    CONV_S_MASK,
    nTS_NS_MASK,
    nTS_S_MASK,
    
    // TSC_CNFG1 register
    PU_100_50,
    Four_Wire_CNFG,
    REF_CNFG,
    
    // TSC_CNFG2 register
    RES_X,
    RES_Y,
    RES_Z1,
    RES_Z2,
    AVG_X,
    AVG_Y,
    AVG_Z1,
    AVG_Z2,
    
    // TSC_CNFG3 register
    T_ACQ_X,
    T_ACQ_Y,
    T_ACQ_Z1,
    T_ACQ_Z2,
    
    // TSC_CNFG4 register
    D_CV_X,
    D_CV_Y,
    D_CV_Z1,
    D_CV_Z2,
    
    // TSC_RES_CNFG1 register
    RES_AUX1,
    RES_VBUS,
    RES_VAC,
    RES_MBATT,
    RES_BBATT,
    RES_ICHG,
    RES_TDIE,
    RES_AUX2,
    
    // TSC_AVG_CNFG1 register
    AVG_AUX1,
    AVG_VBUS,
    AVG_VAC,
    AVG_MBATT,
    AVG_BBATT,
    AVG_ICHG,
    AVG_TDIE,
    AVG_AUX2,
    
    // TSC_ACQ_CNFG1 register
    T_ACQ_AUX1,
    T_ACQ_VBUS,
    T_ACQ_VAC,
    T_ACQ_MBATT,
    
    // TSC_ACQ_CNFG2 register
    T_ACQ_BBATT,
    T_ACQ_ICHG,
    T_ACQ_TDIE,
    T_ACQ_AUX2,
    
    // TSC_ACQ_CNFG3 register
    D_CV_AUX1,
    D_CV_VBUS,
    D_CV_VAC,
    D_CV_MBATT,
    D_CV_BBATT,
    D_CV_ICHG,
    D_CV_TDIE,
    D_CV_AUX2,
    
    // ADC_RESULTS register
    X_MSB,
    X_LSB,
    Y_MSB,
    Y_LSB,
    Z1_MSB,
    Z1_LSB,
    Z2_MSB,
    Z2_LSB,
    AUX1_MSB,
    AUX1_LSB,
    VBUS_MSB,
    VBUS_LSB,
    VAC_MSB,
    VAC_LSB,
    MBATT_MSB,
    MBATT_LSB,
    BBATT_MSB,
    BBATT_LSB,
    ICHG_MSB,
    ICHG_LSB,
    TDIE_MSB,
    TDIE_LSB,
    AUX2_MSB,
    AUX2_LSB,
    
    // TOUCH-SCREEN CONVERSION COMMAND register
    X_Drive,
    X_Measurement,
    Y_Drive,
    Y_Measurement,
    Z1_Drive,
    Z1_Measurement,
    Z2_Drive,
    Z2_Measurement,
    AUX1_Measurement,
    VBUS_Measurement,      
    VAC_Measurement,
    MBATT_Measurement,
    BBATT_Measurement,
    ICHG_Measurement,
    TDIE_Measurement,
    AUX2_Measurement,
    
    //========================================================
    //  A U D I O   S U B S Y S T E M
    //========================================================
    // PGA_CNTL1 register
    VOICE_PGA_CNTL_P,
    VOICE_PGA_CNTL_N,
    VOICE_IN_CONFIG,
    IN1_PGA_CNTL,
    
    // PGA_CNTL2 register
    IN2_PGA_CNTL,
    IN3_PGA_CNTL,
    IN4_PGA_CNTL,
    ZDC,
    
    // LMIX_CNTL register
    VOICE_IN_P_LMIX,
    VOICE_IN_N_LMIX,
    IN1_LMIX,
    IN2_LMIX,
    IN3_LMIX,
    IN4_LMIX,
    
    // RMIX_CNTL register
    VOICE_IN_P_RMIX,
    VOICE_IN_N_RMIX,
    IN1_RMIX,
    IN2_RMIX,
    IN3_RMIX,
    IN4_RMIX,
    
    // MMIX_CNTL register
    MONO_MIX_CNTL,
    
    // HS_RIGHT_GAIN_CNTL register
    RIGHT_HS_GAIN,
    
    // HS_LEFT_GAIN_CNTL register
    LEFT_HS_GAIN,
    
    // LINE_OUT_GAIN_CNTL register
    LINE_OUT_GAIN,
    
    // LS_GAIN_CNTL register
    LS_GAIN,
    
    // AUDIO_CNTL register
    MUTE,
    AUDIO_SHDN,
    //LS_BP_EN,
    AMP_EN_CNTL,
    CLASS_D_OSC_CNTL,
    HS_MONO_SW,
    
    // AUDIO_ENABLE1 register
    LS_BP_EN,
    LS_AMP_EN,
    LS_LINEOUT_EN,
    HS_EN,
    
    //========================================================
    //  C H I P   I D E N T I F I C A T I O N
    //========================================================
    // II1RR register
    IIR1,
    
    // II2RR register
    IIR2,
    
    //========================================================
    //  I N T E R R U P T   &   S T A T U S
    //========================================================
    // IRQ_STAT register
    SFT_nTIRQ,
    SFT_nIRQ,
        
    ENDOFPM

} max8906_pm_function_type;

/* MAX8906 each register info */
typedef const struct {
    const byte  slave_addr;
    const byte  addr;
} max8906_register_type;

typedef struct {
    max8906_pm_register_type irq_reg;
    byte    irq_mask;
} max8906_irq_mask_type;

/* MAX8906 each function info */
typedef const struct {
	const byte  slave_addr;
	const byte  addr;
	const byte  mask;
    const byte  clear;
    const byte  shift;
} max8906_function_type;

/* IRQ routine */
typedef struct {
    byte item_num;
	max8906_pm_function_type irq_reg;
    void (*irq_ptr)(void);
} max8906_irq_table_type;


/* MAX8906 each function info */
typedef const struct {
    const dword  reg_name;
    const max8906_pm_function_type active_discharge;
    const max8906_pm_function_type  ena_src_item;
    const max8906_pm_function_type  sw_ena_dis;
} max8906_regulator_name_type;

/* For Touch-Screen Conversion Command Register */
#define EN_REF 0x04
#define CONT   0x01
#define NON_EN_REF_CONT 0x00

typedef struct {
    word x;
    word y;
#ifdef TSC_5_WIRE_MODE
    word z1;
    word z2;
#endif
} max8906_coordinates_type;

typedef enum {
    T_Period_20uS = 0,
    T_Period_40uS,
    T_Period_80uS,
    T_Period_160uS,
    T_Period_320uS,
    T_Period_640uS,
    T_Period_1280uS,
    T_Period_2560uS
} timer_period_type;


//===========================================================================
// 
//  MAX8906 RTC Section
// 
//===========================================================================


typedef enum {
    TIMEKEEPER = 0,
    RTC_ALARM0,
    RTC_ALARM1
} max8906_rtc_cmd_type;


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
extern boolean Set_MAX8906_PM_REG(max8906_pm_function_type reg_num, byte value);

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
extern boolean Get_MAX8906_PM_REG(max8906_pm_function_type reg_num, byte *reg_buff);

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
extern boolean Set_MAX8906_PM_ADDR(max8906_pm_register_type reg_addr, byte *reg_buff, byte length);

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
extern boolean Get_MAX8906_PM_ADDR(max8906_pm_register_type reg_addr, byte *reg_buff, byte length);

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
extern boolean Set_MAX8906_TSC_CONV_REG(max8906_pm_function_type reg_num, byte cmd);

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
extern boolean Get_MAX8906_TSC_CONV_REG(max8906_pm_function_type reg_num, byte *cmd);

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
extern boolean Set_MAX8906_PM_Regulator_Active_Discharge(byte onoff, dword regulators);

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
extern boolean Set_MAX8906_PM_Regulator_ENA_SRC(flex_power_seq_type sequencer, dword regulators);

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
extern boolean Set_MAX8906_PM_Regulator_SW_Enable(byte onoff, dword regulators);

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
extern boolean Set_MAX8906_PM_PWR_SEQ_Timer_Period(max8906_pm_function_type cntl_item, timer_period_type value);

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
extern boolean Get_MAX8906_PM_PWR_SEQ_Timer_Period(max8906_pm_function_type cntl_item, timer_period_type *value);

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
extern boolean Set_MAX8906_PM_PWR_SEQ_Ena_Src(max8906_pm_function_type cntl_item, byte value);

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
extern boolean Get_MAX8906_PM_PWR_SEQ_Ena_Src(max8906_pm_function_type cntl_item, byte *value);

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
extern boolean Set_MAX8906_PM_PWR_SEQ_SW_Enable(max8906_pm_function_type cntl_item, byte value);

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
extern boolean Get_MAX8906_PM_PWR_SEQ_SW_Enable(max8906_pm_function_type cntl_item, byte *value);

/*===========================================================================
 T O U C H   S C R E E N
===========================================================================*/
extern void Set_MAX8906_PM_TSC_init(void);

extern void Set_MAX8906_PM_TSC_detect_isr(void);

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
extern boolean Set_MAX8906_TSC_measurement(max8906_coordinates_type *tsc_coordinates);


/*===========================================================================

      R T C     S E C T I O N

===========================================================================*/

/*===========================================================================

FUNCTION Set_MAX8906_RTC                                

DESCRIPTION
    This function write the value at the selected register address
    in the RTC section.

INPUT PARAMETERS
    max8906_rtc_cmd_type :     RTC_TIME   = timekeeper register 0x0~0x3
                               RTC_ALARM  = alarm register 0x8~0xB

    byte* max8906_rtc_ptr : the write value for registers.

RETURN VALUE
    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
extern boolean Set_MAX8906_RTC(max8906_rtc_cmd_type rtc_cmd,byte *max8906_rtc_ptr);

/*===========================================================================

FUNCTION Get_MAX8906_RTC                                

DESCRIPTION
    This function read the value at the selected register address
    in the RTC section.

INPUT PARAMETERS
    max8906_rtc_cmd_type :     RTC_TIME   = timekeeper register 0x0~0x3
                               RTC_ALARM  = alarm register 0x8~0xB

    byte* max8906_rtc_ptr : the return value for registers.

RETURN VALUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
extern boolean Get_MAX8906_RTC(max8906_rtc_cmd_type rtc_cmd, byte *max8906_rtc_ptr);


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
extern void MAX8906_IRQ_init(void);

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
extern boolean Set_MAX8906_PM_IRQ(max8906_irq_type irq_name, void (*max8906_isr)(void));

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
extern void MAX8906_PM_IRQ_isr(void);

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
extern void MAX8906_PM_TIRQ_isr(void);


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
extern void MAX8906_PM_init(void);

#endif

