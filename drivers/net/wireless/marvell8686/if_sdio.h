/** @file if_sdio.h
 *  @brief This file contains SDIO IF (interface) module
 *  related macros, enum, and structure.
 *
 *  Copyright © Marvell International Ltd. and/or its affiliates, 2003-2006
 */
/****************************************************
Change log:
	10/12/05: add Doxygen format comments 
****************************************************/

#ifndef	_IF_SDIO_H_
#define	_IF_SDIO_H_

#include	"include.h"
#include	"sdio.h"

#define SD_BUS_WIDTH_1			0x00
#define SD_BUS_WIDTH_4			0x02
#define SD_BUS_WIDTH_MASK		0x03
#define ASYNC_INT_MODE			0x20

/* Host Control Registers */
#define IO_PORT_0_REG			0x00
#define IO_PORT_1_REG			0x01
#define IO_PORT_2_REG			0x02
#define CONFIGURATION_REG		0x03
#define HOST_WO_CMD53_FINISH_HOST	(0x1U << 2)
#define HOST_POWER_UP			(0x1U << 1)
#define HOST_POWER_DOWN			(0x1U << 0)
#define HOST_INT_MASK_REG		0x04
#define UP_LD_HOST_INT_MASK		(0x1U)
#define DN_LD_HOST_INT_MASK		(0x2U)
#define HOST_INTSTATUS_REG		0x05
#define UP_LD_HOST_INT_STATUS		(0x1U)
#define DN_LD_HOST_INT_STATUS		(0x2U)
#define HOST_INT_RSR_REG		0x06
#define UP_LD_HOST_INT_RSR		(0x1U)
#define HOST_INT_STATUS_REG		0x07
#define UP_LD_CRC_ERR			(0x1U << 2)
#define UP_LD_RESTART              	(0x1U << 1)
#define DN_LD_RESTART              	(0x1U << 0)

/* Card Control Registers */
#define SQ_READ_BASE_ADDRESS_A0_REG  	0x10
#define SQ_READ_BASE_ADDRESS_A1_REG  	0x11
#define SQ_READ_BASE_ADDRESS_A2_REG  	0x12
#define SQ_READ_BASE_ADDRESS_A3_REG  	0x13
#define SQ_READ_BASE_ADDRESS_B0_REG  	0x14
#define SQ_READ_BASE_ADDRESS_B1_REG  	0x15
#define SQ_READ_BASE_ADDRESS_B2_REG  	0x16
#define SQ_READ_BASE_ADDRESS_B3_REG  	0x17
#define CARD_STATUS_REG              	0x20
#define CARD_IO_READY              	(0x1U << 3)
#define CIS_CARD_RDY                 	(0x1U << 2)
#define UP_LD_CARD_RDY               	(0x1U << 1)
#define DN_LD_CARD_RDY               	(0x1U << 0)
#define HOST_INTERRUPT_MASK_REG      	0x24
#define HOST_POWER_INT_MASK          	(0x1U << 3)
#define ABORT_CARD_INT_MASK          	(0x1U << 2)
#define UP_LD_CARD_INT_MASK          	(0x1U << 1)
#define DN_LD_CARD_INT_MASK          	(0x1U << 0)
#define CARD_INTERRUPT_STATUS_REG    	0x28
#define POWER_UP_INT                 	(0x1U << 4)
#define POWER_DOWN_INT               	(0x1U << 3)
#define CARD_INTERRUPT_RSR_REG       	0x2c
#define POWER_UP_RSR                 	(0x1U << 4)
#define POWER_DOWN_RSR               	(0x1U << 3)
#define DEBUG_0_REG                  	0x30
#define SD_TESTBUS0                  	(0x1U)
#define DEBUG_1_REG                  	0x31
#define SD_TESTBUS1                  	(0x1U)
#define DEBUG_2_REG                  	0x32
#define SD_TESTBUS2                  	(0x1U)
#define DEBUG_3_REG                  	0x33
#define SD_TESTBUS3                  	(0x1U)
#define CARD_OCR_0_REG               	0x34
#define CARD_OCR_1_REG               	0x35
#define CARD_OCR_3_REG               	0x36
#define CARD_CONFIG_REG              	0x38
#define CARD_REVISION_REG            	0x3c
#define CMD53_FINISH_GBUS            	(0x1U << 1)
#define SD_NEG_EDGE                  	(0x1U << 0)

/* Special registers in function 0 of the SDxx card */
#define	SCRATCH_0_REG			0x80fe
#define	SCRATCH_1_REG			0x80ff

#define HOST_F1_RD_BASE_0		0x0010
#define HOST_F1_RD_BASE_1		0x0011
#define HOST_F1_CARD_RDY		0x0020

/********************************************************
		Global Functions
********************************************************/

#endif /* _IF_SDIO_H */
