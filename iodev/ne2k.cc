/////////////////////////////////////////////////////////////////////////
// $Id: ne2k.cc,v 1.48.2.1 2003-03-28 09:26:07 slechta Exp $
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

// Peter Grehan (grehan@iprg.nokia.com) coded all of this
// NE2000/ether stuff.

// Define BX_PLUGGABLE in files that can be compiled into plugins.  For
// platforms that require a special tag on exported symbols, BX_PLUGGABLE 
// is used to know when we are exporting symbols and when we are importing.
#define BX_PLUGGABLE
 
#include "bochs.h"
#if BX_NE2K_SUPPORT

#define LOG_THIS theNE2kDevice->

bx_ne2k_c *theNE2kDevice = NULL;

  int
libne2k_LTX_plugin_init(plugin_t *plugin, plugintype_t type, int argc, char *argv[])
{
  theNE2kDevice = new bx_ne2k_c ();
  bx_devices.pluginNE2kDevice = theNE2kDevice;
  BX_REGISTER_DEVICE_DEVMODEL(plugin, type, theNE2kDevice, BX_PLUGIN_NE2K);
  return(0); // Success
}

  void
libne2k_LTX_plugin_fini(void)
{
}
  
bx_ne2k_c::bx_ne2k_c(void)
{
  put("NE2K");
  settype(NE2KLOG);
  s.tx_timer_index = BX_NULL_TIMER_HANDLE;
}


bx_ne2k_c::~bx_ne2k_c(void)
{
  // nothing for now
}

//
// reset - restore state to power-up, cancelling all i/o
//
void
bx_ne2k_c::reset(unsigned type)
{
  BX_DEBUG (("reset"));
  // Zero out registers and memory
  memset( & BX_NE2K_THIS s.CR,  0, sizeof(BX_NE2K_THIS s.CR) );
  memset( & BX_NE2K_THIS s.ISR, 0, sizeof(BX_NE2K_THIS s.ISR));
  memset( & BX_NE2K_THIS s.IMR, 0, sizeof(BX_NE2K_THIS s.IMR));
  memset( & BX_NE2K_THIS s.DCR, 0, sizeof(BX_NE2K_THIS s.DCR));
  memset( & BX_NE2K_THIS s.TCR, 0, sizeof(BX_NE2K_THIS s.TCR));
  memset( & BX_NE2K_THIS s.TSR, 0, sizeof(BX_NE2K_THIS s.TSR));
  memset( & BX_NE2K_THIS s.RCR, 0, sizeof(BX_NE2K_THIS s.RCR));
  memset( & BX_NE2K_THIS s.RSR, 0, sizeof(BX_NE2K_THIS s.RSR));
  BX_NE2K_THIS s.local_dma  = 0;
  BX_NE2K_THIS s.page_start = 0;
  BX_NE2K_THIS s.page_stop  = 0;
  BX_NE2K_THIS s.bound_ptr  = 0;
  BX_NE2K_THIS s.tx_page_start = 0;
  BX_NE2K_THIS s.num_coll   = 0;
  BX_NE2K_THIS s.tx_bytes   = 0;
  BX_NE2K_THIS s.fifo       = 0;
  BX_NE2K_THIS s.remote_dma = 0;
  BX_NE2K_THIS s.remote_start = 0;
  BX_NE2K_THIS s.remote_bytes = 0;
  BX_NE2K_THIS s.tallycnt_0 = 0;
  BX_NE2K_THIS s.tallycnt_1 = 0;
  BX_NE2K_THIS s.tallycnt_2 = 0;

  memset( & BX_NE2K_THIS s.physaddr, 0, sizeof(BX_NE2K_THIS s.physaddr));
  memset( & BX_NE2K_THIS s.mchash, 0, sizeof(BX_NE2K_THIS s.mchash));
  BX_NE2K_THIS s.curr_page = 0;

  BX_NE2K_THIS s.rempkt_ptr   = 0;
  BX_NE2K_THIS s.localpkt_ptr = 0;
  BX_NE2K_THIS s.address_cnt  = 0;

  memset( & BX_NE2K_THIS s.mem, 0, sizeof(BX_NE2K_THIS s.mem));
  
  // Set power-up conditions
  BX_NE2K_THIS s.CR.stop      = 1;
    BX_NE2K_THIS s.CR.rdma_cmd  = 4;
  BX_NE2K_THIS s.ISR.reset    = 1;
  BX_NE2K_THIS s.DCR.longaddr = 1;
  DEV_pic_lower_irq(BX_NE2K_THIS s.base_irq);
}

//
// read_cr/write_cr - utility routines for handling reads/writes to
// the Command Register
//
Bit32u
bx_ne2k_c::read_cr(void)
{
  Bit32u val = 
         (((BX_NE2K_THIS s.CR.pgsel    & 0x03) << 6) |
	  ((BX_NE2K_THIS s.CR.rdma_cmd & 0x07) << 3) |
	  (BX_NE2K_THIS s.CR.tx_packet << 2) |
	  (BX_NE2K_THIS s.CR.start     << 1) |
	  (BX_NE2K_THIS s.CR.stop));
  BX_DEBUG(("read CR returns 0x%08x", val));
  return val;
}

void
bx_ne2k_c::write_cr(Bit32u value)
{
    BX_DEBUG(("wrote 0x%02x to CR", value));
  // Validate remote-DMA
  if ((value & 0x38) == 0x00)
    return;
  //  BX_PANIC(("CR write - invalid rDMA value 0"));

  // Check for s/w reset
  if (value & 0x01) {
    BX_NE2K_THIS s.ISR.reset = 1;
    BX_NE2K_THIS s.CR.stop   = 1;
  } else {
    BX_NE2K_THIS s.CR.stop = 0;
  }

  BX_NE2K_THIS s.CR.rdma_cmd = (value & 0x38) >> 3;

  // If start command issued, the RST bit in the ISR
  // must be cleared
  if ((value & 0x02) && !BX_NE2K_THIS s.CR.start) {
    BX_NE2K_THIS s.ISR.reset = 0;
  }

  BX_NE2K_THIS s.CR.start = ((value & 0x02) == 0x02);
  BX_NE2K_THIS s.CR.pgsel = (value & 0xc0) >> 6;

    // Check for send-packet command
    if (BX_NE2K_THIS s.CR.rdma_cmd == 3) {
	// Set up DMA read from receive ring
	BX_NE2K_THIS s.remote_start = BX_NE2K_THIS s.remote_dma = BX_NE2K_THIS s.bound_ptr * 256;
	BX_NE2K_THIS s.remote_bytes = *((Bit16u*) & BX_NE2K_THIS s.mem[BX_NE2K_THIS s.bound_ptr * 256 + 2 - BX_NE2K_MEMSTART]);
	BX_INFO(("Sending buffer #x%x length %d",
		  BX_NE2K_THIS s.remote_start,
		  BX_NE2K_THIS s.remote_bytes));
    }

  // Check for start-tx
    if ((value & 0x04) && BX_NE2K_THIS s.TCR.loop_cntl) {
	if (BX_NE2K_THIS s.TCR.loop_cntl != 1) {
	    BX_INFO(("Loop mode %d not supported.", BX_NE2K_THIS s.TCR.loop_cntl));
	} else {
	    rx_frame (& BX_NE2K_THIS s.mem[BX_NE2K_THIS s.tx_page_start*256 - BX_NE2K_MEMSTART],
		      BX_NE2K_THIS s.tx_bytes);
	}
    } else if (value & 0x04) {
    if (BX_NE2K_THIS s.CR.stop || !BX_NE2K_THIS s.CR.start)
      BX_PANIC(("CR write - tx start, dev in reset"));
    
    if (BX_NE2K_THIS s.tx_bytes == 0)
      BX_PANIC(("CR write - tx start, tx bytes == 0"));

#ifdef notdef    
    // XXX debug stuff
    printf("packet tx (%d bytes):\t", BX_NE2K_THIS s.tx_bytes);
    for (int i = 0; i < BX_NE2K_THIS s.tx_bytes; i++) {
      printf("%02x ", BX_NE2K_THIS s.mem[BX_NE2K_THIS s.tx_page_start*256 - 
				BX_NE2K_MEMSTART + i]);
      if (i && (((i+1) % 16) == 0)) 
	printf("\t");
    }
    printf("");
#endif    

    // Send the packet to the system driver
    BX_NE2K_THIS ethdev->sendpkt(& BX_NE2K_THIS s.mem[BX_NE2K_THIS s.tx_page_start*256 - BX_NE2K_MEMSTART], BX_NE2K_THIS s.tx_bytes);

    // some more debug
    if (BX_NE2K_THIS s.tx_timer_active)
      BX_PANIC(("CR write, tx timer still active"));
    
    // Schedule a timer to trigger a tx-complete interrupt
    // The number of microseconds is the bit-time / 10.
    // The bit-time is the preamble+sfd (64 bits), the
    // inter-frame gap (96 bits), the CRC (4 bytes), and the
    // the number of bits in the frame (s.tx_bytes * 8).
    //
    bx_pc_system.activate_timer(BX_NE2K_THIS s.tx_timer_index,
				(64 + 96 + 4*8 + BX_NE2K_THIS s.tx_bytes*8)/10,
				0); // not continuous
  }

  // Linux probes for an interrupt by setting up a remote-DMA read
  // of 0 bytes with remote-DMA completion interrupts enabled.
  // Detect this here
  if (BX_NE2K_THIS s.CR.rdma_cmd == 0x01 &&
      BX_NE2K_THIS s.CR.start &&
      BX_NE2K_THIS s.remote_bytes == 0) {
    BX_NE2K_THIS s.ISR.rdma_done = 1;
    if (BX_NE2K_THIS s.IMR.rdma_inte) {
      DEV_pic_raise_irq(BX_NE2K_THIS s.base_irq);
    }
  }
}

