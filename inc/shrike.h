/*
 * Copyright (c) 2003-2010 Eric Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This is the main header file, usually the only one #include'd
 */

#ifndef SHRIKE_H
#define SHRIKE_H

/* *INDENT-OFF* */

#include "sysconf.h"

/* I N C L U D E S */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <setjmp.h>
#include <sys/stat.h>
#include <ctype.h>
#include <regex.h>
#include <fcntl.h>

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

/* socket stuff */
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <grp.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/socket.h>

/* D E F I N E S */
#define BUFSIZE  1024            /* maximum size of a buffer */
#define MODESTACK_WAIT 500
#define MAXMODES 4
#define MAXPARAMSLEN (510-32-64-34-(7+MAXMODES))
#define MAX_EVENTS 25
#define TOKEN_UNMATCHED -1
#define TOKEN_ERROR -2

#undef DEBUG_BALLOC

#ifdef DEBUG_BALLOC
#define BALLOC_MAGIC 0x3d3a3c3d
#endif

#ifdef LARGE_NETWORK
#define HASHSIZE        hashsize(15)
#define HEAP_CHANNEL    1024
#define HEAP_CHANUSER   1024
#define HEAP_USER       1024
#define HEAP_SERVER     16
#define HEAP_NODE       1024
#define HEAP_CHANACS    1024
#else
#define HASHSIZE        hashsize(11)
#define HEAP_CHANNEL    64
#define HEAP_CHANUSER   128
#define HEAP_USER       128
#define HEAP_SERVER     8
#define HEAP_NODE       128
#define HEAP_CHANACS    128
#endif

#define CACHEFILE_HEAP_SIZE     32
#define CACHELINE_HEAP_SIZE     64

#ifndef uint8_t
#define uint8_t u_int8_t
#define uint16_t u_int16_t
#define uint32_t u_int32_t
#define uint64_t u_int64_t
#endif

#ifndef timersub
#define timersub(tvp, uvp, vvp)                                         \
        do {                                                            \
                (vvp)->tv_sec = (tvp)->tv_sec - (uvp)->tv_sec;          \
                (vvp)->tv_usec = (tvp)->tv_usec - (uvp)->tv_usec;       \
                if ((vvp)->tv_usec < 0) {                               \
                        (vvp)->tv_sec--;                                \
                        (vvp)->tv_usec += 1000000;                      \
                }                                                       \
        } while (0)
#endif

/* hashing macros */
#define hashsize(n) ((uint32_t)1 << (n))
#define hashmask(n) (hashsize(n) - 1)

/* T Y P E D E F S */
typedef enum { ERROR = -1, FALSE, TRUE } l_boolean_t;
#undef boolean_t
#define boolean_t l_boolean_t

typedef struct node_ node_t;
typedef struct list_ list_t;
typedef struct sra_ sra_t;
typedef struct tld_ tld_t;
typedef struct kline_ kline_t;
typedef struct server_ server_t;
typedef struct user_ user_t;
typedef struct channel_ channel_t;
typedef struct chanuser_ chanuser_t;
typedef struct myuser_ myuser_t;
typedef struct mychan_ mychan_t;
typedef struct chanacs_ chanacs_t;
typedef void EVH(void *);

typedef struct _configfile CONFIGFILE;
typedef struct _configentry CONFIGENTRY;

/* macros for linked lists */
#define LIST_FOREACH(n, head) for (n = (head); n; n = n->next)
#define LIST_FOREACH_NEXT(n, head) for (n = (head); n->next; n = n->next)

#define LIST_LENGTH(list) (list)->count

#define LIST_FOREACH_SAFE(n, tn, head) for (n = (head), tn = n ? n->next : NULL; n != NULL; n = tn, tn = n ? n->next : NULL)

/* S T R U C T U R E S */
struct me
{
  char *name;                   /* server's name on IRC               */
  char *desc;                   /* server's description               */
  char *uplink;                 /* the server we connect to           */
  char *actual;                 /* the reported name of the uplink    */
  uint16_t port;                /* port we connect to our uplink on   */
  char *pass;                   /* password we use for linking        */
  char *vhost;                  /* IP we bind outgoing stuff to       */
  uint16_t recontime;           /* time between reconnection attempts */
  uint16_t restarttime;         /* time before restarting             */
  uint32_t expire;              /* time before registrations expire   */
  char *netname;                /* IRC network name                   */
  char *adminname;              /* SRA's name (for ADMIN)             */
  char *adminemail;             /* SRA's email (for ADMIN             */
  char *mta;                    /* path to mta program           */

  uint8_t loglevel;             /* logging level                      */
  uint32_t maxfd;               /* how many fds do we have?           */
  time_t start;                 /* starting time                      */
  server_t *me;                 /* pointer to our server struct       */
  boolean_t connected;          /* are we connected?                  */
  boolean_t bursting;           /* are we bursting?                   */

  uint16_t maxusers;            /* maximum usernames from one email   */
  uint16_t maxchans;            /* maximum chans from one username    */
  uint8_t auth;                 /* registration auth type             */

