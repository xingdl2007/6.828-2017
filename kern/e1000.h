#ifndef JOS_KERN_E1000_H
#define JOS_KERN_E1000_H

#include "inc/types.h"
#include "inc/memlayout.h"

#define TXDESC_SIZE  32    // tx descriptor ring size
#define RXDESC_SIZE  128   // rx descriptor ring size
#define ETH_MAX_SIZE 2048  // > max ethernet frame size (1518)

volatile uint32_t *e1000;

/* Register Set. (82543, 82544)
 *
 * Registers are defined to be 32 bits and should be accessed as 32 bit values.
 * These registers are physically located on the NIC, but are mapped into the
 * host memory address space.
 *
 * RW - register is both readable and writable
 * RO - register is read only
 * WO - register is write only
 * R/clr - register is read only and is cleared when read
 * A - register array
 */

#define E1000_CTRL      0x00000  /* Device Control - RW */
#define E1000_STATUS    0x00008  /* Device Status - RO */
#define E1000_IMS       0x000D0  /* Interrupt Mask Set - RW */
#define E1000_IMC       0x000D8  /* Interrupt Mask Clear - WO */

/* Receive Control Related */
#define E1000_RCTL      0x00100  /* RX Control - RW */
#define E1000_RDBAL     0x02800  /* RX Descriptor Base Address Low - RW */
#define E1000_RDBAH     0x02804  /* RX Descriptor Base Address High - RW */
#define E1000_RDLEN     0x02808  /* RX Descriptor Length - RW */
#define E1000_RDH       0x02810  /* RX Descriptor Head - RW */
#define E1000_RDT       0x02818  /* RX Descriptor Tail - RW */

#define E1000_RAL(n)    (0x05400+8*n) /* Receive Address Low - RW Array */
#define E1000_RAH(n)    (0x05408+8*n) /* Receive Address High- RW Array */
#define E1000_MTA(n)    (0x05200+4*n) /* Multicast Table Array - RW Array */

/* Transmit Control Related*/
#define E1000_TCTL      0x00400  /* TX Control - RW */
#define E1000_TIPG      0x00410  /* TX Inter-packet gap -RW */
#define E1000_TDBAL     0x03800  /* TX Descriptor Base Address Low - RW */
#define E1000_TDBAH     0x03804  /* TX Descriptor Base Address High - RW */
#define E1000_TDLEN     0x03808  /* TX Descriptor Length - RW */
#define E1000_TDH       0x03810  /* TX Descriptor Head - RW */
#define E1000_TDT       0x03818  /* TX Descripotr Tail - RW */

/* Transmit Control Register*/
#define E1000_TCTL_EN   0x00000002    /* enable tx */
#define E1000_TCTL_PSP  0x00000008    /* pad short packets */
#define E1000_TCTL_CT   0x00000ff0    /* collision threshold */
#define E1000_TCTL_COLD 0x003ff000    /* collision distance */

/* Transmit Descriptor Status bits*/
#define E1000_TD_STA_DD  0x1      /* Bit 0 */
#define E1000_TD_CMD_EOP 0x1      /* Bit 0 */
#define E1000_TD_CMD_RS  0x8      /* Bit 3 */

/* Receive Control */
#define E1000_RCTL_RST   0x00000001    /* Software reset */
#define E1000_RCTL_EN    0x00000002    /* enable */
#define E1000_RCTL_SECRC 0x04000000    /* Strip Ethernet CRC */

/* Transmit Descriptor */
struct e1000_tx_desc {
	uint64_t buffer_addr; /* Address of the descriptor's data buffer */
	union {
		uint32_t data;
		struct {
			uint16_t length;    /* Data buffer length */
			uint8_t cso;        /* Checksum offset */
			uint8_t cmd;        /* Descriptor control */
		} flags;
	} lower;
	union {
		uint32_t data;
		struct {
			uint8_t status;     /* Descriptor status */
			uint8_t css;        /* Checksum start */
			uint16_t special;
		} fields;
	} upper;
};

/* Receive Descriptor */
struct e1000_rx_desc {
	uint64_t buffer_addr; /* Address of the descriptor's data buffer */
	uint16_t length;      /* Length of data DMAed into data buffer */
	uint16_t csum;        /* Packet checksum */
	uint8_t  status;      /* Descriptor status */
	uint8_t  errors;      /* Descriptor Errors */
	uint16_t special;
};

struct eth_frame{
	char data[ETH_MAX_SIZE];
};

void e1000_tx_init();
void e1000_rx_init();
bool e1000_snd_pkt(const char *pkt, uint32_t len);

#endif	// JOS_KERN_E1000_H
