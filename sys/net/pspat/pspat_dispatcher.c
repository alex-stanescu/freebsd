#ifndef PSPAT
#define PSPAT
#endif

#include "pspat_dispatcher.h"
#include "mailbox.h"
#include "pspat_opts.h"
#include "pspat_arbiter.h"

#include <sys/types.h>
#include <sys/mbuf.h>
#include <sys/proc.h>
#include <netpfil/ipfw/ip_dn_io.h>
MALLOC_DECLARE(M_PSPAT);

/*
 * Dispatches a mbuf
 */

static void
dispatch(struct pspat_packet *packet) {
	struct mbuf *m = packet->buf;
	curthread->td_vnet = packet->vnet;
	if (pspat_debug_xmit) {
		printf("Dispatching from dispatcher: %p\n", m);
	}
	dummynet_send(m);
	free(packet, M_PSPAT);
}


int pspat_dispatcher_run(struct pspat_dispatcher *d) {
	struct pspat_mailbox *m = d->mb;
	struct pspat_packet *packet = NULL;
	int ndeq = 0;
	while (ndeq < pspat_dispatch_batch && ((packet = pspat_mb_extract(m)) != NULL)) {
		dispatch(packet);
		ndeq ++;
	}

	pspat_dispatch_deq += ndeq;
	pspat_mb_clear(m);

	if(pspat_debug_xmit && ndeq) {
		printf("PSPAT Sender processed %d mbfs\n", ndeq);
	}

	return ndeq;
}

/*
 * Shuts down the dispatcher
 */
void pspat_dispatcher_shutdown(struct pspat_dispatcher *d) {
	struct pspat_packet *packet;
	int n = 0;

	/* Drain the sender mailbox. */
	while ( (packet = pspat_mb_extract(d->mb)) != NULL ) {
		m_free(packet->buf);
		free(packet, M_PSPAT);
		n ++;
	}
	printf("%s: Sender MB drained, found %d mbfs\n", __func__, n);
}
