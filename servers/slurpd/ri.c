/*
 * Copyright (c) 1996 Regents of the University of Michigan.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of Michigan at Ann Arbor. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 */

/*
 * ri.c - routines used to manipulate Ri structures.  An Ri (Replica
 * information) struct contains all information about one replica
 * instance.  The Ri struct is defined in slurp.h
 */


#include "portable.h"

#include <stdio.h>
#include <ac/signal.h>

#include "slurp.h"
#include "globals.h"


/* External references */
extern void write_reject LDAP_P(( Ri *, Re *, int, char * ));
extern void do_nothing LDAP_P(());

/* Forward references */
static int ismine LDAP_P(( Ri  *, Re  * ));
static int isnew LDAP_P(( Ri  *, Re  * ));
void tsleep LDAP_P(( time_t ));


/*
 * Process any unhandled replication entries in the queue.
 */
static int
Ri_process(
    Ri *ri
)
{
    Rq		*rq = sglob->rq;
    Re		*re, *new_re;
    int		i;
    int		rc ;
    char	*errmsg;

#ifdef HAVE_LINUX_THREADS
    (void) SIGNAL( SIGSTKFLT, do_nothing );
#else
    (void) SIGNAL( SIGUSR1, do_nothing );
#endif
    (void) SIGNAL( SIGPIPE, SIG_IGN );
    if ( ri == NULL ) {
	Debug( LDAP_DEBUG_ANY, "Error: Ri_process: ri == NULL!\n", 0, 0, 0 );
	return -1;
    }

    /*
     * Startup code.  See if there's any work to do.  If not, wait on the
     * rq->rq_more condition variable.
     */
    rq->rq_lock( rq );
    while ( !sglob->slurpd_shutdown &&
	    (( re = rq->rq_gethead( rq )) == NULL )) {
	/* No work - wait on condition variable */
	pthread_cond_wait( &rq->rq_more, &rq->rq_mutex );
    }

    /*
     * When we get here, there's work in the queue, and we have the
     * queue locked.  re should be pointing to the head of the queue.
     */
    rq->rq_unlock( rq );
    while ( !sglob->slurpd_shutdown ) {
	if ( re != NULL ) {
	    if ( !ismine( ri, re )) {
		/* The Re doesn't list my host:port */
		Debug( LDAP_DEBUG_TRACE,
			"Replica %s:%d, skip repl record for %s (not mine)\n",
			ri->ri_hostname, ri->ri_port, re->re_dn );
	    } else if ( !isnew( ri, re )) {
		/* This Re is older than my saved status information */
		Debug( LDAP_DEBUG_TRACE,
			"Replica %s:%d, skip repl record for %s (old)\n",
			ri->ri_hostname, ri->ri_port, re->re_dn );
	    } else {
		rc = do_ldap( ri, re, &errmsg );
		switch ( rc ) {
		case DO_LDAP_ERR_RETRYABLE:
		    tsleep( RETRY_SLEEP_TIME );
		    Debug( LDAP_DEBUG_ANY,
			    "Retrying operation for DN %s on replica %s:%d\n",
			    re->re_dn, ri->ri_hostname, ri->ri_port );
		    continue;
		    break;
		case DO_LDAP_ERR_FATAL:
		    /* Non-retryable error.  Write rejection log. */
		    write_reject( ri, re, ri->ri_ldp->ld_errno, errmsg );
		    /* Update status ... */
		    (void) sglob->st->st_update( sglob->st, ri->ri_stel, re );
		    /* ... and write to disk */
		    (void) sglob->st->st_write( sglob->st );
		    break;
		default:
		    /* LDAP op completed ok - update status... */
		    (void) sglob->st->st_update( sglob->st, ri->ri_stel, re );
		    /* ... and write to disk */
		    (void) sglob->st->st_write( sglob->st );
		    break;
		}
	    }
	} else {
	    Debug( LDAP_DEBUG_ANY, "Error: re is null in Ri_process\n",
		    0, 0, 0 );
	}
	rq->rq_lock( rq );
	while ( !sglob->slurpd_shutdown &&
		((new_re = re->re_getnext( re )) == NULL )) {
	    if ( sglob->one_shot_mode ) {
		return 0;
	    }
	    /* No work - wait on condition variable */
	    pthread_cond_wait( &rq->rq_more, &rq->rq_mutex );
	}
	re->re_decrefcnt( re );
	re = new_re;
	rq->rq_unlock( rq );
	if ( sglob->slurpd_shutdown ) {
	    return 0;
	}
    }
    return 0;
}


/*
 * Wake a replication thread which may be sleeping.
 * Send it a SIG(STKFLT|USR1).
 */
static void
Ri_wake(
    Ri *ri
) 
{
    if ( ri == NULL ) {
	return;
    }
#ifdef HAVE_LINUX_THREADS
    pthread_kill( ri->ri_tid, SIGSTKFLT );
    (void) SIGNAL( SIGSTKFLT, do_nothing );
#else
    pthread_kill( ri->ri_tid, SIGUSR1 );
    (void) SIGNAL( SIGUSR1, do_nothing );
#endif
}



/* 
 * Allocate and initialize an Ri struct.
 */
int
Ri_init(
    Ri	**ri
)
{
    (*ri) = ( Ri * ) malloc( sizeof( Ri ));
    if ( *ri == NULL ) {
	return -1;
    }

    /* Initialize member functions */
    (*ri)->ri_process = Ri_process;
    (*ri)->ri_wake = Ri_wake;

    /* Initialize private data */
    (*ri)->ri_hostname = NULL;
    (*ri)->ri_port = 0;
    (*ri)->ri_ldp = NULL;
    (*ri)->ri_bind_method = 0;
    (*ri)->ri_bind_dn = NULL;
    (*ri)->ri_password = NULL;
    (*ri)->ri_principal = NULL;
    (*ri)->ri_srvtab = NULL;
    (*ri)->ri_curr = NULL;

    return 0;
}




/*
 * Return 1 if the hostname and port in re match the hostname and port
 * in ri, otherwise return zero.
 */
static int
ismine(
    Ri	*ri,
    Re	*re
)
{
    Rh	*rh;
    int	i;

    if ( ri == NULL || re == NULL || ri->ri_hostname == NULL ||
	    re->re_replicas == NULL ) {
	return 0;
    }
    rh = re->re_replicas;
    for ( i = 0; rh[ i ].rh_hostname != NULL; i++ ) {
	if ( !strcmp( rh[ i ].rh_hostname, ri->ri_hostname) &&
		rh[ i ].rh_port == ri->ri_port ) {
	    return 1;
	}
    }
    return 0;
}




/*
 * Return 1 if the Re's timestamp/seq combination are greater than the
 * timestamp and seq in the Ri's ri_stel member.  In other words, if we
 * find replication entries in the log which we've already processed,
 * don't process them.  If the re is "old," return 0.
 * No check for NULL pointers is done.
 */
static int
isnew(
    Ri	*ri,
    Re	*re
)
{
    int	x;
    int	ret;

    /* Lock the St struct to avoid a race */
    sglob->st->st_lock( sglob->st );
    x = strcmp( re->re_timestamp, ri->ri_stel->last );
    if ( x > 0 ) {
	/* re timestamp is newer */
	ret = 1;
    } else if ( x < 0 ) {
	ret = 0;
    } else {
	/* timestamps were equal */
	if ( re->re_seq > ri->ri_stel->seq ) {
	    /* re seq is newer */
	    ret = 1;
	} else {
	    ret = 0;
	}
    }
    sglob->st->st_unlock( sglob->st );
    return ret;
}


