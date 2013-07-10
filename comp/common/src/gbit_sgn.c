
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <gbit_sgn.h>

void gbit_sgn_set(char *value,int pos)
{
    value[pos/8] |= (0x80 >> (pos%8) );
}

void gbit_sgn_reset(char *value,int pos)
{
    value[pos/8] &= ~(0x80 >> (pos%8) );
}

/* End of glog.c */
