/////////////////////////////////////////////////////////////////////////
// $Id: serial.cc,v 1.48 2004/02/28 22:06:36 vruppert Exp $
/////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2004  MandrakeSoft S.A.
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


// Peter Grehan (grehan@iprg.nokia.com) coded the original version of this
// serial emulation. He implemented a single 8250, and allow terminal
// input/output to stdout on FreeBSD.
// The current version emulates up to 4 UART 16550A with FIFO. Terminal
// input/output now works on some more platforms.


// Define BX_PLUGGABLE in files that can be compiled into plugins.  For
// platforms that require a special tag on exported symbols, BX_PLUGGABLE 
// is used to know when we are exporting symbols and when we are importing.
#define BX_PLUGGABLE

#include "bochs.h"

#define LOG_THIS theSerialDevice->

bx_serial_c *theSerialDevice = NULL;

  int
libserial_LTX_plugin_init(plugin_t *plugin, plugintype_t type, int argc, char *argv[])
{
  theSerialDevice = new bx_serial_c ();
  bx_devices.pluginSerialDevice = theSerialDevice;
  BX_REGISTER_DEVICE_DEVMODEL(plugin, type, theSerialDevice, BX_PLUGIN_SERIAL);
  return(0); // Success
}

  void
libserial_LTX_plugin_fini(void)
{
}

bx_serial_c::bx_serial_c(void)
{
  put("SER");
  settype(SERLOG);
  for (int i=0; i<BX_SERIAL_MAXDEV; i++) {
    s[i].tty_id = -1;
    s[i].tx_timer_index = BX_NULL_TIMER_HANDLE;
    s[i].rx_timer_index = BX_NULL_TIMER_HANDLE;
    s[i].fifo_timer_index = BX_NULL_TIMER_HANDLE;
  }
}

bx_serial_c::~bx_serial_c(void)
{
  for (int i=0; i<BX_SERIAL_MAXDEV; i++) {
    if (bx_options.com[i].Oenabled->get ()) {
#if USE_RAW_SERIAL
      delete [] BX_SER_THIS s[i].raw;
#elif defined(SERIAL_ENABLE)
      if (s[i].tty_id >= 0) {
        tcsetattr(s[i].tty_id, TCSAFLUSH, &s[i].term_orig);
      }
#endif
    }
  }
}


  void