//
// chipmem_read/chipmem_write - access the 64K private RAM.
// The ne2000 memory is accessed through the data port of
// the asic (offset 0) after setting up a remote-DMA transfer.
// Both byte and word accesses are allowed.
// The first 16 bytes contains the MAC address at even locations,
// and there is 16K of buffer memory starting at 16K
//
Bit32u BX_CPP_AttrRegparmN(2)
bx_ne2k_c::chipmem_read(Bit32u address, unsigned int io_len)
{
  Bit32u retval = 0;

  if ((io_len == 2) && (address & 0x1)) 
    BX_PANIC(("unaligned chipmem word read"));

  // ROM'd MAC address
  if ((address >=0) && (address <= 31)) {
    retval = BX_NE2K_THIS s.macaddr[address];
    if (io_len == 2) {
      retval |= (BX_NE2K_THIS s.macaddr[address + 1] << 8);
    }
    return (retval);
  }

  if ((address >= BX_NE2K_MEMSTART) && (address < BX_NE2K_MEMEND)) {
    retval = BX_NE2K_THIS s.mem[address - BX_NE2K_MEMSTART];
    if (io_len == 2) {
      retval |= (BX_NE2K_THIS s.mem[address - BX_NE2K_MEMSTART + 1] << 8);
    }
    return (retval);
  }

  BX_DEBUG(("out-of-bounds chipmem read, %04X", address));

  return (0xff);
}

void BX_CPP_AttrRegparmN(3)
bx_ne2k_c::chipmem_write(Bit32u address, Bit32u value, unsigned io_len)
{
  if ((io_len == 2) && (address & 0x1)) 
    BX_PANIC(("unaligned chipmem word write"));

  if ((address >= BX_NE2K_MEMSTART) && (address < BX_NE2K_MEMEND)) {
    BX_NE2K_THIS s.mem[address - BX_NE2K_MEMSTART] = value & 0xff;
    if (io_len == 2)
      BX_NE2K_THIS s.mem[address - BX_NE2K_MEMSTART + 1] = value >> 8;
  } else
    BX_DEBUG(("out-of-bounds chipmem write, %04X", address));
}

//
// asic_read/asic_write - This is the high 16 bytes of i/o space
// (the lower 16 bytes is for the DS8390). Only two locations
// are used: offset 0, which is used for data transfer, and
// offset 0xf, which is used to reset the device.
// The data transfer port is used to as 'external' DMA to the
// DS8390. The chip has to have the DMA registers set up, and
// after that, insw/outsw instructions can be used to move
// the appropriate number of bytes to/from the device.
//
Bit32u BX_CPP_AttrRegparmN(2)
bx_ne2k_c::asic_read(Bit32u offset, unsigned int io_len)
{
  Bit32u retval = 0;

  switch (offset) {
  case 0x0:  // Data register
    //
    // A read remote-DMA command must have been issued,
    // and the source-address and length registers must  
    // have been initialised.
    //
    if (io_len > BX_NE2K_THIS s.remote_bytes)
      {
       BX_ERROR(("ne2K: dma read underrun iolen=%d remote_bytes=%d",io_len,BX_NE2K_THIS s.remote_bytes));
       //return 0;
      }

    //BX_INFO(("ne2k read DMA: addr=%4x remote_bytes=%d",BX_NE2K_THIS s.remote_dma,BX_NE2K_THIS s.remote_bytes));
    retval = chipmem_read(BX_NE2K_THIS s.remote_dma, io_len);
    //
    // The 8390 bumps the address and decreases the byte count
    // by the selected word size after every access, not by
    // the amount of data requested by the host (io_len).
    //
    BX_NE2K_THIS s.remote_dma += (BX_NE2K_THIS s.DCR.wdsize + 1);
    if (BX_NE2K_THIS s.remote_dma == BX_NE2K_THIS s.page_stop << 8) {
      BX_NE2K_THIS s.remote_dma = BX_NE2K_THIS s.page_start << 8;
    }
    // keep s.remote_bytes from underflowing
    if (BX_NE2K_THIS s.remote_bytes > 1)
      BX_NE2K_THIS s.remote_bytes -= (BX_NE2K_THIS s.DCR.wdsize + 1);
    else
      BX_NE2K_THIS s.remote_bytes = 0;

	// If all bytes have been written, signal remote-DMA complete
	if (BX_NE2K_THIS s.remote_bytes == 0) {
	    BX_NE2K_THIS s.ISR.rdma_done = 1;
	    if (BX_NE2K_THIS s.IMR.rdma_inte) {
		DEV_pic_raise_irq(BX_NE2K_THIS s.base_irq);
	    }
	}
    break;

  case 0xf:  // Reset register
    theNE2kDevice->reset(BX_RESET_SOFTWARE);
    break;

  default:
    BX_INFO(("asic read invalid address %04x", (unsigned) offset));
    break;
  }

  return (retval);
}

void
bx_ne2k_c::asic_write(Bit32u offset, Bit32u value, unsigned io_len)
{
  BX_DEBUG(("asic write addr=0x%02x, value=0x%04x", (unsigned) offset, (unsigned) value));
  switch (offset) {
  case 0x0:  // Data register - see asic_read for a description

    if ((io_len == 2) && (BX_NE2K_THIS s.DCR.wdsize == 0)) {
      BX_PANIC(("dma write length 2 on byte mode operation"));
      break;
    }

    if (BX_NE2K_THIS s.remote_bytes == 0)
      BX_PANIC(("ne2K: dma write, byte count 0"));

    chipmem_write(BX_NE2K_THIS s.remote_dma, value, io_len);
    // is this right ??? asic_read uses DCR.wordsize
    BX_NE2K_THIS s.remote_dma   += io_len;
    if (BX_NE2K_THIS s.remote_dma == BX_NE2K_THIS s.page_stop << 8) {
      BX_NE2K_THIS s.remote_dma = BX_NE2K_THIS s.page_start << 8;
    }

    BX_NE2K_THIS s.remote_bytes -= io_len;
    if (BX_NE2K_THIS s.remote_bytes > BX_NE2K_MEMSIZ)
      BX_NE2K_THIS s.remote_bytes = 0;

    // If all bytes have been written, signal remote-DMA complete
    if (BX_NE2K_THIS s.remote_bytes == 0) {
      BX_NE2K_THIS s.ISR.rdma_done = 1;
      if (BX_NE2K_THIS s.IMR.rdma_inte) {
	  DEV_pic_raise_irq(BX_NE2K_THIS s.base_irq);
      }
    }
    break;

  case 0xf:  // Reset register
    theNE2kDevice->reset(BX_RESET_SOFTWARE);
    break;

  default: // this is invalid, but happens under win95 device detection
    BX_INFO(("asic write invalid address %04x, ignoring", (unsigned) offset));
    break ;
  }
}

//
// page0_read/page0_write - These routines handle reads/writes to
// the 'zeroth' page of the DS8390 register file
//
Bit32u
bx_ne2k_c::page0_read(Bit32u offset, unsigned int io_len)
{
  BX_DEBUG(("page 0 read from port %04x, len=%u", (unsigned) offset,
	   (unsigned) io_len));
  if (io_len > 1) {
    BX_ERROR(("bad length! page 0 read from port %04x, len=%u", (unsigned) offset,
             (unsigned) io_len)); /* encountered with win98 hardware probe */
	return 0;
  }


  switch (offset) {
  case 0x0:  // CR
    return (read_cr());
    break;
    
  case 0x1:  // CLDA0
    return (BX_NE2K_THIS s.local_dma & 0xff);
    break;

  case 0x2:  // CLDA1
    return (BX_NE2K_THIS s.local_dma >> 8);
    break;

  case 0x3:  // BNRY
    return (BX_NE2K_THIS s.bound_ptr);
    break;

  case 0x4:  // TSR
    return ((BX_NE2K_THIS s.TSR.ow_coll    << 7) |
	    (BX_NE2K_THIS s.TSR.cd_hbeat   << 6) |
	    (BX_NE2K_THIS s.TSR.fifo_ur    << 5) |
	    (BX_NE2K_THIS s.TSR.no_carrier << 4) |
	    (BX_NE2K_THIS s.TSR.aborted    << 3) |
	    (BX_NE2K_THIS s.TSR.collided   << 2) |
	    (BX_NE2K_THIS s.TSR.tx_ok));
    break;

  case 0x5:  // NCR
    return (BX_NE2K_THIS s.num_coll);
    break;
    
  case 0x6:  // FIFO
    return (BX_NE2K_THIS s.fifo);
    break;

  case 0x7:  // ISR
    return ((BX_NE2K_THIS s.ISR.reset     << 7) |
	    (BX_NE2K_THIS s.ISR.rdma_done << 6) |
	    (BX_NE2K_THIS s.ISR.cnt_oflow << 5) |
	    (BX_NE2K_THIS s.ISR.overwrite << 4) |
	    (BX_NE2K_THIS s.ISR.tx_err    << 3) |
	    (BX_NE2K_THIS s.ISR.rx_err    << 2) |
	    (BX_NE2K_THIS s.ISR.pkt_tx    << 1) |
	    (BX_NE2K_THIS s.ISR.pkt_rx));
    break;
    
  case 0x8:  // CRDA0
    return (BX_NE2K_THIS s.remote_dma & 0xff);
    break;

  case 0x9:  // CRDA1
    return (BX_NE2K_THIS s.remote_dma >> 8);
    break;

  case 0xa:  // reserved
    BX_INFO(("reserved read - page 0, 0xa"));
    return (0xff);
    break;

  case 0xb:  // reserved
    BX_INFO(("reserved read - page 0, 0xb"));
    return (0xff);
    break;
    
  case 0xc:  // RSR
    return ((BX_NE2K_THIS s.RSR.deferred    << 7) |
	    (BX_NE2K_THIS s.RSR.rx_disabled << 6) |
	    (BX_NE2K_THIS s.RSR.rx_mbit     << 5) |
	    (BX_NE2K_THIS s.RSR.rx_missed   << 4) |
	    (BX_NE2K_THIS s.RSR.fifo_or     << 3) |
	    (BX_NE2K_THIS s.RSR.bad_falign  << 2) |
	    (BX_NE2K_THIS s.RSR.bad_crc     << 1) |
	    (BX_NE2K_THIS s.RSR.rx_ok));
    break;
    
  case 0xd:  // CNTR0
    return (BX_NE2K_THIS s.tallycnt_0);
    break;

  case 0xe:  // CNTR1
    return (BX_NE2K_THIS s.tallycnt_1);
    break;

  case 0xf:  // CNTR2
    return (BX_NE2K_THIS s.tallycnt_2);
    break;

  default:
    BX_PANIC(("page 0 offset %04x out of range", (unsigned) offset));
  }

  return(0);
}

