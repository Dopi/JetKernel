/*******************************************************************************

  Intel 10 Gigabit PCI Express Linux driver
  Copyright(c) 1999 - 2008 Intel Corporation.

  This program is free software; you can redistribute it and/or modify it
  under the terms and conditions of the GNU General Public License,
  version 2, as published by the Free Software Foundation.

  This program is distributed in the hope it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  You should have received a copy of the GNU General Public License along with
  this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.

  The full GNU General Public License is included in this distribution in
  the file called "COPYING".

  Contact Information:
  e1000-devel Mailing List <e1000-devel@lists.sourceforge.net>
  Intel Corporation, 5200 N.E. Elam Young Parkway, Hillsboro, OR 97124-6497

*******************************************************************************/

#ifndef _IXGBE_TYPE_H_
#define _IXGBE_TYPE_H_

#include <linux/types.h>

/* Vendor ID */
#define IXGBE_INTEL_VENDOR_ID   0x8086

/* Device IDs */
#define IXGBE_DEV_ID_82598AF_DUAL_PORT   0x10C6
#define IXGBE_DEV_ID_82598AF_SINGLE_PORT 0x10C7
#define IXGBE_DEV_ID_82598EB_SFP_LOM     0x10DB
#define IXGBE_DEV_ID_82598AT             0x10C8
#define IXGBE_DEV_ID_82598EB_CX4         0x10DD
#define IXGBE_DEV_ID_82598_CX4_DUAL_PORT 0x10EC
#define IXGBE_DEV_ID_82598_DA_DUAL_PORT  0x10F1
#define IXGBE_DEV_ID_82598_SR_DUAL_PORT_EM      0x10E1
#define IXGBE_DEV_ID_82598EB_XF_LR       0x10F4

/* General Registers */
#define IXGBE_CTRL      0x00000
#define IXGBE_STATUS    0x00008
#define IXGBE_CTRL_EXT  0x00018
#define IXGBE_ESDP      0x00020
#define IXGBE_EODSDP    0x00028
#define IXGBE_LEDCTL    0x00200
#define IXGBE_FRTIMER   0x00048
#define IXGBE_TCPTIMER  0x0004C

/* NVM Registers */
#define IXGBE_EEC       0x10010
#define IXGBE_EERD      0x10014
#define IXGBE_FLA       0x1001C
#define IXGBE_EEMNGCTL  0x10110
#define IXGBE_EEMNGDATA 0x10114
#define IXGBE_FLMNGCTL  0x10118
#define IXGBE_FLMNGDATA 0x1011C
#define IXGBE_FLMNGCNT  0x10120
#define IXGBE_FLOP      0x1013C
#define IXGBE_GRC       0x10200

/* Interrupt Registers */
#define IXGBE_EICR      0x00800
#define IXGBE_EICS      0x00808
#define IXGBE_EIMS      0x00880
#define IXGBE_EIMC      0x00888
#define IXGBE_EIAC      0x00810
#define IXGBE_EIAM      0x00890
#define IXGBE_EITR(_i)  (((_i) <= 23) ? (0x00820 + ((_i) * 4)) : (0x012300 + ((_i) * 4)))
#define IXGBE_IVAR(_i)  (0x00900 + ((_i) * 4)) /* 24 at 0x900-0x960 */
#define IXGBE_MSIXT     0x00000 /* MSI-X Table. 0x0000 - 0x01C */
#define IXGBE_MSIXPBA   0x02000 /* MSI-X Pending bit array */
#define IXGBE_PBACL(_i) (((_i) == 0) ? (0x11068) : (0x110C0 + ((_i) * 4)))
#define IXGBE_GPIE      0x00898

/* Flow Control Registers */
#define IXGBE_PFCTOP    0x03008
#define IXGBE_FCTTV(_i) (0x03200 + ((_i) * 4)) /* 4 of these (0-3) */
#define IXGBE_FCRTL(_i) (0x03220 + ((_i) * 8)) /* 8 of these (0-7) */
#define IXGBE_FCRTH(_i) (0x03260 + ((_i) * 8)) /* 8 of these (0-7) */
#define IXGBE_FCRTV     0x032A0
#define IXGBE_TFCS      0x0CE00

/* Receive DMA Registers */
#define IXGBE_RDBAL(_i) (((_i) < 64) ? (0x01000 + ((_i) * 0x40)) : (0x0D000 + ((_i - 64) * 0x40)))
#define IXGBE_RDBAH(_i) (((_i) < 64) ? (0x01004 + ((_i) * 0x40)) : (0x0D004 + ((_i - 64) * 0x40)))
#define IXGBE_RDLEN(_i) (((_i) < 64) ? (0x01008 + ((_i) * 0x40)) : (0x0D008 + ((_i - 64) * 0x40)))
#define IXGBE_RDH(_i)   (((_i) < 64) ? (0x01010 + ((_i) * 0x40)) : (0x0D010 + ((_i - 64) * 0x40)))
#define IXGBE_RDT(_i)   (((_i) < 64) ? (0x01018 + ((_i) * 0x40)) : (0x0D018 + ((_i - 64) * 0x40)))
#define IXGBE_RXDCTL(_i) (((_i) < 64) ? (0x01028 + ((_i) * 0x40)) : (0x0D028 + ((_i - 64) * 0x40)))
/*
 * Split and Replication Receive Control Registers
 * 00-15 : 0x02100 + n*4
 * 16-64 : 0x01014 + n*0x40
 * 64-127: 0x0D014 + (n-64)*0x40
 */
#define IXGBE_SRRCTL(_i) (((_i) <= 15) ? (0x02100 + ((_i) * 4)) : \
                          (((_i) < 64) ? (0x01014 + ((_i) * 0x40)) : \
                          (0x0D014 + ((_i - 64) * 0x40))))
/*
 * Rx DCA Control Register:
 * 00-15 : 0x02200 + n*4
 * 16-64 : 0x0100C + n*0x40
 * 64-127: 0x0D00C + (n-64)*0x40
 */
#define IXGBE_DCA_RXCTRL(_i)    (((_i) <= 15) ? (0x02200 + ((_i) * 4)) : \
                                 (((_i) < 64) ? (0x0100C + ((_i) * 0x40)) : \
                                 (0x0D00C + ((_i - 64) * 0x40))))
#define IXGBE_RDRXCTL           0x02F00
#define IXGBE_RXPBSIZE(_i)      (0x03C00 + ((_i) * 4))
                                             /* 8 of these 0x03C00 - 0x03C1C */
#define IXGBE_RXCTRL    0x03000
#define IXGBE_DROPEN    0x03D04
#define IXGBE_RXPBSIZE_SHIFT 10

/* Receive Registers */
#define IXGBE_RXCSUM    0x05000
#define IXGBE_RFCTL     0x05008
#define IXGBE_DRECCCTL  0x02F08
#define IXGBE_DRECCCTL_DISABLE 0
/* Multicast Table Array - 128 entries */
#define IXGBE_MTA(_i)   (0x05200 + ((_i) * 4))
#define IXGBE_RAL(_i)   (((_i) <= 15) ? (0x05400 + ((_i) * 8)) : (0x0A200 + ((_i) * 8)))
#define IXGBE_RAH(_i)   (((_i) <= 15) ? (0x05404 + ((_i) * 8)) : (0x0A204 + ((_i) * 8)))
/* Packet split receive type */
#define IXGBE_PSRTYPE(_i)    (((_i) <= 15) ? (0x05480 + ((_i) * 4)) : (0x0EA00 + ((_i) * 4)))
/* array of 4096 1-bit vlan filters */
#define IXGBE_VFTA(_i)  (0x0A000 + ((_i) * 4))
/*array of 4096 4-bit vlan vmdq indices */
#define IXGBE_VFTAVIND(_j, _i)  (0x0A200 + ((_j) * 0x200) + ((_i) * 4))
#define IXGBE_FCTRL     0x05080
#define IXGBE_VLNCTRL   0x05088
#define IXGBE_MCSTCTRL  0x05090
#define IXGBE_MRQC      0x05818
#define IXGBE_IMIR(_i)  (0x05A80 + ((_i) * 4))  /* 8 of these (0-7) */
#define IXGBE_IMIREXT(_i)       (0x05AA0 + ((_i) * 4))  /* 8 of these (0-7) */
#define IXGBE_IMIRVP    0x05AC0
#define IXGBE_VMD_CTL   0x0581C
#define IXGBE_RETA(_i)  (0x05C00 + ((_i) * 4))  /* 32 of these (0-31) */
#define IXGBE_RSSRK(_i) (0x05C80 + ((_i) * 4))  /* 10 of these (0-9) */


/* Transmit DMA registers */
#define IXGBE_TDBAL(_i) (0x06000 + ((_i) * 0x40)) /* 32 of these (0-31)*/
#define IXGBE_TDBAH(_i) (0x06004 + ((_i) * 0x40))
#define IXGBE_TDLEN(_i) (0x06008 + ((_i) * 0x40))
#define IXGBE_TDH(_i)   (0x06010 + ((_i) * 0x40))
#define IXGBE_TDT(_i)   (0x06018 + ((_i) * 0x40))
#define IXGBE_TXDCTL(_i) (0x06028 + ((_i) * 0x40))
#define IXGBE_TDWBAL(_i) (0x06038 + ((_i) * 0x40))
#define IXGBE_TDWBAH(_i) (0x0603C + ((_i) * 0x40))
#define IXGBE_DTXCTL    0x07E00

#define IXGBE_DCA_TXCTRL(_i)    (0x07200 + ((_i) * 4)) /* 16 of these (0-15) */
#define IXGBE_TIPG      0x0CB00
#define IXGBE_TXPBSIZE(_i)      (0x0CC00 + ((_i) * 4)) /* 8 of these */
#define IXGBE_MNGTXMAP  0x0CD10
#define IXGBE_TIPG_FIBER_DEFAULT 3
#define IXGBE_TXPBSIZE_SHIFT    10

/* Wake up registers */
#define IXGBE_WUC       0x05800
#define IXGBE_WUFC      0x05808
#define IXGBE_WUS       0x05810
#define IXGBE_IPAV      0x05838
#define IXGBE_IP4AT     0x05840 /* IPv4 table 0x5840-0x5858 */
#define IXGBE_IP6AT     0x05880 /* IPv6 table 0x5880-0x588F */

#define IXGBE_WUPL      0x05900
#define IXGBE_WUPM      0x05A00 /* wake up pkt memory 0x5A00-0x5A7C */
#define IXGBE_FHFT      0x09000 /* Flex host filter table 9000-93FC */

/* Music registers */
#define IXGBE_RMCS      0x03D00
#define IXGBE_DPMCS     0x07F40
#define IXGBE_PDPMCS    0x0CD00
#define IXGBE_RUPPBMR   0x050A0
#define IXGBE_RT2CR(_i) (0x03C20 + ((_i) * 4)) /* 8 of these (0-7) */
#define IXGBE_RT2SR(_i) (0x03C40 + ((_i) * 4)) /* 8 of these (0-7) */
#define IXGBE_TDTQ2TCCR(_i)     (0x0602C + ((_i) * 0x40)) /* 8 of these (0-7) */
#define IXGBE_TDTQ2TCSR(_i)     (0x0622C + ((_i) * 0x40)) /* 8 of these (0-7) */
#define IXGBE_TDPT2TCCR(_i)     (0x0CD20 + ((_i) * 4)) /* 8 of these (0-7) */
#define IXGBE_TDPT2TCSR(_i)     (0x0CD40 + ((_i) * 4)) /* 8 of these (0-7) */



