//--------------------------------------------------------
//
//    MELFAS Firmware download base code for MCS6000
//    Version : v01
//    Date    : 2009.01.20
//
//--------------------------------------------------------

#ifndef __MELFAS_FIRMWARE_DOWNLOAD_H__
#define __MELFAS_FIRMWARE_DOWNLOAD_H__

//=====================================================================
//
//   MELFAS Firmware download pharameters
//
//=====================================================================


//----------------------------------------------------
//   Return values of download function
//----------------------------------------------------
#define MCSDL_RET_SUCCESS                            0x00
#define MCSDL_RET_ENTER_DOWNLOAD_MODE_FAILED        0x01
#define MCSDL_RET_ERASE_FLASH_FAILED                0x02
#define MCSDL_RET_PREPARE_ERASE_FLASH_FAILED        0x0B
#define MCSDL_RET_ERASE_VERIFY_FAILED                0x03
#define MCSDL_RET_READ_FLASH_FAILED                    0x04
#define MCSDL_RET_READ_EEPROM_FAILED                0x05
#define MCSDL_RET_READ_INFORMAION_FAILED            0x06
#define MCSDL_RET_PROGRAM_FLASH_FAILED                0x07
#define MCSDL_RET_PROGRAM_EEPROM_FAILED                0x08
#define MCSDL_RET_PREPARE_PROGRAM_FAILED            0x09
#define MCSDL_RET_PROGRAM_VERIFY_FAILED                0x0A

#define MCSDL_RET_WRONG_MODE_ERROR                    0xF0
#define MCSDL_RET_WRONG_SLAVE_SELECTION_ERROR        0xF1
#define MCSDL_RET_WRONG_PARAMETER                    0xF2
#define MCSDL_RET_COMMUNICATION_FAILED                0xF3
#define MCSDL_RET_READING_HEXFILE_FAILED            0xF4
#define MCSDL_RET_FILE_ACCESS_FAILED                0xF5
#define MCSDL_RET_MELLOC_FAILED                        0xF6
#define MCSDL_RET_WRONG_MODULE_REVISION                0xF7


//----------------------------------------------------
// ISP commands
//----------------------------------------------------
#define MCSDL_ISP_CMD_ERASE                            0x02
#define MCSDL_ISP_CMD_ERASE_TIMING                    0x0F
#define MCSDL_ISP_CMD_PROGRAM_FLASH                    0x03
#define MCSDL_ISP_CMD_READ_FLASH                    0x04
#define MCSDL_ISP_CMD_PROGRAM_TIMING                0x0F
#define MCSDL_ISP_CMD_READ_INFORMATION                0x06
#define MCSDL_ISP_CMD_RESET                            0x07


//----------------------------------------------------
// MCS5000's responses
//----------------------------------------------------
#define MCSDL_ISP_ACK_ERASE_DONE                    0x82
#define MCSDL_ISP_ACK_PREPARE_ERASE_DONE            0x8F
#define MCSDL_I2C_ACK_PREPARE_PROGRAM                0x8F
#define MCSDL_MDS_ACK_PROGRAM_FLASH                    0x83
#define MCSDL_MDS_ACK_READ_FLASH                    0x84
#define MCSDL_MDS_ACK_PROGRAM_INFORMATION            0x88
#define MCSDL_MDS_ACK_PROGRAM_LOCKED                0xFE
#define MCSDL_MDS_ACK_READ_LOCKED                    0xFE
#define MCSDL_MDS_ACK_FAIL                            0xFE

#define MCSDL_ISP_PROGRAM_TIMING_VALUE_0            0x00
#define MCSDL_ISP_PROGRAM_TIMING_VALUE_1            0x00
#define MCSDL_ISP_PROGRAM_TIMING_VALUE_2            0x78

#define MCSDL_ISP_ERASE_TIMING_VALUE_0                0x01
#define MCSDL_ISP_ERASE_TIMING_VALUE_1                0xD4
#define MCSDL_ISP_ERASE_TIMING_VALUE_2                0xC0


//----------------------------------------------------
//    I2C ISP
//----------------------------------------------------

#define MCSDL_I2C_SLAVE_ADDR_ORG                    0x7D                            // Original Address

#define MCSDL_I2C_SLAVE_ADDR_SHIFTED                (MCSDL_I2C_SLAVE_ADDR_ORG<<1)    // Adress after sifting.

#define MCSDL_I2C_SLAVE_READY_STATUS                0x55

#define USE_BASEBAND_I2C_FUNCTION                    1                                // Selection of i2c function ( This must be 1 )

#define MELFAS_USE_PROTOCOL_COMMAND_FOR_DOWNLOAD     0                                // If 'enable download command' is needed ( Pinmap dependent option ).


//----------------------------------------------------
//    MELFAS Version information address
//----------------------------------------------------
#define MCSDL_ADDR_MODULE_REVISION                    0x98
#define MCSDL_ADDR_FIRMWARE_VERSION                    0x9C

#define MELFAS_TRANSFER_LENGTH                        64                                // Program & Read flash block size



//============================================================
//
//    Port setting. ( Melfas preset this value. )
//
//============================================================

// If want to set Enable : Set to 1

#define MCSDL_USE_CE_CONTROL                        1
#define MCSDL_USE_INTR_CONTROL                      1
#define MCSDL_USE_VDD_CONTROL                       0
#define MCSDL_USE_RESETB_CONTROL                    0


//============================================================
//
//    Porting factors for Baseband
//
//============================================================

#include "melfas_download_porting.h"


//----------------------------------------------------
//    Functions
//----------------------------------------------------

int mcsdl_download_binary_data(void);            // with binary type .c   file.
int mcsdl_download_binary_file(char *fileName);            // with binary type .bin file.

#if MELFAS_ENABLE_DELAY_TEST                    // For initial porting test.
void mcsdl_delay_test(INT32 nCount);
#endif


#endif        //#ifndef __MELFAS_FIRMWARE_DOWNLOAD_H__

