#include "inc/ns.h"
#include "inc/string.h"
#include "kern/pmap.h"
#include "kern/env.h"
#include "kern/e1000.h"

// LAB 6: Your driver code here
struct eth_frame tx_pkt_buffer[TXDESC_SIZE];
struct eth_frame rx_pkt_buffer[RXDESC_SIZE];

__attribute__((__aligned__(PGSIZE)))
struct e1000_tx_desc tx_desc_ring[TXDESC_SIZE];

__attribute__((__aligned__(PGSIZE)))
struct e1000_rx_desc rx_desc_ring[RXDESC_SIZE];

// software head, not head used by hardware
static uint8_t head = 0;

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
	e1000[E1000_TDBAH/4] = 0;      // 32bit addr
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
e1000_snd_pkt(const char *pkt, uint32_t len)
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
	// Buffer addr initialization, set DD bit
	for(int i = 0; i < RXDESC_SIZE; i++) {
		rx_desc_ring[i].buffer_addr = PADDR(&rx_pkt_buffer[i]);
		rx_desc_ring[i].status = 0;;
	}

	// Set Receive Address
	// Set Qemu's default MAC address: 52:54:00:12:34:56
	e1000[E1000_RAL(0)/4] = (0x52) | (0x54 << 8) | (0) | (0x12 << 24);
	e1000[E1000_RAH(0)/4] = (0x34) | (0x56 << 8) | (1 << 31);

	// Initialize the MTA (Multicast Table Array) to 0b
	for(int i = 0; i < 128; i++) {
		e1000[E1000_MTA(i)/4] = 0;
	}

	// Enable receive timer interrupt
	// Setting the Packet Timer to 0b disables both the Packet Timer and
	// the Absolute Timer (described below) and causes the Receive Timer
	// Interrupt to be generated whenever a new packet has been stored
	// in memory
	e1000[E1000_RDTR/4] = 0;
	//e1000[E1000_IMS/4]  = E1000_IMS_RXT0;

	// Ring buffer, must be physical address
	e1000[E1000_RDBAL/4] = PADDR(rx_desc_ring);
	e1000[E1000_RDBAH/4] = 0;      // 32bit addr
	e1000[E1000_RDLEN/4] = RXDESC_SIZE * sizeof(struct e1000_rx_desc);

	// Make sure head/tail is 0
	e1000[E1000_RDH/4] = 0;
	e1000[E1000_RDT/4] = RXDESC_SIZE - 1;

	// Receive control register
	// 1. disable long packet
	// 2. 2048 buffer size, default
	// 3. strip CRC
	// 4. broadcast enabledPGNUM
	e1000[E1000_RCTL/4] = E1000_RCTL_EN | E1000_RCTL_SECRC
		| E1000_RCTL_BAM;
}

bool
e1000_try_recv(physaddr_t destpa)
{
	struct jif_pkt *pkt = (struct jif_pkt*)KADDR(destpa);
	uint8_t tail = e1000[E1000_RDT/4];
	assert(head < RXDESC_SIZE);
	assert(tail < RXDESC_SIZE);
	bool interrupt = false;

	// Receive descriptor consumed, one per packet
	if((rx_desc_ring[head].status & E1000_RD_STA_DD) &&
	   (rx_desc_ring[head].status & E1000_RD_STA_EOP)) {
		uintptr_t addr = rx_desc_ring[head].buffer_addr;
		memcpy(pkt->jp_data, KADDR(addr),
		       rx_desc_ring[head].length);
		pkt->jp_len = rx_desc_ring[head].length;

		// reset status
		rx_desc_ring[head].status = 0;

		// move head and tail
		while (tail != head) {
			rx_desc_ring[tail].status = 0;
			tail = (tail + 1) % RXDESC_SIZE;
		}
		head = (head + 1) % RXDESC_SIZE;
		e1000[E1000_RDT/4] = tail;

		// disable receive interrupt
		if(interrupt)
			e1000[E1000_IMC/4]  = E1000_IMS_RXT0;
		interrupt = false;
		return true;
	}
	// enable receive interrupt
	if(!interrupt)
		e1000[E1000_IMS/4]  = E1000_IMS_RXT0;
	interrupt = true;
	return false;
}

// for recving
void net_intr()
{
	cprintf("net_intr....\n");
	// find if some env is blocking
	int i;
	for (i = 0; i < NENV; i++) {
		if(envs[i].env_status == ENV_NOT_RUNNABLE &&
		   envs[i].env_pkt_recving) {
			if(e1000_try_recv(envs[i].env_pkt_dstpa)) {
				envs[i].env_pkt_recving = false;
				envs[i].env_pkt_dstpa = 0;
				envs[i].env_tf.tf_regs.reg_eax = 0;
				envs[i].env_status = ENV_RUNNABLE;

				cprintf("net_intr: wake up blocking"
					"env [%08x].\n", curenv->env_id,
					envs[i].env_id);
			}
		}
	}
	// clear interrupt
	e1000[E1000_ICS/4];
}