  time_t uplinkpong;            /* when the uplink last sent a PONG   */
} me;

#define AUTH_NONE  0
#define AUTH_EMAIL 1

struct svs
{
  char *nick;                   /* the IRC client's nickname  */
  char *user;                   /* the IRC client's username  */
  char *host;                   /* the IRC client's hostname  */
  char *real;                   /* the IRC client's realname  */
  char *chan;                   /* channel we join/msg        */

  uint16_t flood_msgs;          /* messages determining flood */
  uint16_t flood_time;          /* time determining flood     */
  uint32_t kline_time;          /* default expire for klines  */

  boolean_t silent;             /* stop sending WALLOPS?      */
  boolean_t join_chans;         /* join registered channels?  */
  boolean_t leave_chans;        /* leave channels when empty? */

  uint16_t defuflags;           /* default username flags     */
  uint16_t defcflags;           /* default channel flags      */

  boolean_t raw;                /* enable raw/inject?         */

  char *global;                 /* nick for global noticer    */

  user_t *svs;                  /* our user_t struct          */
} svs;

/* keep track of how many of what we have */
struct cnt
{
  uint32_t event;
  uint32_t sra;
  uint32_t tld;
  uint32_t kline;
  uint32_t server;
  uint32_t user;
  uint32_t chan;
  uint32_t chanuser;
  uint32_t myuser;
  uint32_t mychan;
  uint32_t chanacs;
  uint32_t node;
  uint32_t bin;
  uint32_t bout;
} cnt;

struct _configfile
{
  char *cf_filename;
  CONFIGENTRY *cf_entries;
  CONFIGFILE *cf_next;
};

struct _configentry
{
  CONFIGFILE    *ce_fileptr;

  int ce_varlinenum;
  char *ce_varname;
  char *ce_vardata;
  int ce_vardatanum;
  int ce_fileposstart;
  int ce_fileposend;

  int ce_sectlinenum;
  CONFIGENTRY *ce_entries;

  CONFIGENTRY *ce_prevlevel;

  CONFIGENTRY *ce_next;
};

struct Token
{
  const char *text;
  int value;
};

/* list node struct */
struct node_
{
  node_t *next, *prev;
  void *data;                   /* pointer to real structure */
};

/* node list struct */
struct list_
{
  node_t *head, *tail;
  int count;                    /* how many entries in the list */
};

/* status information for an allocated block in heap */
struct Block
{
  size_t alloc_size;
  struct Block *next;     /* Next in our chain of blocks */
  void *elems;            /* Points to allocated memory */
  list_t free_list;
  list_t used_list;
};
typedef struct Block Block;

struct MemBlock
{
#ifdef DEBUG_BALLOC
  unsigned long magic;
#endif
  node_t self;
  Block *block;           /* Which block we belong to */
};

typedef struct MemBlock MemBlock;

/* information for the root node of the heap */
struct BlockHeap
{
  node_t hlist;
  size_t elemSize;        /* Size of each element to be stored */
  unsigned long elemsPerBlock;    /* Number of elements per block */
  unsigned long blocksAllocated;  /* Number of blocks allocated */
  unsigned long freeElems;                /* Number of free elements */
  Block *base;            /* Pointer to first block */
};
typedef struct BlockHeap BlockHeap;

/* event list struct */
struct ev_entry
{
  EVH *func;
  void *arg;
  const char *name;
  time_t frequency;
  time_t when;
  boolean_t active;
};

/* sra list struct */
struct sra_ {
  myuser_t *myuser;
  char *name;
};

/* tld list struct */
struct tld_ {
  char *name;
};

/* kline list struct */
struct kline_ {
  char *user;
  char *host;
  char *reason;
  char *setby;

  uint16_t number;
  long duration;
  time_t settime;
  time_t expires;
};

/* global list struct */
struct global_ {
  char *text;
};

/* sendq struct */
struct sendq {
  char *buf;
  int len;
  int pos;
};

/* servers struct */
struct server_
{
  char *name;
  char *desc;

  uint8_t hops;
  uint32_t users;
  uint32_t invis;
  uint32_t opers;
  uint32_t away;

  time_t connected_since;

  uint32_t flags;
  int32_t hash;

  server_t *uplink;
};

#define SF_HIDE        0x00000001

/* channels struct */
struct channel_
{
  char *name;

  uint32_t modes;
  char *key;
  uint32_t limit;

  uint32_t nummembers;

  uint32_t ts;
  int32_t hash;

  list_t members;
};

/* users struct */
struct user_
{
  char *nick;
  char *user;
  char *host;

  list_t channels;

  server_t *server;
  myuser_t *myuser;

  uint8_t offenses;
  uint8_t msgs;
  time_t lastmsg;

  uint32_t flags;
  int32_t hash;
};

#define UF_ISOPER      0x00000001
#define UF_ISAWAY      0x00000002
#define UF_INVIS       0x00000004
#define UF_LOGGEDIN    0x00000008
#define UF_IRCOP       0x00000010
#define UF_ADMIN       0x00000020
#define UF_SRA         0x00000040

