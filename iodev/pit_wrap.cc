/////////////////////////////////////////////////////////////////////////
// $Id: pit_wrap.cc,v 1.29.2.2 2002-10-10 13:10:56 cbothamy Exp $
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


//Realtime Algorithm (with gettimeofday)
//  HAVE:
//    Real number of usec.
//    Emulated number of usec.
//  WANT:
//    Number of ticks to use.
//    Number of emulated usec to wait until next try.
//
//  ticks=number of ticks needed to match total real usec.
//  if(desired ticks > max ticks for elapsed real time)
//     ticks = max ticks for elapsed real time.
//  if(desired ticks > max ticks for elapsed emulated usec)
//     ticks = max ticks for emulated usec.
//  next wait ticks = number of ticks until next event.
//  next wait real usec = (current ticks + next wait ticks) * usec per ticks
//  next wait emulated usec = next wait real usec * emulated usec / real usec
//  if(next wait emulated usec < minimum emulated usec for next wait ticks)
//     next wait emulated usec = minimum emulated usec for next wait ticks.
//  if(next wait emulated usec > max emulated usec wait)
//     next wait emulated usec = max emulated usec wait.
//
//  How to calculate elapsed real time:
//    store an unused time value whenever no ticks are used in a given time.
//    add this to the current elapsed time.
//  How to calculate elapsed emulated time:
//    same as above.
//  Above can be done by not updating last_usec and last_sec.
//
//  How to calculate emulated usec/real usec:
//    Each time there are actual ticks:
//      Alpha_product(old emulated usec, emulated usec);
//      Alpha_product(old real usec, real usec);
//    Divide resulting values.


#include "bochs.h"

#if BX_USE_NEW_PIT

#include "pit_wrap.h"
#define LOG_THIS bx_pit.

bx_pit_c bx_pit;
#if BX_USE_PIT_SMF
#define this (&bx_pit)
#endif

#ifdef OUT
#  undef OUT
#endif

#define DEBUG_REALTIME_WITH_PRINTF 0
#define DEBUG_GETTIMEOFDAY_WITH_PRINTF 1


#define TIME_DIVIDER (1)
#define TIME_MULTIPLIER (1)
#define TIME_HEADSTART (1)
#define MIN_USEC_PER_SECOND (150000)
//USEC_ALPHA is multiplier for the past.
//USEC_ALPHA_B is 1-USEC_ALPHA, or multiplier for the present.
#define USEC_ALPHA ((double)(.8))
#define USEC_ALPHA_B ((double)(((double)1)-USEC_ALPHA))
#define USEC_ALPHA2 ((double)(.5))
#define USEC_ALPHA2_B ((double)(((double)1)-USEC_ALPHA2))
#define ALPHA_LOWER(old,new) ((Bit64u)((old<new)?((USEC_ALPHA*((double)(old)))+(USEC_ALPHA_B*((double)new))):((USEC_ALPHA2*((double)old))+(USEC_ALPHA2_B*((double)new)))))
#define MIN_MULT (0.9)
#define MIN_MULT_FLOOR (0.75)
#define MAX_MULT (1.25)
#define MAX_MULT_CEILING (1.5)
#define BX_MAX(a,b) ( ((a)>(b))?(a):(b) )
#define BX_MIN(a,b) ( ((a)>(b))?(b):(a) )

//How many timer ticks per usecond.
//1.193181MHz Clock
//1193/1000 Ticks Per usecond.
#define TICKS_PER_SECOND (1193181)
#define USEC_PER_SECOND (1000000)
#define TIME_MULT 1.193
#define REAL_TICKS_TO_USEC(a) ( ((a)*USEC_PER_SECOND)/TICKS_PER_SECOND )
#define REAL_USEC_TO_TICKS(a) ( ((a)*TICKS_PER_SECOND)/USEC_PER_SECOND )
#define AHEAD_CEILING ((Bit64u)(TICKS_PER_SECOND*2))
#define TICKS_TO_USEC(a) ((BX_USE_REALTIME_PIT)?( ((a)*BX_PIT_THIS s.usec_per_second)/BX_PIT_THIS s.ticks_per_second ):( ((a)*USEC_PER_SECOND)/TICKS_PER_SECOND ))
#define USEC_TO_TICKS(a) ((BX_USE_REALTIME_PIT)?( ((a)*BX_PIT_THIS s.ticks_per_second)/BX_PIT_THIS s.usec_per_second ):( ((a)*TICKS_PER_SECOND)/USEC_PER_SECOND ))

