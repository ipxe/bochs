// $Id: devices.cc,v 1.34.2.16 2002-10-20 20:49:04 cbothamy Exp $
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


#include "bochs.h"
#define LOG_THIS bx_devices.



/* main memory size (in Kbytes)
 * subtract 1k for extended BIOS area
 * report only base memory, not extended mem
 */
#define BASE_MEMORY_IN_K  640


bx_devices_c bx_devices;

// constructor for bx_devices_c
bx_devices_c::bx_devices_c(void)
{
  put("DEV");
  settype(DEVLOG);

#if BX_PCI_SUPPORT
  pci = NULL;
  pci2isa = NULL;
#endif

  pit = NULL;
  sb16 = NULL;
  ne2k = NULL;
  g2h = NULL;
#if BX_IODEBUG_SUPPORT
  iodebug = NULL;
#endif
  pluginKeyboard = &stubKeyboard;
  pluginHardDrive = &stubHardDrive;
  pluginSerialDevice = NULL;
  pluginParallelDevice = NULL;
  pluginUnmapped = NULL;
  pluginBiosDevice = NULL;
  pluginCmosDevice = &stubCmos;
  pluginDmaDevice = NULL;
  pluginPicDevice = NULL;
  pluginVgaDevice = NULL;
  pluginFloppyDevice = NULL;
}


bx_devices_c::~bx_devices_c(void)
{
  // nothing needed for now
  BX_DEBUG(("Exit."));
}


  void
bx_devices_c::init(BX_MEM_C *newmem)
{
  unsigned i;

  BX_DEBUG(("Init $Id: devices.cc,v 1.34.2.16 2002-10-20 20:49:04 cbothamy Exp $"));
  mem = newmem;

  /* no read / write handlers defined */
  num_read_handles = 0;
  num_write_handles = 0;

  /* default read/write handler */
  default_read_handler_id = -1;
  default_write_handler_id = -1;

  /* set unused elements to appropriate values */
  for (i=0; i < BX_MAX_IO_DEVICES + 1; i++) {
    io_read_handler[i].funct  = NULL;
    io_write_handler[i].funct = NULL;
    }

  for (i=0; i < 0x10000; i++) {
    read_handler_id[i] = BX_MAX_IO_DEVICES;  // not assigned
    write_handler_id[i] = BX_MAX_IO_DEVICES;  // not assigned
    }

  for (i=0; i < BX_MAX_IRQS; i++) {
    irq_handler_name[i] = NULL;
    }

  timer_handle = BX_NULL_TIMER_HANDLE;

  // unmapped is core because it must be initialized before anybody else
  BX_LOAD_PLUGIN(unmapped, PLUGTYPE_CORE);

#warning these should only be loaded if they are enabled.
#warning and they should only be optional if the init order is irrelevant
  BX_LOAD_PLUGIN(biosdev, PLUGTYPE_CORE);
  BX_LOAD_PLUGIN(cmos, PLUGTYPE_CORE);
  BX_LOAD_PLUGIN(dma, PLUGTYPE_CORE);
  BX_LOAD_PLUGIN(pic, PLUGTYPE_CORE);
  BX_LOAD_PLUGIN(vga, PLUGTYPE_CORE);
  /// optional plugins
  BX_LOAD_PLUGIN(floppy, PLUGTYPE_OPTIONAL);
  BX_LOAD_PLUGIN(harddrv, PLUGTYPE_OPTIONAL);
  BX_LOAD_PLUGIN(keyboard, PLUGTYPE_OPTIONAL);
  if (is_serial_enabled ())
    BX_LOAD_PLUGIN(serial, PLUGTYPE_OPTIONAL);
  if (is_parallel_enabled ()) 
    BX_LOAD_PLUGIN(parallel, PLUGTYPE_OPTIONAL);


  // Start with all IO port address registered to unmapped handler
  // MUST be called first
  pluginUnmapped->init ();

  // BIOS log 
  pluginBiosDevice->init ();

  // CMOS RAM & RTC
  pluginCmosDevice->init ();

#if BX_SUPPORT_VGA
  /*--- VGA adapter ---*/
  pluginVgaDevice->init ();
#else
  /*--- HGA adapter ---*/
  bx_init_hga_hardware();
#endif

#if BX_PCI_SUPPORT
  // PCI logic (i440FX)
  pci = & bx_pci;
  pci->init();
  pci2isa = & bx_pci2isa;
  pci2isa->init();
#endif

#if BX_SUPPORT_APIC
    // I/O APIC 82093AA
    ioapic = & bx_ioapic;
    ioapic->init (this);
#endif

  /*--- 8237 DMA ---*/
  pluginDmaDevice->init();

  /*--- 8259A PIC ---*/
  pluginPicDevice->init();

#if BX_SUPPORT_SB16
  //--- SOUND ---
  sb16 = &bx_sb16;
  sb16->init();
#endif

  /*--- 8254 PIT ---*/
  pit = & bx_pit;
  pit->init();

#if BX_USE_SLOWDOWN_TIMER
  bx_slowdown_timer.init();
#endif

#if BX_IODEBUG_SUPPORT
  iodebug = &bx_iodebug;
  iodebug->init();
#endif

#if BX_NE2K_SUPPORT
  // NE2000 NIC
  ne2k = &bx_ne2k;
  ne2k->init();
  BX_DEBUG(("ne2k"));
#endif  // #if BX_NE2K_SUPPORT

#if 0
  // Guest to Host interface.  Used with special guest drivers
  // which move data to/from the host environment.
  g2h = &bx_g2h;
  g2h->init();
#endif

  // system hardware
  register_io_read_handler( this,
                            &read_handler,
                            0x0092,
                            "Port 92h System Control" );
  register_io_write_handler(this,
                            &write_handler,
                            0x0092,
                            "Port 92h System Control" );

  // misc. CMOS
  Bit16u extended_memory_in_k = mem->get_memory_in_k() - 1024;
  BX_SET_CMOS_REG(0x15, (Bit8u) BASE_MEMORY_IN_K);
  BX_SET_CMOS_REG(0x16, (Bit8u) (BASE_MEMORY_IN_K >> 8));
  BX_SET_CMOS_REG(0x17, (Bit8u) extended_memory_in_k);
  BX_SET_CMOS_REG(0x18, (Bit8u) (extended_memory_in_k >> 8));
  BX_SET_CMOS_REG(0x30, (Bit8u) extended_memory_in_k);
  BX_SET_CMOS_REG(0x31, (Bit8u) (extended_memory_in_k >> 8));

  /* now perform checksum of CMOS memory */
  BX_CMOS_CHECKSUM();

  timer_handle = bx_pc_system.register_timer( this, timer_handler,
    (unsigned) BX_IODEV_HANDLER_PERIOD, 1, 1, "devices.cc");

  // Clear fields for bulk IO acceleration transfers.
  bulkIOHostAddr = 0;
  bulkIOQuantumsRequested = 0;
  bulkIOQuantumsTransferred = 0;

  bx_init_plugins();
}


  void
