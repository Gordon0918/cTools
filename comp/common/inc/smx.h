
#ifndef _SMX_H_
#define _SMX_H_

#ifdef cplusplus
extern "C" {
#endif



#ifndef ALPHABET_SIZE
#define ALPHABET_SIZE   256
#endif

#define SMX_FAIL_STATE   -1


/*
*
*/
typedef struct _smx_pattern
{
    struct  _smx_pattern    *next;

    unsigned char           *patrn;
    int                     nlen;
    int                     nid;
    void                    *udata;
 }SMX_PATTERN;

typedef struct _smx_state
{
    /* Next state - based on input character */
    int                     NextState[ ALPHABET_SIZE ];

    /* List of patterns that end here, if any */
    SMX_PATTERN             *MatchList;
}SMX_STATE;

/*
*   Simple State Machine Struct - one per group of pattterns
*/
typedef struct {

    int                     smxMaxStates;
    int                     smxNumStates;

    int                     nMaxPats;
    int                     nNumPats;

    int                     nPatternLen;
    int                     nocase;

    unsigned char           *smxPatternBegin;
    unsigned char           *smxPatStBegin;

    SMX_STATE               *smxStateTable;
}SMX_STRUCT;

#define     MATCH_GOON      0
#define     MATCH_OVER      1

typedef int (*MatchCall)(SMX_PATTERN *, void *);

extern SMX_STRUCT* smxBuild(int nMaxSNum, int nMaxPatLen, int nocase);
extern int smxDestroy(SMX_STRUCT *pSmx);
extern SMX_PATTERN *smxNewPattern(SMX_STRUCT *pSmx, unsigned char *pPattern, int nLen, int nId, void *pUdata);
extern int smxAddStates(SMX_STRUCT *pSmx, SMX_PATTERN *pPat);
extern int smxAddPattern(SMX_STRUCT *pSmx, unsigned char *pPattern, int nLen, int nId, void *pUdata);
extern int smxMatch(SMX_STRUCT *pSmx, unsigned char *pSrc, int nlen, MatchCall pfMC, void *pArg);
extern int smxMatchR(SMX_STRUCT *pSmx, unsigned char *pSrc, int nlen, MatchCall pfMC, void *pArg);

#ifdef cplusplus
}
#endif

#endif