bx_pit_c::bx_pit_c( void )
{
  put("PIT");
  settype(PITLOG);
  s.speaker_data_on=0;

  /* 8254 PIT (Programmable Interval Timer) */

  BX_PIT_THIS s.timer_handle[1] = BX_NULL_TIMER_HANDLE;
  BX_PIT_THIS s.timer_handle[2] = BX_NULL_TIMER_HANDLE;
  BX_PIT_THIS s.timer_handle[0] = BX_NULL_TIMER_HANDLE;
}

bx_pit_c::~bx_pit_c( void )
{
}

  int
bx_pit_c::init( void )
{
  bx_devices.register_irq(0, "8254 PIT");
  bx_devices.register_io_read_handler(this, read_handler, 0x0040, "8254 PIT");
  bx_devices.register_io_read_handler(this, read_handler, 0x0041, "8254 PIT");
  bx_devices.register_io_read_handler(this, read_handler, 0x0042, "8254 PIT");
  bx_devices.register_io_read_handler(this, read_handler, 0x0043, "8254 PIT");
  bx_devices.register_io_read_handler(this, read_handler, 0x0061, "8254 PIT");

  bx_devices.register_io_write_handler(this, write_handler, 0x0040, "8254 PIT");
  bx_devices.register_io_write_handler(this, write_handler, 0x0041, "8254 PIT");
  bx_devices.register_io_write_handler(this, write_handler, 0x0042, "8254 PIT");
  bx_devices.register_io_write_handler(this, write_handler, 0x0043, "8254 PIT");
  bx_devices.register_io_write_handler(this, write_handler, 0x0061, "8254 PIT");

  BX_DEBUG(("pit: starting init"));

  BX_PIT_THIS s.speaker_data_on = 0;
  BX_PIT_THIS s.refresh_clock_div2 = 0;

  BX_PIT_THIS s.timer.init();

  BX_PIT_THIS s.timer_handle[0] = bx_pc_system.register_timer(this, timer_handler, (unsigned) 100 , 1, 1, "pit_wrap");
  BX_DEBUG(("pit: RESETting timer."));
  bx_pc_system.deactivate_timer(BX_PIT_THIS s.timer_handle[0]);
  BX_DEBUG(("deactivated timer."));
  if(BX_PIT_THIS s.timer.get_next_event_time()) {
    bx_pc_system.activate_timer(BX_PIT_THIS s.timer_handle[0], 
				BX_MAX(1,TICKS_TO_USEC(BX_PIT_THIS s.timer.get_next_event_time())),
				0);
    BX_DEBUG(("activated timer."));
  }
  BX_PIT_THIS s.last_next_event_time = BX_PIT_THIS s.timer.get_next_event_time();
  BX_PIT_THIS s.last_usec=bx_pc_system.time_usec();

  BX_PIT_THIS s.total_ticks=0;

  if (BX_USE_REALTIME_PIT) {
    BX_PIT_THIS s.usec_per_second=USEC_PER_SECOND;
    BX_PIT_THIS s.ticks_per_second=TICKS_PER_SECOND;
    BX_PIT_THIS s.total_sec=0;
    BX_PIT_THIS s.stored_delta=0;
#if BX_HAVE_REALTIME_USEC
    BX_PIT_THIS s.last_time=((bx_get_realtime64_usec()*(Bit64u)TIME_MULTIPLIER/(Bit64u)TIME_DIVIDER))+(Bit64u)TIME_HEADSTART*(Bit64u)USEC_PER_SECOND;
#else
    BX_PIT_THIS s.last_time=((time(NULL)*TIME_MULTIPLIER/TIME_DIVIDER)+TIME_HEADSTART)*USEC_PER_SECOND;
#endif
    BX_PIT_THIS s.max_ticks = AHEAD_CEILING;
  } else {
    BX_PIT_THIS s.total_usec=0;
  }

  BX_DEBUG(("pit: finished init"));

  BX_DEBUG(("s.last_usec=%d",BX_PIT_THIS s.last_usec));
  BX_DEBUG(("s.timer_id=%d",BX_PIT_THIS s.timer_handle[0]));
  BX_DEBUG(("s.timer.get_next_event_time=%d",BX_PIT_THIS s.timer.get_next_event_time()));
  BX_DEBUG(("s.last_next_event_time=%d",BX_PIT_THIS s.last_next_event_time));

  return(1);
}

  void