bx_serial_c::init(void)
{
  Bit16u ports[BX_SERIAL_MAXDEV] = {0x03f8, 0x02f8, 0x03e8, 0x02e8};
  char name[16];

  /*
   * Put the UART registers into their RESET state
   */
  for (unsigned i=0; i<BX_N_SERIAL_PORTS; i++) {
    if (bx_options.com[i].Oenabled->get ()) {
      sprintf(name, "Serial Port %d", i + 1);
      /* serial interrupt */
      BX_SER_THIS s[i].IRQ = 4 - (i & 1);
      if (i < 2) {
        DEV_register_irq(BX_SER_THIS s[i].IRQ, name);
      }
      /* internal state */
      BX_SER_THIS s[i].ls_ipending = 0;
      BX_SER_THIS s[i].ms_ipending = 0;
      BX_SER_THIS s[i].rx_ipending = 0;
      BX_SER_THIS s[i].fifo_ipending = 0;
      BX_SER_THIS s[i].ls_interrupt = 0;
      BX_SER_THIS s[i].ms_interrupt = 0;
      BX_SER_THIS s[i].rx_interrupt = 0;
      BX_SER_THIS s[i].tx_interrupt = 0;
      BX_SER_THIS s[i].fifo_interrupt = 0;

      if (BX_SER_THIS s[i].tx_timer_index == BX_NULL_TIMER_HANDLE) {
        BX_SER_THIS s[i].tx_timer_index =
          bx_pc_system.register_timer(this, tx_timer_handler, 0,
                                      0,0, "serial.tx"); // one-shot, inactive
      }

      if (BX_SER_THIS s[i].rx_timer_index == BX_NULL_TIMER_HANDLE) {
        BX_SER_THIS s[i].rx_timer_index =
          bx_pc_system.register_timer(this, rx_timer_handler, 0,
                                      0,0, "serial.rx"); // one-shot, inactive
      }
      if (BX_SER_THIS s[i].fifo_timer_index == BX_NULL_TIMER_HANDLE) {
        BX_SER_THIS s[i].fifo_timer_index =
          bx_pc_system.register_timer(this, fifo_timer_handler, 0,
                                      0,0, "serial.fifo"); // one-shot, inactive
      }
      BX_SER_THIS s[i].rx_pollstate = BX_SER_RXIDLE;

      /* int enable: b0000 0000 */
      BX_SER_THIS s[i].int_enable.rxdata_enable = 0;
      BX_SER_THIS s[i].int_enable.txhold_enable = 0;
      BX_SER_THIS s[i].int_enable.rxlstat_enable = 0;
      BX_SER_THIS s[i].int_enable.modstat_enable = 0;

      /* int ID: b0000 0001 */
      BX_SER_THIS s[i].int_ident.ipending = 1;
      BX_SER_THIS s[i].int_ident.int_ID = 0;

      /* FIFO control: b0000 0000 */
      BX_SER_THIS s[i].fifo_cntl.enable = 0;
      BX_SER_THIS s[i].fifo_cntl.rxtrigger = 0;
      BX_SER_THIS s[i].rx_fifo_end = 0;
      BX_SER_THIS s[i].tx_fifo_end = 0;

      /* Line Control reg: b0000 0000 */
      BX_SER_THIS s[i].line_cntl.wordlen_sel = 0;
      BX_SER_THIS s[i].line_cntl.stopbits = 0;
      BX_SER_THIS s[i].line_cntl.parity_enable = 0;
      BX_SER_THIS s[i].line_cntl.evenparity_sel = 0;
      BX_SER_THIS s[i].line_cntl.stick_parity = 0;
      BX_SER_THIS s[i].line_cntl.break_cntl = 0;
      BX_SER_THIS s[i].line_cntl.dlab = 0;

      /* Modem Control reg: b0000 0000 */
      BX_SER_THIS s[i].modem_cntl.dtr = 0;
      BX_SER_THIS s[i].modem_cntl.rts = 0;
      BX_SER_THIS s[i].modem_cntl.out1 = 0;
      BX_SER_THIS s[i].modem_cntl.out2 = 0;
      BX_SER_THIS s[i].modem_cntl.local_loopback = 0;

      /* Line Status register: b0110 0000 */
      BX_SER_THIS s[i].line_status.rxdata_ready = 0;
      BX_SER_THIS s[i].line_status.overrun_error = 0;
      BX_SER_THIS s[i].line_status.parity_error = 0;
      BX_SER_THIS s[i].line_status.framing_error = 0;
      BX_SER_THIS s[i].line_status.break_int = 0;
      BX_SER_THIS s[i].line_status.thr_empty = 1;
      BX_SER_THIS s[i].line_status.tsr_empty = 1;
      BX_SER_THIS s[i].line_status.fifo_error = 0;

      /* Modem Status register: bXXXX 0000 */
      BX_SER_THIS s[i].modem_status.delta_cts = 0;
      BX_SER_THIS s[i].modem_status.delta_dsr = 0;
      BX_SER_THIS s[i].modem_status.ri_trailedge = 0;
      BX_SER_THIS s[i].modem_status.delta_dcd = 0;
      BX_SER_THIS s[i].modem_status.cts = 0;
      BX_SER_THIS s[i].modem_status.dsr = 0;
      BX_SER_THIS s[i].modem_status.ri = 0;
      BX_SER_THIS s[i].modem_status.dcd = 0;

      BX_SER_THIS s[i].scratch = 0;      /* scratch register */
      BX_SER_THIS s[i].divisor_lsb = 1;  /* divisor-lsb register */
      BX_SER_THIS s[i].divisor_msb = 0;  /* divisor-msb register */

      BX_SER_THIS s[i].baudrate = 115200;

      for (unsigned addr=ports[i]; addr<(unsigned)(ports[i]+8); addr++) {
        BX_DEBUG(("com%d register read/write: 0x%04x",i+1, addr));
        DEV_register_ioread_handler(this, read_handler, addr, name, 1);
        DEV_register_iowrite_handler(this, write_handler, addr, name, 1);
      }
#if USE_RAW_SERIAL
      BX_SER_THIS s[i].raw = new serial_raw(bx_options.com[i].Odev->getptr ());
#elif defined(SERIAL_ENABLE)
      if (strlen(bx_options.com[i].Odev->getptr ()) > 0) {
        BX_SER_THIS s[i].tty_id = open(bx_options.com[i].Odev->getptr (), O_RDWR|O_NONBLOCK,600);
        if (BX_SER_THIS s[i].tty_id < 0)
          BX_PANIC(("open of com%d (%s) failed\n", i+1, bx_options.com[i].Odev->getptr ()));
        BX_DEBUG(("com%d tty_id: %d", i+1, BX_SER_THIS s[i].tty_id));
        tcgetattr(BX_SER_THIS s[i].tty_id, &BX_SER_THIS s[i].term_orig);
        bcopy((caddr_t) &BX_SER_THIS s[i].term_orig, (caddr_t) &BX_SER_THIS s[i].term_new, sizeof(struct termios));
        cfmakeraw(&BX_SER_THIS s[i].term_new);
        BX_SER_THIS s[i].term_new.c_oflag |= OPOST | ONLCR;  // Enable NL to CR-NL translation
#ifndef TRUE_CTLC
        // ctl-C will exit Bochs, or trap to the debugger
        BX_SER_THIS s[i].term_new.c_iflag &= ~IGNBRK;
        BX_SER_THIS s[i].term_new.c_iflag |= BRKINT;
        BX_SER_THIS s[i].term_new.c_lflag |= ISIG;
#else
        // ctl-C will be delivered to the serial port
        BX_SER_THIS s[i].term_new.c_iflag |= IGNBRK;
        BX_SER_THIS s[i].term_new.c_iflag &= ~BRKINT;
#endif    /* !def TRUE_CTLC */
        BX_SER_THIS s[i].term_new.c_iflag = 0;
        BX_SER_THIS s[i].term_new.c_oflag = 0;
        BX_SER_THIS s[i].term_new.c_cflag = CS8|CREAD|CLOCAL;
        BX_SER_THIS s[i].term_new.c_lflag = 0;
        BX_SER_THIS s[i].term_new.c_cc[VMIN] = 1;
        BX_SER_THIS s[i].term_new.c_cc[VTIME] = 0;
        //BX_SER_THIS s[i].term_new.c_iflag |= IXOFF;
        tcsetattr(BX_SER_THIS s[i].tty_id, TCSAFLUSH, &BX_SER_THIS s[i].term_new);
      }
#endif   /* def SERIAL_ENABLE */
      BX_INFO(("com%d at 0x%04x irq %d", i+1, ports[i], BX_SER_THIS s[i].IRQ));
    }
  }
}

  void
bx_serial_c::reset(unsigned type)
{
}

  void
bx_serial_c::lower_interrupt(Bit8u port)
{
  /* If there are no more ints pending, clear the irq */
  if ((BX_SER_THIS s[port].rx_interrupt == 0) &&
      (BX_SER_THIS s[port].tx_interrupt == 0) &&
      (BX_SER_THIS s[port].ls_interrupt == 0) &&
      (BX_SER_THIS s[port].ms_interrupt == 0) &&
      (BX_SER_THIS s[port].fifo_interrupt == 0)) {
    DEV_pic_lower_irq(BX_SER_THIS s[port].IRQ);
  }
}

  void
