
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <smx.h>

#define MEMERRLOG(p,s) if(!p){fprintf(stderr, "SMX-No Memory: %s!\n", s);}

typedef struct chartable
{
    unsigned char    lowLetter;
    unsigned char    upLetter;
}CHAR_TABLE;

CHAR_TABLE  letterTable[] = {
    {'a', 'A'}, {'b', 'B'}, {'c', 'C'}, {'d', 'D'}, {'e', 'E'}, {'f', 'F'}, {'g', 'G'},
    {'h', 'H'}, {'i', 'I'}, {'j', 'J'}, {'k', 'K'}, {'l', 'L'}, {'m', 'M'}, {'n', 'N'},
    {'o', 'O'}, {'p', 'P'}, {'q', 'Q'}, {'r', 'R'}, {'s', 'S'}, {'t', 'T'},
    {'u', 'U'}, {'v', 'V'}, {'w', 'W'}, {'x', 'X'}, {'y', 'Y'}, {'z', 'Z'},
};

#if 0
SMX_STRUCT* smxNew()
{
    SMX_STRUCT *pNew = NULL;

    if ((pNew = malloc(sizeof(SMX_STRUCT))) == NULL)
    {
        /*to log*/
        return NULL;
    }

    memset(pNew, 0, sizeof(SMX_STRUCT));

    return pNew;
}
#endif

SMX_STRUCT* smxBuild(int nPatternNum, int nPatternLen, int nocase)
{
    unsigned char   *pInitMem = NULL, *pCurMem = NULL;
    int             nInitSize = 0;
    SMX_STRUCT      *pSmx     = NULL;
    int             i, k;

    nPatternLen = ((nPatternLen + 3) >> 2) << 2;  /* align 4 */
    nInitSize += sizeof(SMX_STRUCT);
    nInitSize += nPatternNum * nPatternLen * sizeof(SMX_STATE);
    nInitSize += nPatternNum * sizeof(SMX_PATTERN);
    nInitSize += nPatternNum * (nPatternLen + 1);


    if ((pInitMem = (unsigned char *)malloc(nInitSize)) == NULL)
    {
        MEMERRLOG(pInitMem, "smxBuild");
        return NULL;
    }

    memset(pInitMem, 0, nInitSize);
    pCurMem = pInitMem;
    
    pSmx     = (SMX_STRUCT *)pCurMem;
    pCurMem += sizeof(SMX_STRUCT);

    
    pSmx->smxMaxStates      = nPatternNum * nPatternLen;
    pSmx->smxNumStates      = 0;

    pSmx->smxStateTable     = (SMX_STATE *)pCurMem;
    pCurMem += nPatternNum * nPatternLen * sizeof(SMX_STATE);

    /* Initialize all States NextStates to FAILED */
    for (k = 0; k < pSmx->smxMaxStates; k++)
    {
        for (i = 0; i < ALPHABET_SIZE; i++)
        {
            pSmx->smxStateTable[k].NextState[i] = SMX_FAIL_STATE;
        }
    }

    pSmx->nMaxPats          = nPatternNum;
    pSmx->nNumPats          = 0;
    pSmx->smxPatStBegin     = pCurMem;
    pCurMem += nPatternNum * sizeof(SMX_PATTERN);

    pSmx->nPatternLen       = nPatternLen;
    pSmx->nocase            = nocase;
    pSmx->smxPatternBegin   = pCurMem;

    return pSmx;
}


int smxDestroy(SMX_STRUCT *pSmx)
{
    if (pSmx == NULL)
        return -1;

    free(pSmx);
    return 0;
}

SMX_PATTERN *smxNewPattern(SMX_STRUCT *pSmx, unsigned char *pPattern, int nLen, int nId, void *pUdata)
{
    SMX_PATTERN *pNewPat = NULL;
    
    if (pSmx == NULL || pPattern == NULL || nLen >= pSmx->nPatternLen)
    {
        return NULL;
    }

    if (pSmx->nNumPats >= pSmx->nMaxPats)
    {
        return NULL;
    }

    pNewPat = (SMX_PATTERN *)(pSmx->smxPatStBegin + (pSmx->nNumPats * sizeof(SMX_PATTERN)));
    pNewPat->patrn = pSmx->smxPatternBegin + pSmx->nNumPats * pSmx->nPatternLen;

    memcpy(pNewPat->patrn, pPattern, nLen);
    pNewPat->nlen   = nLen;
    pNewPat->nid    = nId;
    pNewPat->udata  = pUdata;

    pSmx->nNumPats ++;
    return pNewPat;
}