void
bx_ne2k_c::page0_write(Bit32u offset, Bit32u value, unsigned io_len)
{
  BX_DEBUG(("page 0 write to port %04x, len=%u", (unsigned) offset,
	   (unsigned) io_len));
    if (io_len != 1) {
	BX_ERROR(("bad length! page 0 write to port %04x, len=%u",
		  (unsigned) offset,
            (unsigned) io_len));
    return;
  }
	
  
  switch (offset) {
  case 0x0:  // CR
    write_cr(value);
    break;

  case 0x1:  // PSTART
    BX_NE2K_THIS s.page_start = value;
    break;

  case 0x2:  // PSTOP
	// BX_INFO(("Writing to PSTOP: %02x", value));
    BX_NE2K_THIS s.page_stop = value;
    break;

  case 0x3:  // BNRY
    BX_NE2K_THIS s.bound_ptr = value;
    break;

  case 0x4:  // TPSR
    BX_NE2K_THIS s.tx_page_start = value;
    break;

  case 0x5:  // TBCR0
    // Clear out low byte and re-insert
    BX_NE2K_THIS s.tx_bytes &= 0xff00;
    BX_NE2K_THIS s.tx_bytes |= (value & 0xff);
    break;

  case 0x6:  // TBCR1
    // Clear out high byte and re-insert
    BX_NE2K_THIS s.tx_bytes &= 0x00ff;
    BX_NE2K_THIS s.tx_bytes |= ((value & 0xff) << 8);
    break;

  case 0x7:  // ISR
    value &= 0x7f;  // clear RST bit - status-only bit
    // All other values are cleared iff the ISR bit is 1
    BX_NE2K_THIS s.ISR.pkt_rx    &= ~((value & 0x01) == 0x01);
    BX_NE2K_THIS s.ISR.pkt_tx    &= ~((value & 0x02) == 0x02);
    BX_NE2K_THIS s.ISR.rx_err    &= ~((value & 0x04) == 0x04);
    BX_NE2K_THIS s.ISR.tx_err    &= ~((value & 0x08) == 0x08);
    BX_NE2K_THIS s.ISR.overwrite &= ~((value & 0x10) == 0x10);
    BX_NE2K_THIS s.ISR.cnt_oflow &= ~((value & 0x20) == 0x20);
    BX_NE2K_THIS s.ISR.rdma_done &= ~((value & 0x40) == 0x40);
    value = ((BX_NE2K_THIS s.ISR.rdma_done << 6) |
             (BX_NE2K_THIS s.ISR.cnt_oflow << 5) |
             (BX_NE2K_THIS s.ISR.overwrite << 4) |
             (BX_NE2K_THIS s.ISR.tx_err    << 3) |
             (BX_NE2K_THIS s.ISR.rx_err    << 2) |
             (BX_NE2K_THIS s.ISR.pkt_tx    << 1) |
             (BX_NE2K_THIS s.ISR.pkt_rx));
    value &= ((BX_NE2K_THIS s.IMR.rdma_inte << 6) |
              (BX_NE2K_THIS s.IMR.cofl_inte << 5) |
              (BX_NE2K_THIS s.IMR.overw_inte << 4) |
              (BX_NE2K_THIS s.IMR.txerr_inte << 3) |
              (BX_NE2K_THIS s.IMR.rxerr_inte << 2) |
              (BX_NE2K_THIS s.IMR.tx_inte << 1) |
              (BX_NE2K_THIS s.IMR.rx_inte));
    if (value == 0)
      DEV_pic_lower_irq(BX_NE2K_THIS s.base_irq);
    break;

  case 0x8:  // RSAR0
    // Clear out low byte and re-insert
    BX_NE2K_THIS s.remote_start &= 0xff00;
    BX_NE2K_THIS s.remote_start |= (value & 0xff);
    BX_NE2K_THIS s.remote_dma = BX_NE2K_THIS s.remote_start;
    break;

  case 0x9:  // RSAR1
    // Clear out high byte and re-insert
    BX_NE2K_THIS s.remote_start &= 0x00ff;
    BX_NE2K_THIS s.remote_start |= ((value & 0xff) << 8);
    BX_NE2K_THIS s.remote_dma = BX_NE2K_THIS s.remote_start;
    break;

  case 0xa:  // RBCR0
    // Clear out low byte and re-insert
    BX_NE2K_THIS s.remote_bytes &= 0xff00;
    BX_NE2K_THIS s.remote_bytes |= (value & 0xff);
    break;

  case 0xb:  // RBCR1
    // Clear out high byte and re-insert
    BX_NE2K_THIS s.remote_bytes &= 0x00ff;
    BX_NE2K_THIS s.remote_bytes |= ((value & 0xff) << 8);
    break;

  case 0xc:  // RCR
    // Check if the reserved bits are set
    if (value & 0xc0)
      BX_INFO(("RCR write, reserved bits set"));

    // Set all other bit-fields
    BX_NE2K_THIS s.RCR.errors_ok = ((value & 0x01) == 0x01);
    BX_NE2K_THIS s.RCR.runts_ok  = ((value & 0x02) == 0x02);
    BX_NE2K_THIS s.RCR.broadcast = ((value & 0x04) == 0x04);
    BX_NE2K_THIS s.RCR.multicast = ((value & 0x08) == 0x08);
    BX_NE2K_THIS s.RCR.promisc   = ((value & 0x10) == 0x10);
    BX_NE2K_THIS s.RCR.monitor   = ((value & 0x20) == 0x20);

    // Monitor bit is a little suspicious...
    if (value & 0x20)
      BX_INFO(("RCR write, monitor bit set!"));
    break;

  case 0xd:  // TCR
    // Check reserved bits
    if (value & 0xe0)
      BX_ERROR(("TCR write, reserved bits set"));

    // Test loop mode (not supported)
    if (value & 0x06) {
      BX_NE2K_THIS s.TCR.loop_cntl = (value & 0x6) >> 1;
	    BX_INFO(("TCR write, loop mode %d not supported", BX_NE2K_THIS s.TCR.loop_cntl));
    } else {
      BX_NE2K_THIS s.TCR.loop_cntl = 0;
    }

    // Inhibit-CRC not supported.
    if (value & 0x01)
      BX_PANIC(("TCR write, inhibit-CRC not supported"));

    // Auto-transmit disable very suspicious
    if (value & 0x08)
      BX_PANIC(("TCR write, auto transmit disable not supported"));

    // Allow collision-offset to be set, although not used
    BX_NE2K_THIS s.TCR.coll_prio = ((value & 0x08) == 0x08);
    break;

  case 0xe:  // DCR
    // Don't allow loopback mode to be set
    if (!(value & 0x08)) {
      BX_ERROR(("DCR write, loopback mode selected"));
	  // XXX This is a HACK, lets fix this right!
	  value -= 8;
	}

    // It is questionable to set longaddr and auto_rx, since they
    // aren't supported on the ne2000. Print a warning and continue
    if (value & 0x04)
      BX_INFO(("DCR write - LAS set ???"));
    if (value & 0x10)
      BX_INFO(("DCR write - AR set ???"));

    // Set other values.
    BX_NE2K_THIS s.DCR.wdsize   = ((value & 0x01) == 0x01);
    BX_NE2K_THIS s.DCR.endian   = ((value & 0x02) == 0x02);
    BX_NE2K_THIS s.DCR.longaddr = ((value & 0x04) == 0x04); // illegal ?
    BX_NE2K_THIS s.DCR.auto_rx  = ((value & 0x10) == 0x10); // also illegal ?
    BX_NE2K_THIS s.DCR.fifo_size = (value & 0x50) >> 5;
    break;

  case 0xf:  // IMR
    // Check for reserved bit
    if (value & 0x80)
      BX_PANIC(("IMR write, reserved bit set"));

    // Set other values
    BX_NE2K_THIS s.IMR.rx_inte    = ((value & 0x01) == 0x01);
    BX_NE2K_THIS s.IMR.tx_inte    = ((value & 0x02) == 0x02);
    BX_NE2K_THIS s.IMR.rxerr_inte = ((value & 0x04) == 0x04);
    BX_NE2K_THIS s.IMR.txerr_inte = ((value & 0x08) == 0x08);
    BX_NE2K_THIS s.IMR.overw_inte = ((value & 0x10) == 0x10);
    BX_NE2K_THIS s.IMR.cofl_inte  = ((value & 0x20) == 0x20);
    BX_NE2K_THIS s.IMR.rdma_inte  = ((value & 0x40) == 0x40);
    break;

  default:
    BX_PANIC(("page 0 write, bad offset %0x", offset));
  }
}