bx_serial_c::raise_interrupt(Bit8u port, int type)
{
  bx_bool gen_int = 0;

  switch (type) {
    case BX_SER_INT_IER: /* IER has changed */
      gen_int = 1;
      break;
    case BX_SER_INT_RXDATA:
      if (BX_SER_THIS s[port].int_enable.rxdata_enable) {
        BX_SER_THIS s[port].rx_interrupt = 1;
        gen_int = 1;
      } else {
        BX_SER_THIS s[port].rx_ipending = 1;
      }
      break;
    case BX_SER_INT_TXHOLD:
      if (BX_SER_THIS s[port].int_enable.txhold_enable) {
        BX_SER_THIS s[port].tx_interrupt = 1;
        gen_int = 1;
      }
      break;
    case BX_SER_INT_RXLSTAT:
      if (BX_SER_THIS s[port].int_enable.rxlstat_enable) {
        BX_SER_THIS s[port].ls_interrupt = 1;
        gen_int = 1;
      } else {
        BX_SER_THIS s[port].ls_ipending = 1;
      }
      break;
    case BX_SER_INT_MODSTAT:
      if ((BX_SER_THIS s[port].ms_ipending == 1) &&
          (BX_SER_THIS s[port].int_enable.modstat_enable == 1)) {
        BX_SER_THIS s[port].ms_interrupt = 1;
        BX_SER_THIS s[port].ms_ipending = 0;
        gen_int = 1;
      }
      break;
    case BX_SER_INT_FIFO:
      if (BX_SER_THIS s[port].int_enable.rxdata_enable) {
        BX_SER_THIS s[port].fifo_interrupt = 1;
        gen_int = 1;
      } else {
        BX_SER_THIS s[port].fifo_ipending = 1;
      }
      break;
  }
  if (gen_int && BX_SER_THIS s[port].modem_cntl.out2) {
    DEV_pic_raise_irq(BX_SER_THIS s[port].IRQ);
  }
}

  // static IO port read callback handler
  // redirects to non-static class handler to avoid virtual functions

  Bit32u
bx_serial_c::read_handler(void *this_ptr, Bit32u address, unsigned io_len)
{
#if !BX_USE_SER_SMF
  bx_serial_c *class_ptr = (bx_serial_c *) this_ptr;

  return( class_ptr->read(address, io_len) );
}


  Bit32u
