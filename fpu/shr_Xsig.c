/*---------------------------------------------------------------------------+
 |  shr_Xsig.S                                                               |
 |  $Id: shr_Xsig.c,v 1.2.34.1 2004/04/09 12:29:50 sshwarts Exp $
 |                                                                           |
 | 12 byte right shift function                                              |
 |                                                                           |
 | Copyright (C) 1992,1994,1995                                              |
 |                       W. Metzenthen, 22 Parker St, Ormond, Vic 3163,      |
 |                       Australia.  E-mail billm@jacobi.maths.monash.edu.au |
 |                                                                           |
 |                                                                           |
 |   Extended shift right function.                                          |
 |   Fastest for small shifts.                                               |
 |   Shifts the 12 byte quantity pointed to by the first arg (arg)           |
 |   right by the number of bits specified by the second arg (nr).           |
 |                                                                           |
 +---------------------------------------------------------------------------*/

#include "fpu_emu.h"
#include "poly.h"

void shr_Xsig(Xsig *arg, const int nr)
{
  int n = nr;

  while ( n >= 32 )
    {
      arg->lsw = arg->midw;
      arg->midw = arg->msw;
      arg->msw = 0;
      n -= 32;
    }

  if ( n <= 0 )
    return;

  arg->lsw = (arg->lsw >> n) | (arg->midw << (32-n));
  arg->midw = (arg->midw >> n) | (arg->msw << (32-n));
  arg->msw >>= n;
}