bx_pit_c::reset(unsigned type)
{
}

void
bx_pit_c::timer_handler(void *this_ptr) {
  bx_pit_c * class_ptr = (bx_pit_c *) this_ptr;

  class_ptr->handle_timer();
}

void
bx_pit_c::handle_timer() {
  Bit64u time_passed = bx_pc_system.time_usec()-BX_PIT_THIS s.last_usec;
  Bit32u time_passed32 = time_passed;

  BX_DEBUG(("pit: entering timer handler"));

  if(time_passed32) {
    periodic(time_passed32);
  }
  BX_PIT_THIS s.last_usec=BX_PIT_THIS s.last_usec + time_passed;
  if(time_passed ||
     (BX_PIT_THIS s.last_next_event_time
      != BX_PIT_THIS s.timer.get_next_event_time())
     ) {
    BX_DEBUG(("pit: RESETting timer."));
    bx_pc_system.deactivate_timer(BX_PIT_THIS s.timer_handle[0]);
    BX_DEBUG(("deactivated timer."));
    if(BX_PIT_THIS s.timer.get_next_event_time()) {
      bx_pc_system.activate_timer(BX_PIT_THIS s.timer_handle[0], 
				  BX_MAX(1,TICKS_TO_USEC(BX_PIT_THIS s.timer.get_next_event_time())),
				  0);
      BX_DEBUG(("activated timer."));
    }
    BX_PIT_THIS s.last_next_event_time = BX_PIT_THIS s.timer.get_next_event_time();
  }
  BX_DEBUG(("s.last_usec=%d",BX_PIT_THIS s.last_usec));
  BX_DEBUG(("s.timer_id=%d",BX_PIT_THIS s.timer_handle[0]));
  BX_DEBUG(("s.timer.get_next_event_time=%x",BX_PIT_THIS s.timer.get_next_event_time()));
  BX_DEBUG(("s.last_next_event_time=%d",BX_PIT_THIS s.last_next_event_time));
}


  // static IO port read callback handler
  // redirects to non-static class handler to avoid virtual functions

  Bit32u
bx_pit_c::read_handler(void *this_ptr, Bit32u address, unsigned io_len)
{
#if !BX_USE_PIT_SMF
  bx_pit_c *class_ptr = (bx_pit_c *) this_ptr;

  return( class_ptr->read(address, io_len) );
}


  Bit32u
