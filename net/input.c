#include "ns.h"
#include "inc/lib.h"
#include "inc/memlayout.h"

extern union Nsipc nsipcbuf;

void
input(envid_t ns_envid)
{
	binaryname = "ns_input";

	// LAB 6: Your code here:
	// 	- read a packet from the device driver
	//	- send it to the network server
	// Hint: When you IPC a page to the network server, it will be
	// reading from it for a while, so don't immediately receive
	// another packet in to the same physical page.
	static union Nsipc nsipcbuf2 __attribute__((aligned(PGSIZE)));
	physaddr_t addr, addr1, addr2;
	int r;

	// COW, private copy
	memset(&nsipcbuf, 0, PGSIZE);
	memset(&nsipcbuf2, 0, PGSIZE);

	addr1 = PTE_ADDR(uvpt[PGNUM(&nsipcbuf)]);
	addr2 = PTE_ADDR(uvpt[PGNUM(&nsipcbuf2)]);

	addr = addr1;
	void *buffer = &nsipcbuf;
	while (1) {
		// 1. call sys_pkt_recv
		//sys_pkt_recv(addr);
		while(sys_pkt_recv(addr) !=0)
			sys_yield();
		assert(nsipcbuf.pkt.jp_len != 0);

		// 2. ipc_send to network core
		ipc_send(ns_envid, NSREQ_INPUT, buffer, PTE_P | PTE_U);

		// waiting for network server to return page
		while(1) {
			if(pageref(&nsipcbuf) == 1) {
				addr = addr1;
				buffer = &nsipcbuf;
				break;
			}else if(pageref(&nsipcbuf2) == 1) {
				addr = addr2;
				buffer = &nsipcbuf2;
				break;
			}
			sys_yield();
		}
	}
}
