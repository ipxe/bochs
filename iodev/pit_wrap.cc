////////////////////////////////////////////////////////////////////////
// $Id: pit_wrap.cc,v 1.48.2.7 2003/03/28 09:26:09 slechta Exp $
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


//Important constant #defines:
#define USEC_PER_SECOND (1000000)
//1.193181MHz Clock
#define TICKS_PER_SECOND (1193181)


// define a macro to convert floating point numbers into 64-bit integers.
// In MSVC++ you can convert a 64-bit float into a 64-bit signed integer,
// but it will not convert a 64-bit float into a 64-bit unsigned integer.
// This macro works around that.
#define F2I(x)  ((Bit64u)(Bit64s) (x))
#define I2F(x)  ((double)(Bit64s) (x))

//CONFIGURATION #defines:


//MAINLINE Configuration (For realtime PIT):

//How much faster than real time we can go:
#define MAX_MULT (1.25)

//Minimum number of emulated useconds per second.
//  Now calculated using BX_MIN_IPS, the minimum number of
//   instructions per second.
#if 1
#  define MIN_USEC_PER_SECOND (((((Bit64u)USEC_PER_SECOND)*((Bit64u)BX_MIN_IPS))/((Bit64u)(bx_options.Oips->get())))+(Bit64u)1)
#else
#  define MIN_USEC_PER_SECOND (150000)
#endif

#if !BX_HAVE_REALTIME_USEC
//These are only used if we don't have a way of getting accurate time.
//How much slower than real time we can go:
#  define MIN_MULT (0.9)

//How much farther ahead we can get before we just stop
//  and wait for real time to catch up:
#  define AHEAD_CEILING ((Bit64u)(TICKS_PER_SECOND*2))
#endif



//DEBUG configuration:

//Debug with printf options.
#define DEBUG_REALTIME_WITH_PRINTF 0

//Use to test execution at multiples of real time.
#define TIME_DIVIDER (1)
#define TIME_MULTIPLIER (1)
#define TIME_HEADSTART (0)


#define GET_PIT_REALTIME64_USEC() (((bx_get_realtime64_usec()*(Bit64u)TIME_MULTIPLIER/(Bit64u)TIME_DIVIDER)))
//Set up Logging.
#define LOG_THIS bx_pit.

//A single instance.
bx_pit_c bx_pit;
#if BX_USE_PIT_SMF
#define this (&bx_pit)
#endif

//Workaround for environments where OUT is defined.
#ifdef OUT
#  undef OUT
#endif


//Generic MAX and MIN Functions
#define BX_MAX(a,b) ( ((a)>(b))?(a):(b) )
#define BX_MIN(a,b) ( ((a)>(b))?(b):(a) )


//USEC_ALPHA is multiplier for the past.
//USEC_ALPHA_B is 1-USEC_ALPHA, or multiplier for the present.
#define USEC_ALPHA ((double)(.8))
#define USEC_ALPHA_B ((double)(((double)1)-USEC_ALPHA))
#define USEC_ALPHA2 ((double)(.5))
#define USEC_ALPHA2_B ((double)(((double)1)-USEC_ALPHA2))
#define ALPHA_LOWER(old,new) ((Bit64u)((old<new)?((USEC_ALPHA*(I2F(old)))+(USEC_ALPHA_B*(I2F(new)))):((USEC_ALPHA2*(I2F(old)))+(USEC_ALPHA2_B*(I2F(new))))))


//PIT tick to usec conversion functions:
//Direct conversions:
#define REAL_TICKS_TO_USEC(a) ( ((a)*USEC_PER_SECOND)/TICKS_PER_SECOND )
#define REAL_USEC_TO_TICKS(a) ( ((a)*TICKS_PER_SECOND)/USEC_PER_SECOND )