bx_devices_c::reset(unsigned type)
{
#if BX_PCI_SUPPORT
  pci->reset(type);
  pci2isa->reset(type);
#endif


#  if BX_SUPPORT_IOAPIC
    ioapic->reset (type);
#  endif

#if !BX_PLUGINS 

  pluginDmaDevice->reset(type);
  pluginCmosDevice->reset(type);
  pluginBiosDevice->reset(type);
  pluginUnmapped->reset(type);

#if BX_SUPPORT_VGA
  pluginVgaDevice->reset(type);
#else
  // reset hga hardware?
#endif

  pluginPicDevice->reset(type);

#endif

#if BX_SUPPORT_SB16
  sb16->reset(type);
#endif
  pit->reset(type);
#if BX_USE_SLOWDOWN_TIMER
  bx_slowdown_timer.reset(type);
#endif
#if BX_IODEBUG_SUPPORT
  iodebug->reset(type);
#endif
#if BX_NE2K_SUPPORT
  ne2k->reset(type);
#endif

  bx_reset_plugins(type);
}


  Bit32u
bx_devices_c::read_handler(void *this_ptr, Bit32u address, unsigned io_len)
{
#if !BX_USE_DEV_SMF
  bx_devices_c *class_ptr = (bx_devices_c *) this_ptr;

  return( class_ptr->port92_read(address, io_len) );
}


  Bit32u
bx_devices_c::port92_read(Bit32u address, unsigned io_len)
{
#else
  UNUSED(this_ptr);
#endif  // !BX_USE_DEV_SMF
  if (io_len > 1)
    BX_PANIC(("port 92h: io read from address %08x, len=%u",
             (unsigned) address, (unsigned) io_len));

  BX_DEBUG(("port92h read partially supported!!!"));
  BX_DEBUG(("  returning %02x", (unsigned) (BX_GET_ENABLE_A20() << 1)));
  return(BX_GET_ENABLE_A20() << 1);
}


  void
bx_devices_c::write_handler(void *this_ptr, Bit32u address, Bit32u value, unsigned io_len)
{
#if !BX_USE_DEV_SMF
  bx_devices_c *class_ptr = (bx_devices_c *) this_ptr;

  class_ptr->port92_write(address, value, io_len);
}

  void
