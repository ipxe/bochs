/////////////////////////////////////////////////////////////////////////
// $Id: pit_wrap.h,v 1.16.6.3 2003/03/28 09:26:09 slechta Exp $
/////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2001  MandrakeSoft S.A.
//
//    MandrakeSoft S.A.
//    43, rue d'Aboukir
//    75002 Paris - France
//    http://www.linux-mandrake.com/
//    http://www.mandrakesoft.com/
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

#ifndef _BX_PIT_WRAP_H
#define _BX_PIT_WRAP_H

#include "bochs.h"

#if BX_USE_NEW_PIT

#include "pit82c54.h"

#if BX_USE_PIT_SMF
#  define BX_PIT_SMF  static
#  define BX_PIT_THIS bx_pit.
#else
#  define BX_PIT_SMF
#  define BX_PIT_THIS this->
#endif

#ifdef OUT
#  undef OUT
#endif

class bx_pit_c : public logfunctions {
public:
  bx_pit_c( void );
  ~bx_pit_c( void );
  BX_PIT_SMF int init( void );
  BX_PIT_SMF void register_state(bx_param_c *list_p);
  BX_PIT_SMF void reset( unsigned type);
  BX_PIT_SMF bx_bool periodic( Bit32u   usec_delta );

  BX_PIT_SMF int SaveState( class state_file *fd );
  BX_PIT_SMF int LoadState( class state_file *fd );

private:

  static Bit32u read_handler(void *this_ptr, Bit32u address, unsigned io_len);
  static void   write_handler(void *this_ptr, Bit32u address, Bit32u value, unsigned io_len);
#if !BX_USE_PIT_SMF
  Bit32u   read( Bit32u   addr, unsigned int len );
  void write( Bit32u   addr, Bit32u   Value, unsigned int len );
#endif

  struct s_type {
    pit_82C54 timer;
    Bit8u   speaker_data_on;
    bx_bool refresh_clock_div2;
    int  timer_handle[3];
    Bit64u last_usec;
    Bit32u last_next_event_time;
    Bit64u total_ticks;
    Bit64u usec_per_second;
    Bit64u ticks_per_second;
    Bit64u total_sec;
    Bit64u last_time;
    Bit64u last_sec_usec;
    Bit64u max_ticks;
    Bit64u stored_delta;
    Bit64u total_usec;
    Bit64u use_realtime;
    Bit64u em_last_realtime;
    Bit64u last_realtime_delta;
    Bit64u last_realtime_ticks;
    } s;

  static void timer_handler(void *this_ptr);
  BX_PIT_SMF void handle_timer();

  BX_PIT_SMF void  write_count_reg( Bit8u   value, unsigned timerid );
  BX_PIT_SMF Bit8u read_counter( unsigned timerid );
  BX_PIT_SMF void  latch( unsigned timerid );
  BX_PIT_SMF void  set_GATE(unsigned pit_id, unsigned value);
  BX_PIT_SMF void  start(unsigned timerid);

  BX_PIT_SMF void  second_update_data(void);
};

extern bx_pit_c bx_pit;

#endif  // #if BX_USE_NEW_PIT
#endif  // #ifndef _BX_PIT_WRAP_H