//Conversion between emulated useconds and optionally realtime ticks.
#define TICKS_TO_USEC(a) ((BX_PIT_THIS s.use_realtime)?( ((a)*BX_PIT_THIS s.usec_per_second)/BX_PIT_THIS s.ticks_per_second ):( REAL_TICKS_TO_USEC(a) ))
#define USEC_TO_TICKS(a) ((BX_PIT_THIS s.use_realtime)?( ((a)*BX_PIT_THIS s.ticks_per_second)/BX_PIT_THIS s.usec_per_second ):( REAL_USEC_TO_TICKS(a) ))


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

  BX_PIT_THIS s.use_realtime = 0 ; // was: bx_options.Orealtime_pit->get ();

  BX_PIT_THIS s.timer.init();

  if (BX_PIT_THIS s.timer_handle[0] == BX_NULL_TIMER_HANDLE) {
    BX_PIT_THIS s.timer_handle[0] = bx_virt_timer.register_timer(this, timer_handler, (unsigned) 100 , 1, 1, "pit_wrap");
  }
  BX_DEBUG(("pit: RESETting timer."));
  bx_virt_timer.deactivate_timer(BX_PIT_THIS s.timer_handle[0]);
  BX_DEBUG(("deactivated timer."));
  if(BX_PIT_THIS s.timer.get_next_event_time()) {
    bx_virt_timer.activate_timer(BX_PIT_THIS s.timer_handle[0], 
				BX_MAX(1,TICKS_TO_USEC(BX_PIT_THIS s.timer.get_next_event_time())),
				0);
    BX_DEBUG(("activated timer."));
  }
  BX_PIT_THIS s.last_next_event_time = BX_PIT_THIS s.timer.get_next_event_time();
  BX_PIT_THIS s.last_usec=bx_virt_timer.time_usec();

  BX_PIT_THIS s.total_ticks=0;

  if (BX_PIT_THIS s.use_realtime) {
    BX_PIT_THIS s.usec_per_second=USEC_PER_SECOND;
    BX_PIT_THIS s.ticks_per_second=TICKS_PER_SECOND;
    BX_PIT_THIS s.total_sec=0;
    BX_PIT_THIS s.stored_delta=0;
#if BX_HAVE_REALTIME_USEC
    BX_PIT_THIS s.last_time=GET_PIT_REALTIME64_USEC()+(Bit64u)TIME_HEADSTART*(Bit64u)USEC_PER_SECOND;
    BX_PIT_THIS s.em_last_realtime=0;
    BX_PIT_THIS s.last_realtime_delta=0;
    BX_PIT_THIS s.last_realtime_ticks=0;
#else
    BX_PIT_THIS s.last_time=((time(NULL)*TIME_MULTIPLIER/TIME_DIVIDER)+TIME_HEADSTART)*USEC_PER_SECOND;
    BX_PIT_THIS s.max_ticks = AHEAD_CEILING;
#endif
  } else {
    BX_PIT_THIS s.total_usec=0;
  }

  BX_DEBUG(("pit: finished init"));

  BX_DEBUG(("s.last_usec=%llu",BX_PIT_THIS s.last_usec));
  BX_DEBUG(("s.timer_id=%d",BX_PIT_THIS s.timer_handle[0]));
  BX_DEBUG(("s.timer.get_next_event_time=%d",BX_PIT_THIS s.timer.get_next_event_time()));
  BX_DEBUG(("s.last_next_event_time=%d",BX_PIT_THIS s.last_next_event_time));

  return(1);
}


void 
bx_pit_c::register_state(bx_param_c *list_p)
{
  BXRS_START(bx_pit_c, this, desc, list_p, 25);
  BXRS_STRUCT_START(struct s_type, s)
  {
    BXRS_OBJ      (pit_82C54, timer       );
    BXRS_NUM      (Bit8u  , speaker_data_on        );
    BXRS_BOOL     (bx_bool, refresh_clock_div2     );
    BXRS_ARRAY_NUM(int    , timer_handle, 3        );
    BXRS_NUM      (Bit64u , last_usec              );
    BXRS_NUM      (Bit32u , last_next_event_time   );
    BXRS_NUM      (Bit64u , total_ticks            );
    BXRS_NUM      (Bit64u , usec_per_second        );
    BXRS_NUM      (Bit64u , ticks_per_second       );
    BXRS_NUM      (Bit64u , total_sec              );
    BXRS_NUM      (Bit64u , last_time              );
    BXRS_NUM      (Bit64u , last_sec_usec          );
    BXRS_NUM      (Bit64u , max_ticks              );
    BXRS_NUM      (Bit64u , stored_delta           );
    BXRS_NUM      (Bit64u , total_usec             );
    BXRS_NUM      (Bit64u , use_realtime           );
    BXRS_NUM      (Bit64u , em_last_realtime       );
    BXRS_NUM      (Bit64u , last_realtime_delta    );
    BXRS_NUM      (Bit64u , last_realtime_ticks    );
  }
  BXRS_STRUCT_END;
  BXRS_END;
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
  Bit64u time_passed = bx_virt_timer.time_usec()-BX_PIT_THIS s.last_usec;
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
    bx_virt_timer.deactivate_timer(BX_PIT_THIS s.timer_handle[0]);
    BX_DEBUG(("deactivated timer."));
    if(BX_PIT_THIS s.timer.get_next_event_time()) {
      bx_virt_timer.activate_timer(BX_PIT_THIS s.timer_handle[0], 
				  BX_MAX(1,TICKS_TO_USEC(BX_PIT_THIS s.timer.get_next_event_time())),
				  0);
      BX_DEBUG(("activated timer."));
    }
    BX_PIT_THIS s.last_next_event_time = BX_PIT_THIS s.timer.get_next_event_time();
  }
  BX_DEBUG(("s.last_usec=%llu",BX_PIT_THIS s.last_usec));
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
      BX_PIT_THIS s.refresh_clock_div2 = ((bx_virt_timer.time_usec() / 15) & 1);
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
  Bit64u time_passed = bx_virt_timer.time_usec()-BX_PIT_THIS s.last_usec;
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
    DEV_pic_raise_irq(0);
  } else {
    DEV_pic_lower_irq(0);
  }

  if(time_passed ||
     (BX_PIT_THIS s.last_next_event_time
      != BX_PIT_THIS s.timer.get_next_event_time())
     ) {
    BX_DEBUG(("pit: RESETting timer."));
    bx_virt_timer.deactivate_timer(BX_PIT_THIS s.timer_handle[0]);
    BX_DEBUG(("deactivated timer."));
    if(BX_PIT_THIS s.timer.get_next_event_time()) {
      bx_virt_timer.activate_timer(BX_PIT_THIS s.timer_handle[0], 
				  BX_MAX(1,TICKS_TO_USEC(BX_PIT_THIS s.timer.get_next_event_time())),
				  0);
      BX_DEBUG(("activated timer."));
    }
    BX_PIT_THIS s.last_next_event_time = BX_PIT_THIS s.timer.get_next_event_time();
  }
  BX_DEBUG(("s.last_usec=%llu",BX_PIT_THIS s.last_usec));
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


  bx_bool