bx_devices_c::port92_write(Bit32u address, Bit32u value, unsigned io_len)
{
#else
  UNUSED(this_ptr);
#endif  // !BX_USE_DEV_SMF
  Boolean bx_cpu_reset;

  if (io_len > 1)
    BX_PANIC(("port 92h: io read from address %08x, len=%u",
             (unsigned) address, (unsigned) io_len));

  BX_DEBUG(("port92h write of %02x partially supported!!!",
    (unsigned) value));
  BX_DEBUG(("A20: set_enable_a20() called"));
  BX_SET_ENABLE_A20( (value & 0x02) >> 1 );
  BX_DEBUG(("A20: now %u", (unsigned) BX_GET_ENABLE_A20()));
  bx_cpu_reset  = (value & 0x01); /* high speed reset */
  if (bx_cpu_reset) {
    BX_PANIC(("PORT 92h write: CPU reset requested!"));
    }
}

  void
bx_devices_c::timer_handler(void *this_ptr)
{
  bx_devices_c *class_ptr = (bx_devices_c *) this_ptr;

  class_ptr->timer();
}

  void
bx_devices_c::timer()
{
#if (BX_USE_NEW_PIT==0)
  if ( pit->periodic( BX_IODEV_HANDLER_PERIOD ) ) {
    // This is a hack to make the IRQ0 work
    BX_PIC_LOWER_IRQ(0);
    BX_PIC_RAISE_IRQ(0);
    }
#endif


  // separate calls to bx_gui->handle_events from the keyboard code.
  {
    static int multiple=0;
    if ( ++multiple==10)
    {
      multiple=0;
      bx_gui->handle_events();
    }
  }

// KPL Removed lapic periodic timer registration here.
}


  Boolean
bx_devices_c::register_irq(unsigned irq, const char *name)
{
  if (irq >= BX_MAX_IRQS) {
    BX_PANIC(("IO device %s registered with IRQ=%d above %u",
             name, irq, (unsigned) BX_MAX_IRQS-1));
    return false;
    }
  if (irq_handler_name[irq]) {
    BX_PANIC(("IRQ %u conflict, %s with %s", irq,
      irq_handler_name[irq], name));
    return false;
    }
  irq_handler_name[irq] = name;
  return true;
}

  Boolean
bx_devices_c::unregister_irq(unsigned irq, const char *name)
{
  if (irq >= BX_MAX_IRQS) {
    BX_PANIC(("IO device %s tried to unregister IRQ %d above %u",
             name, irq, (unsigned) BX_MAX_IRQS-1));
    return false;
    }

  if (!irq_handler_name[irq]) {
    BX_INFO(("IO device %s tried to unregister IRQ %d, not registered",
	      name, irq));
    return false;
  }

  if (strcmp(irq_handler_name[irq], name)) {
    BX_INFO(("IRQ %u not registered to %s but to %s", irq,
      name, irq_handler_name[irq]));
    return false;
    }
  irq_handler_name[irq] = NULL;
  return true;
}

  Boolean
bx_devices_c::register_io_read_handler( void *this_ptr, bx_read_handler_t f,
                                        Bit32u addr, const char *name )
{
  unsigned handle;

  addr &= 0x0000ffff;

  /* first find existing handle for function or create new one */
  for (handle=0; handle < num_read_handles; handle++) {
    if (io_read_handler[handle].funct == f) break;
    }

  if (handle >= num_read_handles) {
    /* no existing handle found, create new one */
    if (num_read_handles >= BX_MAX_IO_DEVICES) {
      BX_INFO(("too many IO devices installed."));
      BX_PANIC(("  try increasing BX_MAX_IO_DEVICES"));
      }
    num_read_handles++;
    io_read_handler[handle].funct          = f;
    io_read_handler[handle].this_ptr       = this_ptr;
    io_read_handler[handle].handler_name   = name;
    }

  /* change table to reflect new handler id for that address */
  if (read_handler_id[addr] < BX_MAX_IO_DEVICES) {
    // another handler is already registered for that address

    // if it is not the Unmapped port handler, bail
    if ( strcmp( io_read_handler[read_handler_id[addr]].handler_name, "Unmapped" ) ) {
      BX_INFO(("IO device address conflict(read) at IO address %Xh",
        (unsigned) addr));
      BX_INFO(("  conflicting devices: %s & %s",
        io_read_handler[handle].handler_name, io_read_handler[read_handler_id[addr]].handler_name));
      return false; // address not available, return false.
      }
    }
  read_handler_id[addr] = handle;
  return true; // address mapped successfully
}



  Boolean