bx_pit_c::read( Bit32u   address, unsigned int io_len )
{
#else
  UNUSED(this_ptr);
#endif  // !BX_USE_PIT_SMF
  BX_DEBUG(("pit: entering read handler"));

  handle_timer();

  if (io_len > 1)
    BX_PANIC(("pit: io read from port %04x, len=%u", (unsigned) address,
             (unsigned) io_len));

  if (bx_dbg.pit)
    BX_INFO(("pit: io read from port %04x", (unsigned) address));

  switch (address) {

    case 0x40: /* timer 0 - system ticks */
      return(BX_PIT_THIS s.timer.read(0));
      break;
    case 0x41: /* timer 1 read */
      return(BX_PIT_THIS s.timer.read(1));
      break;
    case 0x42: /* timer 2 read */
      return(BX_PIT_THIS s.timer.read(2));
      break;
    case 0x43: /* timer 1 read */
      return(BX_PIT_THIS s.timer.read(3));
      break;

    case 0x61:
      /* AT, port 61h */
      BX_PIT_THIS s.refresh_clock_div2 = !BX_PIT_THIS s.refresh_clock_div2;
      return( (BX_PIT_THIS s.timer.read_OUT(2)<<5) |
              (BX_PIT_THIS s.refresh_clock_div2<<4) |
              (BX_PIT_THIS s.speaker_data_on<<1) |
              (BX_PIT_THIS s.timer.read_GATE(2)?1:0) );
      break;

    default:
      BX_PANIC(("pit: unsupported io read from port %04x", address));
  }
  return(0); /* keep compiler happy */
}


  // static IO port write callback handler
  // redirects to non-static class handler to avoid virtual functions

  void
bx_pit_c::write_handler(void *this_ptr, Bit32u address, Bit32u dvalue, unsigned io_len)
{
#if !BX_USE_PIT_SMF
  bx_pit_c *class_ptr = (bx_pit_c *) this_ptr;

  class_ptr->write(address, dvalue, io_len);
}

  void
bx_pit_c::write( Bit32u   address, Bit32u   dvalue,
                unsigned int io_len )
{
#else
  UNUSED(this_ptr);
#endif  // !BX_USE_PIT_SMF
  Bit8u   value;
  Bit64u time_passed = bx_pc_system.time_usec()-BX_PIT_THIS s.last_usec;
  Bit32u time_passed32 = time_passed;

  BX_DEBUG(("pit: entering write handler"));

  if(time_passed32) {
    periodic(time_passed32);
  }
  BX_PIT_THIS s.last_usec=BX_PIT_THIS s.last_usec + time_passed;

  value = (Bit8u  ) dvalue;

  if (io_len > 1)
    BX_PANIC(("pit: io write to port %04x, len=%u", (unsigned) address,
             (unsigned) io_len));

  if (bx_dbg.pit)
    BX_INFO(("pit: write to port %04x = %02x",
      (unsigned) address, (unsigned) value));

  switch (address) {
    case 0x40: /* timer 0: write count register */
      BX_PIT_THIS s.timer.write(0,value);
      break;

    case 0x41: /* timer 1: write count register */
      BX_PIT_THIS s.timer.write( 1,value );
      break;

    case 0x42: /* timer 2: write count register */
      BX_PIT_THIS s.timer.write( 2,value );
      break;

    case 0x43: /* timer 0-2 mode control */
      BX_PIT_THIS s.timer.write( 3,value );
      break;

    case 0x61:
      BX_PIT_THIS s.speaker_data_on = (value >> 1) & 0x01;
/*??? only on AT+ */
      BX_PIT_THIS s.timer.set_GATE(2, value & 0x01);
#if BX_CPU_LEVEL < 2
      /* ??? XT: */
      bx_kbd_port61h_write(value);
#endif
      break;

    default:
      BX_PANIC(("pit: unsupported io write to port %04x = %02x",
        (unsigned) address, (unsigned) value));
  }

  if ((BX_PIT_THIS s.timer.read_OUT(0))==1) {
    BX_PIC_RAISE_IRQ(0);
  } else {
    BX_PIC_LOWER_IRQ(0);
  }

  if(time_passed ||
     (BX_PIT_THIS s.last_next_event_time
      != BX_PIT_THIS s.timer.get_next_event_time())
     ) {
    BX_DEBUG(("pit: RESETting timer."));
    bx_pc_system.deactivate_timer(BX_PIT_THIS s.timer_handle[0]);
    BX_DEBUG(("deactivated timer."));
    if(BX_PIT_THIS s.timer.get_next_event_time()) {
      bx_pc_system.activate_timer(BX_PIT_THIS s.timer_handle[0], 
				  BX_MAX(1,TICKS_TO_USEC(BX_PIT_THIS s.timer.get_next_event_time())),
				  0);
      BX_DEBUG(("activated timer."));
    }
    BX_PIT_THIS s.last_next_event_time = BX_PIT_THIS s.timer.get_next_event_time();
  }
  BX_DEBUG(("s.last_usec=%d",BX_PIT_THIS s.last_usec));
  BX_DEBUG(("s.timer_id=%d",BX_PIT_THIS s.timer_handle[0]));
  BX_DEBUG(("s.timer.get_next_event_time=%x",BX_PIT_THIS s.timer.get_next_event_time()));
  BX_DEBUG(("s.last_next_event_time=%d",BX_PIT_THIS s.last_next_event_time));

}




  int
