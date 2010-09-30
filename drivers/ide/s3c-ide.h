/*
 * drivers/ide/arm/s3c-ide.h
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

typedef enum {
	PIO0, PIO1, PIO2, PIO3, PIO4
} PIOMODE;

typedef enum {
	UDMA0, UDMA1, UDMA2, UDMA3, UDMA4
} UDMAMODE;

typedef enum {
	ATA_CMD_STOP, ATA_CMD_START, ATA_CMD_ABORT, ATA_CMD_CONTINUE
} ATA_TRANSFER_CMD;

typedef enum {
	ATA_INT_XFER_DONE, ATA_INT_UDMA_HOLD, ATA_INT_IRQ,
	ATA_INT_TBUF_FULL, ATA_INT_SBUF_EMPTY
} ATA_INT_SRC;

typedef enum {
	PIO_CPU, PIO_DMA, MULTIWORD_DMA, UDMA
} ATA_MODE;

typedef enum {
	IDLE, BUSYW, PREP, BUSYR, PAUSER, PAUSEW, PAUSER2
} BUS_STATE;

#ifdef CONFIG_BLK_DEV_IDE_S3C_UDMA
        #define DMA_WAIT_TIMEOUT        100
        #define NUM_DESCRIPTORS         PRD_ENTRIES
#else
        #define NUM_DESCRIPTORS         2
#endif

#ifdef CONFIG_PM
/*
* This will enable the device to be powered up when write() or read()
* is called. If this is not defined, the driver will return -EBUSY.
*/
#define WAKE_ON_ACCESS 1

typedef struct
{
        spinlock_t         lock;       /* Used to block on state transitions */
        unsigned	   stopped;    /* USed to signaling device is stopped */
} pm_state;
#endif

typedef struct
{
        ulong addr;       /* Used to block on state transitions */
        ulong len;        /* Power Managers device structure */
} dma_queue_t;

typedef struct
{
	u32		tx_dev_id, rx_dev_id, target_dev_id;
	ide_hwif_t	*hwif;
#ifdef CONFIG_BLK_DEV_IDE_S3C_UDMA
	ide_drive_t	*drive;

	uint		index;				/* current queue index */
	uint		queue_size;			/* total queue size requested */
	dma_queue_t	table[NUM_DESCRIPTORS];
	uint		irq_sta;
	uint		pseudo_dma;			/* in DMA session */
#endif
	struct platform_device	*dev;
	int		irq;
	ulong		piotime[5];
	ulong		udmatime[5];
	struct clk	*clk;

#ifdef CONFIG_PM
	pm_state	pm;
#endif
} s3c_ide_hwif_t;
