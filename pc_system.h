/////////////////////////////////////////////////////////////////////////
// $Id: pc_system.h,v 1.15 2002/10/02 05:16:01 kevinlawton Exp $
/////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2002  MandrakeSoft S.A.
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




#define BX_MAX_TIMERS 16
#define BX_NULL_TIMER_HANDLE 10000 /* set uninitialized timer handles to this */


#if BX_SHOW_IPS
extern unsigned long ips_count;
#endif


typedef void (*bx_timer_handler_t)(void *);


extern class bx_pc_system_c bx_pc_system;

#ifdef PROVIDE_M_IPS
extern double m_ips;
#endif

class bx_pc_system_c : private logfunctions {
private:

  // ===============================
  // Timer oriented private features
  // ===============================

  struct {
    Bit64u  period;     // Timer periodocity in cpu ticks.
    Bit64u  remaining;  // Remaining cpu ticks until current period elapses.
    Boolean active;     // 0=inactive, 1=active.
    Boolean continuous; // 0=one-shot timer, 1=continuous periodicity.
    Boolean triggered;  // 0=timer was just triggered by recent number of
                        //   elapsed cpu ticks.  It is possible for more
                        //   than one timer to fire on the same ticks
                        //   boundary, thus there is one flag for each
                        //   possible timer here.
    bx_timer_handler_t funct;  // A callback function for when the
                               //   timer fires.
    void *this_ptr;            // The this-> pointer for C++ callbacks
                               //   has to be stored as well.
    } timer[BX_MAX_TIMERS];

  unsigned   num_timers;  // Number of currently allocated timers.
  Bit64u     num_cpu_ticks_in_period; // Num cpu ticks in current period.
  Bit64u     num_cpu_ticks_left; // Num ticks remaining before current period
                                 //   elapses.  Always <= num_cpu_ticks_in_period.
  //
  // The following 4 appear to be a giant hack.  I'm going to look into
  // cleaning this up.  (KPL)
  //
  Bit64u     counter;
  int        counter_timer_index;
  static const Bit64u COUNTER_INTERVAL;
  static void counter_timer_handler(void* this_ptr);

  // When any kind of event occurs, including the registration of a
  // new timer, this convenience function is used to expire the number
  // of ticks which have occurred thus far in the current interval
  // for each of the active timers.
  void       expire_ticks(void);

#if !defined(PROVIDE_M_IPS)
  // This is the emulator speed, as measured in millions of
  // x86 instructions per second that it can emulate on some hypothetically
  // nomimal workload.
  double     m_ips; // Millions of Instructions Per Second
#endif

  // This handler is called when the function which increment the clock
  // ticks finds that an event has occurred.
  void   timer_handler(void);

public:

  // ==============================
  // Timer oriented public features
  // ==============================

  void   init_ips(Bit32u ips);
  int    register_timer( void *this_ptr, bx_timer_handler_t, Bit32u useconds,
                         Boolean continuous, Boolean active, const char *id);
  void   start_timers(void);
  void   activate_timer( unsigned timer_index, Bit32u useconds,
                         Boolean continuous );
  void   deactivate_timer( unsigned timer_index );
  static BX_CPP_INLINE void tick1(void) {
#if BX_SHOW_IPS
  {
  extern unsigned long ips_count;
  ips_count++;
  }
#endif
    if (--bx_pc_system.num_cpu_ticks_left == 0) {
      bx_pc_system.timer_handler();
      }
    }
  static BX_CPP_INLINE void tickn(Bit64u n) {
#if BX_SHOW_IPS
  {
  extern unsigned long ips_count;
  ips_count += n;
  }
#endif
    if (bx_pc_system.num_cpu_ticks_left > n) {
      bx_pc_system.num_cpu_ticks_left -= n;
      return;
      }
    while (n >= bx_pc_system.num_cpu_ticks_left) {
      n -= bx_pc_system.num_cpu_ticks_left;
      bx_pc_system.num_cpu_ticks_left = 0;
      bx_pc_system.timer_handler();
      }
    }

  int register_timer_ticks(void* this_ptr, bx_timer_handler_t, Bit64u ticks, Boolean continuous, Boolean active, const char *id);
  void activate_timer_ticks(unsigned index, Bit64u instructions, Boolean continuous);
  Bit64u time_usec();
  Bit64u time_ticks();

  static BX_CPP_INLINE Bit64u  getNumCpuTicksLeftNextEvent(void) {
    return bx_pc_system.num_cpu_ticks_left;
    }
#if BX_DEBUGGER
  static void timebp_handler(void* this_ptr);
#endif


  // ===========================
  // Non-timer oriented features
  // ===========================

  Boolean HRQ;     // Hold Request
  //Boolean INTR;    // Interrupt


    // Address line 20 control:
    //   1 = enabled: extended memory is accessible
    //   0 = disabled: A20 address line is forced low to simulate
    //       an 8088 address map
  Boolean enable_a20;

    // start out masking physical memory addresses to:
    //   8086:      20 bits
    //    286:      24 bits
    //    386:      32 bits
    // when A20 line is disabled, mask physical memory addresses to:
    //    286:      20 bits
    //    386:      20 bits
    //
  Bit32u  a20_mask;

  void set_HRQ(Boolean val);  // set the Hold ReQuest line
  void set_INTR(Boolean value); // set the INTR line to value

  int IntEnabled( void );
  int InterruptSignal( PCS_OP operation );
  int ResetSignal( PCS_OP operation );
  Bit8u  IAC(void);

  bx_pc_system_c(void);

  Bit32u  inp(Bit16u addr, unsigned io_len);
  void    outp(Bit16u addr, Bit32u value, unsigned io_len);
  void    set_enable_a20(Bit8u value);
  Boolean get_enable_a20(void);
  void    exit(void);

  };