bx_pit_c::SaveState( class state_file *fd )
{
  fd->write_check ("8254 start");
  fd->write (&BX_PIT_THIS s, sizeof (BX_PIT_THIS s));
  fd->write_check ("8254 end");
  return(0);
}


  int
bx_pit_c::LoadState( class state_file *fd )
{
  fd->read_check ("8254 start");
  fd->read (&BX_PIT_THIS s, sizeof (BX_PIT_THIS s));
  fd->read_check ("8254 end");
  return(0);
}


#if 0
  void
bx_kbd_port61h_write(Bit8u   value)
{
//  PcError("KBD_PORT61H_WRITE(): not implemented yet");
  UNUSED( value );
}
#endif


  Boolean
bx_pit_c::periodic( Bit32u   usec_delta )
{
  Boolean prev_timer0_out = BX_PIT_THIS s.timer.read_OUT(0);
  Boolean want_interrupt = 0;
  Bit32u ticks_delta = 0;

#ifdef BX_SCHEDULED_DIE_TIME
  if (bx_pc_system.time_ticks() > BX_SCHEDULED_DIE_TIME) {
    BX_ERROR (("ticks exceeded scheduled die time, quitting"));
    BX_EXIT (2);
  }
#endif

  if (BX_USE_REALTIME_PIT) {
#if BX_HAVE_REALTIME_USEC
    Bit64u real_time_delta = bx_get_realtime64_usec() - BX_PIT_THIS s.last_time;
    Bit64u real_time_total = real_time_delta + BX_PIT_THIS s.total_sec;
    Bit64u em_time_delta = (Bit64u)usec_delta + (Bit64u)BX_PIT_THIS s.stored_delta;
    BX_PIT_THIS s.ticks_per_second = TICKS_PER_SECOND;

    //Start out with the number of ticks we would like
    // to have to line up with real time.
    ticks_delta = REAL_USEC_TO_TICKS(real_time_total) - BX_PIT_THIS s.total_ticks;
    if(REAL_USEC_TO_TICKS(real_time_total) < BX_PIT_THIS s.total_ticks) {
      //This slows us down if we're already ahead.
      //  probably only an issue on startup, but it solves some problems.
      ticks_delta = 0;
    }
    if(ticks_delta > REAL_USEC_TO_TICKS((Bit64u)(MAX_MULT * real_time_delta))) {
      //This keeps us from going too fast in relation to real time.
      ticks_delta = REAL_USEC_TO_TICKS((Bit64u)(MAX_MULT * real_time_delta));
      BX_PIT_THIS s.ticks_per_second = (Bit64u)(MAX_MULT * TICKS_PER_SECOND);
    }
    if(ticks_delta > em_time_delta * TICKS_PER_SECOND / MIN_USEC_PER_SECOND) {
      //This keeps us from having too few instructions between ticks.
      ticks_delta = em_time_delta * TICKS_PER_SECOND / MIN_USEC_PER_SECOND;
    }
    if(ticks_delta > BX_PIT_THIS s.timer.get_next_event_time()) {
      //This keeps us from missing ticks.
      ticks_delta = BX_PIT_THIS s.timer.get_next_event_time();
    }

    if(ticks_delta) {
#if DEBUG_GETTIMEOFDAY_WITH_PRINTF
      if(((BX_PIT_THIS s.last_time + real_time_delta) / USEC_PER_SECOND) > (BX_PIT_THIS s.last_time / USEC_PER_SECOND)) {
	printf("useconds: %lld, expected ticks: %lld, ticks: %lld, diff: %lld\n",
	       (Bit64u) BX_PIT_THIS s.total_sec,
	       (Bit64u)REAL_USEC_TO_TICKS(BX_PIT_THIS s.total_sec),
	       (Bit64u)BX_PIT_THIS s.total_ticks,
	       (Bit64u)(REAL_USEC_TO_TICKS(BX_PIT_THIS s.total_sec) - BX_PIT_THIS s.total_ticks)
	       );
      }
#endif
      BX_PIT_THIS s.last_time += real_time_delta;
      BX_PIT_THIS s.total_sec += real_time_delta;
      BX_PIT_THIS s.last_sec_usec += em_time_delta;
      //    BX_PIT_THIS s.total_usec += em_time_delta;
      BX_PIT_THIS s.stored_delta = 0;
      BX_PIT_THIS s.total_ticks += ticks_delta;
    } else {
      BX_PIT_THIS s.stored_delta = em_time_delta;
    }

    Bit64u a,b;
    a=(BX_PIT_THIS s.usec_per_second);
    b=((Bit64u)1000000 * em_time_delta / real_time_delta);

    BX_PIT_THIS s.usec_per_second = ALPHA_LOWER(a,b);
#else
    ticks_delta=(Bit32u)(USEC_TO_TICKS(usec_delta));
    if((BX_PIT_THIS s.total_ticks + ticks_delta) < (BX_PIT_THIS s.max_ticks)) {
      BX_PIT_THIS s.total_ticks += ticks_delta;
    } else {
      if(BX_PIT_THIS s.total_ticks              >= (BX_PIT_THIS s.max_ticks)) {
	ticks_delta = 0;
      } else {
	ticks_delta =                              (BX_PIT_THIS s.max_ticks) - BX_PIT_THIS s.total_ticks;
	BX_PIT_THIS s.total_ticks += ticks_delta;
      }
    }
    second_update_data();
#endif
  } else {
    BX_PIT_THIS s.total_usec += usec_delta;
    ticks_delta=(Bit32u)((USEC_TO_TICKS((Bit64u)(BX_PIT_THIS s.total_usec)))-BX_PIT_THIS s.total_ticks);
    BX_PIT_THIS s.total_ticks += ticks_delta;

    while ((BX_PIT_THIS s.total_ticks >= 1193181) && (BX_PIT_THIS s.total_usec >= 1000000)) {
      BX_PIT_THIS s.total_ticks -= 1193181;
      BX_PIT_THIS s.total_usec  -= 1000000;
    }
  }

  while(ticks_delta>0) {
    Bit32u maxchange=BX_PIT_THIS s.timer.get_next_event_time();
    Bit32u timedelta=maxchange;
    if((maxchange==0) || (maxchange>ticks_delta)) {
      timedelta=ticks_delta;
    }
    BX_PIT_THIS s.timer.clock_all(timedelta);
    if ( (prev_timer0_out==0) ) {
      if ((BX_PIT_THIS s.timer.read_OUT(0))==1) {
	BX_PIC_RAISE_IRQ(0);
        prev_timer0_out=1;
      }
    } else {
      if ((BX_PIT_THIS s.timer.read_OUT(0))==0) {
	BX_PIC_LOWER_IRQ(0);
        prev_timer0_out=0;
      }
    }
    prev_timer0_out=BX_PIT_THIS s.timer.read_OUT(0);
    ticks_delta-=timedelta;
  }

  return(want_interrupt);
}