bx_serial_c::read(Bit32u address, unsigned io_len)
{
#else
  UNUSED(this_ptr);
#endif  // !BX_USE_SER_SMF
  Bit8u offset, val;
  Bit8u port = 0;

  BX_DEBUG(("register read from address 0x%04x - ", address));

  offset = address & 0x07;
  switch (address & 0x03f8) {
    case 0x03f8: port = 0; break;
    case 0x02f8: port = 1; break;
    case 0x03e8: port = 2; break;
    case 0x02e8: port = 3; break;
  }

  switch (offset) {
    case BX_SER_RBR: /* receive buffer, or divisor latch LSB if DLAB set */
      if (BX_SER_THIS s[port].line_cntl.dlab) {
        val = BX_SER_THIS s[port].divisor_lsb;
      } else {
        if (BX_SER_THIS s[port].fifo_cntl.enable) {
          val = BX_SER_THIS s[port].rx_fifo[0];
          if (BX_SER_THIS s[port].rx_fifo_end > 0) {
            memcpy(&BX_SER_THIS s[port].rx_fifo[0], &BX_SER_THIS s[port].rx_fifo[1], 15);
            BX_SER_THIS s[port].rx_fifo_end--;
          }
          if (BX_SER_THIS s[port].rx_fifo_end == 0) {
            BX_SER_THIS s[port].line_status.rxdata_ready = 0;
            BX_SER_THIS s[port].rx_interrupt = 0;
            BX_SER_THIS s[port].rx_ipending = 0;
            BX_SER_THIS s[port].fifo_interrupt = 0;
            BX_SER_THIS s[port].fifo_ipending = 0;
            lower_interrupt(port);
          }
        } else {
          val = BX_SER_THIS s[port].rxbuffer;
          BX_SER_THIS s[port].line_status.rxdata_ready = 0;
          BX_SER_THIS s[port].rx_interrupt = 0;
          BX_SER_THIS s[port].rx_ipending = 0;
          lower_interrupt(port);
        }
      }
      break;

    case BX_SER_IER: /* interrupt enable register, or div. latch MSB */
      if (BX_SER_THIS s[port].line_cntl.dlab) {
        val = BX_SER_THIS s[port].divisor_msb;
      } else {
        val = BX_SER_THIS s[port].int_enable.rxdata_enable |
              (BX_SER_THIS s[port].int_enable.txhold_enable  << 1) |
              (BX_SER_THIS s[port].int_enable.rxlstat_enable << 2) |
              (BX_SER_THIS s[port].int_enable.modstat_enable << 3);
      }
      break;

    case BX_SER_IIR: /* interrupt ID register */
      /*
       * Set the interrupt ID based on interrupt source
       */
      if (BX_SER_THIS s[port].ls_interrupt) {
        BX_SER_THIS s[port].int_ident.int_ID = 0x3;
        BX_SER_THIS s[port].int_ident.ipending = 0;
      } else if (BX_SER_THIS s[port].fifo_interrupt) {
        BX_SER_THIS s[port].int_ident.int_ID = 0x6;
        BX_SER_THIS s[port].int_ident.ipending = 0;
      } else if (BX_SER_THIS s[port].rx_interrupt) {
        BX_SER_THIS s[port].int_ident.int_ID = 0x2;
        BX_SER_THIS s[port].int_ident.ipending = 0;
      } else if (BX_SER_THIS s[port].tx_interrupt) {
        BX_SER_THIS s[port].int_ident.int_ID = 0x1;
        BX_SER_THIS s[port].int_ident.ipending = 0;
      } else if (BX_SER_THIS s[port].ms_interrupt) {
        BX_SER_THIS s[port].int_ident.int_ID = 0x0;
        BX_SER_THIS s[port].int_ident.ipending = 0;
      } else {
        BX_SER_THIS s[port].int_ident.int_ID = 0x0;
        BX_SER_THIS s[port].int_ident.ipending = 1;
      }
      BX_SER_THIS s[port].tx_interrupt = 0;
      lower_interrupt(port);

      val = BX_SER_THIS s[port].int_ident.ipending  |
            (BX_SER_THIS s[port].int_ident.int_ID << 1) |
            (BX_SER_THIS s[port].fifo_cntl.enable ? 0xc0 : 0x00);
      break;

    case BX_SER_LCR: /* Line control register */
      val = BX_SER_THIS s[port].line_cntl.wordlen_sel |
            (BX_SER_THIS s[port].line_cntl.stopbits       << 2) |
            (BX_SER_THIS s[port].line_cntl.parity_enable  << 3) |
            (BX_SER_THIS s[port].line_cntl.evenparity_sel << 4) |
            (BX_SER_THIS s[port].line_cntl.stick_parity   << 5) |
            (BX_SER_THIS s[port].line_cntl.break_cntl     << 6) |
            (BX_SER_THIS s[port].line_cntl.dlab           << 7);
      break;

    case BX_SER_MCR: /* MODEM control register */
      val = BX_SER_THIS s[port].modem_cntl.dtr |
            (BX_SER_THIS s[port].modem_cntl.rts << 1) |
            (BX_SER_THIS s[port].modem_cntl.out1 << 2) |
            (BX_SER_THIS s[port].modem_cntl.out2 << 3) |
            (BX_SER_THIS s[port].modem_cntl.local_loopback << 4);
      break;

    case BX_SER_LSR: /* Line status register */
      val = BX_SER_THIS s[port].line_status.rxdata_ready |
            (BX_SER_THIS s[port].line_status.overrun_error  << 1) |
            (BX_SER_THIS s[port].line_status.parity_error   << 2) |
            (BX_SER_THIS s[port].line_status.framing_error  << 3) |
            (BX_SER_THIS s[port].line_status.break_int      << 4) |
            (BX_SER_THIS s[port].line_status.thr_empty      << 5) |
            (BX_SER_THIS s[port].line_status.tsr_empty      << 6) |
            (BX_SER_THIS s[port].line_status.fifo_error     << 7);
      BX_SER_THIS s[port].line_status.overrun_error = 0;
      BX_SER_THIS s[port].line_status.break_int = 0;
      BX_SER_THIS s[port].ls_interrupt = 0;
      BX_SER_THIS s[port].ls_ipending = 0;
      lower_interrupt(port);
      break;

    case BX_SER_MSR: /* MODEM status register */
      val = BX_SER_THIS s[port].modem_status.delta_cts |
            (BX_SER_THIS s[port].modem_status.delta_dsr    << 1) |
            (BX_SER_THIS s[port].modem_status.ri_trailedge << 2) |
            (BX_SER_THIS s[port].modem_status.delta_dcd    << 3) |
            (BX_SER_THIS s[port].modem_status.cts          << 4) |
            (BX_SER_THIS s[port].modem_status.dsr          << 5) |
            (BX_SER_THIS s[port].modem_status.ri           << 6) |
            (BX_SER_THIS s[port].modem_status.dcd          << 7);
      BX_SER_THIS s[port].modem_status.delta_cts = 0;
      BX_SER_THIS s[port].modem_status.delta_dsr = 0;
      BX_SER_THIS s[port].modem_status.ri_trailedge = 0;
      BX_SER_THIS s[port].modem_status.delta_dcd = 0;
      BX_SER_THIS s[port].ms_interrupt = 0;
      BX_SER_THIS s[port].ms_ipending = 0;
      lower_interrupt(port);
      break;

    case BX_SER_SCR: /* scratch register */
      val = BX_SER_THIS s[port].scratch;
      break;

    default:
      val = 0; // keep compiler happy
      BX_PANIC(("unsupported io read from address=0x%04x!", address));
      break;
  }

  BX_DEBUG(("val =  0x%02x", (unsigned) val));

  return(val);
}


  // static IO port write callback handler
  // redirects to non-static class handler to avoid virtual functions

void
bx_serial_c::write_handler(void *this_ptr, Bit32u address, Bit32u value, unsigned io_len)
{
#if !BX_USE_SER_SMF
  bx_serial_c *class_ptr = (bx_serial_c *) this_ptr;

  class_ptr->write(address, value, io_len);
}

