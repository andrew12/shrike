/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains misc routines.
 *
 * $$Id$
 */

#include "../inc/shrike.h"

FILE *log_file;

#if !HAVE_STRLCAT
/* i hate linux. */
size_t strlcat(char *dst, const char *src, size_t siz)
{
  char *d = dst;
  const char *s = src;
  size_t n = siz;
  size_t dlen;

  /* find the end of dst and adjust bytes left but don't go past end */
  while (n-- != 0 && *d != '\0')
    d++;

  dlen = d - dst;
  n = siz - dlen;

  if (n == 0)
    return (dlen + strlen(s));

  while (*s != '\0')
  {
    if (n != 1)
    {
      *d++ = *s;
      n--;
    }

    s++;
  }

  *d = '\0';

  return (dlen + (s - src));    /* count does not include NUL */
}
#endif

#if !HAVE_STRLCPY
/* i hate linux. */
size_t strlcpy(char *dst, const char *src, size_t siz)
{
  char *d = dst;
  const char *s = src;
  size_t n = siz;

  if (n != 0 && --n != 0)
  {
    do
    {
      if (!(*d++ = *s++))
        break;
    } while (--n != 0);
  }

  if (!n)
  {
    if (siz != 0)
      *d = '\0';

    while (*s++);
  }
  return (s - src - 1);
}
#endif

#if HAVE_GETTIMEOFDAY
/* starts a timer */
void s_time(struct timeval *sttime)
{
  gettimeofday(sttime, NULL);
}
#endif

#if HAVE_GETTIMEOFDAY
/* ends a timer */
void e_time(struct timeval sttime, struct timeval *ttime)
{
  struct timeval now;

  gettimeofday(&now, NULL);
  timersub(&now, &sttime, ttime);
}
#endif

#if HAVE_GETTIMEOFDAY
/* translates microseconds into miliseconds */
int32_t tv2ms(struct timeval *tv)
{
  return (tv->tv_sec * 1000) + (int32_t) (tv->tv_usec / 1000);
}
#endif

/* replaces tabs with a single ASCII 32 */
void tb2sp(char *line)
{
  char *c;

  while ((c = strchr(line, '\t')))
    *c = ' ';
}

/* copy at most len-1 characters from a string to a buffer, NULL terminate */
char *strscpy(char *d, const char *s, size_t len)
{
  char *d_orig = d;

  if (!len)
    return d;
  while (--len && (*d++ = *s++));
  *d = 0;
  return d_orig;
}

/* does malloc()'s job and dies if malloc() fails */
void *smalloc(long size)
{
  void *buf;

  buf = malloc(size);
  if (!buf)
    raise(SIGUSR1);
  return buf;
}

/* does calloc()'s job and dies if calloc() fails */
void *scalloc(long elsize, long els)
{
  void *buf = calloc(elsize, els);

  if (!buf)
    raise(SIGUSR1);
  return buf;
}

/* does realloc()'s job and dies if realloc() fails */
void *srealloc(void *oldptr, long newsize)
{
  void *buf = realloc(oldptr, newsize);

  if (!buf)
    raise(SIGUSR1);
  return buf;
}

/* does strdup()'s job, only with the above memory functions */
char *sstrdup(const char *s)
{
  char *t = smalloc(strlen(s) + 1);

  strcpy(t, s);
  return t;
}

/* removes unwanted chars from a line */
void strip(char *line)
{
  char *c;

  if (line)
  {
    if ((c = strchr(line, '\n')))
      *c = '\0';
    if ((c = strchr(line, '\r')))
      *c = '\0';
    if ((c = strchr(line, '\1')))
      *c = '\0';
  }
}

/* opens shrike.log */
void log_open(void)
{
  if (log_file)
    return;

  if (!(log_file = fopen("var/shrike.log", "a")))
    exit(EXIT_FAILURE);
}