/* struct for channel users */
struct chanuser_
{
  channel_t *chan;
  user_t *user;
  uint32_t modes;
};

#define CMODE_BAN       0x00000000      /* IGNORE */
#define CMODE_INVITE    0x00000001
#define CMODE_KEY       0x00000002
#define CMODE_LIMIT     0x00000004
#define CMODE_MOD       0x00000008
#define CMODE_NOEXT     0x00000010
#define CMODE_OP        0x00000020      /* SPECIAL */
#define CMODE_PRIV      0x00000040      /* AKA PARA */
#define CMODE_SEC       0x00000080
#define CMODE_TOPIC     0x00000100
#define CMODE_VOICE     0x00000200      /* SPECIAL */
#define CMODE_EXEMPT    0x00000000      /* IGNORE */
#define CMODE_INVEX     0x00000000      /* IGNORE */

/* struct for registered usernames */
struct myuser_
{
  char *name;
  char *pass;
  char *email;

  user_t *user;
  time_t registered;
  time_t lastlogin;

  boolean_t identified;
  uint16_t failnum;
  char *lastfail;
  time_t lastfailon;

  list_t chanacs;
  sra_t *sra;

  unsigned long key;
  char *temp;

  uint32_t flags;
  int32_t hash;
};

#define MU_HOLD        0x00000001
#define MU_NEVEROP     0x00000002
#define MU_NOOP        0x00000004
#define MU_WAITAUTH    0x00000008
#define MU_HIDEMAIL    0x00000010

#define MU_IRCOP       0x00001000
#define MU_SRA         0x00002000

/* struct for registered channels */
struct mychan_
{
  char *name;
  char *pass;

  myuser_t *founder;
  myuser_t *successor;
  channel_t *chan;
  list_t chanacs;
  time_t registered;
  time_t used;

  int mlock_on;
  int mlock_off;
  int mlock_limit;
  char *mlock_key;

  uint32_t flags;
  int32_t hash; 
};

#define MC_HOLD        0x00000001
#define MC_NEVEROP     0x00000002
#define MC_RESTRICTED  0x00000004
#define MC_SECURE      0x00000008
#define MC_VERBOSE     0x00000010

/* struct for channel access list */
struct chanacs_
{
  myuser_t *myuser;
  mychan_t *mychan;
  char *host;
  uint32_t level; /* XXX - kill */
  uint32_t flags;
};

#define CA_NONE          0x00000001
#define CA_VOP           0x00000002
#define CA_AOP           0x00000004
#define CA_SOP           0x00000008
#define CA_FOUNDER       0x00000010
#define CA_SUCCESSOR     0x00000020

#define ACL_NONE         0x00000001 /* no flags                        */
#define ACL_KICK         0x00000002 /* auto kick/ban                   */
#define ACL_VOICE        0x00000004 /* auto voice                      */
#define ACL_OP           0x00000008 /* auto op                         */
#define ACL_SET          0x00000010 /* access to a subset of SET flags */
#define ACL_INVITE       0x00000020 /* access to INVITE                */
#define ACL_RECOVER      0x00000040 /* access to RECOVER               */
#define ACL_ACLVIEW      0x00000080 /* access to view access lists     */
#define ACL_ACL          0x00000100 /* access to modify access lists   */
#define ACL_SUCCESSOR    0x00000200 /* traditional successor           */
#define ACL_FOUNDER      0x00000400 /* traditional founder             */

/* struct for irc message hash table */
struct message_
{
  const char *name;
  void (*func) (char *origin, uint8_t parc, char *parv[]);
};

/* struct for command hash table */
struct command_
{
  const char *name;
  uint8_t access;
  void (*func) (char *nick);
};

/* struct for set command hash table */
struct set_command_
{
  const char *name;
  uint8_t access;
  void (*func) (char *origin, char *name, char *params);
};

/* struct for help command hash table */
struct help_command_
{
  const char *name;
  uint8_t access;
  char *file;
};

#define AC_NONE  0
#define AC_IRCOP 1
#define AC_SRA   2

/* run flags */
int runflags;

#define RF_LIVE         0x00000001      /* don't fork  */
#define RF_SHUTDOWN     0x00000002      /* shut down   */
#define RF_STARTING     0x00000004      /* starting up */
#define RF_RESTART      0x00000008      /* restart     */
#define RF_REHASHING    0x00000010      /* rehashing   */

/* log levels */
#define LG_NONE         0x00000001      /* don't log                */
#define LG_INFO         0x00000002      /* log general info         */
#define LG_ERROR        0x00000004      /* log real important stuff */
#define LG_DEBUG        0x00000008      /* log debugging stuff      */

/* bursting timer */
#if HAVE_GETTIMEOFDAY
struct timeval burstime;
#endif

/* *INDENT-ON* */

/* down here so stuff it uses in here works */
#include "extern.h"

#endif /* SHRIKE_H */