void
bx_serial_c::write(Bit32u address, Bit32u value, unsigned io_len)
{
#else
  UNUSED(this_ptr);
#endif  // !BX_USE_SER_SMF
  bx_bool prev_cts, prev_dsr, prev_ri, prev_dcd;
  bx_bool new_rx_ien, new_tx_ien, new_ls_ien, new_ms_ien;
  bx_bool new_wordlen, new_stopbits, new_break, new_dlab;
  bx_bool gen_int = 0;
  Bit8u offset, new_parity;
#if USE_RAW_SERIAL
  Bit8u p_mode;
#endif
  Bit8u port = 0;

  BX_DEBUG(("write to address: 0x%04x = 0x%02x", address, value));

  offset = address & 0x07;
  switch (address & 0x03f8) {
    case 0x03f8: port = 0; break;
    case 0x02f8: port = 1; break;
    case 0x03e8: port = 2; break;
    case 0x02e8: port = 3; break;
  }

  switch (offset) {
    case BX_SER_THR: /* transmit buffer, or divisor latch LSB if DLAB set */
      if (BX_SER_THIS s[port].line_cntl.dlab) {
        BX_SER_THIS s[port].divisor_lsb = value;

        if ((value != 0) || (BX_SER_THIS s[port].divisor_msb != 0)) {
          BX_SER_THIS s[port].baudrate = (int) (BX_PC_CLOCK_XTL /
                                         (16 * ((BX_SER_THIS s[port].divisor_msb << 8) |
                                         BX_SER_THIS s[port].divisor_lsb)));
        }
      } else {
        Bit8u bitmask = 0xff >> (3 - BX_SER_THIS s[port].line_cntl.wordlen_sel);
        if (BX_SER_THIS s[port].line_status.thr_empty) {
          if (BX_SER_THIS s[port].fifo_cntl.enable) {
            BX_SER_THIS s[port].tx_fifo[BX_SER_THIS s[port].tx_fifo_end++] = value & bitmask;
          } else {
            BX_SER_THIS s[port].thrbuffer = value & bitmask;
          }
          BX_SER_THIS s[port].line_status.thr_empty = 0;
          if (BX_SER_THIS s[port].line_status.tsr_empty) {
            if (BX_SER_THIS s[port].fifo_cntl.enable) {
              BX_SER_THIS s[port].tsrbuffer = BX_SER_THIS s[port].tx_fifo[0];
              memcpy(&BX_SER_THIS s[port].tx_fifo[0], &BX_SER_THIS s[port].tx_fifo[1], 15);
              BX_SER_THIS s[port].line_status.thr_empty = (--BX_SER_THIS s[port].tx_fifo_end == 0);
            } else {
              BX_SER_THIS s[port].tsrbuffer = BX_SER_THIS s[port].thrbuffer;
              BX_SER_THIS s[port].line_status.thr_empty = 1;
            }
            BX_SER_THIS s[port].line_status.tsr_empty = 0;
            raise_interrupt(port, BX_SER_INT_TXHOLD);
            bx_pc_system.activate_timer(BX_SER_THIS s[port].tx_timer_index,
                                        (int) (1000000.0 / BX_SER_THIS s[port].baudrate *
                                        (BX_SER_THIS s[port].line_cntl.wordlen_sel + 5)),
                                        0); /* not continuous */
          } else {
            BX_SER_THIS s[port].tx_interrupt = 0;
            lower_interrupt(port);
          }
        } else {
          if (BX_SER_THIS s[port].fifo_cntl.enable) {
            if (BX_SER_THIS s[port].tx_fifo_end < 16) {
              BX_SER_THIS s[port].tx_fifo[BX_SER_THIS s[port].tx_fifo_end++] = value & bitmask;
            } else {
              BX_ERROR(("com%d: transmit FIFO overflow", port+1));
            }
          } else {
            BX_ERROR(("com%d: write to tx hold register when not empty", port+1));
          }
        }
      }
      break;

    case BX_SER_IER: /* interrupt enable register, or div. latch MSB */
      if (BX_SER_THIS s[port].line_cntl.dlab) {
        BX_SER_THIS s[port].divisor_msb = value;

        if ((value != 0) || (BX_SER_THIS s[port].divisor_lsb != 0)) {
          BX_SER_THIS s[port].baudrate = (int) (BX_PC_CLOCK_XTL /
                                         (16 * ((BX_SER_THIS s[port].divisor_msb << 8) |
                                         BX_SER_THIS s[port].divisor_lsb)));
        }
      } else {
        new_rx_ien = value & 0x01;
        new_tx_ien = (value & 0x02) >> 1;
        new_ls_ien = (value & 0x04) >> 2;
        new_ms_ien = (value & 0x08) >> 3;
        if (new_ms_ien != BX_SER_THIS s[port].int_enable.modstat_enable) {
          BX_SER_THIS s[port].int_enable.modstat_enable  = new_ms_ien;
          if (BX_SER_THIS s[port].int_enable.modstat_enable == 1) {
            if (BX_SER_THIS s[port].ms_ipending == 1) {
              BX_SER_THIS s[port].ms_interrupt = 1;
              BX_SER_THIS s[port].ms_ipending = 0;
              gen_int = 1;
            }
          } else {
            if (BX_SER_THIS s[port].ms_interrupt == 1) {
              BX_SER_THIS s[port].ms_interrupt = 0;
              BX_SER_THIS s[port].ms_ipending = 1;
              lower_interrupt(port);
            }
          }
        }
        if (new_tx_ien != BX_SER_THIS s[port].int_enable.txhold_enable) {
          BX_SER_THIS s[port].int_enable.txhold_enable  = new_tx_ien;
          if (BX_SER_THIS s[port].int_enable.txhold_enable == 1) {
            BX_SER_THIS s[port].tx_interrupt = BX_SER_THIS s[port].line_status.thr_empty;
            if (BX_SER_THIS s[port].tx_interrupt) gen_int = 1;
          } else {
            BX_SER_THIS s[port].tx_interrupt = 0;
            lower_interrupt(port);
          }
        }
        if (new_rx_ien != BX_SER_THIS s[port].int_enable.rxdata_enable) {
          BX_SER_THIS s[port].int_enable.rxdata_enable  = new_rx_ien;
          if (BX_SER_THIS s[port].int_enable.rxdata_enable == 1) {
            if (BX_SER_THIS s[port].fifo_ipending == 1) {
              BX_SER_THIS s[port].fifo_interrupt = 1;
              BX_SER_THIS s[port].fifo_ipending = 0;
              gen_int = 1;
            }
            if (BX_SER_THIS s[port].rx_ipending == 1) {
              BX_SER_THIS s[port].rx_interrupt = 1;
              BX_SER_THIS s[port].rx_ipending = 0;
              gen_int = 1;
            }
          } else {
            if (BX_SER_THIS s[port].rx_interrupt == 1) {
              BX_SER_THIS s[port].rx_interrupt = 0;
              BX_SER_THIS s[port].rx_ipending = 1;
              lower_interrupt(port);
            }
            if (BX_SER_THIS s[port].fifo_interrupt == 1) {
              BX_SER_THIS s[port].fifo_interrupt = 0;
              BX_SER_THIS s[port].fifo_ipending = 1;
              lower_interrupt(port);
            }
          }
        }
        if (new_ls_ien != BX_SER_THIS s[port].int_enable.rxlstat_enable) {
          BX_SER_THIS s[port].int_enable.rxlstat_enable  = new_ls_ien;
          if (BX_SER_THIS s[port].int_enable.rxlstat_enable == 1) {
            if (BX_SER_THIS s[port].ls_ipending == 1) {
              BX_SER_THIS s[port].ls_interrupt = 1;
              BX_SER_THIS s[port].ls_ipending = 0;
              gen_int = 1;
            }
          } else {
            if (BX_SER_THIS s[port].ls_interrupt == 1) {
              BX_SER_THIS s[port].ls_interrupt = 0;
              BX_SER_THIS s[port].ls_ipending = 1;
              lower_interrupt(port);
            }
          }
        }
        if (gen_int) raise_interrupt(port, BX_SER_INT_IER);
      }
      break;

    case BX_SER_FCR: /* FIFO control register */
      if (!BX_SER_THIS s[port].fifo_cntl.enable && (value & 0x01)) {
        BX_INFO(("com%d: FIFO enabled", port+1));
        BX_SER_THIS s[port].rx_fifo_end = 0;
        BX_SER_THIS s[port].tx_fifo_end = 0;
      }
      BX_SER_THIS s[port].fifo_cntl.enable = value & 0x01;
      if (value & 0x02) {
        BX_SER_THIS s[port].rx_fifo_end = 0;
      }
      if (value & 0x04) {
        BX_SER_THIS s[port].tx_fifo_end = 0;
      }
      BX_SER_THIS s[port].fifo_cntl.rxtrigger = (value & 0xc0) >> 6;
      break;

    case BX_SER_LCR: /* Line control register */
      new_wordlen = value & 0x03;
      new_stopbits = (value & 0x04) >> 2;
      new_parity = (value & 0x38) >> 3;
      new_break = (value & 0x40) >> 6;
      new_dlab = (value & 0x80) >> 7;
#if USE_RAW_SERIAL
      if (BX_SER_THIS s[port].line_cntl.wordlen_sel != new_wordlen) {
        BX_SER_THIS s[port].raw->set_data_bits(new_wordlen + 5);
      }
      if (BX_SER_THIS s[port].line_cntl.stopbits != new_stopbits) {
        BX_SER_THIS s[port].raw->set_stop_bits(new_stopbits ? 2 : 1);
      }
      if ((BX_SER_THIS s[port].line_cntl.parity_enable != (bx_bool)(new_parity & 0x1)) ||
          (BX_SER_THIS s[port].line_cntl.evenparity_sel != (bx_bool)((new_parity & 0x2) >> 1)) ||
          (BX_SER_THIS s[port].line_cntl.stick_parity != (bx_bool)((new_parity & 0x4) >> 2))) {
        if ((new_parity & 0x1) == 0) {
          p_mode = P_NONE;
        } else {
          p_mode = (new_parity >> 1) + 1;
        }
        BX_SER_THIS s[port].raw->set_parity_mode(p_mode);
      }
      if (BX_SER_THIS s[port].line_cntl.break_cntl != new_break) {
        BX_SER_THIS s[port].raw->set_break(new_break);
      }
#endif // USE_RAW_SERIAL

      BX_SER_THIS s[port].line_cntl.wordlen_sel = new_wordlen;
      /* These are ignored, but set them up so they can be read back */
      BX_SER_THIS s[port].line_cntl.stopbits = new_stopbits;
      BX_SER_THIS s[port].line_cntl.parity_enable = new_parity & 0x01;
      BX_SER_THIS s[port].line_cntl.evenparity_sel = (new_parity & 0x02) >> 1;
      BX_SER_THIS s[port].line_cntl.stick_parity = (new_parity & 0x04) >> 2;
      BX_SER_THIS s[port].line_cntl.break_cntl = new_break;
      /* used when doing future writes */
      if (BX_SER_THIS s[port].line_cntl.dlab && !new_dlab) {
        // Start the receive polling process if not already started
        // and there is a valid baudrate.
        if (BX_SER_THIS s[port].rx_pollstate == BX_SER_RXIDLE &&
            BX_SER_THIS s[port].baudrate != 0) {
          BX_SER_THIS s[port].rx_pollstate = BX_SER_RXPOLL;
          bx_pc_system.activate_timer(BX_SER_THIS s[port].rx_timer_index,
                                      (int) (1000000.0 / BX_SER_THIS s[port].baudrate *
                                      (BX_SER_THIS s[port].line_cntl.wordlen_sel + 5)),
                                      0); /* not continuous */
        }
#if USE_RAW_SERIAL
        BX_SER_THIS s[port].raw->set_baudrate(BX_SER_THIS s[port].baudrate);
#endif // USE_RAW_SERIAL
        BX_DEBUG(("com%d: baud rate set - %d", port+1, BX_SER_THIS s[port].baudrate));
      }
      BX_SER_THIS s[port].line_cntl.dlab = new_dlab;
      break;

    case BX_SER_MCR: /* MODEM control register */
      if ((value & 0x01) == 0) {
#if USE_RAW_SERIAL
        BX_SER_THIS s[port].raw->send_hangup();
#endif
      }

      BX_SER_THIS s[port].modem_cntl.dtr  = value & 0x01;
      BX_SER_THIS s[port].modem_cntl.rts  = (value & 0x02) >> 1;
      BX_SER_THIS s[port].modem_cntl.out1 = (value & 0x04) >> 2;
      BX_SER_THIS s[port].modem_cntl.out2 = (value & 0x08) >> 3;
      BX_SER_THIS s[port].modem_cntl.local_loopback = (value & 0x10) >> 4;

      if (BX_SER_THIS s[port].modem_cntl.local_loopback) {
        prev_cts = BX_SER_THIS s[port].modem_status.cts;
        prev_dsr = BX_SER_THIS s[port].modem_status.dsr;
        prev_ri  = BX_SER_THIS s[port].modem_status.ri;
        prev_dcd = BX_SER_THIS s[port].modem_status.dcd;
        BX_SER_THIS s[port].modem_status.cts = BX_SER_THIS s[port].modem_cntl.rts;
        BX_SER_THIS s[port].modem_status.dsr = BX_SER_THIS s[port].modem_cntl.dtr;
        BX_SER_THIS s[port].modem_status.ri  = BX_SER_THIS s[port].modem_cntl.out1;
        BX_SER_THIS s[port].modem_status.dcd = BX_SER_THIS s[port].modem_cntl.out2;
        if (BX_SER_THIS s[port].modem_status.cts != prev_cts) {
          BX_SER_THIS s[port].modem_status.delta_cts = 1;
          BX_SER_THIS s[port].ms_ipending = 1;
        }
        if (BX_SER_THIS s[port].modem_status.dsr != prev_dsr) {
          BX_SER_THIS s[port].modem_status.delta_dsr = 1;
          BX_SER_THIS s[port].ms_ipending = 1;
        }
        if (BX_SER_THIS s[port].modem_status.ri != prev_ri)
          BX_SER_THIS s[port].ms_ipending = 1;
        if ((BX_SER_THIS s[port].modem_status.ri == 0) && (prev_ri == 1))
          BX_SER_THIS s[port].modem_status.ri_trailedge = 1;
        if (BX_SER_THIS s[port].modem_status.dcd != prev_dcd) {
          BX_SER_THIS s[port].modem_status.delta_dcd = 1;
          BX_SER_THIS s[port].ms_ipending = 1;
        }
        raise_interrupt(port, BX_SER_INT_MODSTAT);
      } else {
        /* set these to 0 for the time being */
        BX_SER_THIS s[port].modem_status.cts = 0;
        BX_SER_THIS s[port].modem_status.dsr = 0;
        BX_SER_THIS s[port].modem_status.ri  = 0;
        BX_SER_THIS s[port].modem_status.dcd = 0;
      }
      break;

    case BX_SER_LSR: /* Line status register */
      BX_ERROR(("com%d: write to line status register ignored", port+1));
      break;

    case BX_SER_MSR: /* MODEM status register */
      BX_ERROR(("com%d: write to MODEM status register ignored", port+1));
      break;

    case BX_SER_SCR: /* scratch register */
      BX_SER_THIS s[port].scratch = value;
      break;

    default:
      BX_PANIC(("unsupported io write to address=0x%04x, value = 0x%02x!",
        (unsigned) address, (unsigned) value));
      break;
  }
}