/* logs something to shrike.log */
void slog(uint8_t sameline, uint32_t level, const char *fmt, ...)
{
  va_list args;
  time_t t;
  struct tm tm;
  char buf[32];

  if (level > me.loglevel)
    return;

  va_start(args, fmt);

  if (sameline == 1)
  {
    time(&t);
    tm = *localtime(&t);
    strftime(buf, sizeof(buf) - 1, "[%d/%m %H:%M:%S] ", &tm);
  }

  if (!log_file)
    log_open();

  if (log_file)
  {
    if (sameline == 1)
      fputs(buf, log_file);

    vfprintf(log_file, fmt, args);

    if (!sameline)
      fputc('\n', log_file);

    fflush(log_file);
  }

  if ((runflags & RF_LIVE))
  {
    vfprintf(stderr, fmt, args);

    if (!sameline)
      fputc('\n', stderr);
  }
}

/* return the current time in milliseconds */
uint32_t time_msec(void)
{
#ifdef HAVE_GETTIMEOFDAY
  struct timeval tv;

  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000 + tv.tv_usec / 1000;
#else
  return time(NULL) * 1000;
#endif
}

/* performs a regex match */
uint8_t regex_match(regex_t * preg, char *pattern, char *string,
                    size_t nmatch, regmatch_t pmatch[], int eflags)
{
  static char errmsg[256];
  int errnum;

  /* compile regex */
  preg = (regex_t *) malloc(sizeof(*preg));

  errnum = regcomp(preg, pattern, REG_ICASE | REG_EXTENDED);
  if (errnum != 0)
  {
    regerror(errnum, preg, errmsg, 256);
    slog(0, LG_ERR, "regex_match(): %s\n", errmsg);
    free(preg);
    preg = NULL;
    return 1;
  }

  /* match it */
  if (regexec(preg, string, 5, pmatch, 0) != 0)
    return 1;

  else
    return 0;
}

/* generates a hash value */
uint32_t shash(const unsigned char *text)
{
  unsigned long h = 0, g;

  while (*text)
  {
    h = (h << 4) + tolower(*text++);

    if ((g = (h & 0xF0000000)))
      h ^= g >> 24;

    h &= ~g;
  }

  return (h % HASHSIZE);
}

/* reverse of atoi() */
char *itoa(int num)
{
  static char ret[32];
  sprintf(ret, "%d", num);
  return ret;
}

/* channel modes we support */
struct flag
{
  char mode;
  int32_t flag;
  char prefix;
};

static const struct flag cmodes[] = {
  {'t', CMODE_TOPIC, 0},
  {'n', CMODE_NOEXT, 0},
  {'s', CMODE_SEC, 0},
  {'m', CMODE_MOD, 0},
  {'l', CMODE_LIMIT, 0},
  {'i', CMODE_INVITE, 0},
  {'p', CMODE_PRIV, 0},
  {'k', CMODE_KEY, 0},
  {0, 0}
};

static const struct flag cumodes[] = {
  {'o', CMODE_OP, '@'},
  {'v', CMODE_VOICE, '+'},
  {0, 0, 0}
};

/* convert mode flags to a text mode string */
char *flags_to_string(int32_t flags)
{
  static char buf[32];
  char *s = buf;
  int i;

  for (i = 0; cmodes[i].mode != 0; i++)
    if (flags & cmodes[i].flag)
      *s++ = cmodes[i].mode;

  *s = 0;

  return buf;
}

/* convert a mode character to a flag. */
int32_t mode_to_flag(char c)
{
  int i;

  for (i = 0; cmodes[i].mode != 0 && cmodes[i].mode != c; i++);

  return cmodes[i].flag;
}