/* Stats registers */
#define IXGBE_CRCERRS   0x04000
#define IXGBE_ILLERRC   0x04004
#define IXGBE_ERRBC     0x04008
#define IXGBE_MSPDC     0x04010
#define IXGBE_MPC(_i)   (0x03FA0 + ((_i) * 4)) /* 8 of these 3FA0-3FBC*/
#define IXGBE_MLFC      0x04034
#define IXGBE_MRFC      0x04038
#define IXGBE_RLEC      0x04040
#define IXGBE_LXONTXC   0x03F60
#define IXGBE_LXONRXC   0x0CF60
#define IXGBE_LXOFFTXC  0x03F68
#define IXGBE_LXOFFRXC  0x0CF68
#define IXGBE_PXONTXC(_i)       (0x03F00 + ((_i) * 4)) /* 8 of these 3F00-3F1C*/
#define IXGBE_PXONRXC(_i)       (0x0CF00 + ((_i) * 4)) /* 8 of these CF00-CF1C*/
#define IXGBE_PXOFFTXC(_i)      (0x03F20 + ((_i) * 4)) /* 8 of these 3F20-3F3C*/
#define IXGBE_PXOFFRXC(_i)      (0x0CF20 + ((_i) * 4)) /* 8 of these CF20-CF3C*/
#define IXGBE_PRC64     0x0405C
#define IXGBE_PRC127    0x04060
#define IXGBE_PRC255    0x04064
#define IXGBE_PRC511    0x04068
#define IXGBE_PRC1023   0x0406C
#define IXGBE_PRC1522   0x04070
#define IXGBE_GPRC      0x04074
#define IXGBE_BPRC      0x04078
#define IXGBE_MPRC      0x0407C
#define IXGBE_GPTC      0x04080
#define IXGBE_GORCL     0x04088
#define IXGBE_GORCH     0x0408C
#define IXGBE_GOTCL     0x04090
#define IXGBE_GOTCH     0x04094
#define IXGBE_RNBC(_i)  (0x03FC0 + ((_i) * 4)) /* 8 of these 3FC0-3FDC*/
#define IXGBE_RUC       0x040A4
#define IXGBE_RFC       0x040A8
#define IXGBE_ROC       0x040AC
#define IXGBE_RJC       0x040B0
#define IXGBE_MNGPRC    0x040B4
#define IXGBE_MNGPDC    0x040B8
#define IXGBE_MNGPTC    0x0CF90
#define IXGBE_TORL      0x040C0
#define IXGBE_TORH      0x040C4
#define IXGBE_TPR       0x040D0
#define IXGBE_TPT       0x040D4
#define IXGBE_PTC64     0x040D8
#define IXGBE_PTC127    0x040DC
#define IXGBE_PTC255    0x040E0
#define IXGBE_PTC511    0x040E4
#define IXGBE_PTC1023   0x040E8
#define IXGBE_PTC1522   0x040EC
#define IXGBE_MPTC      0x040F0
#define IXGBE_BPTC      0x040F4
#define IXGBE_XEC       0x04120

#define IXGBE_RQSMR(_i) (0x02300 + ((_i) * 4)) /* 16 of these */
#define IXGBE_TQSMR(_i) (((_i) <= 7) ? (0x07300 + ((_i) * 4)) : (0x08600 + ((_i) * 4)))

#define IXGBE_QPRC(_i) (0x01030 + ((_i) * 0x40)) /* 16 of these */
#define IXGBE_QPTC(_i) (0x06030 + ((_i) * 0x40)) /* 16 of these */
#define IXGBE_QBRC(_i) (0x01034 + ((_i) * 0x40)) /* 16 of these */
#define IXGBE_QBTC(_i) (0x06034 + ((_i) * 0x40)) /* 16 of these */

/* Management */
#define IXGBE_MAVTV(_i) (0x05010 + ((_i) * 4)) /* 8 of these (0-7) */
#define IXGBE_MFUTP(_i) (0x05030 + ((_i) * 4)) /* 8 of these (0-7) */
#define IXGBE_MANC      0x05820
#define IXGBE_MFVAL     0x05824
#define IXGBE_MANC2H    0x05860
#define IXGBE_MDEF(_i)  (0x05890 + ((_i) * 4)) /* 8 of these (0-7) */
#define IXGBE_MIPAF     0x058B0
#define IXGBE_MMAL(_i)  (0x05910 + ((_i) * 8)) /* 4 of these (0-3) */
#define IXGBE_MMAH(_i)  (0x05914 + ((_i) * 8)) /* 4 of these (0-3) */
#define IXGBE_FTFT      0x09400 /* 0x9400-0x97FC */

/* ARC Subsystem registers */
#define IXGBE_HICR      0x15F00
#define IXGBE_FWSTS     0x15F0C
#define IXGBE_HSMC0R    0x15F04
#define IXGBE_HSMC1R    0x15F08
#define IXGBE_SWSR      0x15F10
#define IXGBE_HFDR      0x15FE8
#define IXGBE_FLEX_MNG  0x15800 /* 0x15800 - 0x15EFC */

/* PCI-E registers */
#define IXGBE_GCR       0x11000
#define IXGBE_GTV       0x11004
#define IXGBE_FUNCTAG   0x11008
#define IXGBE_GLT       0x1100C
#define IXGBE_GSCL_1    0x11010
#define IXGBE_GSCL_2    0x11014
#define IXGBE_GSCL_3    0x11018
#define IXGBE_GSCL_4    0x1101C
#define IXGBE_GSCN_0    0x11020
#define IXGBE_GSCN_1    0x11024
#define IXGBE_GSCN_2    0x11028
#define IXGBE_GSCN_3    0x1102C
#define IXGBE_FACTPS    0x10150
#define IXGBE_PCIEANACTL  0x11040
#define IXGBE_SWSM      0x10140
#define IXGBE_FWSM      0x10148
#define IXGBE_GSSR      0x10160
#define IXGBE_MREVID    0x11064
#define IXGBE_DCA_ID    0x11070
#define IXGBE_DCA_CTRL  0x11074

/* Diagnostic Registers */
#define IXGBE_RDSTATCTL   0x02C20
#define IXGBE_RDSTAT(_i)  (0x02C00 + ((_i) * 4)) /* 0x02C00-0x02C1C */
#define IXGBE_RDHMPN      0x02F08
#define IXGBE_RIC_DW(_i)  (0x02F10 + ((_i) * 4))
#define IXGBE_RDPROBE     0x02F20
#define IXGBE_TDSTATCTL   0x07C20
#define IXGBE_TDSTAT(_i)  (0x07C00 + ((_i) * 4)) /* 0x07C00 - 0x07C1C */
#define IXGBE_TDHMPN      0x07F08
#define IXGBE_TIC_DW(_i)  (0x07F10 + ((_i) * 4))
#define IXGBE_TDPROBE     0x07F20
#define IXGBE_TXBUFCTRL   0x0C600
#define IXGBE_TXBUFDATA0  0x0C610
#define IXGBE_TXBUFDATA1  0x0C614
#define IXGBE_TXBUFDATA2  0x0C618
#define IXGBE_TXBUFDATA3  0x0C61C
#define IXGBE_RXBUFCTRL   0x03600
#define IXGBE_RXBUFDATA0  0x03610
#define IXGBE_RXBUFDATA1  0x03614
#define IXGBE_RXBUFDATA2  0x03618
#define IXGBE_RXBUFDATA3  0x0361C
#define IXGBE_PCIE_DIAG(_i)     (0x11090 + ((_i) * 4)) /* 8 of these */
#define IXGBE_RFVAL     0x050A4
#define IXGBE_MDFTC1    0x042B8
#define IXGBE_MDFTC2    0x042C0
#define IXGBE_MDFTFIFO1 0x042C4
#define IXGBE_MDFTFIFO2 0x042C8
#define IXGBE_MDFTS     0x042CC
#define IXGBE_RXDATAWRPTR(_i)   (0x03700 + ((_i) * 4)) /* 8 of these 3700-370C*/
#define IXGBE_RXDESCWRPTR(_i)   (0x03710 + ((_i) * 4)) /* 8 of these 3710-371C*/
#define IXGBE_RXDATARDPTR(_i)   (0x03720 + ((_i) * 4)) /* 8 of these 3720-372C*/
#define IXGBE_RXDESCRDPTR(_i)   (0x03730 + ((_i) * 4)) /* 8 of these 3730-373C*/
#define IXGBE_TXDATAWRPTR(_i)   (0x0C700 + ((_i) * 4)) /* 8 of these C700-C70C*/
#define IXGBE_TXDESCWRPTR(_i)   (0x0C710 + ((_i) * 4)) /* 8 of these C710-C71C*/
#define IXGBE_TXDATARDPTR(_i)   (0x0C720 + ((_i) * 4)) /* 8 of these C720-C72C*/
#define IXGBE_TXDESCRDPTR(_i)   (0x0C730 + ((_i) * 4)) /* 8 of these C730-C73C*/
#define IXGBE_PCIEECCCTL 0x1106C
#define IXGBE_PBTXECC   0x0C300
#define IXGBE_PBRXECC   0x03300
#define IXGBE_GHECCR    0x110B0

/* MAC Registers */
#define IXGBE_PCS1GCFIG 0x04200
#define IXGBE_PCS1GLCTL 0x04208
#define IXGBE_PCS1GLSTA 0x0420C
#define IXGBE_PCS1GDBG0 0x04210
#define IXGBE_PCS1GDBG1 0x04214
#define IXGBE_PCS1GANA  0x04218
#define IXGBE_PCS1GANLP 0x0421C
#define IXGBE_PCS1GANNP 0x04220
#define IXGBE_PCS1GANLPNP 0x04224
#define IXGBE_HLREG0    0x04240
#define IXGBE_HLREG1    0x04244
#define IXGBE_PAP       0x04248
#define IXGBE_MACA      0x0424C
#define IXGBE_APAE      0x04250
#define IXGBE_ARD       0x04254
#define IXGBE_AIS       0x04258
#define IXGBE_MSCA      0x0425C
#define IXGBE_MSRWD     0x04260
#define IXGBE_MLADD     0x04264
#define IXGBE_MHADD     0x04268
#define IXGBE_TREG      0x0426C
#define IXGBE_PCSS1     0x04288
#define IXGBE_PCSS2     0x0428C
#define IXGBE_XPCSS     0x04290
#define IXGBE_SERDESC   0x04298
#define IXGBE_MACS      0x0429C
#define IXGBE_AUTOC     0x042A0
#define IXGBE_LINKS     0x042A4
#define IXGBE_AUTOC2    0x042A8
#define IXGBE_AUTOC3    0x042AC
#define IXGBE_ANLP1     0x042B0
#define IXGBE_ANLP2     0x042B4
#define IXGBE_ATLASCTL  0x04800

/* RDRXCTL Bit Masks */
#define IXGBE_RDRXCTL_RDMTS_1_2     0x00000000 /* Rx Desc Min Threshold Size */
#define IXGBE_RDRXCTL_MVMEN         0x00000020
#define IXGBE_RDRXCTL_DMAIDONE      0x00000008 /* DMA init cycle done */

/* CTRL Bit Masks */
#define IXGBE_CTRL_GIO_DIS      0x00000004 /* Global IO Master Disable bit */
#define IXGBE_CTRL_LNK_RST      0x00000008 /* Link Reset. Resets everything. */
#define IXGBE_CTRL_RST          0x04000000 /* Reset (SW) */

/* FACTPS */
#define IXGBE_FACTPS_LFS        0x40000000 /* LAN Function Select */

/* MHADD Bit Masks */
#define IXGBE_MHADD_MFS_MASK    0xFFFF0000
#define IXGBE_MHADD_MFS_SHIFT   16