void
bx_serial_c::rx_fifo_enq(Bit8u port, Bit8u data)
{
  bx_bool gen_int = 0;

  if (BX_SER_THIS s[port].fifo_cntl.enable) {
    if (BX_SER_THIS s[port].rx_fifo_end == 16) {
      BX_ERROR(("com%d: receive FIFO overflow", port+1));
      BX_SER_THIS s[port].line_status.overrun_error = 1;
      raise_interrupt(port, BX_SER_INT_RXLSTAT);
    } else {
      BX_SER_THIS s[port].rx_fifo[BX_SER_THIS s[port].rx_fifo_end++] = data;
      switch (BX_SER_THIS s[port].fifo_cntl.rxtrigger) {
        case 1:
          if (BX_SER_THIS s[port].rx_fifo_end == 4) gen_int = 1;
          break;
        case 2:
          if (BX_SER_THIS s[port].rx_fifo_end == 8) gen_int = 1;
          break;
        case 3:
          if (BX_SER_THIS s[port].rx_fifo_end == 14) gen_int = 1;
          break;
        default:
          gen_int = 1;
      }
      if (gen_int) {
        bx_pc_system.deactivate_timer(BX_SER_THIS s[port].fifo_timer_index);
        BX_SER_THIS s[port].line_status.rxdata_ready = 1;
        raise_interrupt(port, BX_SER_INT_RXDATA);
      } else {
        bx_pc_system.activate_timer(BX_SER_THIS s[port].fifo_timer_index,
                                    (int) (1000000.0 / BX_SER_THIS s[port].baudrate *
                                    (BX_SER_THIS s[port].line_cntl.wordlen_sel + 5) * 16),
                                    0); /* not continuous */
      }
    }
  } else {
    if (BX_SER_THIS s[port].line_status.rxdata_ready == 1) {
      BX_ERROR(("com%d: overrun error", port+1));
      BX_SER_THIS s[port].line_status.overrun_error = 1;
      raise_interrupt(port, BX_SER_INT_RXLSTAT);
    }
    BX_SER_THIS s[port].rxbuffer = data;
    BX_SER_THIS s[port].line_status.rxdata_ready = 1;
    raise_interrupt(port, BX_SER_INT_RXDATA);
  }
}