/* return the time elapsed since an event */
char *time_ago(time_t event)
{
  static char ret[128];
  int years, weeks, days, hours, minutes, seconds;

  event = time(NULL) - event;
  years = weeks = days = hours = minutes = seconds = 0;

  while (event >= 60 * 60 * 24 * 365)
  {
    event -= 60 * 60 * 24 * 365;
    years++;
  }
  while (event >= 60 * 60 * 24 * 7)
  {
    event -= 60 * 60 * 24 * 7;
    weeks++;
  }
  while (event >= 60 * 60 * 24)
  {
    event -= 60 * 60 * 24;
    days++;
  }
  while (event >= 60 * 60)
  {
    event -= 60 * 60;
    hours++;
  }
  while (event >= 60)
  {
    event -= 60;
    minutes++;
  }

  seconds = event;

  if (years)
    snprintf(ret, sizeof(ret),
             "%d year%s, %d week%s, %d day%s, %02d:%02d:%02d",
             years, years == 1 ? "" : "s",
             weeks, weeks == 1 ? "" : "s",
             days, days == 1 ? "" : "s", hours, minutes, seconds);
  else if (weeks)
    snprintf(ret, sizeof(ret), "%d week%s, %d day%s, %02d:%02d:%02d",
             weeks, weeks == 1 ? "" : "s",
             days, days == 1 ? "" : "s", hours, minutes, seconds);
  else if (days)
    snprintf(ret, sizeof(ret), "%d day%s, %02d:%02d:%02d",
             days, days == 1 ? "" : "s", hours, minutes, seconds);
  else if (hours)
    snprintf(ret, sizeof(ret), "%d hour%s, %d minute%s, %d second%s",
             hours, hours == 1 ? "" : "s",
             minutes, minutes == 1 ? "" : "s",
             seconds, seconds == 1 ? "" : "s");
  else if (minutes)
    snprintf(ret, sizeof(ret), "%d minute%s, %d second%s",
             minutes, minutes == 1 ? "" : "s",
             seconds, seconds == 1 ? "" : "s");
  else
    snprintf(ret, sizeof(ret), "%d second%s",
             seconds, seconds == 1 ? "" : "s");

  return ret;
}

/* various access level checkers */
boolean_t is_founder(mychan_t *mychan, myuser_t *myuser)
{
  if (mychan->founder == myuser)
    return TRUE;

  if ((chanacs_find(mychan, myuser, CA_FOUNDER)))
    return TRUE;

  return FALSE;
}

boolean_t is_xop(mychan_t *mychan, myuser_t *myuser, uint8_t level)
{
  chanacs_t *ca;

  if ((ca = chanacs_find(mychan, myuser, level)))
    return TRUE;

  return FALSE;
}

boolean_t is_on_mychan(mychan_t *mychan, myuser_t *myuser)
{
  chanuser_t *cu;

  if ((cu = chanuser_find(mychan->chan, myuser->user)))
    return TRUE;

  return FALSE;
}

boolean_t should_op(mychan_t *mychan, myuser_t *myuser)
{
  chanuser_t *cu;

  if (MC_NEVEROP & mychan->flags)
    return FALSE;

  if (MU_NEVEROP & myuser->flags)
    return FALSE;

  cu = chanuser_find(mychan->chan, myuser->user);
  if (!cu)
    return FALSE;

  if (CMODE_OP & cu->modes)
    return FALSE;

  if (is_founder(mychan, myuser))
    return TRUE;

  if (is_xop(mychan, myuser, (CA_SOP | CA_AOP)))
    return TRUE;

  return FALSE;
}

boolean_t should_voice(mychan_t *mychan, myuser_t *myuser)
{
  chanuser_t *cu;

  if (MC_NEVEROP & mychan->flags)
    return FALSE;

  if (MU_NEVEROP & myuser->flags)
    return FALSE;

  cu = chanuser_find(mychan->chan, myuser->user);
  if (!cu)
    return FALSE;

  if (CMODE_VOICE & cu->modes)
    return FALSE;

  if (is_xop(mychan, myuser, CA_VOP))
    return TRUE;

  return FALSE;
}

boolean_t is_sra(myuser_t *myuser)
{
  if (!myuser)
    return FALSE;

  if (myuser->sra)
    return TRUE;

  return FALSE;
}

boolean_t is_ircop(user_t *user)
{
  if (UF_IRCOP & user->flags)
    return TRUE;

  return FALSE;
}

boolean_t is_admin(user_t *user)
{
  if (UF_ADMIN & user->flags)
    return TRUE;

  return FALSE;
}
