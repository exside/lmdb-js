/* back-bdb.h - ldap ldbm back-end header file */
/* $OpenLDAP$ */
/*
 * Copyright 2000 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#ifndef _BACK_BDB_H_
#define _BACK_BDB_H_

#include <portable.h>
#include <db.h>

#include "slap.h"

LDAP_BEGIN_DECL

/* #define BDB_INDEX 1 */
/* #define BDB_REINDEX 1 */
/* #define BDB_FILTER_INDICES 1 */
#define BDB_CONFIG_INDICES 1

#define DN_BASE_PREFIX		SLAP_INDEX_EQUALITY_PREFIX
#define DN_ONE_PREFIX	 	'%'
#define DN_SUBTREE_PREFIX 	'@'

#define DBTzero(t)			(memset((t), 0, sizeof(DBT)))
#define DBT2bv(t,bv)		((bv)->bv_val = (t)->data, \
								(bv)->bv_len = (t)->size)
#define bv2DBT(bv,t)		((t)->data = (bv)->bv_val, \
								(t)->size = (bv)->bv_len )

#define DEFAULT_MODE		0600

#define BDB_TXN_RETRIES	16

#define BDB_DBENV_HOME	LDAP_RUNDIR LDAP_DIRSEP "openldap-bdb"

#ifdef BDB_SUBDIRS
#define BDB_TMP_SUBDIR	LDAP_DIRSEP "tmp"
#define BDB_LG_SUBDIR	LDAP_DIRSEP "log"
#define BDB_DATA_SUBDIR	LDAP_DIRSEP "data"
#endif

#define BDB_SUFFIX		".bdb"
#define BDB_NEXTID		0
#define BDB_DN2ID		1
#define BDB_ID2ENTRY	2
#define BDB_INDICES		3

struct bdb_db_info {
	DB			*bdi_db;
};

struct bdb_info {
	DB_ENV		*bi_dbenv;

	/* DB_ENV parameters */
	/* The DB_ENV can be tuned via DB_CONFIG */
	char		*bi_dbenv_home;
	u_int32_t	bi_dbenv_xflags; /* extra flags */
	int			bi_dbenv_mode;

	int			bi_ndatabases;
	struct bdb_db_info **bi_databases;

	int			bi_txn_cp;
	u_int32_t	bi_txn_cp_min;
	u_int32_t	bi_txn_cp_kbyte;

#ifndef NO_THREADS
	int			bi_lock_detect;
	int			bi_lock_detect_seconds;
	ldap_pvt_thread_t	bi_lock_detect_tid;
#endif

#if BDB_CONFIG_INDICES
	slap_mask_t	bi_defaultmask;
	Avlnode		*bi_attrs;
#endif

	ID			bi_lastid;
};

#define bi_nextid	bi_databases[BDB_NEXTID]
#define bi_id2entry	bi_databases[BDB_ID2ENTRY]
#define bi_dn2id	bi_databases[BDB_DN2ID]

struct bdb_op_info {
	BackendDB*	boi_bdb;
	DB_TXN*		boi_txn;
	int			boi_err;
};

LDAP_END_DECL

#include "proto-bdb.h"

#endif /* _BACK_BDB_H_ */