/* Extended Device Control */
#define IXGBE_CTRL_EXT_NS_DIS   0x00010000 /* No Snoop disable */
#define IXGBE_CTRL_EXT_RO_DIS   0x00020000 /* Relaxed Ordering disable */
#define IXGBE_CTRL_EXT_DRV_LOAD 0x10000000 /* Driver loaded bit for FW */

/* Direct Cache Access (DCA) definitions */
#define IXGBE_DCA_CTRL_DCA_ENABLE  0x00000000 /* DCA Enable */
#define IXGBE_DCA_CTRL_DCA_DISABLE 0x00000001 /* DCA Disable */

#define IXGBE_DCA_CTRL_DCA_MODE_CB1 0x00 /* DCA Mode CB1 */
#define IXGBE_DCA_CTRL_DCA_MODE_CB2 0x02 /* DCA Mode CB2 */

#define IXGBE_DCA_RXCTRL_CPUID_MASK 0x0000001F /* Rx CPUID Mask */
#define IXGBE_DCA_RXCTRL_DESC_DCA_EN (1 << 5) /* DCA Rx Desc enable */
#define IXGBE_DCA_RXCTRL_HEAD_DCA_EN (1 << 6) /* DCA Rx Desc header enable */
#define IXGBE_DCA_RXCTRL_DATA_DCA_EN (1 << 7) /* DCA Rx Desc payload enable */
#define IXGBE_DCA_RXCTRL_DESC_RRO_EN (1 << 9) /* DCA Rx rd Desc Relax Order */
#define IXGBE_DCA_RXCTRL_DESC_WRO_EN (1 << 13) /* DCA Rx wr Desc Relax Order */
#define IXGBE_DCA_RXCTRL_DESC_HSRO_EN (1 << 15) /* DCA Rx Split Header RO */

#define IXGBE_DCA_TXCTRL_CPUID_MASK 0x0000001F /* Tx CPUID Mask */
#define IXGBE_DCA_TXCTRL_DESC_DCA_EN (1 << 5) /* DCA Tx Desc enable */
#define IXGBE_DCA_TXCTRL_TX_WB_RO_EN (1 << 11) /* Tx Desc writeback RO bit */
#define IXGBE_DCA_MAX_QUEUES_82598   16 /* DCA regs only on 16 queues */

/* MSCA Bit Masks */
#define IXGBE_MSCA_NP_ADDR_MASK      0x0000FFFF /* MDI Address (new protocol) */
#define IXGBE_MSCA_NP_ADDR_SHIFT     0
#define IXGBE_MSCA_DEV_TYPE_MASK     0x001F0000 /* Device Type (new protocol) */
#define IXGBE_MSCA_DEV_TYPE_SHIFT    16 /* Register Address (old protocol */
#define IXGBE_MSCA_PHY_ADDR_MASK     0x03E00000 /* PHY Address mask */
#define IXGBE_MSCA_PHY_ADDR_SHIFT    21 /* PHY Address shift*/
#define IXGBE_MSCA_OP_CODE_MASK      0x0C000000 /* OP CODE mask */
#define IXGBE_MSCA_OP_CODE_SHIFT     26 /* OP CODE shift */
#define IXGBE_MSCA_ADDR_CYCLE        0x00000000 /* OP CODE 00 (addr cycle) */
#define IXGBE_MSCA_WRITE             0x04000000 /* OP CODE 01 (write) */
#define IXGBE_MSCA_READ              0x08000000 /* OP CODE 10 (read) */
#define IXGBE_MSCA_READ_AUTOINC      0x0C000000 /* OP CODE 11 (read, auto inc)*/
#define IXGBE_MSCA_ST_CODE_MASK      0x30000000 /* ST Code mask */
#define IXGBE_MSCA_ST_CODE_SHIFT     28 /* ST Code shift */
#define IXGBE_MSCA_NEW_PROTOCOL      0x00000000 /* ST CODE 00 (new protocol) */
#define IXGBE_MSCA_OLD_PROTOCOL      0x10000000 /* ST CODE 01 (old protocol) */
#define IXGBE_MSCA_MDI_COMMAND       0x40000000 /* Initiate MDI command */
#define IXGBE_MSCA_MDI_IN_PROG_EN    0x80000000 /* MDI in progress enable */

/* MSRWD bit masks */
#define IXGBE_MSRWD_WRITE_DATA_MASK     0x0000FFFF
#define IXGBE_MSRWD_WRITE_DATA_SHIFT    0
#define IXGBE_MSRWD_READ_DATA_MASK      0xFFFF0000
#define IXGBE_MSRWD_READ_DATA_SHIFT     16

/* Atlas registers */
#define IXGBE_ATLAS_PDN_LPBK    0x24
#define IXGBE_ATLAS_PDN_10G     0xB
#define IXGBE_ATLAS_PDN_1G      0xC
#define IXGBE_ATLAS_PDN_AN      0xD

/* Atlas bit masks */
#define IXGBE_ATLASCTL_WRITE_CMD        0x00010000
#define IXGBE_ATLAS_PDN_TX_REG_EN       0x10
#define IXGBE_ATLAS_PDN_TX_10G_QL_ALL   0xF0
#define IXGBE_ATLAS_PDN_TX_1G_QL_ALL    0xF0
#define IXGBE_ATLAS_PDN_TX_AN_QL_ALL    0xF0


/* Device Type definitions for new protocol MDIO commands */
#define IXGBE_MDIO_PMA_PMD_DEV_TYPE               0x1
#define IXGBE_MDIO_PCS_DEV_TYPE                   0x3
#define IXGBE_MDIO_PHY_XS_DEV_TYPE                0x4
#define IXGBE_MDIO_AUTO_NEG_DEV_TYPE              0x7
#define IXGBE_MDIO_VENDOR_SPECIFIC_1_DEV_TYPE     0x1E   /* Device 30 */
#define IXGBE_TWINAX_DEV                          1

#define IXGBE_MDIO_COMMAND_TIMEOUT     100 /* PHY Timeout for 1 GB mode */

#define IXGBE_MDIO_VENDOR_SPECIFIC_1_CONTROL      0x0    /* VS1 Control Reg */
#define IXGBE_MDIO_VENDOR_SPECIFIC_1_STATUS       0x1    /* VS1 Status Reg */
#define IXGBE_MDIO_VENDOR_SPECIFIC_1_LINK_STATUS  0x0008 /* 1 = Link Up */
#define IXGBE_MDIO_VENDOR_SPECIFIC_1_SPEED_STATUS 0x0010 /* 0 - 10G, 1 - 1G */
#define IXGBE_MDIO_VENDOR_SPECIFIC_1_10G_SPEED    0x0018
#define IXGBE_MDIO_VENDOR_SPECIFIC_1_1G_SPEED     0x0010

#define IXGBE_MDIO_AUTO_NEG_CONTROL    0x0 /* AUTO_NEG Control Reg */
#define IXGBE_MDIO_AUTO_NEG_STATUS     0x1 /* AUTO_NEG Status Reg */
#define IXGBE_MDIO_PHY_XS_CONTROL      0x0 /* PHY_XS Control Reg */
#define IXGBE_MDIO_PHY_XS_RESET        0x8000 /* PHY_XS Reset */
#define IXGBE_MDIO_PHY_ID_HIGH         0x2 /* PHY ID High Reg*/
#define IXGBE_MDIO_PHY_ID_LOW          0x3 /* PHY ID Low Reg*/
#define IXGBE_MDIO_PHY_SPEED_ABILITY   0x4 /* Speed Ability Reg */
#define IXGBE_MDIO_PHY_SPEED_10G       0x0001 /* 10G capable */
#define IXGBE_MDIO_PHY_SPEED_1G        0x0010 /* 1G capable */

#define IXGBE_MDIO_PMA_PMD_SDA_SCL_ADDR     0xC30A /* PHY_XS SDA/SCL Address Reg */
#define IXGBE_MDIO_PMA_PMD_SDA_SCL_DATA     0xC30B /* PHY_XS SDA/SCL Data Reg */
#define IXGBE_MDIO_PMA_PMD_SDA_SCL_STAT     0xC30C /* PHY_XS SDA/SCL Status Reg */

/* MII clause 22/28 definitions */
#define IXGBE_MDIO_PHY_LOW_POWER_MODE  0x0800

#define IXGBE_MII_SPEED_SELECTION_REG  0x10
#define IXGBE_MII_RESTART              0x200
#define IXGBE_MII_AUTONEG_COMPLETE     0x20
#define IXGBE_MII_AUTONEG_REG          0x0

#define IXGBE_PHY_REVISION_MASK        0xFFFFFFF0
#define IXGBE_MAX_PHY_ADDR             32

/* PHY IDs */
#define TN1010_PHY_ID    0x00A19410
#define TNX_FW_REV       0xB
#define QT2022_PHY_ID    0x0043A400
#define ATH_PHY_ID       0x03429050

/* PHY Types */
#define IXGBE_M88E1145_E_PHY_ID  0x01410CD0

/* Special PHY Init Routine */
#define IXGBE_PHY_INIT_OFFSET_NL 0x002B
#define IXGBE_PHY_INIT_END_NL    0xFFFF
#define IXGBE_CONTROL_MASK_NL    0xF000
#define IXGBE_DATA_MASK_NL       0x0FFF
#define IXGBE_CONTROL_SHIFT_NL   12
#define IXGBE_DELAY_NL           0
#define IXGBE_DATA_NL            1
#define IXGBE_CONTROL_NL         0x000F
#define IXGBE_CONTROL_EOL_NL     0x0FFF
#define IXGBE_CONTROL_SOL_NL     0x0000

/* General purpose Interrupt Enable */
#define IXGBE_SDP0_GPIEN         0x00000001 /* SDP0 */
#define IXGBE_SDP1_GPIEN         0x00000002 /* SDP1 */
#define IXGBE_GPIE_MSIX_MODE     0x00000010 /* MSI-X mode */
#define IXGBE_GPIE_OCD           0x00000020 /* Other Clear Disable */
#define IXGBE_GPIE_EIMEN         0x00000040 /* Immediate Interrupt Enable */
#define IXGBE_GPIE_EIAME         0x40000000
#define IXGBE_GPIE_PBA_SUPPORT   0x80000000

/* Transmit Flow Control status */
#define IXGBE_TFCS_TXOFF         0x00000001
#define IXGBE_TFCS_TXOFF0        0x00000100
#define IXGBE_TFCS_TXOFF1        0x00000200
#define IXGBE_TFCS_TXOFF2        0x00000400
#define IXGBE_TFCS_TXOFF3        0x00000800
#define IXGBE_TFCS_TXOFF4        0x00001000
#define IXGBE_TFCS_TXOFF5        0x00002000
#define IXGBE_TFCS_TXOFF6        0x00004000
#define IXGBE_TFCS_TXOFF7        0x00008000

/* TCP Timer */
#define IXGBE_TCPTIMER_KS            0x00000100
#define IXGBE_TCPTIMER_COUNT_ENABLE  0x00000200
#define IXGBE_TCPTIMER_COUNT_FINISH  0x00000400
#define IXGBE_TCPTIMER_LOOP          0x00000800
#define IXGBE_TCPTIMER_DURATION_MASK 0x000000FF