//
// page1_read/page1_write - These routines handle reads/writes to
// the first page of the DS8390 register file
//
Bit32u
bx_ne2k_c::page1_read(Bit32u offset, unsigned int io_len)
{
  BX_DEBUG(("page 1 read from port %04x, len=%u", (unsigned) offset,
	   (unsigned) io_len));
  if (io_len > 1)
    BX_PANIC(("bad length! page 1 read from port %04x, len=%u", (unsigned) offset,
             (unsigned) io_len));

  switch (offset) {
  case 0x0:  // CR
    return (read_cr());
    break;
    
  case 0x1:  // PAR0-5
  case 0x2:
  case 0x3:
  case 0x4:
  case 0x5:
  case 0x6:
    return (BX_NE2K_THIS s.physaddr[offset - 1]);
    break;

  case 0x7:  // CURR
      BX_DEBUG(("returning current page: %02x", (BX_NE2K_THIS s.curr_page)));
    return (BX_NE2K_THIS s.curr_page);

  case 0x8:  // MAR0-7
  case 0x9:
  case 0xa:
  case 0xb:
  case 0xc:
  case 0xd:
  case 0xe:
  case 0xf:
    return (BX_NE2K_THIS s.mchash[offset - 8]);
    break;

  default:
    BX_PANIC(("page 1 r offset %04x out of range", (unsigned) offset));
  }

  return (0);
}

void
bx_ne2k_c::page1_write(Bit32u offset, Bit32u value, unsigned io_len)
{
  BX_DEBUG(("page 1 w offset %04x", (unsigned) offset));
  switch (offset) {
  case 0x0:  // CR
    write_cr(value);
    break;  

  case 0x1:  // PAR0-5
  case 0x2:
  case 0x3:
  case 0x4:
  case 0x5:
  case 0x6:
    BX_NE2K_THIS s.physaddr[offset - 1] = value;
    break;
    
  case 0x7:  // CURR
    BX_NE2K_THIS s.curr_page = value;
    break;

  case 0x8:  // MAR0-7
  case 0x9:
  case 0xa:
  case 0xb:
  case 0xc:
  case 0xd:
  case 0xe:
  case 0xf:
    BX_NE2K_THIS s.mchash[offset - 8] = value;
    break;

  default:
    BX_PANIC(("page 1 w offset %04x out of range", (unsigned) offset));
  }  
}


//
// page2_read/page2_write - These routines handle reads/writes to
// the second page of the DS8390 register file
//
Bit32u
bx_ne2k_c::page2_read(Bit32u offset, unsigned int io_len)
{
  BX_DEBUG(("page 2 read from port %04x, len=%u", (unsigned) offset, (unsigned) io_len));

  if (io_len > 1)
    BX_PANIC(("bad length!  page 2 read from port %04x, len=%u", (unsigned) offset, (unsigned) io_len));

  switch (offset) {
  case 0x0:  // CR
    return (read_cr());
    break;

  case 0x1:  // PSTART
    return (BX_NE2K_THIS s.page_start);
    break;

  case 0x2:  // PSTOP
    return (BX_NE2K_THIS s.page_stop);
    break;

  case 0x3:  // Remote Next-packet pointer
    return (BX_NE2K_THIS s.rempkt_ptr);
    break;

  case 0x4:  // TPSR
    return (BX_NE2K_THIS s.tx_page_start);
    break;

  case 0x5:  // Local Next-packet pointer
    return (BX_NE2K_THIS s.localpkt_ptr);
    break;

  case 0x6:  // Address counter (upper)
    return (BX_NE2K_THIS s.address_cnt >> 8);
    break;

  case 0x7:  // Address counter (lower)
    return (BX_NE2K_THIS s.address_cnt & 0xff);
    break;

  case 0x8:  // Reserved
  case 0x9:
  case 0xa:
  case 0xb:
    BX_ERROR(("reserved read - page 2, 0x%02x", (unsigned) offset));
    return (0xff);
    break;

  case 0xc:  // RCR
    return ((BX_NE2K_THIS s.RCR.monitor   << 5) |
	    (BX_NE2K_THIS s.RCR.promisc   << 4) |
	    (BX_NE2K_THIS s.RCR.multicast << 3) |
	    (BX_NE2K_THIS s.RCR.broadcast << 2) |
	    (BX_NE2K_THIS s.RCR.runts_ok  << 1) |
	    (BX_NE2K_THIS s.RCR.errors_ok));
    break;

  case 0xd:  // TCR
    return ((BX_NE2K_THIS s.TCR.coll_prio   << 4) |
	    (BX_NE2K_THIS s.TCR.ext_stoptx  << 3) |
	    ((BX_NE2K_THIS s.TCR.loop_cntl & 0x3) << 1) |
	    (BX_NE2K_THIS s.TCR.crc_disable));
    break;

  case 0xe:  // DCR
    return (((BX_NE2K_THIS s.DCR.fifo_size & 0x3) << 5) |
	    (BX_NE2K_THIS s.DCR.auto_rx  << 4) |
	    (BX_NE2K_THIS s.DCR.loop     << 3) |
	    (BX_NE2K_THIS s.DCR.longaddr << 2) |
	    (BX_NE2K_THIS s.DCR.endian   << 1) |
	    (BX_NE2K_THIS s.DCR.wdsize));
    break;

  case 0xf:  // IMR
    return ((BX_NE2K_THIS s.IMR.rdma_inte  << 6) |
	    (BX_NE2K_THIS s.IMR.cofl_inte  << 5) |
	    (BX_NE2K_THIS s.IMR.overw_inte << 4) |
	    (BX_NE2K_THIS s.IMR.txerr_inte << 3) |
	    (BX_NE2K_THIS s.IMR.rxerr_inte << 2) |
	    (BX_NE2K_THIS s.IMR.tx_inte    << 1) |
	    (BX_NE2K_THIS s.IMR.rx_inte));
    break;

  default:
    BX_PANIC(("page 2 offset %04x out of range", (unsigned) offset));
  }

  return (0);
};

void
bx_ne2k_c::page2_write(Bit32u offset, Bit32u value, unsigned io_len)
{
  // Maybe all writes here should be BX_PANIC()'d, since they
  // affect internal operation, but let them through for now
  // and print a warning.
  if (offset != 0)
    BX_ERROR(("page 2 write ?"));

  switch (offset) {
  case 0x0:  // CR
    write_cr(value);
    break; 

  case 0x1:  // CLDA0
    // Clear out low byte and re-insert
    BX_NE2K_THIS s.local_dma &= 0xff00;
    BX_NE2K_THIS s.local_dma |= (value & 0xff);
    break;

  case 0x2:  // CLDA1
    // Clear out high byte and re-insert
    BX_NE2K_THIS s.local_dma &= 0x00ff;
    BX_NE2K_THIS s.local_dma |= ((value & 0xff) << 8);
    break;

  case 0x3:  // Remote Next-pkt pointer
    BX_NE2K_THIS s.rempkt_ptr = value;
    break;

  case 0x4:
    BX_PANIC(("page 2 write to reserved offset 4"));
    break;

  case 0x5:  // Local Next-packet pointer
    BX_NE2K_THIS s.localpkt_ptr = value;
    break;

  case 0x6:  // Address counter (upper)
    // Clear out high byte and re-insert
    BX_NE2K_THIS s.address_cnt &= 0x00ff;
    BX_NE2K_THIS s.address_cnt |= ((value & 0xff) << 8);
    break;

  case 0x7:  // Address counter (lower)
    // Clear out low byte and re-insert
    BX_NE2K_THIS s.address_cnt &= 0xff00;
    BX_NE2K_THIS s.address_cnt |= (value & 0xff);
    break;

  case 0x8:
  case 0x9:
  case 0xa:
  case 0xb:
  case 0xc:
  case 0xd:
  case 0xe:
  case 0xf:
    BX_PANIC(("page 2 write to reserved offset %0x", offset));
    break;
   
  default:
    BX_PANIC(("page 2 write, illegal offset %0x", offset));
    break;
  }
}
  
//
// page3_read/page3_write - writes to this page are illegal
//
Bit32u
bx_ne2k_c::page3_read(Bit32u offset, unsigned int io_len)
{
  BX_PANIC(("page 3 read attempted"));
  return (0);
}

