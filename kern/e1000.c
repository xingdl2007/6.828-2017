#include "kern/pmap.h"
#include "kern/e1000.h"

// LAB 6: Your driver code here
struct eth_frame tx_pkt_buffer[TXDESC_SIZE];

__attribute__((__aligned__(PGSIZE)))
struct e1000_tx_desc tx_desc_ring[TXDESC_SIZE] = {
	[0] = {0, {0}, {0}},
};

void e1000_tx_init() {
	// Buffer addr initialization
	for(int i = 0; i < TXDESC_SIZE; i++) {
		tx_desc_ring[i].buffer_addr = PADDR(&tx_pkt_buffer[i]);
	}

	// Ring buffer, must be physical address
	e1000[E1000_TDBAL/4] = PADDR(tx_desc_ring);
	e1000[E1000_TDBAH/4] = 0;                     // 32bit addr
	e1000[E1000_TDLEN/4] = TXDESC_SIZE * 128;

	// Make sure head/tail is 0
	e1000[E1000_TDH/4] = 0;
	e1000[E1000_TDT/4] = 0;

	// EN: 1; PSP: 1; CT: 0x10; COLD: 0x40;
	e1000[E1000_TCTL/4] = E1000_TCTL_EN | E1000_TCTL_PSP |
		(0x10 << 4) | (0x40 << 12);

	// IPG: 10; IPG1: 4; IPG2: 6; according to IEEE 802.3 standard
	e1000[E1000_TIPG] = 10 | (4 << 10) | (6 << 20);
}

void e1000_rx_init() {

}