bx_pit_c::periodic( Bit32u   usec_delta )
{
  bx_bool prev_timer0_out = BX_PIT_THIS s.timer.read_OUT(0);
  bx_bool want_interrupt = 0;
  Bit32u ticks_delta = 0;

#ifdef BX_SCHEDULED_DIE_TIME
  if (bx_pc_system.time_ticks() > BX_SCHEDULED_DIE_TIME) {
    BX_ERROR (("ticks exceeded scheduled die time, quitting"));
    BX_EXIT (2);
  }
#endif

  if (BX_PIT_THIS s.use_realtime) {
#if BX_HAVE_REALTIME_USEC
    Bit64u real_time_delta = GET_PIT_REALTIME64_USEC() - BX_PIT_THIS s.last_time;
    Bit64u real_time_total = real_time_delta + BX_PIT_THIS s.total_sec;
    Bit64u em_time_delta = (Bit64u)usec_delta + (Bit64u)BX_PIT_THIS s.stored_delta;
    if(real_time_delta) {
      BX_PIT_THIS s.last_realtime_delta = real_time_delta;
      BX_PIT_THIS s.last_realtime_ticks = BX_PIT_THIS s.total_ticks;
    }
    BX_PIT_THIS s.ticks_per_second = TICKS_PER_SECOND;

    //Start out with the number of ticks we would like
    // to have to line up with real time.
    ticks_delta = REAL_USEC_TO_TICKS(real_time_total) - BX_PIT_THIS s.total_ticks;
    if(REAL_USEC_TO_TICKS(real_time_total) < BX_PIT_THIS s.total_ticks) {
      //This slows us down if we're already ahead.
      //  probably only an issue on startup, but it solves some problems.
      ticks_delta = 0;
    }
    if(ticks_delta + BX_PIT_THIS s.total_ticks - BX_PIT_THIS s.last_realtime_ticks > REAL_USEC_TO_TICKS(F2I(MAX_MULT * I2F(BX_PIT_THIS s.last_realtime_delta)))) {
      //This keeps us from going too fast in relation to real time.
      ticks_delta = REAL_USEC_TO_TICKS(F2I(MAX_MULT * I2F(BX_PIT_THIS s.last_realtime_delta))) + BX_PIT_THIS s.last_realtime_ticks - BX_PIT_THIS s.total_ticks;
      BX_PIT_THIS s.ticks_per_second = F2I(MAX_MULT * I2F(TICKS_PER_SECOND));
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
#  if DEBUG_REALTIME_WITH_PRINTF
      if(((BX_PIT_THIS s.last_time + real_time_delta) / USEC_PER_SECOND) > (BX_PIT_THIS s.last_time / USEC_PER_SECOND)) {
	Bit64u temp1, temp2, temp3, temp4;
	temp1 = (Bit64u) BX_PIT_THIS s.total_sec;
	temp2 = REAL_USEC_TO_TICKS(BX_PIT_THIS s.total_sec);
	temp3 = (Bit64u)BX_PIT_THIS s.total_ticks;
	temp4 = (Bit64u)(REAL_USEC_TO_TICKS(BX_PIT_THIS s.total_sec) - BX_PIT_THIS s.total_ticks);
	printf("useconds: %llu, ",temp1);
	printf("expect ticks: %llu, ",temp2);
	printf("ticks: %llu, ",temp3);
	printf("diff: %llu\n",temp4);
      }
#  endif
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
    if(real_time_delta) {
      //FIXME
      Bit64u em_realtime_delta = BX_PIT_THIS s.last_sec_usec + BX_PIT_THIS s.stored_delta - BX_PIT_THIS s.em_last_realtime;
      b=((Bit64u)USEC_PER_SECOND * em_realtime_delta / real_time_delta);
      BX_PIT_THIS s.em_last_realtime = BX_PIT_THIS s.last_sec_usec + BX_PIT_THIS s.stored_delta;
    } else {
      b=a;
    }
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

    while ((BX_PIT_THIS s.total_ticks >= TICKS_PER_SECOND) && (BX_PIT_THIS s.total_usec >= USEC_PER_SECOND)) {
      BX_PIT_THIS s.total_ticks -= TICKS_PER_SECOND;
      BX_PIT_THIS s.total_usec  -= USEC_PER_SECOND;
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
	DEV_pic_raise_irq(0);
        prev_timer0_out=1;
      }
    } else {
      if ((BX_PIT_THIS s.timer.read_OUT(0))==0) {
	DEV_pic_lower_irq(0);
        prev_timer0_out=0;
      }
    }
    prev_timer0_out=BX_PIT_THIS s.timer.read_OUT(0);
    ticks_delta-=timedelta;
  }

  return(want_interrupt);
}