void
bx_ne2k_c::page3_write(Bit32u offset, Bit32u value, unsigned io_len)
{
  BX_PANIC(("page 3 write attempted"));
}

//
// tx_timer_handler/tx_timer
//
void
bx_ne2k_c::tx_timer_handler(void *this_ptr)
{
  bx_ne2k_c *class_ptr = (bx_ne2k_c *) this_ptr;

  class_ptr->tx_timer();
}

void
bx_ne2k_c::tx_timer(void)
{
  BX_DEBUG(("tx_timer"));
  BX_NE2K_THIS s.TSR.tx_ok = 1;
  // Generate an interrupt if not masked and not one in progress
  if (BX_NE2K_THIS s.IMR.tx_inte && !BX_NE2K_THIS s.ISR.pkt_tx) {
    BX_NE2K_THIS s.ISR.pkt_tx = 1;
    DEV_pic_raise_irq(BX_NE2K_THIS s.base_irq);
  }
  BX_NE2K_THIS s.tx_timer_active = 0;
}


//
// read_handler/read - i/o 'catcher' function called from BOCHS
// mainline when the CPU attempts a read in the i/o space registered
// by this ne2000 instance
//
Bit32u
bx_ne2k_c::read_handler(void *this_ptr, Bit32u address, unsigned io_len)
{
#if !BX_USE_NE2K_SMF
  bx_ne2k_c *class_ptr = (bx_ne2k_c *) this_ptr;

  return( class_ptr->read(address, io_len) );
}

Bit32u
bx_ne2k_c::read(Bit32u address, unsigned io_len)
{
#else
  UNUSED(this_ptr);
#endif  // !BX_USE_NE2K_SMF
  BX_DEBUG(("read addr %x, len %d", address, io_len));
  Bit32u retval = 0;
  int offset = address - BX_NE2K_THIS s.base_address;

  if (offset >= 0x10) {
    retval = asic_read(offset - 0x10, io_len);
  } else {
    switch (BX_NE2K_THIS s.CR.pgsel) {
    case 0x00:
      retval = page0_read(offset, io_len);
      break;

    case 0x01:
      retval = page1_read(offset, io_len);
      break;

    case 0x02:
      retval = page2_read(offset, io_len);
      break;

    case 0x03:
      retval = page3_read(offset, io_len);
      break;

    default:
      BX_PANIC(("ne2K: unknown value of pgsel in read - %d",
	       BX_NE2K_THIS s.CR.pgsel));
    }
  }

  return (retval);
}

//
// write_handler/write - i/o 'catcher' function called from BOCHS
// mainline when the CPU attempts a write in the i/o space registered
// by this ne2000 instance
//
void
bx_ne2k_c::write_handler(void *this_ptr, Bit32u address, Bit32u value, 
			 unsigned io_len)
{
#if !BX_USE_NE2K_SMF
  bx_ne2k_c *class_ptr = (bx_ne2k_c *) this_ptr;
  
  class_ptr->write(address, value, io_len);
}

void
bx_ne2k_c::write(Bit32u address, Bit32u value, unsigned io_len)
{
#else
  UNUSED(this_ptr);
#endif  // !BX_USE_NE2K_SMF
  BX_DEBUG(("write with length %d", io_len));
  int offset = address - BX_NE2K_THIS s.base_address;

  //
  // The high 16 bytes of i/o space are for the ne2000 asic -
  //  the low 16 bytes are for the DS8390, with the current
  //  page being selected by the PS0,PS1 registers in the
  //  command register
  //
  if (offset >= 0x10) {
    asic_write(offset - 0x10, value, io_len);
  } else {
    switch (BX_NE2K_THIS s.CR.pgsel) {
    case 0x00:
      page0_write(offset, value, io_len);
      break;

    case 0x01:
      page1_write(offset, value, io_len);
      break;

    case 0x02:
      page2_write(offset, value, io_len);
      break;

    case 0x03:
      page3_write(offset, value, io_len);
      break;

    default:
      BX_PANIC(("ne2K: unknown value of pgsel in write - %d",
	       BX_NE2K_THIS s.CR.pgsel));
    }
  }
}


/*
 * mcast_index() - return the 6-bit index into the multicast
 * table. Stolen unashamedly from FreeBSD's if_ed.c
 */
unsigned
bx_ne2k_c::mcast_index(const void *dst)
{
#define POLYNOMIAL 0x04c11db6
  unsigned long crc = 0xffffffffL;
  int carry, i, j;
  unsigned char b;
  unsigned char *ep = (unsigned char *) dst;

  for (i = 6; --i >= 0;) {
    b = *ep++;
    for (j = 8; --j >= 0;) {
      carry = ((crc & 0x80000000L) ? 1 : 0) ^ (b & 0x01);
      crc <<= 1;
      b >>= 1;
      if (carry)
	crc = ((crc ^ POLYNOMIAL) | carry);
    }
  }
  return (crc >> 26);
#undef POLYNOMIAL
}

/*
 * Callback from the eth system driver when a frame has arrived
 */
void
bx_ne2k_c::rx_handler(void *arg, const void *buf, unsigned len)
{
    // BX_DEBUG(("rx_handler with length %d", len));
  bx_ne2k_c *class_ptr = (bx_ne2k_c *) arg;
  
  class_ptr->rx_frame(buf, len);
}

/*
 * rx_frame() - called by the platform-specific code when an
 * ethernet frame has been received. The destination address
 * is tested to see if it should be accepted, and if the
 * rx ring has enough room, it is copied into it and
 * the receive process is updated
 */
void
bx_ne2k_c::rx_frame(const void *buf, unsigned io_len)
{
  unsigned pages;
  unsigned avail;
  unsigned idx;
  int wrapped;
  int nextpage;
  unsigned char pkthdr[4];
  unsigned char *pktbuf = (unsigned char *) buf;
  unsigned char *startptr;
  static unsigned char bcast_addr[6] = {0xff,0xff,0xff,0xff,0xff,0xff};

  BX_DEBUG(("rx_frame with length %d", io_len));


  if ((BX_NE2K_THIS s.CR.stop != 0) ||
      (BX_NE2K_THIS s.page_start == 0) ||
      (BX_NE2K_THIS s.TCR.loop_cntl != 0)) {

    return;
  }

  // Add the pkt header + CRC to the length, and work
  // out how many 256-byte pages the frame would occupy
  pages = (io_len + 4 + 4 + 255)/256;

  if (BX_NE2K_THIS s.curr_page < BX_NE2K_THIS s.bound_ptr) {
    avail = BX_NE2K_THIS s.bound_ptr - BX_NE2K_THIS s.curr_page;    
  } else {
    avail = (BX_NE2K_THIS s.page_stop - BX_NE2K_THIS s.page_start) -
      (BX_NE2K_THIS s.curr_page - BX_NE2K_THIS s.bound_ptr);
    wrapped = 1;
  }

  // Avoid getting into a buffer overflow condition by not attempting
  // to do partial receives. The emulation to handle this condition
  // seems particularly painful.
  if (avail < pages) {
    return;
  }

  if ((io_len < 60) && !BX_NE2K_THIS s.RCR.runts_ok) {
    BX_DEBUG(("rejected small packet, length %d", io_len));
    return;
  }

  // Do address filtering if not in promiscuous mode
  if (! BX_NE2K_THIS s.RCR.promisc) {
    if (!memcmp(buf, bcast_addr, 6)) {
      if (!BX_NE2K_THIS s.RCR.broadcast) {
	return;
      }
    } else if (pktbuf[0] & 0x01) {
	if (! BX_NE2K_THIS s.RCR.multicast) {
	    return;
	}
      idx = mcast_index(buf);
      if (!(BX_NE2K_THIS s.mchash[idx >> 3] & (1 << (idx & 0x7)))) {
	return;
      }
    } else if (0 != memcmp(buf, BX_NE2K_THIS s.physaddr, 6)) {
      return;
    }
  } else {
      BX_DEBUG(("rx_frame promiscuous receive"));
  }

//    BX_INFO(("rx_frame %d to %x:%x:%x:%x:%x:%x from %x:%x:%x:%x:%x:%x",
//  	   io_len,
//  	   pktbuf[0], pktbuf[1], pktbuf[2], pktbuf[3], pktbuf[4], pktbuf[5],
//  	   pktbuf[6], pktbuf[7], pktbuf[8], pktbuf[9], pktbuf[10], pktbuf[11]));

  nextpage = BX_NE2K_THIS s.curr_page + pages;
  if (nextpage >= BX_NE2K_THIS s.page_stop) {
    nextpage -= BX_NE2K_THIS s.page_stop - BX_NE2K_THIS s.page_start;
  }

  // Setup packet header
  pkthdr[0] = 0;	// rx status - old behavior
  pkthdr[0] = 1;        // Probably better to set it all the time
                        // rather than set it to 0, which is clearly wrong.
  if (pktbuf[0] & 0x01) {
    pkthdr[0] |= 0x20;  // rx status += multicast packet
  }
  pkthdr[1] = nextpage;	// ptr to next packet
  pkthdr[2] = (io_len + 4) & 0xff;	// length-low
  pkthdr[3] = (io_len + 4) >> 8;	// length-hi

  // copy into buffer, update curpage, and signal interrupt if config'd
  startptr = & BX_NE2K_THIS s.mem[BX_NE2K_THIS s.curr_page * 256 -
			       BX_NE2K_MEMSTART];
  if ((nextpage > BX_NE2K_THIS s.curr_page) ||
      ((BX_NE2K_THIS s.curr_page + pages) == BX_NE2K_THIS s.page_stop)) {
    memcpy(startptr, pkthdr, 4);
    memcpy(startptr + 4, buf, io_len);
    BX_NE2K_THIS s.curr_page = nextpage;
  } else {
    int endbytes = (BX_NE2K_THIS s.page_stop - BX_NE2K_THIS s.curr_page) 
      * 256;
    memcpy(startptr, pkthdr, 4);
    memcpy(startptr + 4, buf, endbytes - 4);
    startptr = & BX_NE2K_THIS s.mem[BX_NE2K_THIS s.page_start * 256 -
				 BX_NE2K_MEMSTART];
    memcpy(startptr, (void *)(pktbuf + endbytes - 4),
	   io_len - endbytes + 8);    
    BX_NE2K_THIS s.curr_page = nextpage;
  }
  
  BX_NE2K_THIS s.RSR.rx_ok = 1;
  if (pktbuf[0] & 0x80) {
    BX_NE2K_THIS s.RSR.rx_mbit = 1;
  }

  BX_NE2K_THIS s.ISR.pkt_rx = 1;

  if (BX_NE2K_THIS s.IMR.rx_inte) {
    DEV_pic_raise_irq(BX_NE2K_THIS s.base_irq);
  }

}

