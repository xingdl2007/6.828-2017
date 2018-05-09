#include "ns.h"

void
output(envid_t ns_envid)
{
	binaryname = "ns_output";

	// LAB 6: Your code here:
	// 	- read a packet from the network server
	//	- send the packet to the device driver
	uint32_t req;
	int perm, r;
	struct jif_pkt *pkt;
	while (1) {
		req = ipc_recv(0, (void *)REQVA, &perm);
		if(!(perm & PTE_P))
			continue;

		// REQVA
		pkt = (struct jif_pkt*)REQVA;
		if(req == NSREQ_OUTPUT) {
			while((r = sys_pkt_try_send(pkt->jp_data,
						    pkt->jp_len)) != 0)
				if(r == -E_NO_MEM)
					sys_yield();
				else
					panic("sys_pkt_try_send: %e",r);
		} else {
			cprintf("output recv invalid req: %d\n", req);
		}
	}
}