#if !BX_HAVE_REALTIME_USEC
void
bx_pit_c::second_update_data(void) {
  Bit64u timediff;
  timediff=((time(NULL)*TIME_MULTIPLIER/TIME_DIVIDER)*USEC_PER_SECOND)-BX_PIT_THIS s.last_time;
  BX_PIT_THIS s.last_time += timediff;
  if(timediff) {
    Bit64s tickstemp;

    BX_PIT_THIS s.total_sec += timediff;

    BX_PIT_THIS s.max_ticks = BX_MIN( (((BX_PIT_THIS s.total_sec)*(Bit64u)(TICKS_PER_SECOND))/USEC_PER_SECOND) + AHEAD_CEILING , BX_PIT_THIS s.total_ticks + (Bit64u)(TICKS_PER_SECOND*MAX_MULT) );

#if DEBUG_REALTIME_WITH_PRINTF
    printf("timediff: %llu, total_sec: %llu, total_ticks: %llu\n",timediff, BX_PIT_THIS s.total_sec, BX_PIT_THIS s.total_ticks);
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
	printf("Running WAY too slow. tps:%llu\n",tickstemp);
      } else {
	printf("Running slow.         tps:%llu\n",tickstemp);
      }
#endif
      tickstemp = (Bit64u)(TICKS_PER_SECOND*MAX_MULT);
#if DEBUG_REALTIME_WITH_PRINTF
      printf("..................new tps:%llu\n",tickstemp);
#endif
    } else if(tickstemp < (TICKS_PER_SECOND*MIN_MULT)) {
#if DEBUG_REALTIME_WITH_PRINTF
      if(tickstemp<0) {
        printf("Running WAY too fast. tps:%llu\n",tickstemp);
      } else {
        printf("Running fast.         tps:%llu\n",tickstemp);
      }
#endif
      tickstemp = (Bit64u)(TICKS_PER_SECOND*MIN_MULT);
#if DEBUG_REALTIME_WITH_PRINTF
      printf("..................new tps:%llu\n",tickstemp);
#endif
    }

    BX_PIT_THIS s.ticks_per_second = tickstemp;

    //    BX_PIT_THIS s.usec_per_second = ALPHA_LOWER(BX_PIT_THIS s.usec_per_second,((bx_virt_timer.time_usec()-BX_PIT_THIS s.last_sec_usec)*USEC_PER_SECOND/timediff));
    BX_PIT_THIS s.usec_per_second = ((bx_virt_timer.time_usec()-BX_PIT_THIS s.last_sec_usec)*USEC_PER_SECOND/timediff);
    BX_PIT_THIS s.usec_per_second = BX_MAX(BX_PIT_THIS s.usec_per_second , MIN_USEC_PER_SECOND);
    BX_PIT_THIS s.last_sec_usec = bx_virt_timer.time_usec();
#if DEBUG_REALTIME_WITH_PRINTF
    printf("Parms: ticks_per_second=%llu, usec_per_second=%llu\n",BX_PIT_THIS s.ticks_per_second, BX_PIT_THIS s.usec_per_second);
    printf("total_usec: %llu\n", bx_virt_timer.time_usec());
#endif
  }
}
#endif // #if !BX_HAVE_REALTIME_USEC
#endif // #if BX_USE_NEW_PIT