void
bx_ne2k_c::init(void)
{
  BX_DEBUG(("Init $Id: ne2k.cc,v 1.48.2.1 2003-03-28 09:26:07 slechta Exp $"));


  // Bring the register state into power-up state
  theNE2kDevice->reset(BX_RESET_HARDWARE);

  // Read in values from config file
  BX_NE2K_THIS s.base_address = bx_options.ne2k.Oioaddr->get ();
  BX_NE2K_THIS s.base_irq     = bx_options.ne2k.Oirq->get ();
  memcpy(BX_NE2K_THIS s.physaddr, bx_options.ne2k.Omacaddr->getptr (), 6);

  if (BX_NE2K_THIS s.tx_timer_index == BX_NULL_TIMER_HANDLE) {
    BX_NE2K_THIS s.tx_timer_index =
      bx_pc_system.register_timer(this, tx_timer_handler, 0,
                                  0,0, "ne2k"); // one-shot, inactive
  }
  // Register the IRQ and i/o port addresses
  DEV_register_irq(BX_NE2K_THIS s.base_irq, "NE2000 ethernet NIC");

  for (unsigned addr = BX_NE2K_THIS s.base_address; 
       addr <= BX_NE2K_THIS s.base_address + 0x20; 
       addr++) {
    DEV_register_ioread_handler(this, read_handler, addr, "ne2000 NIC", 1);
    DEV_register_iowrite_handler(this, write_handler, addr, "ne2000 NIC", 1);
  }
  BX_INFO(("port 0x%x/32 irq %d mac %02x:%02x:%02x:%02x:%02x:%02x",
           BX_NE2K_THIS s.base_address,
           BX_NE2K_THIS s.base_irq,
           BX_NE2K_THIS s.physaddr[0],
           BX_NE2K_THIS s.physaddr[1],
           BX_NE2K_THIS s.physaddr[2],
           BX_NE2K_THIS s.physaddr[3],
           BX_NE2K_THIS s.physaddr[4],
           BX_NE2K_THIS s.physaddr[5]));

  // Initialise the mac address area by doubling the physical address
  BX_NE2K_THIS s.macaddr[0]  = BX_NE2K_THIS s.physaddr[0];
  BX_NE2K_THIS s.macaddr[1]  = BX_NE2K_THIS s.physaddr[0];
  BX_NE2K_THIS s.macaddr[2]  = BX_NE2K_THIS s.physaddr[1];
  BX_NE2K_THIS s.macaddr[3]  = BX_NE2K_THIS s.physaddr[1];
  BX_NE2K_THIS s.macaddr[4]  = BX_NE2K_THIS s.physaddr[2];
  BX_NE2K_THIS s.macaddr[5]  = BX_NE2K_THIS s.physaddr[2];
  BX_NE2K_THIS s.macaddr[6]  = BX_NE2K_THIS s.physaddr[3];
  BX_NE2K_THIS s.macaddr[7]  = BX_NE2K_THIS s.physaddr[3];
  BX_NE2K_THIS s.macaddr[8]  = BX_NE2K_THIS s.physaddr[4];
  BX_NE2K_THIS s.macaddr[9]  = BX_NE2K_THIS s.physaddr[4];
  BX_NE2K_THIS s.macaddr[10] = BX_NE2K_THIS s.physaddr[5];
  BX_NE2K_THIS s.macaddr[11] = BX_NE2K_THIS s.physaddr[5];
    
  // ne2k signature
  for (int i = 12; i < 32; i++) 
    BX_NE2K_THIS s.macaddr[i] = 0x57;
    
  // Attach to the simulated ethernet dev
  BX_NE2K_THIS ethdev = eth_locator_c::create(bx_options.ne2k.Oethmod->getptr (), 
                                              bx_options.ne2k.Oethdev->getptr (),
                                              (const char *) bx_options.ne2k.Omacaddr->getptr (),
                                              rx_handler, 
                                              this);

  if (BX_NE2K_THIS ethdev == NULL) {
    BX_PANIC(("could not find eth module %s", bx_options.ne2k.Oethmod->getptr ()));
    // if they continue, use null.
    BX_INFO(("could not find eth module %s - using null instead",
             bx_options.ne2k.Oethmod->getptr ()));

    BX_NE2K_THIS ethdev = eth_locator_c::create("null", NULL,
                                                (const char *) bx_options.ne2k.Omacaddr->getptr (),
                                                rx_handler, 
                                                this);
    if (BX_NE2K_THIS ethdev == NULL)
      BX_PANIC(("could not locate null module"));
  }
}

void
bx_ne2k_c::register_state(bx_param_c *list_p)
{
  BXRS_START(bx_ne2k_c, this, "", list_p, 35);
  {
    BXRS_OBJ(bx_ne2k_t, s);
    // BJS TODO: implement eth_pktmover_c *ethdev
    // eth_pktmover_c *ethdev;
  }
  BXRS_END;
}


#if BX_DEBUGGER

/*
 * this implements the info ne2k commands in the debugger.
 * info ne2k - shows all registers
 * info ne2k page N - shows all registers in a page
 * info ne2k page N reg M - shows just one register
 */

#define SHOW_FIELD(reg,field) do { \
  if (n>0 && !(n%5)) dbg_printf ("\n  "); \
  dbg_printf ("%s=%d ", #field, BX_NE2K_THIS s.reg.field); \
  n++; \
} while (0);
#define BX_HIGH_BYTE(x) ((0xff00 & (x)) >> 8)
#define BX_LOW_BYTE(x) (0x00ff & (x))
#define BX_DUPLICATE(n) if (brief && num!=n) break;

