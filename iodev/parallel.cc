/////////////////////////////////////////////////////////////////////////
// $Id: parallel.cc,v 1.22.6.3 2003-04-06 17:29:49 bdenney Exp $
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
//
////////////////////////////////////////////////////////
// This code was just a few stubs until Volker.Ruppert@t-online.de 
// fixed it up in November 2001.


// Define BX_PLUGGABLE in files that can be compiled into plugins.  For
// platforms that require a special tag on exported symbols, BX_PLUGGABLE 
// is used to know when we are exporting symbols and when we are importing.
#define BX_PLUGGABLE

#include "bochs.h"
#define LOG_THIS theParallelDevice->

bx_parallel_c *theParallelDevice = NULL;

  int
libparallel_LTX_plugin_init(plugin_t *plugin, plugintype_t type, int argc, char *argv[])
{
  theParallelDevice = new bx_parallel_c ();
  bx_devices.pluginParallelDevice = theParallelDevice;
  BX_REGISTER_DEVICE_DEVMODEL(plugin, type, theParallelDevice, BX_PLUGIN_PARALLEL);
  return(0); // Success
}

  void
libparallel_LTX_plugin_fini(void)
{
}

bx_parallel_c::bx_parallel_c(void)
{
  put("PAR");
  settype(PARLOG);
  s.output = NULL;
}

bx_parallel_c::~bx_parallel_c(void)
{
  if (s.output != NULL)
    fclose(s.output);
}

  void
bx_parallel_c::init(void)
{
  BX_DEBUG(("Init $Id: parallel.cc,v 1.22.6.3 2003-04-06 17:29:49 bdenney Exp $"));

  if (bx_options.par[0].Oenabled->get ()) {

    /* PARALLEL PORT 1 */

    DEV_register_irq(7, "Parallel Port 1");
    BX_INFO (("parallel port 1 at 0x378"));
    for (unsigned addr=0x0378; addr<=0x037A; addr++) {
      DEV_register_ioread_handler(BX_PAR_THIS, read_handler, addr, "Parallel Port 1", 7);
      }
    DEV_register_iowrite_handler(BX_PAR_THIS, write_handler, 0x0378, "Parallel Port 1", 7);
    DEV_register_iowrite_handler(BX_PAR_THIS, write_handler, 0x037A, "Parallel Port 1", 7);

    BX_PAR_THIS_PTR s.STATUS.error = 1;
    BX_PAR_THIS_PTR s.STATUS.slct  = 1;
    BX_PAR_THIS_PTR s.STATUS.pe    = 0;
    BX_PAR_THIS_PTR s.STATUS.ack   = 1;
    BX_PAR_THIS_PTR s.STATUS.busy  = 1;

    BX_PAR_THIS_PTR s.CONTROL.strobe   = 0;
    BX_PAR_THIS_PTR s.CONTROL.autofeed = 0;
    BX_PAR_THIS_PTR s.CONTROL.init     = 1;
    BX_PAR_THIS_PTR s.CONTROL.slct_in  = 1;
    BX_PAR_THIS_PTR s.CONTROL.irq      = 0;
    BX_PAR_THIS_PTR s.CONTROL.input    = 0;

    BX_PAR_THIS_PTR s.initmode = 0;

    if (strlen(bx_options.par[0].Ooutfile->getptr ()) > 0) {
      s.output = fopen(bx_options.par[0].Ooutfile->getptr (), "wb");
      if (!s.output)
        BX_PANIC (("Could not open '%s' to write parport1 output",
                   bx_options.par[0].Ooutfile->getptr ()));
    }
  }
}


  void
bx_parallel_c::register_state(bx_param_c *list_p)
{
  BXRS_START(bx_parallel_c, BX_PAR_THIS, "parallel port", list_p, 1);
  {
    BXRS_OBJ(bx_par_t, s);
  }
  BXRS_END;
}

void
bx_parallel_c::before_save_state()
{
  BX_INFO (("before_save_state"));
}

void
bx_parallel_c::after_restore_state()
{
  BX_INFO (("after_restore_state"));
}



