/*
 * Copyright (c) 1982, 1986, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)tcp_timer.h	8.1 (Berkeley) 6/10/93
 * $FreeBSD: src/sys/netinet/tcp_timer.h,v 1.18.2.1 2002/08/16 22:16:39 dillon Exp $
 */

#ifndef _NETINET_TCP_TIMER_H_
#define _NETINET_TCP_TIMER_H_

/*
 * The TCPT_REXMT timer is used to force retransmissions.
 * The TCP has the TCPT_REXMT timer set whenever segments
 * have been sent for which ACKs are expected but not yet
 * received.  If an ACK is received which advances tp->snd_una,
 * then the retransmit timer is cleared (if there are no more
 * outstanding segments) or reset to the base value (if there
 * are more ACKs expected).  Whenever the retransmit timer goes off,
 * we retransmit one unacknowledged segment, and do a backoff
 * on the retransmit timer.
 *
 * The TCPT_PERSIST timer is used to keep window size information
 * flowing even if the window goes shut.  If all previous transmissions
 * have been acknowledged (so that there are no retransmissions in progress),
 * and the window is too small to bother sending anything, then we start
 * the TCPT_PERSIST timer.  When it expires, if the window is nonzero,
 * we go to transmit state.  Otherwise, at intervals send a single byte
 * into the peer's window to force him to update our window information.
 * We do this at most as often as TCPT_PERSMIN time intervals,
 * but no more frequently than the current estimate of round-trip
 * packet time.  The TCPT_PERSIST timer is cleared whenever we receive
 * a window update from the peer.
 *
 * The TCPT_KEEP timer is used to keep connections alive.  If an
 * connection is idle (no segments received) for TCPTV_KEEP_INIT amount of time,
 * but not yet established, then we drop the connection.  Once the connection
 * is established, if the connection is idle for TCPTV_KEEP_IDLE time
 * (and keepalives have been enabled on the socket), we begin to probe
 * the connection.  We force the peer to send us a segment by sending:
 *	<SEQ=SND.UNA-1><ACK=RCV.NXT><CTL=ACK>
 * This segment is (deliberately) outside the window, and should elicit
 * an ack segment in response from the peer.  If, despite the TCPT_KEEP
 * initiated segments we cannot elicit a response from a peer in TCPT_MAXIDLE
 * amount of time probing, then we drop the connection.
 */

/*
 * Time constants.
 */
#define	TCPTV_MSL	( 30*hz)		/* max seg lifetime (hah!) */
#define	TCPTV_SRTTBASE	0			/* base roundtrip time;
						   if 0, no idea yet */
#define	TCPTV_RTOBASE	(  3*hz)		/* assumed RTO if no info */
#define	TCPTV_SRTTDFLT	(  3*hz)		/* assumed RTT if no info */

#define	TCPTV_PERSMIN	(  5*hz)		/* retransmit persistence */
#define	TCPTV_PERSMAX	( 60*hz)		/* maximum persist interval */

#define	TCPTV_KEEP_INIT	( 75*hz)		/* initial connect keepalive */
#define	TCPTV_KEEP_IDLE	(120*60*hz)		/* dflt time before probing */
#define	TCPTV_KEEPINTVL	( 75*hz)		/* default probe interval */
#define	TCPTV_KEEPCNT	8			/* max probes before drop */

/*
 * TCPTV_MIN represents the minimum allowed retransmit interval.  It
 * is currently one second but will ultimately be reduced to 3 ticks
 * for algorithmic stability, leaving the 200ms variance to deal with
 * delayed-acks, protocol overheads.  A 1 second minimum badly breaks
 * throughput on any network faster then a modem that has minor but
 * continuous packet loss unrelated to congestion, such as on a wireless
 * network.
 */
#define	TCPTV_MIN	( hz )			/* minimum allowable value */
#define TCPTV_CPU_VAR	( hz/5 )		/* cpu variance (200ms) */
#define	TCPTV_REXMTMAX	( 64*hz)		/* max allowable REXMT value */

#define TCPTV_TWTRUNC	8			/* RTO factor to truncate TW */

#define	TCP_LINGERTIME	120			/* linger at most 2 minutes */

#define	TCP_MAXRXTSHIFT	12			/* maximum retransmits */

#define	TCPTV_DELACK	(hz / PR_FASTHZ / 2)	/* 100ms timeout */

#ifdef	TCPTIMERS
static char *tcptimers[] =
    { "REXMT", "PERSIST", "KEEP", "2MSL" };
#endif

/*
 * Force a time value to be in a certain range.
 */
#define	TCPT_RANGESET(tv, value, tvmin, tvmax) do { \
	(tv) = (value) + tcp_rexmit_slop; \
	if ((u_long)(tv) < (u_long)(tvmin)) \
		(tv) = (tvmin); \
	else if ((u_long)(tv) > (u_long)(tvmax)) \
		(tv) = (tvmax); \
} while(0)

#ifdef _KERNEL
extern int tcp_keepinit;		/* time to establish connection */
extern int tcp_keepidle;		/* time before keepalive probes begin */
extern int tcp_keepintvl;		/* time between keepalive probes */
extern int tcp_maxidle;			/* time to drop after starting probes */
extern int tcp_delacktime;		/* time before sending a delayed ACK */
extern int tcp_maxpersistidle;
extern int tcp_rexmit_min;
extern int tcp_rexmit_slop;
extern int tcp_msl;
extern int tcp_ttl;			/* time to live for TCP segs */
extern int tcp_backoff[];

void	tcp_timer_2msl __P((void *xtp));
void	tcp_timer_keep __P((void *xtp));
void	tcp_timer_persist __P((void *xtp));
void	tcp_timer_rexmt __P((void *xtp));
void	tcp_timer_delack __P((void *xtp));

#endif /* _KERNEL */

#endif /* !_NETINET_TCP_TIMER_H_ */