/* HLREG0 Bit Masks */
#define IXGBE_HLREG0_TXCRCEN      0x00000001   /* bit  0 */
#define IXGBE_HLREG0_RXCRCSTRP    0x00000002   /* bit  1 */
#define IXGBE_HLREG0_JUMBOEN      0x00000004   /* bit  2 */
#define IXGBE_HLREG0_TXPADEN      0x00000400   /* bit 10 */
#define IXGBE_HLREG0_TXPAUSEEN    0x00001000   /* bit 12 */
#define IXGBE_HLREG0_RXPAUSEEN    0x00004000   /* bit 14 */
#define IXGBE_HLREG0_LPBK         0x00008000   /* bit 15 */
#define IXGBE_HLREG0_MDCSPD       0x00010000   /* bit 16 */
#define IXGBE_HLREG0_CONTMDC      0x00020000   /* bit 17 */
#define IXGBE_HLREG0_CTRLFLTR     0x00040000   /* bit 18 */
#define IXGBE_HLREG0_PREPEND      0x00F00000   /* bits 20-23 */
#define IXGBE_HLREG0_PRIPAUSEEN   0x01000000   /* bit 24 */
#define IXGBE_HLREG0_RXPAUSERECDA 0x06000000   /* bits 25-26 */
#define IXGBE_HLREG0_RXLNGTHERREN 0x08000000   /* bit 27 */
#define IXGBE_HLREG0_RXPADSTRIPEN 0x10000000   /* bit 28 */

/* VMD_CTL bitmasks */
#define IXGBE_VMD_CTL_VMDQ_EN     0x00000001
#define IXGBE_VMD_CTL_VMDQ_FILTER 0x00000002

/* RDHMPN and TDHMPN bitmasks */
#define IXGBE_RDHMPN_RDICADDR       0x007FF800
#define IXGBE_RDHMPN_RDICRDREQ      0x00800000
#define IXGBE_RDHMPN_RDICADDR_SHIFT 11
#define IXGBE_TDHMPN_TDICADDR       0x003FF800
#define IXGBE_TDHMPN_TDICRDREQ      0x00800000
#define IXGBE_TDHMPN_TDICADDR_SHIFT 11

/* Receive Checksum Control */
#define IXGBE_RXCSUM_IPPCSE     0x00001000   /* IP payload checksum enable */
#define IXGBE_RXCSUM_PCSD       0x00002000   /* packet checksum disabled */

/* FCRTL Bit Masks */
#define IXGBE_FCRTL_XONE        0x80000000  /* bit 31, XON enable */
#define IXGBE_FCRTH_FCEN        0x80000000  /* Rx Flow control enable */

/* PAP bit masks*/
#define IXGBE_PAP_TXPAUSECNT_MASK   0x0000FFFF /* Pause counter mask */

/* RMCS Bit Masks */
#define IXGBE_RMCS_RRM          0x00000002 /* Receive Recycle Mode enable */
/* Receive Arbitration Control: 0 Round Robin, 1 DFP */
#define IXGBE_RMCS_RAC          0x00000004
#define IXGBE_RMCS_DFP          IXGBE_RMCS_RAC /* Deficit Fixed Priority ena */
#define IXGBE_RMCS_TFCE_802_3X  0x00000008 /* Tx Priority flow control ena */
#define IXGBE_RMCS_TFCE_PRIORITY 0x00000010 /* Tx Priority flow control ena */
#define IXGBE_RMCS_ARBDIS       0x00000040 /* Arbitration disable bit */


/* Interrupt register bitmasks */

/* Extended Interrupt Cause Read */
#define IXGBE_EICR_RTX_QUEUE    0x0000FFFF /* RTx Queue Interrupt */
#define IXGBE_EICR_LSC          0x00100000 /* Link Status Change */
#define IXGBE_EICR_MNG          0x00400000 /* Manageability Event Interrupt */
#define IXGBE_EICR_GPI_SDP0     0x01000000 /* Gen Purpose Interrupt on SDP0 */
#define IXGBE_EICR_GPI_SDP1     0x02000000 /* Gen Purpose Interrupt on SDP1 */
#define IXGBE_EICR_PBUR         0x10000000 /* Packet Buffer Handler Error */
#define IXGBE_EICR_DHER         0x20000000 /* Descriptor Handler Error */
#define IXGBE_EICR_TCP_TIMER    0x40000000 /* TCP Timer */
#define IXGBE_EICR_OTHER        0x80000000 /* Interrupt Cause Active */

/* Extended Interrupt Cause Set */
#define IXGBE_EICS_RTX_QUEUE    IXGBE_EICR_RTX_QUEUE /* RTx Queue Interrupt */
#define IXGBE_EICS_LSC          IXGBE_EICR_LSC       /* Link Status Change */
#define IXGBE_EICS_MNG          IXGBE_EICR_MNG       /* MNG Event Interrupt */
#define IXGBE_EICS_GPI_SDP0     IXGBE_EICR_GPI_SDP0  /* SDP0 Gen Purpose Int */
#define IXGBE_EICS_GPI_SDP1     IXGBE_EICR_GPI_SDP1  /* SDP1 Gen Purpose Int */
#define IXGBE_EICS_PBUR         IXGBE_EICR_PBUR      /* Pkt Buf Handler Err */
#define IXGBE_EICS_DHER         IXGBE_EICR_DHER      /* Desc Handler Error */
#define IXGBE_EICS_TCP_TIMER    IXGBE_EICR_TCP_TIMER /* TCP Timer */
#define IXGBE_EICS_OTHER        IXGBE_EICR_OTHER     /* INT Cause Active */

/* Extended Interrupt Mask Set */
#define IXGBE_EIMS_RTX_QUEUE    IXGBE_EICR_RTX_QUEUE /* RTx Queue Interrupt */
#define IXGBE_EIMS_LSC          IXGBE_EICR_LSC       /* Link Status Change */
#define IXGBE_EIMS_MNG          IXGBE_EICR_MNG       /* MNG Event Interrupt */
#define IXGBE_EIMS_GPI_SDP0     IXGBE_EICR_GPI_SDP0  /* SDP0 Gen Purpose Int */
#define IXGBE_EIMS_GPI_SDP1     IXGBE_EICR_GPI_SDP1  /* SDP1 Gen Purpose Int */
#define IXGBE_EIMS_PBUR         IXGBE_EICR_PBUR      /* Pkt Buf Handler Err */
#define IXGBE_EIMS_DHER         IXGBE_EICR_DHER      /* Descr Handler Error */
#define IXGBE_EIMS_TCP_TIMER    IXGBE_EICR_TCP_TIMER /* TCP Timer */
#define IXGBE_EIMS_OTHER        IXGBE_EICR_OTHER     /* INT Cause Active */

/* Extended Interrupt Mask Clear */
#define IXGBE_EIMC_RTX_QUEUE    IXGBE_EICR_RTX_QUEUE /* RTx Queue Interrupt */
#define IXGBE_EIMC_LSC          IXGBE_EICR_LSC       /* Link Status Change */
#define IXGBE_EIMC_MNG          IXGBE_EICR_MNG       /* MNG Event Interrupt */
#define IXGBE_EIMC_GPI_SDP0     IXGBE_EICR_GPI_SDP0  /* SDP0 Gen Purpose Int */
#define IXGBE_EIMC_GPI_SDP1     IXGBE_EICR_GPI_SDP1  /* SDP1 Gen Purpose Int */
#define IXGBE_EIMC_PBUR         IXGBE_EICR_PBUR      /* Pkt Buf Handler Err */
#define IXGBE_EIMC_DHER         IXGBE_EICR_DHER      /* Desc Handler Err */
#define IXGBE_EIMC_TCP_TIMER    IXGBE_EICR_TCP_TIMER /* TCP Timer */
#define IXGBE_EIMC_OTHER        IXGBE_EICR_OTHER     /* INT Cause Active */

#define IXGBE_EIMS_ENABLE_MASK ( \
                                IXGBE_EIMS_RTX_QUEUE       | \
                                IXGBE_EIMS_LSC             | \
                                IXGBE_EIMS_TCP_TIMER       | \
                                IXGBE_EIMS_OTHER)

/* Immediate Interrupt Rx (A.K.A. Low Latency Interrupt) */
#define IXGBE_IMIR_PORT_IM_EN     0x00010000  /* TCP port enable */
#define IXGBE_IMIR_PORT_BP        0x00020000  /* TCP port check bypass */
#define IXGBE_IMIREXT_SIZE_BP     0x00001000  /* Packet size bypass */
#define IXGBE_IMIREXT_CTRL_URG    0x00002000  /* Check URG bit in header */
#define IXGBE_IMIREXT_CTRL_ACK    0x00004000  /* Check ACK bit in header */
#define IXGBE_IMIREXT_CTRL_PSH    0x00008000  /* Check PSH bit in header */
#define IXGBE_IMIREXT_CTRL_RST    0x00010000  /* Check RST bit in header */
#define IXGBE_IMIREXT_CTRL_SYN    0x00020000  /* Check SYN bit in header */
#define IXGBE_IMIREXT_CTRL_FIN    0x00040000  /* Check FIN bit in header */
#define IXGBE_IMIREXT_CTRL_BP     0x00080000  /* Bypass check of control bits */

/* Interrupt clear mask */
#define IXGBE_IRQ_CLEAR_MASK    0xFFFFFFFF

/* Interrupt Vector Allocation Registers */
#define IXGBE_IVAR_REG_NUM      25
#define IXGBE_IVAR_TXRX_ENTRY   96
#define IXGBE_IVAR_RX_ENTRY     64
#define IXGBE_IVAR_RX_QUEUE(_i)    (0 + (_i))
#define IXGBE_IVAR_TX_QUEUE(_i)    (64 + (_i))
#define IXGBE_IVAR_TX_ENTRY     32

#define IXGBE_IVAR_TCP_TIMER_INDEX       96 /* 0 based index */
#define IXGBE_IVAR_OTHER_CAUSES_INDEX    97 /* 0 based index */

#define IXGBE_MSIX_VECTOR(_i)   (0 + (_i))

#define IXGBE_IVAR_ALLOC_VAL    0x80 /* Interrupt Allocation valid */

/* VLAN Control Bit Masks */
#define IXGBE_VLNCTRL_VET       0x0000FFFF  /* bits 0-15 */
#define IXGBE_VLNCTRL_CFI       0x10000000  /* bit 28 */
#define IXGBE_VLNCTRL_CFIEN     0x20000000  /* bit 29 */
#define IXGBE_VLNCTRL_VFE       0x40000000  /* bit 30 */
#define IXGBE_VLNCTRL_VME       0x80000000  /* bit 31 */


#define IXGBE_ETHERNET_IEEE_VLAN_TYPE 0x8100  /* 802.1q protocol */

/* STATUS Bit Masks */
#define IXGBE_STATUS_LAN_ID     0x0000000C /* LAN ID */
#define IXGBE_STATUS_GIO        0x00080000 /* GIO Master Enable Status */

#define IXGBE_STATUS_LAN_ID_0   0x00000000 /* LAN ID 0 */
#define IXGBE_STATUS_LAN_ID_1   0x00000004 /* LAN ID 1 */

/* ESDP Bit Masks */
#define IXGBE_ESDP_SDP4 0x00000001 /* SDP4 Data Value */
#define IXGBE_ESDP_SDP5 0x00000002 /* SDP5 Data Value */
#define IXGBE_ESDP_SDP4_DIR     0x00000004 /* SDP4 IO direction */
#define IXGBE_ESDP_SDP5_DIR     0x00000008 /* SDP5 IO direction */