bx_devices_c::register_io_write_handler( void *this_ptr, bx_write_handler_t f,
                                        Bit32u addr, const char *name )
{
  unsigned handle;

  addr &= 0x0000ffff;

  /* first find existing handle for function or create new one */
  for (handle=0; handle < num_write_handles; handle++) {
    if (io_write_handler[handle].funct == f) break;
    }

  if (handle >= num_write_handles) {
    /* no existing handle found, create new one */
    if (num_write_handles >= BX_MAX_IO_DEVICES) {
      BX_INFO(("too many IO devices installed."));
      BX_PANIC(("  try increasing BX_MAX_IO_DEVICES"));
      }
    num_write_handles++;
    io_write_handler[handle].funct          = f;
    io_write_handler[handle].this_ptr       = this_ptr;
    io_write_handler[handle].handler_name   = name;
    }

  /* change table to reflect new handler id for that address */
  if (write_handler_id[addr] < BX_MAX_IO_DEVICES) {
    // another handler is already registered for that address
 
    // if it is not the Unmapped port handler, bail
    if ( strcmp( io_write_handler[write_handler_id[addr]].handler_name, "Unmapped" ) ) {
      BX_INFO(("IO device address conflict(write) at IO address %Xh",
        (unsigned) addr));
      BX_INFO(("  conflicting devices: %s & %s",
        io_write_handler[handle].handler_name, io_write_handler[write_handler_id[addr]].handler_name));
      return false; //unable to map iodevice.
      }
    }
  write_handler_id[addr] = handle;
  return true; // done!
}


// Registration of default handlers (mainly be the unmapped device)
// The trick here is to define a handler for the max index, so
// unregisterd io address will get handled ny the default function
// This will be helpful when we want to unregister io handlers

  Boolean
bx_devices_c::register_default_io_read_handler( void *this_ptr, bx_read_handler_t f,
                                        const char *name )
{
  unsigned handle;

  /* handle is fixed to the MAX */
  handle = BX_MAX_IO_DEVICES;

  if (io_read_handler[handle].funct != NULL) {
    BX_INFO(("Default io read handler already registered '%s'",io_read_handler[handle].handler_name));
    return false;
    }

  io_read_handler[handle].funct          = f;
  io_read_handler[handle].this_ptr       = this_ptr;
  io_read_handler[handle].handler_name   = name;

  return true; 
}



  Boolean
bx_devices_c::register_default_io_write_handler( void *this_ptr, bx_write_handler_t f,
                                        const char *name )
{
  unsigned handle;

  /* handle is fixed to the MAX */
  handle = BX_MAX_IO_DEVICES;

  if (io_write_handler[handle].funct != NULL) {
    BX_INFO(("Default io write handler already registered '%s'",io_write_handler[handle].handler_name));
    return false;
    }

  io_write_handler[handle].funct          = f;
  io_write_handler[handle].this_ptr       = this_ptr;
  io_write_handler[handle].handler_name   = name;

  return true; 
}



/*
 * Read a byte of data from the IO memory address space
 */

  Bit32u
bx_devices_c::inp(Bit16u addr, unsigned io_len)
{
  Bit8u handle;
  Bit32u ret;

  BX_INSTR_INP(addr, io_len);

  handle = read_handler_id[addr];
  // FIXME, we want to make sure that there is a default handler there
  ret = (* io_read_handler[handle].funct)(io_read_handler[handle].this_ptr,
                           (Bit32u) addr, io_len);
  BX_INSTR_INP2(addr, io_len, ret);
  BX_DBG_IO_REPORT(addr, io_len, BX_READ, ret);
  return(ret);
}


/*
 * Write a byte of data to the IO memory address space.
 */

  void
bx_devices_c::outp(Bit16u addr, Bit32u value, unsigned io_len)
{
  Bit8u handle;

  BX_INSTR_OUTP(addr, io_len);
  BX_INSTR_OUTP2(addr, io_len, value);

  BX_DBG_IO_REPORT(addr, io_len, BX_WRITE, value);
  handle = write_handler_id[addr];
  // FIXME, we want to make sure that there is a default handler there
  (* io_write_handler[handle].funct)(io_write_handler[handle].this_ptr,
                     (Bit32u) addr, value, io_len);
}

Boolean bx_devices_c::is_serial_enabled ()
{
  for (int i=0; i<BX_N_SERIAL_PORTS; i++) {
    if (SIM->get_param_bool (BXP_COMx_ENABLED(i+1))->get())
      return true;
  }
  return false;
}

Boolean bx_devices_c::is_parallel_enabled ()
{
  for (int i=0; i<BX_N_PARALLEL_PORTS; i++) {
    if (SIM->get_param_bool (BXP_PARPORTx_ENABLED(i+1))->get())
      return true;
  }
  return false;
}