void
bx_par_t::register_state(bx_param_c *list_p)
{
  BXRS_START(bx_par_t, this, "", list_p, 10);
  {
    BXRS_NUM(Bit8u, data);
    BXRS_STRUCT_START(struct STATUS_t, STATUS);
    {
      BXRS_BOOL(bx_bool, error);
      BXRS_BOOL(bx_bool, slct);
      BXRS_BOOL(bx_bool, pe);
      BXRS_BOOL(bx_bool, ack);
      BXRS_BOOL(bx_bool, busy);
    }
    BXRS_END;

    BXRS_STRUCT_START(struct CONTROL_t, CONTROL);
    {
      BXRS_BOOL(bx_bool, strobe);
      BXRS_BOOL(bx_bool, autofeed);
      BXRS_BOOL(bx_bool, init);
      BXRS_BOOL(bx_bool, slct_in);
      BXRS_BOOL(bx_bool, irq);
      BXRS_BOOL(bx_bool, input);
    }
    BXRS_END;

    // BJS TODO: register FILE *output;
    BXRS_BOOL(bx_bool, initmode);
  }
  BXRS_END;
}

  void
bx_parallel_c::reset(unsigned type)
{
}

  void
bx_parallel_c::virtual_printer(void)
{
  if (BX_PAR_THIS_PTR s.STATUS.slct) {
    if (BX_PAR_THIS_PTR s.output != NULL) {
      fputc(BX_PAR_THIS_PTR s.data, BX_PAR_THIS_PTR s.output);
      fflush (BX_PAR_THIS_PTR s.output);
      }
    if (BX_PAR_THIS_PTR s.CONTROL.irq == 1) {
      DEV_pic_raise_irq(7);
      }
    BX_PAR_THIS_PTR s.STATUS.ack = 0;
    BX_PAR_THIS_PTR s.STATUS.busy = 1;
    }
  else {
    BX_ERROR(("data is valid, but printer is offline"));
    }
}

  // static IO port read callback handler
  // redirects to non-static class handler to avoid virtual functions

  Bit32u
bx_parallel_c::read_handler(void *this_ptr, Bit32u address, unsigned io_len)
{
#if !BX_USE_PAR_SMF
  bx_parallel_c *class_ptr = (bx_parallel_c *) this_ptr;

  return( class_ptr->read(address, io_len) );
}


  Bit32u
bx_parallel_c::read(Bit32u address, unsigned io_len)
{
#else
  UNUSED(this_ptr);
#endif  // !BX_USE_PAR_SMF

  Bit32u retval;

  if (io_len == 1) {
    switch (address) {
      case 0x0378:
        if (!BX_PAR_THIS_PTR s.CONTROL.input) {
          return (Bit32u)BX_PAR_THIS_PTR s.data;
          }
        else {
          BX_ERROR(("read: input mode not supported"));
          return (0xFF);
          }
        break;
      case 0x0379:
	{
	  retval = ((BX_PAR_THIS_PTR s.STATUS.busy  << 7) |
		    (BX_PAR_THIS_PTR s.STATUS.ack   << 6) |
		    (BX_PAR_THIS_PTR s.STATUS.pe    << 5) |
		    (BX_PAR_THIS_PTR s.STATUS.slct  << 4) |
		    (BX_PAR_THIS_PTR s.STATUS.error << 3));
	  if (BX_PAR_THIS_PTR s.STATUS.ack == 0) {
	    BX_PAR_THIS_PTR s.STATUS.ack = 1;
            if (BX_PAR_THIS_PTR s.CONTROL.irq == 1) {
              DEV_pic_lower_irq(7);
	      }
	    }
	  if (BX_PAR_THIS_PTR s.initmode == 1) {
	    BX_PAR_THIS_PTR s.STATUS.busy  = 1;
	    BX_PAR_THIS_PTR s.STATUS.slct  = 1;
	    BX_PAR_THIS_PTR s.initmode = 0;
	    }
	  BX_DEBUG(("read: status register returns 0x%02x", retval));
	  return retval;
	}
	break;
      case 0x037A:
        {
          retval = ((BX_PAR_THIS_PTR s.CONTROL.input    << 5) |
                    (BX_PAR_THIS_PTR s.CONTROL.irq      << 4) |
		    (BX_PAR_THIS_PTR s.CONTROL.slct_in  << 3) |
		    (BX_PAR_THIS_PTR s.CONTROL.init     << 2) |
		    (BX_PAR_THIS_PTR s.CONTROL.autofeed << 1) |
		    (BX_PAR_THIS_PTR s.CONTROL.strobe));
	  BX_DEBUG(("read: control register returns 0x%02x", retval));
	  return retval;
	}
	break;
      }
    }
  /* PARALLEL PORT 1 */
  return(0);
}


  // static IO port write callback handler
  // redirects to non-static class handler to avoid virtual functions

  void