/* LEDCTL Bit Masks */
#define IXGBE_LED_IVRT_BASE      0x00000040
#define IXGBE_LED_BLINK_BASE     0x00000080
#define IXGBE_LED_MODE_MASK_BASE 0x0000000F
#define IXGBE_LED_OFFSET(_base, _i) (_base << (8 * (_i)))
#define IXGBE_LED_MODE_SHIFT(_i) (8*(_i))
#define IXGBE_LED_IVRT(_i)       IXGBE_LED_OFFSET(IXGBE_LED_IVRT_BASE, _i)
#define IXGBE_LED_BLINK(_i)      IXGBE_LED_OFFSET(IXGBE_LED_BLINK_BASE, _i)
#define IXGBE_LED_MODE_MASK(_i)  IXGBE_LED_OFFSET(IXGBE_LED_MODE_MASK_BASE, _i)

/* LED modes */
#define IXGBE_LED_LINK_UP       0x0
#define IXGBE_LED_LINK_10G      0x1
#define IXGBE_LED_MAC           0x2
#define IXGBE_LED_FILTER        0x3
#define IXGBE_LED_LINK_ACTIVE   0x4
#define IXGBE_LED_LINK_1G       0x5
#define IXGBE_LED_ON            0xE
#define IXGBE_LED_OFF           0xF

/* AUTOC Bit Masks */
#define IXGBE_AUTOC_KX4_SUPP    0x80000000
#define IXGBE_AUTOC_KX_SUPP     0x40000000
#define IXGBE_AUTOC_PAUSE       0x30000000
#define IXGBE_AUTOC_RF          0x08000000
#define IXGBE_AUTOC_PD_TMR      0x06000000
#define IXGBE_AUTOC_AN_RX_LOOSE 0x01000000
#define IXGBE_AUTOC_AN_RX_DRIFT 0x00800000
#define IXGBE_AUTOC_AN_RX_ALIGN 0x007C0000
#define IXGBE_AUTOC_AN_RESTART  0x00001000
#define IXGBE_AUTOC_FLU         0x00000001
#define IXGBE_AUTOC_LMS_SHIFT   13
#define IXGBE_AUTOC_LMS_MASK            (0x7 << IXGBE_AUTOC_LMS_SHIFT)
#define IXGBE_AUTOC_LMS_1G_LINK_NO_AN   (0x0 << IXGBE_AUTOC_LMS_SHIFT)
#define IXGBE_AUTOC_LMS_10G_LINK_NO_AN  (0x1 << IXGBE_AUTOC_LMS_SHIFT)
#define IXGBE_AUTOC_LMS_1G_AN           (0x2 << IXGBE_AUTOC_LMS_SHIFT)
#define IXGBE_AUTOC_LMS_KX4_AN          (0x4 << IXGBE_AUTOC_LMS_SHIFT)
#define IXGBE_AUTOC_LMS_KX4_AN_1G_AN    (0x6 << IXGBE_AUTOC_LMS_SHIFT)
#define IXGBE_AUTOC_LMS_ATTACH_TYPE     (0x7 << IXGBE_AUTOC_10G_PMA_PMD_SHIFT)

#define IXGBE_AUTOC_1G_PMA_PMD         0x00000200
#define IXGBE_AUTOC_10G_PMA_PMD        0x00000180
#define IXGBE_AUTOC_10G_PMA_PMD_SHIFT 7
#define IXGBE_AUTOC_1G_PMA_PMD_SHIFT 9
#define IXGBE_AUTOC_10G_XAUI   (0x0 << IXGBE_AUTOC_10G_PMA_PMD_SHIFT)
#define IXGBE_AUTOC_10G_KX4    (0x1 << IXGBE_AUTOC_10G_PMA_PMD_SHIFT)
#define IXGBE_AUTOC_10G_CX4    (0x2 << IXGBE_AUTOC_10G_PMA_PMD_SHIFT)
#define IXGBE_AUTOC_1G_BX      (0x0 << IXGBE_AUTOC_1G_PMA_PMD_SHIFT)
#define IXGBE_AUTOC_1G_KX      (0x1 << IXGBE_AUTOC_1G_PMA_PMD_SHIFT)

/* LINKS Bit Masks */
#define IXGBE_LINKS_KX_AN_COMP  0x80000000
#define IXGBE_LINKS_UP          0x40000000
#define IXGBE_LINKS_SPEED       0x20000000
#define IXGBE_LINKS_MODE        0x18000000
#define IXGBE_LINKS_RX_MODE     0x06000000
#define IXGBE_LINKS_TX_MODE     0x01800000
#define IXGBE_LINKS_XGXS_EN     0x00400000
#define IXGBE_LINKS_PCS_1G_EN   0x00200000
#define IXGBE_LINKS_1G_AN_EN    0x00100000
#define IXGBE_LINKS_KX_AN_IDLE  0x00080000
#define IXGBE_LINKS_1G_SYNC     0x00040000
#define IXGBE_LINKS_10G_ALIGN   0x00020000
#define IXGBE_LINKS_10G_LANE_SYNC 0x00017000
#define IXGBE_LINKS_TL_FAULT    0x00001000
#define IXGBE_LINKS_SIGNAL      0x00000F00

#define IXGBE_LINK_UP_TIME      90 /* 9.0 Seconds */
#define IXGBE_AUTO_NEG_TIME     45 /* 4.5 Seconds */

/* SW Semaphore Register bitmasks */
#define IXGBE_SWSM_SMBI 0x00000001 /* Driver Semaphore bit */
#define IXGBE_SWSM_SWESMBI 0x00000002 /* FW Semaphore bit */
#define IXGBE_SWSM_WMNG 0x00000004 /* Wake MNG Clock */

/* GSSR definitions */
#define IXGBE_GSSR_EEP_SM     0x0001
#define IXGBE_GSSR_PHY0_SM    0x0002
#define IXGBE_GSSR_PHY1_SM    0x0004
#define IXGBE_GSSR_MAC_CSR_SM 0x0008
#define IXGBE_GSSR_FLASH_SM   0x0010

/* EEC Register */
#define IXGBE_EEC_SK        0x00000001 /* EEPROM Clock */
#define IXGBE_EEC_CS        0x00000002 /* EEPROM Chip Select */
#define IXGBE_EEC_DI        0x00000004 /* EEPROM Data In */
#define IXGBE_EEC_DO        0x00000008 /* EEPROM Data Out */
#define IXGBE_EEC_FWE_MASK  0x00000030 /* FLASH Write Enable */
#define IXGBE_EEC_FWE_DIS   0x00000010 /* Disable FLASH writes */
#define IXGBE_EEC_FWE_EN    0x00000020 /* Enable FLASH writes */
#define IXGBE_EEC_FWE_SHIFT 4
#define IXGBE_EEC_REQ       0x00000040 /* EEPROM Access Request */
#define IXGBE_EEC_GNT       0x00000080 /* EEPROM Access Grant */
#define IXGBE_EEC_PRES      0x00000100 /* EEPROM Present */
#define IXGBE_EEC_ARD       0x00000200 /* EEPROM Auto Read Done */
/* EEPROM Addressing bits based on type (0-small, 1-large) */
#define IXGBE_EEC_ADDR_SIZE 0x00000400
#define IXGBE_EEC_SIZE      0x00007800 /* EEPROM Size */

#define IXGBE_EEC_SIZE_SHIFT          11
#define IXGBE_EEPROM_WORD_SIZE_SHIFT  6
#define IXGBE_EEPROM_OPCODE_BITS      8

/* Checksum and EEPROM pointers */
#define IXGBE_EEPROM_CHECKSUM   0x3F
#define IXGBE_EEPROM_SUM        0xBABA
#define IXGBE_PCIE_ANALOG_PTR   0x03
#define IXGBE_ATLAS0_CONFIG_PTR 0x04
#define IXGBE_ATLAS1_CONFIG_PTR 0x05
#define IXGBE_PCIE_GENERAL_PTR  0x06
#define IXGBE_PCIE_CONFIG0_PTR  0x07
#define IXGBE_PCIE_CONFIG1_PTR  0x08
#define IXGBE_CORE0_PTR         0x09
#define IXGBE_CORE1_PTR         0x0A
#define IXGBE_MAC0_PTR          0x0B
#define IXGBE_MAC1_PTR          0x0C
#define IXGBE_CSR0_CONFIG_PTR   0x0D
#define IXGBE_CSR1_CONFIG_PTR   0x0E
#define IXGBE_FW_PTR            0x0F
#define IXGBE_PBANUM0_PTR       0x15
#define IXGBE_PBANUM1_PTR       0x16

/* Legacy EEPROM word offsets */
#define IXGBE_ISCSI_BOOT_CAPS           0x0033
#define IXGBE_ISCSI_SETUP_PORT_0        0x0030
#define IXGBE_ISCSI_SETUP_PORT_1        0x0034

/* EEPROM Commands - SPI */
#define IXGBE_EEPROM_MAX_RETRY_SPI      5000 /* Max wait 5ms for RDY signal */
#define IXGBE_EEPROM_STATUS_RDY_SPI     0x01
#define IXGBE_EEPROM_READ_OPCODE_SPI    0x03  /* EEPROM read opcode */
#define IXGBE_EEPROM_WRITE_OPCODE_SPI   0x02  /* EEPROM write opcode */
#define IXGBE_EEPROM_A8_OPCODE_SPI      0x08  /* opcode bit-3 = addr bit-8 */
#define IXGBE_EEPROM_WREN_OPCODE_SPI    0x06  /* EEPROM set Write Ena latch */
/* EEPROM reset Write Enable latch */
#define IXGBE_EEPROM_WRDI_OPCODE_SPI    0x04
#define IXGBE_EEPROM_RDSR_OPCODE_SPI    0x05  /* EEPROM read Status reg */
#define IXGBE_EEPROM_WRSR_OPCODE_SPI    0x01  /* EEPROM write Status reg */
#define IXGBE_EEPROM_ERASE4K_OPCODE_SPI 0x20  /* EEPROM ERASE 4KB */
#define IXGBE_EEPROM_ERASE64K_OPCODE_SPI  0xD8  /* EEPROM ERASE 64KB */
#define IXGBE_EEPROM_ERASE256_OPCODE_SPI  0xDB  /* EEPROM ERASE 256B */

/* EEPROM Read Register */
#define IXGBE_EEPROM_READ_REG_DATA   16   /* data offset in EEPROM read reg */
#define IXGBE_EEPROM_READ_REG_DONE   2    /* Offset to READ done bit */
#define IXGBE_EEPROM_READ_REG_START  1    /* First bit to start operation */
#define IXGBE_EEPROM_READ_ADDR_SHIFT 2    /* Shift to the address bits */

#define IXGBE_ETH_LENGTH_OF_ADDRESS   6

#ifndef IXGBE_EEPROM_GRANT_ATTEMPTS
#define IXGBE_EEPROM_GRANT_ATTEMPTS 1000 /* EEPROM # attempts to gain grant */
#endif

#ifndef IXGBE_EERD_ATTEMPTS
/* Number of 5 microseconds we wait for EERD read to complete */
#define IXGBE_EERD_ATTEMPTS 100000
#endif

/* PCI Bus Info */
#define IXGBE_PCI_LINK_STATUS     0xB2
#define IXGBE_PCI_LINK_WIDTH      0x3F0
#define IXGBE_PCI_LINK_WIDTH_1    0x10
#define IXGBE_PCI_LINK_WIDTH_2    0x20
#define IXGBE_PCI_LINK_WIDTH_4    0x40
#define IXGBE_PCI_LINK_WIDTH_8    0x80
#define IXGBE_PCI_LINK_SPEED      0xF
#define IXGBE_PCI_LINK_SPEED_2500 0x1
#define IXGBE_PCI_LINK_SPEED_5000 0x2

/* Number of 100 microseconds we wait for PCI Express master disable */
#define IXGBE_PCI_MASTER_DISABLE_TIMEOUT 800