void
bx_pit_c::second_update_data(void) {
  Bit64u timediff;
#if BX_HAVE_REALTIME_USEC
  timediff=((bx_get_realtime64_usec()*(Bit64u)TIME_MULTIPLIER/(Bit64u)TIME_DIVIDER))-(Bit64u)BX_PIT_THIS s.last_time;
#else
  timediff=((time(NULL)*TIME_MULTIPLIER/TIME_DIVIDER)*USEC_PER_SECOND)-BX_PIT_THIS s.last_time;
#endif
  BX_PIT_THIS s.last_time += timediff;
  if(timediff) {
    Bit64s tickstemp;

    BX_PIT_THIS s.total_sec += timediff;

    BX_PIT_THIS s.max_ticks = BX_MIN( (((BX_PIT_THIS s.total_sec)*(Bit64u)(TICKS_PER_SECOND))/USEC_PER_SECOND) + AHEAD_CEILING , BX_PIT_THIS s.total_ticks + (Bit64u)(TICKS_PER_SECOND*MAX_MULT) );

#if DEBUG_REALTIME_WITH_PRINTF
    printf("timediff: %lld, total_sec: %lld, total_ticks: %lld\n",timediff, BX_PIT_THIS s.total_sec, BX_PIT_THIS s.total_ticks);
#endif

    tickstemp = 
      ((((BX_PIT_THIS s.total_sec)*TICKS_PER_SECOND)/USEC_PER_SECOND)-BX_PIT_THIS s.total_ticks)
       + TICKS_PER_SECOND;

//    while((BX_PIT_THIS s.total_sec >= 0) && (BX_PIT_THIS s.total_ticks >= TICKS_PER_SECOND)) {
//      BX_PIT_THIS s.total_sec -= 1;
//      BX_PIT_THIS s.total_ticks -= TICKS_PER_SECOND;
//    }

    if(tickstemp > (TICKS_PER_SECOND*MAX_MULT)) {
#if DEBUG_REALTIME_WITH_PRINTF
      if (tickstemp>(2*TICKS_PER_SECOND)) {
	printf("Running WAY too slow. tps:%lld\n",tickstemp);
      } else {
	printf("Running slow.         tps:%lld\n",tickstemp);
      }
#endif
      tickstemp = (Bit64u)(TICKS_PER_SECOND*MAX_MULT);
#if DEBUG_REALTIME_WITH_PRINTF
      printf("..................new tps:%lld\n",tickstemp);
#endif
    } else if(tickstemp < (TICKS_PER_SECOND*MIN_MULT)) {
#if DEBUG_REALTIME_WITH_PRINTF
      if(tickstemp<0) {
        printf("Running WAY too fast. tps:%lld\n",tickstemp);
      } else {
        printf("Running fast.         tps:%lld\n",tickstemp);
      }
#endif
      tickstemp = (Bit64u)(TICKS_PER_SECOND*MIN_MULT);
#if DEBUG_REALTIME_WITH_PRINTF
      printf("..................new tps:%lld\n",tickstemp);
#endif
    }

    BX_PIT_THIS s.ticks_per_second = tickstemp;

    //    BX_PIT_THIS s.usec_per_second = ALPHA_LOWER(BX_PIT_THIS s.usec_per_second,((bx_pc_system.time_usec()-BX_PIT_THIS s.last_sec_usec)*USEC_PER_SECOND/timediff));
    BX_PIT_THIS s.usec_per_second = ((bx_pc_system.time_usec()-BX_PIT_THIS s.last_sec_usec)*USEC_PER_SECOND/timediff);
    BX_PIT_THIS s.usec_per_second = BX_MAX(BX_PIT_THIS s.usec_per_second , MIN_USEC_PER_SECOND);
    BX_PIT_THIS s.last_sec_usec = bx_pc_system.time_usec();
#if DEBUG_REALTIME_WITH_PRINTF
    printf("Parms: ticks_per_second=%lld, usec_per_second=%lld\n",BX_PIT_THIS s.ticks_per_second, BX_PIT_THIS s.usec_per_second);
    printf("total_usec: %lld\n", bx_pc_system.time_usec());
#endif
  }
}
#endif // #if BX_USE_NEW_PIT