void
bx_serial_c::tx_timer_handler(void *this_ptr)
{
  bx_serial_c *class_ptr = (bx_serial_c *) this_ptr;

  class_ptr->tx_timer();
}


void
bx_serial_c::tx_timer(void)
{
  bx_bool gen_int = 0;
  Bit8u port = 0;
  int timer_id;

  timer_id = bx_pc_system.triggeredTimerID();
  if (timer_id == BX_SER_THIS s[0].tx_timer_index) {
    port = 0;
  } else if (timer_id == BX_SER_THIS s[1].tx_timer_index) {
    port = 1;
  } else if (timer_id == BX_SER_THIS s[2].tx_timer_index) {
    port = 2;
  } else if (timer_id == BX_SER_THIS s[3].tx_timer_index) {
    port = 3;
  }
  if (BX_SER_THIS s[port].modem_cntl.local_loopback) {
    rx_fifo_enq(port, BX_SER_THIS s[port].tsrbuffer);
  } else {
#if USE_RAW_SERIAL
    if (!BX_SER_THIS s[port].raw->ready_transmit())
      BX_PANIC(("com%d: not ready to transmit", port+1));
    BX_SER_THIS s[port].raw->transmit(BX_SER_THIS s[port].tsrbuffer);
#elif defined(SERIAL_ENABLE)
    BX_DEBUG(("com%d: write: '%c'", port+1, BX_SER_THIS s[port].tsrbuffer));
    if (BX_SER_THIS s[port].tty_id >= 0) {
      write(BX_SER_THIS s[port].tty_id, (bx_ptr_t) & BX_SER_THIS s[port].tsrbuffer, 1);
    }
#endif
  }

  BX_SER_THIS s[port].line_status.tsr_empty = 1;
  if (BX_SER_THIS s[port].fifo_cntl.enable && (BX_SER_THIS s[port].tx_fifo_end > 0)) {
    BX_SER_THIS s[port].tsrbuffer = BX_SER_THIS s[port].tx_fifo[0];
    BX_SER_THIS s[port].line_status.tsr_empty = 0;
    memcpy(&BX_SER_THIS s[port].tx_fifo[0], &BX_SER_THIS s[port].tx_fifo[1], 15);
    gen_int = (--BX_SER_THIS s[port].tx_fifo_end == 0);
  } else if (!BX_SER_THIS s[port].line_status.thr_empty) {
    BX_SER_THIS s[port].tsrbuffer = BX_SER_THIS s[port].thrbuffer;
    BX_SER_THIS s[port].line_status.tsr_empty = 0;
    gen_int = 1;
  }
  if (!BX_SER_THIS s[port].line_status.tsr_empty) {
    if (gen_int) {
      BX_SER_THIS s[port].line_status.thr_empty = 1;
      raise_interrupt(port, BX_SER_INT_TXHOLD);
    }
    bx_pc_system.activate_timer(BX_SER_THIS s[port].tx_timer_index,
                                (int) (1000000.0 / BX_SER_THIS s[port].baudrate *
                                (BX_SER_THIS s[port].line_cntl.wordlen_sel + 5)),
                                0); /* not continuous */
  }
}