void
bx_ne2k_c::print_info (FILE *fp, int page, int reg, int brief)
{
  int i;
  int n = 0;
  if (page < 0) {
    for (page=0; page<=2; page++)
      theNE2kDevice->print_info (fp, page, reg, 1);
    // tell them how to use this command
    dbg_printf ("\nHow to use the info ne2k command:\n");
    dbg_printf ("info ne2k - show all registers\n");
    dbg_printf ("info ne2k page N - show registers in page N\n");
    dbg_printf ("info ne2k page N reg M - show just one register\n");
    return;
  }
  if (page > 2) {
    dbg_printf ("NE2K has only pages 0, 1, and 2.  Page %d is out of range.\n", page);
    return;
  }
  if (reg < 0) {
    dbg_printf ("NE2K registers, page %d\n", page);
    dbg_printf ("----------------------\n");
    for (reg=0; reg<=15; reg++)
      theNE2kDevice->print_info (fp, page, reg, 1);
    dbg_printf ("----------------------\n");
    return;
  }
  if (reg > 15) {
    dbg_printf ("NE2K has only registers 0-15 (0x0-0xf).  Register %d is out of range.\n", reg);
    return;
  }
  if (!brief) {
    dbg_printf ("NE2K Info - page %d, register 0x%02x\n", page, reg);
    dbg_printf ("----------------------------------\n");
  }
  int num = page*0x100 + reg;
  switch (num) {
    case 0x0000:
    case 0x0100:
    case 0x0200:
      dbg_printf ("CR (Command register):\n  ");
      SHOW_FIELD (CR, stop);
      SHOW_FIELD (CR, start);
      SHOW_FIELD (CR, tx_packet);
      SHOW_FIELD (CR, rdma_cmd);
      SHOW_FIELD (CR, pgsel);
      dbg_printf ("\n");
      break;
    case 0x0003:
      dbg_printf ("BNRY = Boundary Pointer = 0x%02x\n", BX_NE2K_THIS s.bound_ptr);
      break;
    case 0x0004:
      dbg_printf ("TSR (Transmit Status Register), read-only:\n  ");
      SHOW_FIELD (TSR, tx_ok);
      SHOW_FIELD (TSR, reserved);
      SHOW_FIELD (TSR, collided);
      SHOW_FIELD (TSR, aborted);
      SHOW_FIELD (TSR, no_carrier);
      SHOW_FIELD (TSR, fifo_ur);
      SHOW_FIELD (TSR, cd_hbeat);
      SHOW_FIELD (TSR, ow_coll);
      dbg_printf ("\n");
      // fall through into TPSR, no break line.
    case 0x0204:
      dbg_printf ("TPSR = Transmit Page Start = 0x%02x\n", BX_NE2K_THIS s.tx_page_start);
      break;
    case 0x0005:
    case 0x0006:  BX_DUPLICATE(0x0005);
      dbg_printf ("NCR = Number of Collisions Register (read-only) = 0x%02x\n", BX_NE2K_THIS s.num_coll);
      dbg_printf ("TBCR1,TBCR0 = Transmit Byte Count = %02x %02x\n", 
	  BX_HIGH_BYTE (BX_NE2K_THIS s.tx_bytes),
	  BX_LOW_BYTE (BX_NE2K_THIS s.tx_bytes));
      dbg_printf ("FIFO = %02x\n", BX_NE2K_THIS s.fifo);
      break;
    case 0x0007:
      dbg_printf ("ISR (Interrupt Status Register):\n  ");
      SHOW_FIELD (ISR, pkt_rx);
      SHOW_FIELD (ISR, pkt_tx);
      SHOW_FIELD (ISR, rx_err);
      SHOW_FIELD (ISR, tx_err);
      SHOW_FIELD (ISR, overwrite);
      SHOW_FIELD (ISR, cnt_oflow);
      SHOW_FIELD (ISR, rdma_done);
      SHOW_FIELD (ISR, reset);
      dbg_printf ("\n");
      break;
    case 0x0008:
    case 0x0009:  BX_DUPLICATE(0x0008);
      dbg_printf ("CRDA1,0 = Current remote DMA address = %02x %02x\n", 
	  BX_HIGH_BYTE (BX_NE2K_THIS s.remote_dma),
	  BX_LOW_BYTE (BX_NE2K_THIS s.remote_dma));
      dbg_printf ("RSAR1,0 = Remote start address = %02x %02x\n", 
	  BX_HIGH_BYTE(s.remote_start),
	  BX_LOW_BYTE(s.remote_start));
      break;
    case 0x000a:
    case 0x000b:  BX_DUPLICATE(0x000a);
      dbg_printf ("RCBR1,0 = Remote byte count = %02x\n", BX_NE2K_THIS s.remote_bytes);
      break;
    case 0x000c:
      dbg_printf ("RSR (Receive Status Register), read-only:\n  ");
      SHOW_FIELD (RSR, rx_ok);
      SHOW_FIELD (RSR, bad_crc);
      SHOW_FIELD (RSR, bad_falign);
      SHOW_FIELD (RSR, fifo_or);
      SHOW_FIELD (RSR, rx_missed);
      SHOW_FIELD (RSR, rx_mbit);
      SHOW_FIELD (RSR, rx_disabled);
      SHOW_FIELD (RSR, deferred);
      dbg_printf ("\n");
      // fall through into RCR
    case 0x020c:
      dbg_printf ("RCR (Receive Configuration Register):\n  ");
      SHOW_FIELD (RCR, errors_ok);
      SHOW_FIELD (RCR, runts_ok);
      SHOW_FIELD (RCR, broadcast);
      SHOW_FIELD (RCR, multicast);
      SHOW_FIELD (RCR, promisc);
      SHOW_FIELD (RCR, monitor);
      SHOW_FIELD (RCR, reserved);
      dbg_printf ("\n");
      break;
    case 0x000d:
      dbg_printf ("CNTR0 = Tally Counter 0 (Frame alignment errors) = %02x\n",
	  BX_NE2K_THIS s.tallycnt_0);
      // fall through into TCR
    case 0x020d:
      dbg_printf ("TCR (Transmit Configuration Register):\n  ");
      SHOW_FIELD (TCR, crc_disable);
      SHOW_FIELD (TCR, loop_cntl);
      SHOW_FIELD (TCR, ext_stoptx);
      SHOW_FIELD (TCR, coll_prio);
      SHOW_FIELD (TCR, reserved);
      dbg_printf ("\n");
      break;
    case 0x000e:
      dbg_printf ("CNTR1 = Tally Counter 1 (CRC Errors) = %02x\n",
	  BX_NE2K_THIS s.tallycnt_1);
      // fall through into DCR
    case 0x020e:
      dbg_printf ("DCR (Data Configuration Register):\n  ");
      SHOW_FIELD (DCR, wdsize);
      SHOW_FIELD (DCR, endian);
      SHOW_FIELD (DCR, longaddr);
      SHOW_FIELD (DCR, loop);
      SHOW_FIELD (DCR, auto_rx);
      SHOW_FIELD (DCR, fifo_size);
      dbg_printf ("\n");
      break;
    case 0x000f:
      dbg_printf ("CNTR2 = Tally Counter 2 (Missed Packet Errors) = %02x\n",
	  BX_NE2K_THIS s.tallycnt_2);
      // fall through into IMR
    case 0x020f:
      dbg_printf ("IMR (Interrupt Mask Register)\n  ");
      SHOW_FIELD (IMR, rx_inte);
      SHOW_FIELD (IMR, tx_inte);
      SHOW_FIELD (IMR, rxerr_inte);
      SHOW_FIELD (IMR, txerr_inte);
      SHOW_FIELD (IMR, overw_inte);
      SHOW_FIELD (IMR, cofl_inte);
      SHOW_FIELD (IMR, rdma_inte);
      SHOW_FIELD (IMR, reserved);
      dbg_printf ("\n");
      break;
    case 0x0101:
    case 0x0102:  BX_DUPLICATE(0x0101);
    case 0x0103:  BX_DUPLICATE(0x0101);
    case 0x0104:  BX_DUPLICATE(0x0101);
    case 0x0105:  BX_DUPLICATE(0x0101);
    case 0x0106:  BX_DUPLICATE(0x0101);
      dbg_printf ("MAC address registers are located at page 1, registers 1-6.\n");
      dbg_printf ("The MAC address is ");
      for (i=0; i<=5; i++) 
	dbg_printf ("%02x%c", BX_NE2K_THIS s.physaddr[i], i<5?':' : '\n');
      break;
    case 0x0107:
      dbg_printf ("Current page is 0x%02x\n", BX_NE2K_THIS s.curr_page);
      break;
    case 0x0108:
    case 0x0109:  BX_DUPLICATE(0x0108);
    case 0x010A:  BX_DUPLICATE(0x0108);
    case 0x010B:  BX_DUPLICATE(0x0108);
    case 0x010C:  BX_DUPLICATE(0x0108);
    case 0x010D:  BX_DUPLICATE(0x0108);
    case 0x010E:  BX_DUPLICATE(0x0108);
    case 0x010F:  BX_DUPLICATE(0x0108);
      dbg_printf ("MAR0-7 (Multicast address registers 0-7) are set to:\n");
      for (i=0; i<8; i++) dbg_printf ("%02x ", BX_NE2K_THIS s.mchash[i]);
      dbg_printf ("\nMAR0 is listed first.\n");
      break;
    case 0x0001:
    case 0x0002:  BX_DUPLICATE(0x0001);
    case 0x0201:  BX_DUPLICATE(0x0001);
    case 0x0202:  BX_DUPLICATE(0x0001);
      dbg_printf ("PSTART = Page start register = %02x\n", BX_NE2K_THIS s.page_start);
      dbg_printf ("PSTOP = Page stop register = %02x\n", BX_NE2K_THIS s.page_stop);
      dbg_printf ("Local DMA address = %02x %02x\n", 
	  BX_HIGH_BYTE(BX_NE2K_THIS s.local_dma),
	  BX_LOW_BYTE(BX_NE2K_THIS s.local_dma));
      break;
    case 0x0203:
      dbg_printf ("Remote Next Packet Pointer = %02x\n", BX_NE2K_THIS s.rempkt_ptr);
      break;
    case 0x0205:
      dbg_printf ("Local Next Packet Pointer = %02x\n", BX_NE2K_THIS s.localpkt_ptr);
      break;
    case 0x0206:
    case 0x0207:  BX_DUPLICATE(0x0206);
      dbg_printf ("Address Counter= %02x %02x\n", 
	 BX_HIGH_BYTE(BX_NE2K_THIS s.address_cnt),
	 BX_LOW_BYTE(BX_NE2K_THIS s.address_cnt));
      break;
    case 0x0208:
    case 0x0209:  BX_DUPLICATE(0x0208);
    case 0x020A:  BX_DUPLICATE(0x0208);
    case 0x020B:  BX_DUPLICATE(0x0208);
      if (!brief) dbg_printf ("Reserved\n");
    case 0xffff:
      dbg_printf ("IMR (Interrupt Mask Register):\n  ");
      dbg_printf ("\n");
      break;
    default:
      dbg_printf ("NE2K info: sorry, page %d register %d cannot be displayed.\n", page, reg);
  }
  if (!brief)
    dbg_printf ("\n");
}

#else

void
bx_ne2k_c::print_info (FILE *fp, int page, int reg, int brief)
{
}

#endif