int smxAddStates(SMX_STRUCT *pSmx, SMX_PATTERN *pPat)
{
    unsigned char *pattern;
    int state = 0, next, n;
    
    n = pPat->nlen;
    pattern = pPat->patrn;

    /*
     *  Match up pattern with existing states
     */
    for (; n > 0; pattern++, n--)
    {
        next = pSmx->smxStateTable[state].NextState[*pattern];
        if (next == SMX_FAIL_STATE)
            break;
        state = next;
    }

    /*
      *   Add new states for the rest of the pattern bytes, 1 state per byte
      */
    for (; n > 0; pattern++, n--)
    {
        pSmx->smxNumStates++;
        pSmx->smxStateTable[state].NextState[*pattern] = pSmx->smxNumStates;
        if (pSmx->nocase)
        {
            if ('a' <= *pattern && *pattern <= 'z')
            {
                pSmx->smxStateTable[state].NextState[letterTable[(*pattern - 'a')].upLetter]
                    = pSmx->smxNumStates;
            }
            else if ('A' <= *pattern && *pattern <= 'Z')
            {
                pSmx->smxStateTable[state].NextState[letterTable[(*pattern - 'A')].lowLetter]
                    = pSmx->smxNumStates;   
            }
            else
            {
                /*do nothing*/
            }
        }
        
        state = pSmx->smxNumStates;
    }


    pPat->next = pSmx->smxStateTable[state].MatchList;
    pSmx->smxStateTable[state].MatchList = pPat;

    return 0;
}


int smxAddPattern(SMX_STRUCT *pSmx, unsigned char *pPattern, int nLen, int nId, void *pUdata)
{
    SMX_PATTERN     *pPatSt = NULL;

    if ((pPatSt = smxNewPattern(pSmx, pPattern, nLen, nId, pUdata)) == NULL)
    {
        return -1;
    }

    return smxAddStates(pSmx, pPatSt);
}

#if 0
int smxCompile(SMX_STRUCT *pSmx, int nocase)
{
    int i, k;
    SMX_PATTERN *plist;

    /* Count number of states */
    pSmx->smxMaxStates = 1;
    for (plist = pSmx->smxPatternList; plist != NULL; plist = plist->next)
    {
        pSmx->acsmMaxStates += plist->nlen;
    }
    pSmx->smxStateTable = 
        (SMX_STATE *) AC_MALLOC (sizeof (SMX_STATE) * pSmx->smxMaxStates);

    MEMERRLOG(pSmx->smxMaxStates, "smxCompile");
    memset(pSmx->smxStateTable, 0, sizeof (SMX_STATE) * pSmx->smxMaxStates);

    /* Initialize state zero as a branch */
    pSmx->smxNumStates = 0;

    /* Initialize all States NextStates to FAILED */
    for (k = 0; k < pSmx->smxMaxStates; k++)
    {
        for (i = 0; i < ALPHABET_SIZE; i++)
        {
            pSmx->smxStateTable[k].NextState[i] = SMX_FAIL_STATE;
        }
    }


    /* Add each Pattern to the State Table */
    plist = pSmx->smxPatternList;
    while (plist != NULL)
    {
        pSmx->smxPatternList = plist->next;
        smxAddStates(pSmx, plist, nocase);
    }

    return 0;   
}
#endif

int smxMatch(SMX_STRUCT *pSmx, unsigned char *pSrc, int nlen, MatchCall pfMC, void *pArg)
{
    int             state = 0;
    unsigned char   *Tend = pSrc + nlen;
    SMX_STATE       *StateTable = pSmx->smxStateTable;
    int             nfound = 0;
    unsigned char   *T = pSrc;


    for (; T < Tend; T++)
    {
        state = StateTable[state].NextState[*T];

        if (SMX_FAIL_STATE == state)
        {
            return nfound;
        }

        if( StateTable[state].MatchList != NULL )
        {
            SMX_PATTERN *mlist = StateTable[state].MatchList;
            nfound++;

            if (pfMC && (pfMC(mlist, pArg) == MATCH_OVER))
            {
                return nfound;
            }
        }
    }
    
    return nfound;
}

int smxMatchR(SMX_STRUCT *pSmx, unsigned char *pSrc, int nlen, MatchCall pfMC, void *pArg)
{
    int             state = 0;
    unsigned char   *Tend = pSrc;
    SMX_STATE       *StateTable = pSmx->smxStateTable;
    int             nfound = 0;
    unsigned char   *T = pSrc + nlen - 1;


    for (; T >= Tend; T--)
    {
        state = StateTable[state].NextState[*T];

        if (SMX_FAIL_STATE == state)
        {
            return nfound;
        }

        if( StateTable[state].MatchList != NULL )
        {
            SMX_PATTERN *mlist = StateTable[state].MatchList;
            nfound++;

            if (pfMC && (pfMC(mlist, pArg) == MATCH_OVER))
            {
                return nfound;
            }
        }
    }
    
    return nfound;
}



