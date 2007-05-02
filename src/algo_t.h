/* algo_t.h */
/* See Copyright Notice in the file LICENSE */

#ifndef ALGO_T_H
#define ALGO_T_H

/* REX_API can be overridden from the command line or Makefile */
#ifndef REX_API
#  define REX_API LUALIB_API
#endif

/* Special values for maxmatch in gsub. They all must be negative. */
#define GSUB_UNLIMITED   -1
#define GSUB_CONDITIONAL -2

typedef struct {            /* compile arguments */
  const char * pattern;
  size_t       patlen;
  int          cflags;
  const char * locale;
  const unsigned char * tables;
  int          tablespos;
} TArgComp;

typedef struct {            /* exec arguments */
  const char * text;
  size_t       textlen;
  int          startoffset;
  int          eflags;
  int          funcpos;
  int          maxmatch;
  int          funcpos2;      /* used with gsub */
  int          reptype;       /* used with gsub */
  size_t       ovecsize;      /* used with dfa_exec */
  size_t       wscount;       /* used with dfa_exec */
} TArgExec;

#endif /* ALGO_T_H */