/* Check whether address is multicast.  This is little-endian specific check.*/
#define IXGBE_IS_MULTICAST(Address) \
                (bool)(((u8 *)(Address))[0] & ((u8)0x01))

/* Check whether an address is broadcast. */
#define IXGBE_IS_BROADCAST(Address)                      \
                ((((u8 *)(Address))[0] == ((u8)0xff)) && \
                (((u8 *)(Address))[1] == ((u8)0xff)))

/* RAH */
#define IXGBE_RAH_VIND_MASK     0x003C0000
#define IXGBE_RAH_VIND_SHIFT    18
#define IXGBE_RAH_AV            0x80000000
#define IXGBE_CLEAR_VMDQ_ALL    0xFFFFFFFF

/* Header split receive */
#define IXGBE_RFCTL_ISCSI_DIS       0x00000001
#define IXGBE_RFCTL_ISCSI_DWC_MASK  0x0000003E
#define IXGBE_RFCTL_ISCSI_DWC_SHIFT 1
#define IXGBE_RFCTL_NFSW_DIS        0x00000040
#define IXGBE_RFCTL_NFSR_DIS        0x00000080
#define IXGBE_RFCTL_NFS_VER_MASK    0x00000300
#define IXGBE_RFCTL_NFS_VER_SHIFT   8
#define IXGBE_RFCTL_NFS_VER_2       0
#define IXGBE_RFCTL_NFS_VER_3       1
#define IXGBE_RFCTL_NFS_VER_4       2
#define IXGBE_RFCTL_IPV6_DIS        0x00000400
#define IXGBE_RFCTL_IPV6_XSUM_DIS   0x00000800
#define IXGBE_RFCTL_IPFRSP_DIS      0x00004000
#define IXGBE_RFCTL_IPV6_EX_DIS     0x00010000
#define IXGBE_RFCTL_NEW_IPV6_EXT_DIS 0x00020000

/* Transmit Config masks */
#define IXGBE_TXDCTL_ENABLE     0x02000000 /* Enable specific Tx Queue */
#define IXGBE_TXDCTL_SWFLSH     0x04000000 /* Tx Desc. write-back flushing */
/* Enable short packet padding to 64 bytes */
#define IXGBE_TX_PAD_ENABLE     0x00000400
#define IXGBE_JUMBO_FRAME_ENABLE 0x00000004  /* Allow jumbo frames */
/* This allows for 16K packets + 4k for vlan */
#define IXGBE_MAX_FRAME_SZ      0x40040000

#define IXGBE_TDWBAL_HEAD_WB_ENABLE   0x1      /* Tx head write-back enable */
#define IXGBE_TDWBAL_SEQNUM_WB_ENABLE 0x2      /* Tx seq# write-back enable */

/* Receive Config masks */
#define IXGBE_RXCTRL_RXEN       0x00000001  /* Enable Receiver */
#define IXGBE_RXCTRL_DMBYPS     0x00000002  /* Descriptor Monitor Bypass */
#define IXGBE_RXDCTL_ENABLE     0x02000000  /* Enable specific Rx Queue */

#define IXGBE_FCTRL_SBP 0x00000002 /* Store Bad Packet */
#define IXGBE_FCTRL_MPE 0x00000100 /* Multicast Promiscuous Ena*/
#define IXGBE_FCTRL_UPE 0x00000200 /* Unicast Promiscuous Ena */
#define IXGBE_FCTRL_BAM 0x00000400 /* Broadcast Accept Mode */
#define IXGBE_FCTRL_PMCF 0x00001000 /* Pass MAC Control Frames */
#define IXGBE_FCTRL_DPF 0x00002000 /* Discard Pause Frame */
/* Receive Priority Flow Control Enable */
#define IXGBE_FCTRL_RPFCE 0x00004000
#define IXGBE_FCTRL_RFCE 0x00008000 /* Receive Flow Control Ena */

/* Multiple Receive Queue Control */
#define IXGBE_MRQC_RSSEN                 0x00000001  /* RSS Enable */
#define IXGBE_MRQC_RSS_FIELD_MASK        0xFFFF0000
#define IXGBE_MRQC_RSS_FIELD_IPV4_TCP    0x00010000
#define IXGBE_MRQC_RSS_FIELD_IPV4        0x00020000
#define IXGBE_MRQC_RSS_FIELD_IPV6_EX_TCP 0x00040000
#define IXGBE_MRQC_RSS_FIELD_IPV6_EX     0x00080000
#define IXGBE_MRQC_RSS_FIELD_IPV6        0x00100000
#define IXGBE_MRQC_RSS_FIELD_IPV6_TCP    0x00200000
#define IXGBE_MRQC_RSS_FIELD_IPV4_UDP    0x00400000
#define IXGBE_MRQC_RSS_FIELD_IPV6_UDP    0x00800000
#define IXGBE_MRQC_RSS_FIELD_IPV6_EX_UDP 0x01000000

#define IXGBE_TXD_POPTS_IXSM 0x01       /* Insert IP checksum */
#define IXGBE_TXD_POPTS_TXSM 0x02       /* Insert TCP/UDP checksum */
#define IXGBE_TXD_CMD_EOP    0x01000000 /* End of Packet */
#define IXGBE_TXD_CMD_IFCS   0x02000000 /* Insert FCS (Ethernet CRC) */
#define IXGBE_TXD_CMD_IC     0x04000000 /* Insert Checksum */
#define IXGBE_TXD_CMD_RS     0x08000000 /* Report Status */
#define IXGBE_TXD_CMD_DEXT   0x20000000 /* Descriptor extension (0 = legacy) */
#define IXGBE_TXD_CMD_VLE    0x40000000 /* Add VLAN tag */
#define IXGBE_TXD_STAT_DD    0x00000001 /* Descriptor Done */

/* Receive Descriptor bit definitions */
#define IXGBE_RXD_STAT_DD       0x01    /* Descriptor Done */
#define IXGBE_RXD_STAT_EOP      0x02    /* End of Packet */
#define IXGBE_RXD_STAT_VP       0x08    /* IEEE VLAN Packet */
#define IXGBE_RXD_STAT_UDPCS    0x10    /* UDP xsum calculated */
#define IXGBE_RXD_STAT_L4CS     0x20    /* L4 xsum calculated */
#define IXGBE_RXD_STAT_IPCS     0x40    /* IP xsum calculated */
#define IXGBE_RXD_STAT_PIF      0x80    /* passed in-exact filter */
#define IXGBE_RXD_STAT_CRCV     0x100   /* Speculative CRC Valid */
#define IXGBE_RXD_STAT_VEXT     0x200   /* 1st VLAN found */
#define IXGBE_RXD_STAT_UDPV     0x400   /* Valid UDP checksum */
#define IXGBE_RXD_STAT_DYNINT   0x800   /* Pkt caused INT via DYNINT */
#define IXGBE_RXD_STAT_ACK      0x8000  /* ACK Packet indication */
#define IXGBE_RXD_ERR_CE        0x01    /* CRC Error */
#define IXGBE_RXD_ERR_LE        0x02    /* Length Error */
#define IXGBE_RXD_ERR_PE        0x08    /* Packet Error */
#define IXGBE_RXD_ERR_OSE       0x10    /* Oversize Error */
#define IXGBE_RXD_ERR_USE       0x20    /* Undersize Error */
#define IXGBE_RXD_ERR_TCPE      0x40    /* TCP/UDP Checksum Error */
#define IXGBE_RXD_ERR_IPE       0x80    /* IP Checksum Error */
#define IXGBE_RXDADV_ERR_HBO    0x00800000 /*Header Buffer Overflow */
#define IXGBE_RXDADV_ERR_CE     0x01000000 /* CRC Error */
#define IXGBE_RXDADV_ERR_LE     0x02000000 /* Length Error */
#define IXGBE_RXDADV_ERR_PE     0x08000000 /* Packet Error */
#define IXGBE_RXDADV_ERR_OSE    0x10000000 /* Oversize Error */
#define IXGBE_RXDADV_ERR_USE    0x20000000 /* Undersize Error */
#define IXGBE_RXDADV_ERR_TCPE   0x40000000 /* TCP/UDP Checksum Error */
#define IXGBE_RXDADV_ERR_IPE    0x80000000 /* IP Checksum Error */
#define IXGBE_RXD_VLAN_ID_MASK  0x0FFF  /* VLAN ID is in lower 12 bits */
#define IXGBE_RXD_PRI_MASK      0xE000  /* Priority is in upper 3 bits */
#define IXGBE_RXD_PRI_SHIFT     13
#define IXGBE_RXD_CFI_MASK      0x1000  /* CFI is bit 12 */
#define IXGBE_RXD_CFI_SHIFT     12


/* SRRCTL bit definitions */
#define IXGBE_SRRCTL_BSIZEPKT_SHIFT     10     /* so many KBs */
#define IXGBE_SRRCTL_BSIZEPKT_MASK      0x0000007F
#define IXGBE_SRRCTL_BSIZEHDR_MASK      0x00003F00
#define IXGBE_SRRCTL_DESCTYPE_LEGACY    0x00000000
#define IXGBE_SRRCTL_DESCTYPE_ADV_ONEBUF 0x02000000
#define IXGBE_SRRCTL_DESCTYPE_HDR_SPLIT  0x04000000
#define IXGBE_SRRCTL_DESCTYPE_HDR_REPLICATION_LARGE_PKT 0x08000000
#define IXGBE_SRRCTL_DESCTYPE_HDR_SPLIT_ALWAYS 0x0A000000
#define IXGBE_SRRCTL_DESCTYPE_MASK      0x0E000000

#define IXGBE_RXDPS_HDRSTAT_HDRSP       0x00008000
#define IXGBE_RXDPS_HDRSTAT_HDRLEN_MASK 0x000003FF

#define IXGBE_RXDADV_RSSTYPE_MASK       0x0000000F
#define IXGBE_RXDADV_PKTTYPE_MASK       0x0000FFF0
#define IXGBE_RXDADV_HDRBUFLEN_MASK     0x00007FE0
#define IXGBE_RXDADV_HDRBUFLEN_SHIFT    5
#define IXGBE_RXDADV_SPLITHEADER_EN     0x00001000
#define IXGBE_RXDADV_SPH                0x8000

/* RSS Hash results */
#define IXGBE_RXDADV_RSSTYPE_NONE       0x00000000
#define IXGBE_RXDADV_RSSTYPE_IPV4_TCP   0x00000001
#define IXGBE_RXDADV_RSSTYPE_IPV4       0x00000002
#define IXGBE_RXDADV_RSSTYPE_IPV6_TCP   0x00000003
#define IXGBE_RXDADV_RSSTYPE_IPV6_EX    0x00000004
#define IXGBE_RXDADV_RSSTYPE_IPV6       0x00000005
#define IXGBE_RXDADV_RSSTYPE_IPV6_TCP_EX 0x00000006
#define IXGBE_RXDADV_RSSTYPE_IPV4_UDP   0x00000007
#define IXGBE_RXDADV_RSSTYPE_IPV6_UDP   0x00000008
#define IXGBE_RXDADV_RSSTYPE_IPV6_UDP_EX 0x00000009

