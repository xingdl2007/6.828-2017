#include "inc/string.h"
#include "kern/pmap.h"
#include "kern/e1000.h"

// LAB 6: Your driver code here
struct eth_frame tx_pkt_buffer[TXDESC_SIZE];

__attribute__((__aligned__(PGSIZE)))
struct e1000_tx_desc tx_desc_ring[TXDESC_SIZE];

void
e1000_tx_init()
{
	// Buffer addr initialization, set DD bit
	for(int i = 0; i < TXDESC_SIZE; i++) {
		tx_desc_ring[i].buffer_addr = PADDR(&tx_pkt_buffer[i]);
		tx_desc_ring[i].upper.fields.status |= E1000_TD_STA_DD;
	}

	// Ring buffer, must be physical address
	e1000[E1000_TDBAL/4] = PADDR(tx_desc_ring);
	e1000[E1000_TDBAH/4] = 0;                     // 32bit addr
	e1000[E1000_TDLEN/4] = TXDESC_SIZE * sizeof(struct e1000_tx_desc);

	// Make sure head/tail is 0
	e1000[E1000_TDH/4] = 0;
	e1000[E1000_TDT/4] = 0;

	// EN: 1; PSP: 1; CT: 0x10; COLD: 0x40;
	e1000[E1000_TCTL/4] = E1000_TCTL_EN | E1000_TCTL_PSP |
		(0x10 << 4) | (0x40 << 12);

	// IPG: 10; IPG1: 4; IPG2: 6; according to IEEE 802.3 standard
	e1000[E1000_TIPG] = 10 | (4 << 10) | (6 << 20);
}

/*
  return true when transmit queue is not full, false otherwise.
  Caller should check the return value.
*/
bool
e1000_snd_pkt(void *pkt, uint32_t len)
{
	if(len > ETH_MAX_SIZE) {
		panic("e1000_snd_pkt(): pkt too long %d\n", len);
	}
	uint32_t tail = e1000[E1000_TDT/4];
	assert(tail < TXDESC_SIZE);

	// free transimit descriptor slot
	if(!tx_desc_ring[tail].upper.fields.status & E1000_TD_STA_DD) {
		return false;
	}
	memcpy(KADDR(tx_desc_ring[tail].buffer_addr), pkt, len);
	tx_desc_ring[tail].lower.flags.length = len;

	// enable RS(report status) and set EOP (end of packet)
	tx_desc_ring[tail].lower.flags.cmd |=
		E1000_TD_CMD_RS | E1000_TD_CMD_EOP;
	// clear DD
	tx_desc_ring[tail].upper.fields.status = 0;

	// update tail
	tail = (tail + 1) % TXDESC_SIZE;
	e1000[E1000_TDT/4] = tail;
	return true;
}

void
e1000_rx_init()
{

}