void
bx_ne2k_t::register_state(bx_param_c *list_p)
{
  BXRS_START(bx_ne2k_t, this, "ne2k register state", list_p, 20);
  {
    
    BXRS_STRUCT_START_D(struct CR_t, CR, "Command Register - 00h read/write");
    {
      BXRS_BOOL_D(bx_bool, stop,     "STP - Software Reset command");
      BXRS_BOOL_D(bx_bool, start,    "START - start the NIC");
      BXRS_BOOL_D(bx_bool, tx_packet,"TXP - initiate packet transmission");
      BXRS_NUM_D (Bit8u  , rdma_cmd, "RD0,RD1,RD2 - Remote DMA command");
      BXRS_NUM_D (Bit8u  , pgsel,    "PS0,PS1 - Page select");
    }
    BXRS_STRUCT_END;
      
    BXRS_STRUCT_START_D(struct ISR_t, ISR, "Interrupt Status Register - 07h read/write");
    {
      BXRS_BOOL_D(bx_bool, pkt_rx,   "PRX - packet received with no errors");
      BXRS_BOOL_D(bx_bool, pkt_tx,   "PTX - packet transmitted with no errors");
      BXRS_BOOL_D(bx_bool, rx_err,   "RXE - packet received with 1 or more errors");
      BXRS_BOOL_D(bx_bool, tx_err,   "TXE - packet tx'd       \"  \" \"    \"    \"");
      BXRS_BOOL_D(bx_bool, overwrite,"OVW - rx buffer resources exhausted");
      BXRS_BOOL_D(bx_bool, cnt_oflow,"CNT - network tally counter MSB's set");
      BXRS_BOOL_D(bx_bool, rdma_done,"RDC - remote DMA complete");
      BXRS_BOOL_D(bx_bool, reset,    "RST - reset status");
    }
    BXRS_STRUCT_END;

    BXRS_STRUCT_START_D(struct IMR_t, IMR, "Interrupt Mask Register - 0fh write");
    {
      BXRS_BOOL_D(bx_bool, rx_inte,   "PRXE - packet rx interrupt enable");
      BXRS_BOOL_D(bx_bool, tx_inte,   "PTXE - packet tx interrput enable");
      BXRS_BOOL_D(bx_bool, rxerr_inte,"RXEE - rx error interrupt enable");
      BXRS_BOOL_D(bx_bool, txerr_inte,"TXEE - tx error interrupt enable");
      BXRS_BOOL_D(bx_bool, overw_inte,"OVWE - overwrite warn int enable");
      BXRS_BOOL_D(bx_bool, cofl_inte, "CNTE - counter o'flow int enable");
      BXRS_BOOL_D(bx_bool, rdma_inte, "RDCE - remote DMA complete int enable");
      BXRS_BOOL_D(bx_bool, reserved,  " D7 - reserved");
    }
    BXRS_STRUCT_END;

    BXRS_STRUCT_START_D(struct DCR_t, DCR, "Data Configuration Register - 0eh write");
    {
      BXRS_BOOL_D(bx_bool, wdsize,   "WTS - 8/16-bit select");
      BXRS_BOOL_D(bx_bool, endian,   "BOS - byte-order select");
      BXRS_BOOL_D(bx_bool, longaddr, "LAS - long-address select");
      BXRS_BOOL_D(bx_bool, loop,     "LS  - loopback select");
      BXRS_BOOL_D(bx_bool, auto_rx,  "AR  - auto-remove rx packets with remote DMA");
      BXRS_NUM_D (Bit8u  , fifo_size, "FT0,FT1 - fifo threshold");
    }
    BXRS_STRUCT_END;

    BXRS_STRUCT_START_D(struct TCR_t, TCR, "Transmit Configuration Register - 0dh write");
    {
      BXRS_BOOL_D(bx_bool, crc_disable,"CRC - inhibit tx CRC");
      BXRS_NUM_D (Bit8u  , loop_cntl,  "LB0,LB1 - loopback control");
      BXRS_BOOL_D(bx_bool, ext_stoptx, "ATD - allow tx disable by external mcast");
      BXRS_BOOL_D(bx_bool, coll_prio,  "OFST - backoff algorithm select");
      BXRS_NUM_D (Bit8u  , reserved,   " D5,D6,D7 - reserved");
    }
    BXRS_STRUCT_END;

    BXRS_STRUCT_START_D(struct TSR_t,  TSR, "Transmit Status Register - 04h read");
    {
      BXRS_BOOL_D(bx_bool, tx_ok,	    "PTX - tx complete without error");
      BXRS_BOOL_D(bx_bool, reserved,  " D1 - reserved");
      BXRS_BOOL_D(bx_bool, collided,  "COL - tx collided >= 1 times");
      BXRS_BOOL_D(bx_bool, aborted,   "ABT - aborted due to excessive collisions");
      BXRS_BOOL_D(bx_bool, no_carrier,"CRS - carrier-sense lost");
      BXRS_BOOL_D(bx_bool, fifo_ur,   "FU  - FIFO underrun");
      BXRS_BOOL_D(bx_bool, cd_hbeat,  "CDH - no tx cd-heartbeat from transceiver");
      BXRS_BOOL_D(bx_bool, ow_coll,   "OWC - out-of-window collision");
    }
    BXRS_STRUCT_END;

    BXRS_STRUCT_START_D(struct RCR_t, RCR, "Receive Configuration Register - 0ch write");
    {
      BXRS_BOOL_D(bx_bool, errors_ok,"SEP - accept pkts with rx errors");
      BXRS_BOOL_D(bx_bool, runts_ok, "AR  - accept < 64-byte runts");
      BXRS_BOOL_D(bx_bool, broadcast,"AB  - accept eth broadcast address");
      BXRS_BOOL_D(bx_bool, multicast,"AM  - check mcast hash array");
      BXRS_BOOL_D(bx_bool, promisc,  "PRO - accept all packets");
      BXRS_BOOL_D(bx_bool, monitor,  "MON - check pkts, but don't rx");
      BXRS_NUM_D (Bit8u  , reserved, " D6,D7 - reserved");
    }
    BXRS_STRUCT_END;

    BXRS_STRUCT_START_D(struct RSR_t, RSR, "Receive Status Register - 0ch read");
    {
      BXRS_BOOL_D(bx_bool, rx_ok,      "PRX - rx complete without error");
      BXRS_BOOL_D(bx_bool, bad_crc,    "CRC - Bad CRC detected");
      BXRS_BOOL_D(bx_bool, bad_falign, "FAE - frame alignment error");
      BXRS_BOOL_D(bx_bool, fifo_or,    "FO  - FIFO overrun");
      BXRS_BOOL_D(bx_bool, rx_missed,  "MPA - missed packet error");
      BXRS_BOOL_D(bx_bool, rx_mbit,    "PHY - unicast or mcast/bcast address match");
      BXRS_BOOL_D(bx_bool, rx_disabled,"DIS - set when in monitor mode");
      BXRS_BOOL_D(bx_bool, deferred,   "DFR - collision active");
    }
    BXRS_STRUCT_END;

    BXRS_NUM_D(Bit16u , local_dma,    "01,02h read , current local DMA addr");
    BXRS_NUM_D(Bit8u  , page_start,   "01h write , page start register");
    BXRS_NUM_D(Bit8u  , page_stop,    "02h write , page stop register");
    BXRS_NUM_D(Bit8u  , bound_ptr,    "03h read/write , boundary pointer");
    BXRS_NUM_D(Bit8u  , tx_page_start,"04h write , transmit page start register");
    BXRS_NUM_D(Bit8u  , num_coll,     "05h read  , number-of-collisions register");
    BXRS_NUM_D(Bit16u , tx_bytes,     "05,06h write , transmit byte-count register");
    BXRS_NUM_D(Bit8u  , fifo,         "06h read  , FIFO");
    BXRS_NUM_D(Bit16u , remote_dma,   "08,09h read , current remote DMA addr");
    BXRS_NUM_D(Bit16u , remote_start, "08,09h write , remote start address register");
    BXRS_NUM_D(Bit16u , remote_bytes, "0a,0bh write , remote byte-count register");
    BXRS_NUM_D(Bit8u  , tallycnt_0,   "0dh read  , tally counter 0 (frame align errors)");
    BXRS_NUM_D(Bit8u  , tallycnt_1,   "0eh read  , tally counter 1 (CRC errors)");
    BXRS_NUM_D(Bit8u  , tallycnt_2,   "0fh read  , tally counter 2 (missed pkt errors)");
    BXRS_ARRAY_NUM_D(Bit8u,  physaddr, 6,  "01-06h read/write , MAC address");
    BXRS_NUM_D(Bit8u  , curr_page,    "07h read/write , current page register");
    BXRS_ARRAY_NUM_D(Bit8u,  mchash, 8,    "08-0fh read/write , multicast hash array");
    BXRS_NUM_D(Bit8u  , rempkt_ptr,   "03h read/write , remote next-packet pointer");
    BXRS_NUM_D(Bit8u  , localpkt_ptr, "05h read/write , local next-packet pointer");
    BXRS_NUM_D(Bit16u , address_cnt,  "06,07h read/write , address counter");


    BXRS_ARRAY_NUM_D(Bit8u, macaddr, 32, "ASIC ROM'd MAC address, even bytes");
    BXRS_ARRAY_NUM_D(Bit8u, mem, BX_NE2K_MEMSIZ,  "on-chip packet memory");

    BXRS_NUM(Bit32u, base_address);
    BXRS_NUM(int   , base_irq);
    BXRS_NUM(int   , tx_timer_index);
    BXRS_NUM(int   , tx_timer_active);
  }
  BXRS_END;
}
#endif /* if BX_NE2K_SUPPORT */