bx_parallel_c::write_handler(void *this_ptr, Bit32u address, Bit32u value, unsigned io_len)
{
#if !BX_USE_PAR_SMF
  bx_parallel_c *class_ptr = (bx_parallel_c *) this_ptr;

  class_ptr->write(address, value, io_len);
}

  void
bx_parallel_c::write(Bit32u address, Bit32u value, unsigned io_len)
{
#else
  UNUSED(this_ptr);
#endif  // !BX_USE_PAR_SMF

  if (io_len == 1) {
    switch (address) {
      case 0x0378:
        BX_PAR_THIS_PTR s.data = (Bit8u)value;
        BX_DEBUG(("write: data output register = 0x%02x", (Bit8u)value));
        break;
      case 0x037A:
	{
	  if ((value & 0x01) == 0x01) {
	    if (BX_PAR_THIS_PTR s.CONTROL.strobe == 0) {
	      BX_PAR_THIS_PTR s.CONTROL.strobe = 1;
	      virtual_printer(); // data is valid now
	      }
	  } else {
	    if (BX_PAR_THIS_PTR s.CONTROL.strobe == 1) {
	      BX_PAR_THIS_PTR s.CONTROL.strobe = 0;
	      }
	    }
	  BX_PAR_THIS_PTR s.CONTROL.autofeed = ((value & 0x02) == 0x02);
	  if ((value & 0x04) == 0x04) {
	    if (BX_PAR_THIS_PTR s.CONTROL.init == 0) {
	      BX_PAR_THIS_PTR s.CONTROL.init = 1;
	      BX_PAR_THIS_PTR s.STATUS.busy  = 0;
	      BX_PAR_THIS_PTR s.STATUS.slct  = 0;
	      BX_PAR_THIS_PTR s.initmode = 1;
	      BX_DEBUG(("printer init requested"));
	      }
	  } else {
	    if (BX_PAR_THIS_PTR s.CONTROL.init == 1) {
	      BX_PAR_THIS_PTR s.CONTROL.init = 0;
	      }
	    }
	  if ((value & 0x08) == 0x08) {
	    if (BX_PAR_THIS_PTR s.CONTROL.slct_in == 0) {
	      BX_PAR_THIS_PTR s.CONTROL.slct_in = 1;
	      BX_DEBUG(("printer now online"));
	      }
	  } else {
	    if (BX_PAR_THIS_PTR s.CONTROL.slct_in == 1) {
	      BX_PAR_THIS_PTR s.CONTROL.slct_in = 0;
	      BX_DEBUG(("printer now offline"));
	      }
	    }
	  BX_PAR_THIS_PTR s.STATUS.slct = BX_PAR_THIS_PTR s.CONTROL.slct_in;
	  if ((value & 0x10) == 0x10) {
	    if (BX_PAR_THIS_PTR s.CONTROL.irq == 0) {
	      BX_PAR_THIS_PTR s.CONTROL.irq = 1;
	      BX_DEBUG(("irq mode selected"));
	      }
	  } else {
	    if (BX_PAR_THIS_PTR s.CONTROL.irq == 1) {
	      BX_PAR_THIS_PTR s.CONTROL.irq = 0;
	      BX_DEBUG(("polling mode selected"));
	      }
	    }
	  if ((value & 0x20) == 0x20) {
	    if (BX_PAR_THIS_PTR s.CONTROL.input == 0) {
	      BX_PAR_THIS_PTR s.CONTROL.input = 1;
	      BX_DEBUG(("data input mode selected"));
	      }
	  } else {
	    if (BX_PAR_THIS_PTR s.CONTROL.input == 1) {
	      BX_PAR_THIS_PTR s.CONTROL.input = 0;
	      BX_DEBUG(("data output mode selected"));
	      }
	    }
	  if ((value & 0xC0) > 0) {
	    BX_ERROR(("write: unsupported control bit ignored"));
	    }
	}
	break;
      }
    }
  /* PARALLEL PORT 1 */
}