/* RSS Packet Types as indicated in the receive descriptor. */
#define IXGBE_RXDADV_PKTTYPE_NONE       0x00000000
#define IXGBE_RXDADV_PKTTYPE_IPV4       0x00000010 /* IPv4 hdr present */
#define IXGBE_RXDADV_PKTTYPE_IPV4_EX    0x00000020 /* IPv4 hdr + extensions */
#define IXGBE_RXDADV_PKTTYPE_IPV6       0x00000040 /* IPv6 hdr present */
#define IXGBE_RXDADV_PKTTYPE_IPV6_EX    0x00000080 /* IPv6 hdr + extensions */
#define IXGBE_RXDADV_PKTTYPE_TCP        0x00000100 /* TCP hdr present */
#define IXGBE_RXDADV_PKTTYPE_UDP        0x00000200 /* UDP hdr present */
#define IXGBE_RXDADV_PKTTYPE_SCTP       0x00000400 /* SCTP hdr present */
#define IXGBE_RXDADV_PKTTYPE_NFS        0x00000800 /* NFS hdr present */
/* Masks to determine if packets should be dropped due to frame errors */
#define IXGBE_RXD_ERR_FRAME_ERR_MASK ( \
                                      IXGBE_RXD_ERR_CE | \
                                      IXGBE_RXD_ERR_LE | \
                                      IXGBE_RXD_ERR_PE | \
                                      IXGBE_RXD_ERR_OSE | \
                                      IXGBE_RXD_ERR_USE)

#define IXGBE_RXDADV_ERR_FRAME_ERR_MASK ( \
                                      IXGBE_RXDADV_ERR_CE | \
                                      IXGBE_RXDADV_ERR_LE | \
                                      IXGBE_RXDADV_ERR_PE | \
                                      IXGBE_RXDADV_ERR_OSE | \
                                      IXGBE_RXDADV_ERR_USE)

/* Multicast bit mask */
#define IXGBE_MCSTCTRL_MFE      0x4

/* Number of Transmit and Receive Descriptors must be a multiple of 8 */
#define IXGBE_REQ_TX_DESCRIPTOR_MULTIPLE  8
#define IXGBE_REQ_RX_DESCRIPTOR_MULTIPLE  8
#define IXGBE_REQ_TX_BUFFER_GRANULARITY   1024

/* Vlan-specific macros */
#define IXGBE_RX_DESC_SPECIAL_VLAN_MASK  0x0FFF /* VLAN ID in lower 12 bits */
#define IXGBE_RX_DESC_SPECIAL_PRI_MASK   0xE000 /* Priority in upper 3 bits */
#define IXGBE_RX_DESC_SPECIAL_PRI_SHIFT  0x000D /* Priority in upper 3 of 16 */
#define IXGBE_TX_DESC_SPECIAL_PRI_SHIFT  IXGBE_RX_DESC_SPECIAL_PRI_SHIFT


/* Transmit Descriptor - Legacy */
struct ixgbe_legacy_tx_desc {
	u64 buffer_addr;       /* Address of the descriptor's data buffer */
	union {
		__le32 data;
		struct {
			__le16 length;    /* Data buffer length */
			u8 cso;           /* Checksum offset */
			u8 cmd;           /* Descriptor control */
		} flags;
	} lower;
	union {
		__le32 data;
		struct {
			u8 status;        /* Descriptor status */
			u8 css;           /* Checksum start */
			__le16 vlan;
		} fields;
	} upper;
};

/* Transmit Descriptor - Advanced */
union ixgbe_adv_tx_desc {
	struct {
		__le64 buffer_addr;      /* Address of descriptor's data buf */
		__le32 cmd_type_len;
		__le32 olinfo_status;
	} read;
	struct {
		__le64 rsvd;       /* Reserved */
		__le32 nxtseq_seed;
		__le32 status;
	} wb;
};

/* Receive Descriptor - Legacy */
struct ixgbe_legacy_rx_desc {
	__le64 buffer_addr; /* Address of the descriptor's data buffer */
	__le16 length;      /* Length of data DMAed into data buffer */
	__le16 csum;        /* Packet checksum */
	u8 status;          /* Descriptor status */
	u8 errors;          /* Descriptor Errors */
	__le16 vlan;
};

/* Receive Descriptor - Advanced */
union ixgbe_adv_rx_desc {
	struct {
		__le64 pkt_addr; /* Packet buffer address */
		__le64 hdr_addr; /* Header buffer address */
	} read;
	struct {
		struct {
			union {
				__le32 data;
				struct {
					__le16 pkt_info; /* RSS, Pkt type */
					__le16 hdr_info; /* Splithdr, hdrlen */
				} hs_rss;
			} lo_dword;
			union {
				__le32 rss; /* RSS Hash */
				struct {
					__le16 ip_id; /* IP id */
					__le16 csum; /* Packet Checksum */
				} csum_ip;
			} hi_dword;
		} lower;
		struct {
			__le32 status_error; /* ext status/error */
			__le16 length; /* Packet length */
			__le16 vlan; /* VLAN tag */
		} upper;
	} wb;  /* writeback */
};

/* Context descriptors */
struct ixgbe_adv_tx_context_desc {
	__le32 vlan_macip_lens;
	__le32 seqnum_seed;
	__le32 type_tucmd_mlhl;
	__le32 mss_l4len_idx;
};

/* Adv Transmit Descriptor Config Masks */
#define IXGBE_ADVTXD_DTALEN_MASK      0x0000FFFF /* Data buf length(bytes) */
#define IXGBE_ADVTXD_DTYP_MASK  0x00F00000 /* DTYP mask */
#define IXGBE_ADVTXD_DTYP_CTXT  0x00200000 /* Advanced Context Desc */
#define IXGBE_ADVTXD_DTYP_DATA  0x00300000 /* Advanced Data Descriptor */
#define IXGBE_ADVTXD_DCMD_EOP   IXGBE_TXD_CMD_EOP  /* End of Packet */
#define IXGBE_ADVTXD_DCMD_IFCS  IXGBE_TXD_CMD_IFCS /* Insert FCS */
#define IXGBE_ADVTXD_DCMD_RS    IXGBE_TXD_CMD_RS   /* Report Status */
#define IXGBE_ADVTXD_DCMD_DDTYP_ISCSI 0x10000000    /* DDP hdr type or iSCSI */
#define IXGBE_ADVTXD_DCMD_DEXT  IXGBE_TXD_CMD_DEXT /* Desc ext (1=Adv) */
#define IXGBE_ADVTXD_DCMD_VLE   IXGBE_TXD_CMD_VLE  /* VLAN pkt enable */
#define IXGBE_ADVTXD_DCMD_TSE   0x80000000 /* TCP Seg enable */
#define IXGBE_ADVTXD_STAT_DD    IXGBE_TXD_STAT_DD  /* Descriptor Done */
#define IXGBE_ADVTXD_STAT_SN_CRC      0x00000002 /* NXTSEQ/SEED pres in WB */
#define IXGBE_ADVTXD_STAT_RSV   0x0000000C /* STA Reserved */
#define IXGBE_ADVTXD_IDX_SHIFT  4 /* Adv desc Index shift */
#define IXGBE_ADVTXD_CC         0x00000080 /* Check Context */
#define IXGBE_ADVTXD_POPTS_SHIFT      8  /* Adv desc POPTS shift */
#define IXGBE_ADVTXD_POPTS_IXSM (IXGBE_TXD_POPTS_IXSM << \
                                 IXGBE_ADVTXD_POPTS_SHIFT)
#define IXGBE_ADVTXD_POPTS_TXSM (IXGBE_TXD_POPTS_TXSM << \
                                 IXGBE_ADVTXD_POPTS_SHIFT)
#define IXGBE_ADVTXD_POPTS_ISCO_1ST  0x00000000 /* 1st TSO of iSCSI PDU */
#define IXGBE_ADVTXD_POPTS_ISCO_MDL  0x00000800 /* Middle TSO of iSCSI PDU */
#define IXGBE_ADVTXD_POPTS_ISCO_LAST 0x00001000 /* Last TSO of iSCSI PDU */
#define IXGBE_ADVTXD_POPTS_ISCO_FULL 0x00001800 /* 1st&Last TSO-full iSCSI PDU */
#define IXGBE_ADVTXD_POPTS_RSV       0x00002000 /* POPTS Reserved */
#define IXGBE_ADVTXD_PAYLEN_SHIFT    14 /* Adv desc PAYLEN shift */
#define IXGBE_ADVTXD_MACLEN_SHIFT    9  /* Adv ctxt desc mac len shift */
#define IXGBE_ADVTXD_VLAN_SHIFT      16  /* Adv ctxt vlan tag shift */
#define IXGBE_ADVTXD_TUCMD_IPV4      0x00000400  /* IP Packet Type: 1=IPv4 */
#define IXGBE_ADVTXD_TUCMD_IPV6      0x00000000  /* IP Packet Type: 0=IPv6 */
#define IXGBE_ADVTXD_TUCMD_L4T_UDP   0x00000000  /* L4 Packet TYPE of UDP */
#define IXGBE_ADVTXD_TUCMD_L4T_TCP   0x00000800  /* L4 Packet TYPE of TCP */
#define IXGBE_ADVTXD_TUCMD_L4T_SCTP  0x00001000  /* L4 Packet TYPE of SCTP */
#define IXGBE_ADVTXD_TUCMD_MKRREQ    0x00002000 /*Req requires Markers and CRC*/
#define IXGBE_ADVTXD_L4LEN_SHIFT     8  /* Adv ctxt L4LEN shift */
#define IXGBE_ADVTXD_MSS_SHIFT       16  /* Adv ctxt MSS shift */

/* Autonegotiation advertised speeds */
typedef u32 ixgbe_autoneg_advertised;
/* Link speed */
typedef u32 ixgbe_link_speed;
#define IXGBE_LINK_SPEED_UNKNOWN   0
#define IXGBE_LINK_SPEED_100_FULL  0x0008
#define IXGBE_LINK_SPEED_1GB_FULL  0x0020
#define IXGBE_LINK_SPEED_10GB_FULL 0x0080
#define IXGBE_LINK_SPEED_82598_AUTONEG (IXGBE_LINK_SPEED_1GB_FULL | \
                                        IXGBE_LINK_SPEED_10GB_FULL)

/* Physical layer type */
typedef u32 ixgbe_physical_layer;
#define IXGBE_PHYSICAL_LAYER_UNKNOWN      0
#define IXGBE_PHYSICAL_LAYER_10GBASE_T    0x0001
#define IXGBE_PHYSICAL_LAYER_1000BASE_T   0x0002
#define IXGBE_PHYSICAL_LAYER_100BASE_T    0x0004
#define IXGBE_PHYSICAL_LAYER_SFP_PLUS_CU  0x0008
#define IXGBE_PHYSICAL_LAYER_10GBASE_LR   0x0010
#define IXGBE_PHYSICAL_LAYER_10GBASE_LRM  0x0020
#define IXGBE_PHYSICAL_LAYER_10GBASE_SR   0x0040
#define IXGBE_PHYSICAL_LAYER_10GBASE_KX4  0x0080
#define IXGBE_PHYSICAL_LAYER_10GBASE_CX4  0x0100
#define IXGBE_PHYSICAL_LAYER_1000BASE_KX  0x0200
#define IXGBE_PHYSICAL_LAYER_1000BASE_BX  0x0400


enum ixgbe_eeprom_type {
	ixgbe_eeprom_uninitialized = 0,
	ixgbe_eeprom_spi,
	ixgbe_eeprom_none /* No NVM support */
};

enum ixgbe_mac_type {
	ixgbe_mac_unknown = 0,
	ixgbe_mac_82598EB,
	ixgbe_num_macs
};

enum ixgbe_phy_type {
	ixgbe_phy_unknown = 0,
	ixgbe_phy_tn,
	ixgbe_phy_qt,
	ixgbe_phy_xaui,
	ixgbe_phy_nl,
	ixgbe_phy_tw_tyco,
	ixgbe_phy_tw_unknown,
	ixgbe_phy_sfp_avago,
	ixgbe_phy_sfp_ftl,
	ixgbe_phy_sfp_unknown,
	ixgbe_phy_generic
};