void
bx_serial_c::rx_timer_handler(void *this_ptr)
{
  bx_serial_c *class_ptr = (bx_serial_c *) this_ptr;

  class_ptr->rx_timer();
}


void
bx_serial_c::rx_timer(void)
{
#if BX_HAVE_SELECT
#ifndef __BEOS__
  struct timeval tval;
  fd_set fds;
#endif
#endif
  Bit8u port = 0;
  int timer_id;

  timer_id = bx_pc_system.triggeredTimerID();
  if (timer_id == BX_SER_THIS s[0].rx_timer_index) {
    port = 0;
  } else if (timer_id == BX_SER_THIS s[1].rx_timer_index) {
    port = 1;
  } else if (timer_id == BX_SER_THIS s[2].rx_timer_index) {
    port = 2;
  } else if (timer_id == BX_SER_THIS s[3].rx_timer_index) {
    port = 3;
  }
  int bdrate = BX_SER_THIS s[port].baudrate / (BX_SER_THIS s[port].line_cntl.wordlen_sel + 5);
  unsigned char chbuf = 0;

#if BX_HAVE_SELECT
#ifndef __BEOS__
  tval.tv_sec  = 0;
  tval.tv_usec = 0;

// MacOS: I'm not sure what to do with this, since I don't know
// what an fd_set is or what FD_SET() or select() do. They aren't
// declared in the CodeWarrior standard library headers. I'm just
// leaving it commented out for the moment.

  FD_ZERO(&fds);
  if (BX_SER_THIS s[port].tty_id >= 0) FD_SET(BX_SER_THIS s[port].tty_id, &fds);

  if ((BX_SER_THIS s[port].line_status.rxdata_ready == 0) ||
      (BX_SER_THIS s[port].fifo_cntl.enable)) {
#if USE_RAW_SERIAL
    bx_bool rdy;
    uint16 data;
    if ((rdy = BX_SER_THIS s[port].raw->ready_receive())) {
      data = BX_SER_THIS s[port].raw->receive();
      if (data == C_BREAK) {
        BX_DEBUG(("com%d: got BREAK", port+1));
        BX_SER_THIS s[port].line_status.break_int = 1;
        rdy = 0;
      }
    }
    if (rdy) {
      chbuf = data;
#elif defined(SERIAL_ENABLE)
    if ((BX_SER_THIS s[port].tty_id >= 0) && (select(BX_SER_THIS s[port].tty_id + 1, &fds, NULL, NULL, &tval) == 1)) {
      (void) read(BX_SER_THIS s[port].tty_id, &chbuf, 1);
      BX_DEBUG(("com%d: read: '%c'", port+1, chbuf));
#else
    if (0) {
#endif
      if (!BX_SER_THIS s[port].modem_cntl.local_loopback) {
        rx_fifo_enq(port, chbuf);
      }
    } else {
      if (!BX_SER_THIS s[port].fifo_cntl.enable) {
        bdrate = (int) (1000000.0 / 100000); // Poll frequency is 100ms
      }
    }
  } else {
    // Poll at 4x baud rate to see if the next-char can
    // be read
    bdrate *= 4;
  }
#endif
#endif

  bx_pc_system.activate_timer(BX_SER_THIS s[port].rx_timer_index,
                              (int) (1000000.0 / bdrate),
                              0); /* not continuous */
}


void
bx_serial_c::fifo_timer_handler(void *this_ptr)
{
  bx_serial_c *class_ptr = (bx_serial_c *) this_ptr;

  class_ptr->fifo_timer();
}


void
bx_serial_c::fifo_timer(void)
{
  Bit8u port = 0;
  int timer_id;

  timer_id = bx_pc_system.triggeredTimerID();
  if (timer_id == BX_SER_THIS s[0].fifo_timer_index) {
    port = 0;
  } else if (timer_id == BX_SER_THIS s[1].fifo_timer_index) {
    port = 1;
  } else if (timer_id == BX_SER_THIS s[2].fifo_timer_index) {
    port = 2;
  } else if (timer_id == BX_SER_THIS s[3].fifo_timer_index) {
    port = 3;
  }
  BX_SER_THIS s[port].line_status.rxdata_ready = 1;
  raise_interrupt(port, BX_SER_INT_FIFO);
}