/*
 * SFP+ module type IDs:
 *
 * ID	Module Type
 * =============
 * 0	SFP_DA_CU
 * 1	SFP_SR
 * 2	SFP_LR
 */
enum ixgbe_sfp_type {
	ixgbe_sfp_type_da_cu = 0,
	ixgbe_sfp_type_sr = 1,
	ixgbe_sfp_type_lr = 2,
	ixgbe_sfp_type_not_present = 0xFFFE,
	ixgbe_sfp_type_unknown = 0xFFFF
};

enum ixgbe_media_type {
	ixgbe_media_type_unknown = 0,
	ixgbe_media_type_fiber,
	ixgbe_media_type_copper,
	ixgbe_media_type_backplane,
	ixgbe_media_type_virtual
};

/* Flow Control Settings */
enum ixgbe_fc_type {
	ixgbe_fc_none = 0,
	ixgbe_fc_rx_pause,
	ixgbe_fc_tx_pause,
	ixgbe_fc_full,
	ixgbe_fc_default
};

struct ixgbe_addr_filter_info {
	u32 num_mc_addrs;
	u32 rar_used_count;
	u32 mc_addr_in_rar_count;
	u32 mta_in_use;
	u32 overflow_promisc;
	bool user_set_promisc;
};

/* Flow control parameters */
struct ixgbe_fc_info {
	u32 high_water; /* Flow Control High-water */
	u32 low_water; /* Flow Control Low-water */
	u16 pause_time; /* Flow Control Pause timer */
	bool send_xon; /* Flow control send XON */
	bool strict_ieee; /* Strict IEEE mode */
	enum ixgbe_fc_type type; /* Type of flow control */
	enum ixgbe_fc_type original_type;
};

/* Statistics counters collected by the MAC */
struct ixgbe_hw_stats {
	u64 crcerrs;
	u64 illerrc;
	u64 errbc;
	u64 mspdc;
	u64 mpctotal;
	u64 mpc[8];
	u64 mlfc;
	u64 mrfc;
	u64 rlec;
	u64 lxontxc;
	u64 lxonrxc;
	u64 lxofftxc;
	u64 lxoffrxc;
	u64 pxontxc[8];
	u64 pxonrxc[8];
	u64 pxofftxc[8];
	u64 pxoffrxc[8];
	u64 prc64;
	u64 prc127;
	u64 prc255;
	u64 prc511;
	u64 prc1023;
	u64 prc1522;
	u64 gprc;
	u64 bprc;
	u64 mprc;
	u64 gptc;
	u64 gorc;
	u64 gotc;
	u64 rnbc[8];
	u64 ruc;
	u64 rfc;
	u64 roc;
	u64 rjc;
	u64 mngprc;
	u64 mngpdc;
	u64 mngptc;
	u64 tor;
	u64 tpr;
	u64 tpt;
	u64 ptc64;
	u64 ptc127;
	u64 ptc255;
	u64 ptc511;
	u64 ptc1023;
	u64 ptc1522;
	u64 mptc;
	u64 bptc;
	u64 xec;
	u64 rqsmr[16];
	u64 tqsmr[8];
	u64 qprc[16];
	u64 qptc[16];
	u64 qbrc[16];
	u64 qbtc[16];
};

/* forward declaration */
struct ixgbe_hw;

/* iterator type for walking multicast address lists */
typedef u8* (*ixgbe_mc_addr_itr) (struct ixgbe_hw *hw, u8 **mc_addr_ptr,
                                  u32 *vmdq);

/* Function pointer table */
struct ixgbe_eeprom_operations {
	s32 (*init_params)(struct ixgbe_hw *);
	s32 (*read)(struct ixgbe_hw *, u16, u16 *);
	s32 (*write)(struct ixgbe_hw *, u16, u16);
	s32 (*validate_checksum)(struct ixgbe_hw *, u16 *);
	s32 (*update_checksum)(struct ixgbe_hw *);
};

struct ixgbe_mac_operations {
	s32 (*init_hw)(struct ixgbe_hw *);
	s32 (*reset_hw)(struct ixgbe_hw *);
	s32 (*start_hw)(struct ixgbe_hw *);
	s32 (*clear_hw_cntrs)(struct ixgbe_hw *);
	enum ixgbe_media_type (*get_media_type)(struct ixgbe_hw *);
	s32 (*get_supported_physical_layer)(struct ixgbe_hw *);
	s32 (*get_mac_addr)(struct ixgbe_hw *, u8 *);
	s32 (*stop_adapter)(struct ixgbe_hw *);
	s32 (*get_bus_info)(struct ixgbe_hw *);
	s32 (*read_analog_reg8)(struct ixgbe_hw*, u32, u8*);
	s32 (*write_analog_reg8)(struct ixgbe_hw*, u32, u8);

	/* Link */
	s32 (*setup_link)(struct ixgbe_hw *);
	s32 (*setup_link_speed)(struct ixgbe_hw *, ixgbe_link_speed, bool,
	                        bool);
	s32 (*check_link)(struct ixgbe_hw *, ixgbe_link_speed *, bool *, bool);
	s32 (*get_link_capabilities)(struct ixgbe_hw *, ixgbe_link_speed *,
	                             bool *);

	/* LED */
	s32 (*led_on)(struct ixgbe_hw *, u32);
	s32 (*led_off)(struct ixgbe_hw *, u32);
	s32 (*blink_led_start)(struct ixgbe_hw *, u32);
	s32 (*blink_led_stop)(struct ixgbe_hw *, u32);

	/* RAR, Multicast, VLAN */
	s32 (*set_rar)(struct ixgbe_hw *, u32, u8 *, u32, u32);
	s32 (*clear_rar)(struct ixgbe_hw *, u32);
	s32 (*set_vmdq)(struct ixgbe_hw *, u32, u32);
	s32 (*clear_vmdq)(struct ixgbe_hw *, u32, u32);
	s32 (*init_rx_addrs)(struct ixgbe_hw *);
	s32 (*update_uc_addr_list)(struct ixgbe_hw *, u8 *, u32,
	                           ixgbe_mc_addr_itr);
	s32 (*update_mc_addr_list)(struct ixgbe_hw *, u8 *, u32,
	                           ixgbe_mc_addr_itr);
	s32 (*enable_mc)(struct ixgbe_hw *);
	s32 (*disable_mc)(struct ixgbe_hw *);
	s32 (*clear_vfta)(struct ixgbe_hw *);
	s32 (*set_vfta)(struct ixgbe_hw *, u32, u32, bool);
	s32 (*init_uta_tables)(struct ixgbe_hw *);

	/* Flow Control */
	s32 (*setup_fc)(struct ixgbe_hw *, s32);
};

struct ixgbe_phy_operations {
	s32 (*identify)(struct ixgbe_hw *);
	s32 (*identify_sfp)(struct ixgbe_hw *);
	s32 (*reset)(struct ixgbe_hw *);
	s32 (*read_reg)(struct ixgbe_hw *, u32, u32, u16 *);
	s32 (*write_reg)(struct ixgbe_hw *, u32, u32, u16);
	s32 (*setup_link)(struct ixgbe_hw *);
	s32 (*setup_link_speed)(struct ixgbe_hw *, ixgbe_link_speed, bool,
	                        bool);
	s32 (*check_link)(struct ixgbe_hw *, ixgbe_link_speed *, bool *);
	s32 (*get_firmware_version)(struct ixgbe_hw *, u16 *);
	s32 (*read_i2c_byte)(struct ixgbe_hw *, u8, u8, u8 *);
	s32 (*write_i2c_byte)(struct ixgbe_hw *, u8, u8, u8);
	s32 (*read_i2c_eeprom)(struct ixgbe_hw *, u8 , u8 *);
	s32 (*write_i2c_eeprom)(struct ixgbe_hw *, u8, u8);
};

struct ixgbe_eeprom_info {
	struct ixgbe_eeprom_operations  ops;
	enum ixgbe_eeprom_type          type;
	u32				semaphore_delay;
	u16                             word_size;
	u16                             address_bits;
};

struct ixgbe_mac_info {
	struct ixgbe_mac_operations     ops;
	enum ixgbe_mac_type             type;
	u8                              addr[IXGBE_ETH_LENGTH_OF_ADDRESS];
	u8                              perm_addr[IXGBE_ETH_LENGTH_OF_ADDRESS];
	s32                             mc_filter_type;
	u32                             mcft_size;
	u32                             vft_size;
	u32                             num_rar_entries;
	u32                             max_tx_queues;
	u32                             max_rx_queues;
	u32                             link_attach_type;
	u32                             link_mode_select;
	bool                            link_settings_loaded;
	bool                            autoneg;
	bool                            autoneg_failed;
};

struct ixgbe_phy_info {
	struct ixgbe_phy_operations     ops;
	enum ixgbe_phy_type             type;
	u32                             addr;
	u32                             id;
	enum ixgbe_sfp_type             sfp_type;
	u32                             revision;
	enum ixgbe_media_type           media_type;
	bool                            reset_disable;
	ixgbe_autoneg_advertised        autoneg_advertised;
	bool                            autoneg_wait_to_complete;
};

struct ixgbe_hw {
	u8 __iomem			*hw_addr;
	void				*back;
	struct ixgbe_mac_info		mac;
	struct ixgbe_addr_filter_info	addr_ctrl;
	struct ixgbe_fc_info		fc;
	struct ixgbe_phy_info		phy;
	struct ixgbe_eeprom_info	eeprom;
	u16				device_id;
	u16				vendor_id;
	u16				subsystem_device_id;
	u16				subsystem_vendor_id;
	u8				revision_id;
	bool				adapter_stopped;
};

struct ixgbe_info {
	enum ixgbe_mac_type		mac;
	s32 				(*get_invariants)(struct ixgbe_hw *);
	struct ixgbe_mac_operations	*mac_ops;
	struct ixgbe_eeprom_operations	*eeprom_ops;
	struct ixgbe_phy_operations	*phy_ops;
};


/* Error Codes */
#define IXGBE_ERR_EEPROM                        -1
#define IXGBE_ERR_EEPROM_CHECKSUM               -2
#define IXGBE_ERR_PHY                           -3
#define IXGBE_ERR_CONFIG                        -4
#define IXGBE_ERR_PARAM                         -5
#define IXGBE_ERR_MAC_TYPE                      -6
#define IXGBE_ERR_UNKNOWN_PHY                   -7
#define IXGBE_ERR_LINK_SETUP                    -8
#define IXGBE_ERR_ADAPTER_STOPPED               -9
#define IXGBE_ERR_INVALID_MAC_ADDR              -10
#define IXGBE_ERR_DEVICE_NOT_SUPPORTED          -11
#define IXGBE_ERR_MASTER_REQUESTS_PENDING       -12
#define IXGBE_ERR_INVALID_LINK_SETTINGS         -13
#define IXGBE_ERR_AUTONEG_NOT_COMPLETE          -14
#define IXGBE_ERR_RESET_FAILED                  -15
#define IXGBE_ERR_SWFW_SYNC                     -16
#define IXGBE_ERR_PHY_ADDR_INVALID              -17
#define IXGBE_ERR_I2C                           -18
#define IXGBE_ERR_SFP_NOT_SUPPORTED             -19
#define IXGBE_ERR_SFP_NOT_PRESENT               -20
#define IXGBE_NOT_IMPLEMENTED                   0x7FFFFFFF

#endif /* _IXGBE_TYPE_H_ */
