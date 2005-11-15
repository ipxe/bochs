/////////////////////////////////////////////////////////////////////////
// $Id: pci.cc,v 1.41 2005/11/15 17:19:28 vruppert Exp $
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
// i440FX Support - PMC/DBX
//

// Define BX_PLUGGABLE in files that can be compiled into plugins.  For
// platforms that require a special tag on exported symbols, BX_PLUGGABLE 
// is used to know when we are exporting symbols and when we are importing.
#define BX_PLUGGABLE

#include "iodev.h"
#if BX_SUPPORT_PCI

#define LOG_THIS thePciBridge->

bx_pci_c *thePciBridge = NULL;

  int
libpci_LTX_plugin_init(plugin_t *plugin, plugintype_t type, int argc, char *argv[])
{
  thePciBridge = new bx_pci_c ();
  bx_devices.pluginPciBridge = thePciBridge;
  BX_REGISTER_DEVICE_DEVMODEL(plugin, type, thePciBridge, BX_PLUGIN_PCI);
  return(0); // Success
}

  void
libpci_LTX_plugin_fini(void)
{
}

bx_pci_c::bx_pci_c(void)
{
  put("PCI");
  settype(PCILOG);
}

bx_pci_c::~bx_pci_c(void)
{
  // nothing for now
  BX_DEBUG(("Exit."));
}


  void
bx_pci_c::init(void)
{
  // called once when bochs initializes
  unsigned i;
  BX_PCI_THIS num_pci_handles = 0;

  /* set unused elements to appropriate values */
  for (i=0; i < BX_MAX_PCI_DEVICES; i++) {
    BX_PCI_THIS pci_handler[i].read  = NULL;
    BX_PCI_THIS pci_handler[i].write = NULL;
    }

  for (i=0; i < 0x100; i++) {
    BX_PCI_THIS pci_handler_id[i] = BX_MAX_PCI_DEVICES;  // not assigned
  }

  for (i=0; i < BX_N_PCI_SLOTS; i++) {
    BX_PCI_THIS slot_used[i] = 0;  // no device connected
  }
  BX_PCI_THIS slots_checked = 0;

  // confAddr accepts dword i/o only
  DEV_register_ioread_handler(this, read_handler, 0x0CF8, "i440FX", 4);
  DEV_register_iowrite_handler(this, write_handler, 0x0CF8, "i440FX", 4);

  for (i=0x0CFC; i<=0x0CFF; i++) {
    DEV_register_ioread_handler(this, read_handler, i, "i440FX", 7);
  }
  for (i=0x0CFC; i<=0x0CFF; i++) {
    DEV_register_iowrite_handler(this, write_handler, i, "i440FX", 7);
  }

  Bit8u devfunc = BX_PCI_DEVICE(0,0);
  DEV_register_pci_handlers(this, pci_read_handler, pci_write_handler,
                            &devfunc, BX_PLUGIN_PCI, "440FX Host bridge");

  for (i=0; i<256; i++)
    BX_PCI_THIS s.i440fx.pci_conf[i] = 0x0;
  // readonly registers
  BX_PCI_THIS s.i440fx.pci_conf[0x00] = 0x86;
  BX_PCI_THIS s.i440fx.pci_conf[0x01] = 0x80;
  BX_PCI_THIS s.i440fx.pci_conf[0x02] = 0x37;
  BX_PCI_THIS s.i440fx.pci_conf[0x03] = 0x12;
  BX_PCI_THIS s.i440fx.pci_conf[0x0b] = 0x06;
}

  void
bx_pci_c::reset(unsigned type)
{
  unsigned i;

  if (!BX_PCI_THIS slots_checked) {
    for (i=0; i<BX_N_PCI_SLOTS; i++) {
      if (bx_options.pcislot[i].Oused->get() && !BX_PCI_THIS slot_used[i]) {
        BX_PANIC(("Unknown plugin '%s' at PCI slot #%d", bx_options.pcislot[i].Odevname->getptr(), i+1));
      }
    }
    BX_PCI_THIS slots_checked = 1;
  }

  BX_PCI_THIS s.i440fx.confAddr = 0;
  BX_PCI_THIS s.i440fx.confData = 0;

  BX_PCI_THIS s.i440fx.pci_conf[0x04] = 0x06;
  BX_PCI_THIS s.i440fx.pci_conf[0x05] = 0x00;
  BX_PCI_THIS s.i440fx.pci_conf[0x06] = 0x80;
  BX_PCI_THIS s.i440fx.pci_conf[0x07] = 0x02;
  BX_PCI_THIS s.i440fx.pci_conf[0x0d] = 0x00;
  BX_PCI_THIS s.i440fx.pci_conf[0x0f] = 0x00;
  BX_PCI_THIS s.i440fx.pci_conf[0x50] = 0x00;
  BX_PCI_THIS s.i440fx.pci_conf[0x51] = 0x01;
  BX_PCI_THIS s.i440fx.pci_conf[0x52] = 0x00;
  BX_PCI_THIS s.i440fx.pci_conf[0x53] = 0x80;
  BX_PCI_THIS s.i440fx.pci_conf[0x54] = 0x00;
  BX_PCI_THIS s.i440fx.pci_conf[0x55] = 0x00;
  BX_PCI_THIS s.i440fx.pci_conf[0x56] = 0x00;
  BX_PCI_THIS s.i440fx.pci_conf[0x57] = 0x01;
  BX_PCI_THIS s.i440fx.pci_conf[0x58] = 0x10;
  for (i=0x59; i<0x60; i++)
    BX_PCI_THIS s.i440fx.pci_conf[i] = 0x00;
}



  // static IO port read callback handler
  // redirects to non-static class handler to avoid virtual functions

  Bit32u
bx_pci_c::read_handler(void *this_ptr, Bit32u address, unsigned io_len)
{
#if !BX_USE_PCI_SMF
  bx_pci_c *class_ptr = (bx_pci_c *) this_ptr;

  return( class_ptr->read(address, io_len) );
}


  Bit32u
bx_pci_c::read(Bit32u address, unsigned io_len)
{
#else
  UNUSED(this_ptr);
#endif // !BX_USE_PCI_SMF

  switch (address) {
    case 0x0CF8:
      {
        return BX_PCI_THIS s.i440fx.confAddr;
      }
      break;
    case 0x0CFC:
    case 0x0CFD:
    case 0x0CFE:
    case 0x0CFF:
      {
      Bit32u handle, retval;
      Bit8u devfunc, regnum;

      if ((BX_PCI_THIS s.i440fx.confAddr & 0x80FF0000) == 0x80000000) {
        devfunc = (BX_PCI_THIS s.i440fx.confAddr >> 8) & 0xff;
        regnum = (BX_PCI_THIS s.i440fx.confAddr & 0xfc) + (address & 0x03);
        handle = BX_PCI_THIS pci_handler_id[devfunc];
        if ((io_len <= 4) && (handle < BX_MAX_PCI_DEVICES))
          retval = (* BX_PCI_THIS pci_handler[handle].read)
                     (BX_PCI_THIS pci_handler[handle].this_ptr, regnum, io_len);
        else
          retval = 0xFFFFFFFF;
        }
      else
        retval = 0xFFFFFFFF;
      BX_PCI_THIS s.i440fx.confData = retval;
      return retval;
      }
    }

  BX_PANIC(("unsupported IO read to port 0x%x",
           (unsigned) address));
  return(0xffffffff);
}


  // static IO port write callback handler
  // redirects to non-static class handler to avoid virtual functions

  void
bx_pci_c::write_handler(void *this_ptr, Bit32u address, Bit32u value, unsigned io_len)
{
#if !BX_USE_PCI_SMF
  bx_pci_c *class_ptr = (bx_pci_c *) this_ptr;

  class_ptr->write(address, value, io_len);
}

  void
bx_pci_c::write(Bit32u address, Bit32u value, unsigned io_len)
{
#else
  UNUSED(this_ptr);
#endif // !BX_USE_PCI_SMF

  switch (address) {
    case 0xCF8:
      {
        BX_PCI_THIS s.i440fx.confAddr = value;
        if ((value & 0x80FFFF00) == 0x80000000) {
          BX_DEBUG(("440FX PMC register 0x%02x selected", value & 0xfc));
        } else if ((value & 0x80000000) == 0x80000000) {
          BX_DEBUG(("440FX request for bus 0x%02x device 0x%02x function 0x%02x",
                    (value >> 16) & 0xFF, (value >> 11) & 0x1F, (value >> 8) & 0x07));
        }
      }
      break;

    case 0xCFC:
    case 0xCFD:
    case 0xCFE:
    case 0xCFF:
      {
      Bit32u handle;
      Bit8u devfunc, regnum;

      if ((BX_PCI_THIS s.i440fx.confAddr & 0x80FF0000) == 0x80000000) {
        devfunc = (BX_PCI_THIS s.i440fx.confAddr >> 8) & 0xff;
        regnum = (BX_PCI_THIS s.i440fx.confAddr & 0xfc) + (address & 0x03);
        handle = BX_PCI_THIS pci_handler_id[devfunc];
        if ((io_len <= 4) && (handle < BX_MAX_PCI_DEVICES)) {
          if (((regnum>=4) && (regnum<=7)) || (regnum==12) || (regnum==13) || (regnum>14)) {
            (* BX_PCI_THIS pci_handler[handle].write)
               (BX_PCI_THIS pci_handler[handle].this_ptr, regnum, value, io_len);
            BX_PCI_THIS s.i440fx.confData = value << (8 * (address & 0x03));
            }
          else
            BX_DEBUG(("read only register, write ignored"));
          }
        }
      }
      break;

    default:
      BX_PANIC(("IO write to port 0x%x", (unsigned) address));
    }
}


  // static pci configuration space read callback handler
  // redirects to non-static class handler to avoid virtual functions

  Bit32u
bx_pci_c::pci_read_handler(void *this_ptr, Bit8u address, unsigned io_len)
{
#if !BX_USE_PCI_SMF
  bx_pci_c *class_ptr = (bx_pci_c *) this_ptr;

  return( class_ptr->pci_read(address, io_len) );
}


  Bit32u
bx_pci_c::pci_read(Bit8u address, unsigned io_len)
{
#else
  UNUSED(this_ptr);
#endif // !BX_USE_PCI_SMF

  Bit32u val440fx = 0;

  if (io_len <= 4) {
    for (unsigned i=0; i<io_len; i++) {
      val440fx |= (BX_PCI_THIS s.i440fx.pci_conf[address+i] << (i*8));
    }
    BX_DEBUG(("440FX PMC read register 0x%02x value 0x%08x", address, val440fx));
    return val440fx;
    }
  else
    return(0xffffffff);
}


  // static pci configuration space write callback handler
  // redirects to non-static class handler to avoid virtual functions

  void
bx_pci_c::pci_write_handler(void *this_ptr, Bit8u address, Bit32u value, unsigned io_len)
{
#if !BX_USE_PCI_SMF
  bx_pci_c *class_ptr = (bx_pci_c *) this_ptr;

  class_ptr->pci_write(address, value, io_len);
}

  void
bx_pci_c::pci_write(Bit8u address, Bit32u value, unsigned io_len)
{
#else
  UNUSED(this_ptr);
#endif // !BX_USE_PCI_SMF

  Bit8u value8;

  if ((address >= 0x10) && (address < 0x34))
    return;
  if (io_len <= 4) {
    for (unsigned i=0; i<io_len; i++) {
      value8 = (value >> (i*8)) & 0xFF;
      switch (address+i) {
        case 0x04:
          BX_PCI_THIS s.i440fx.pci_conf[address+i] = (value8 & 0x40) | 0x06;
          break;
        case 0x06:
        case 0x0c:
          break;
        default:
          BX_PCI_THIS s.i440fx.pci_conf[address+i] = value8;
          BX_DEBUG(("440FX PMC write register 0x%02x value 0x%02x", address+i,
                    value8));
        }
      }
    }
}


  Bit8u
bx_pci_c::rd_memType (Bit32u addr)
{
   switch ((addr & 0xFC000) >> 12) {
      case 0xC0:
           return (BX_PCI_THIS s.i440fx.pci_conf[0x5A] & 0x1);
      case 0xC4:
           return ( (BX_PCI_THIS s.i440fx.pci_conf[0x5A] >> 4) & 0x1);
      case 0xC8:
           return (BX_PCI_THIS s.i440fx.pci_conf[0x5B] & 0x1);
      case 0xCC:
           return ( (BX_PCI_THIS s.i440fx.pci_conf[0x5B] >> 4) & 0x1);


      case 0xD0:
           return (BX_PCI_THIS s.i440fx.pci_conf[0x5C] & 0x1);
      case 0xD4:
           return ( (BX_PCI_THIS s.i440fx.pci_conf[0x5C] >> 4) & 0x1);
      case 0xD8:
           return (BX_PCI_THIS s.i440fx.pci_conf[0x5D] & 0x1);
      case 0xDC:
           return ( (BX_PCI_THIS s.i440fx.pci_conf[0x5D] >> 4) & 0x1);

      case 0xE0:
           return (BX_PCI_THIS s.i440fx.pci_conf[0x5E] & 0x1);
      case 0xE4:
           return ( (BX_PCI_THIS s.i440fx.pci_conf[0x5E] >> 4) & 0x1);
      case 0xE8:
           return (BX_PCI_THIS s.i440fx.pci_conf[0x5F] & 0x1);
      case 0xEC:
           return ( (BX_PCI_THIS s.i440fx.pci_conf[0x5F] >> 4) & 0x1);

      case 0xF0:
      case 0xF4:
      case 0xF8:
      case 0xFC:
           return ( (BX_PCI_THIS s.i440fx.pci_conf[0x59] >> 4) & 0x1);

      default:
           BX_PANIC(("rd_memType () Error: Memory Type not known !"));
           return(0); // keep compiler happy
           break;
   }

}

  Bit8u
bx_pci_c::wr_memType (Bit32u addr)
{
   switch ((addr & 0xFC000) >> 12) {
      case 0xC0:
           return ( (BX_PCI_THIS s.i440fx.pci_conf[0x5A] >> 1) & 0x1);
      case 0xC4:
           return ( (BX_PCI_THIS s.i440fx.pci_conf[0x5A] >> 5) & 0x1);
      case 0xC8:
           return ( (BX_PCI_THIS s.i440fx.pci_conf[0x5B] >> 1) & 0x1);
      case 0xCC:
           return ( (BX_PCI_THIS s.i440fx.pci_conf[0x5B] >> 5) & 0x1);


      case 0xD0:
           return ( (BX_PCI_THIS s.i440fx.pci_conf[0x5C] >> 1) & 0x1);
      case 0xD4:
           return ( (BX_PCI_THIS s.i440fx.pci_conf[0x5C] >> 5) & 0x1);
      case 0xD8:
           return ( (BX_PCI_THIS s.i440fx.pci_conf[0x5D] >> 1) & 0x1);
      case 0xDC:
           return ( (BX_PCI_THIS s.i440fx.pci_conf[0x5D] >> 5) & 0x1);

      case 0xE0:
           return ( (BX_PCI_THIS s.i440fx.pci_conf[0x5E] >> 1) & 0x1);
      case 0xE4:
           return ( (BX_PCI_THIS s.i440fx.pci_conf[0x5E] >> 5) & 0x1);
      case 0xE8:
           return ( (BX_PCI_THIS s.i440fx.pci_conf[0x5F] >> 1) & 0x1);
      case 0xEC:
           return ( (BX_PCI_THIS s.i440fx.pci_conf[0x5F] >> 5) & 0x1);

      case 0xF0:
      case 0xF4:
      case 0xF8:
      case 0xFC:
           return ( (BX_PCI_THIS s.i440fx.pci_conf[0x59] >> 5) & 0x1);

      default:
           BX_PANIC(("wr_memType () Error: Memory Type not known !"));
           return(0); // keep compiler happy
           break;
   }
}

  void
bx_pci_c::print_i440fx_state()
{
  int  i;

  BX_DEBUG(( "i440fxConfAddr:0x%08x", BX_PCI_THIS s.i440fx.confAddr ));
  BX_DEBUG(( "i440fxConfData:0x%08x", BX_PCI_THIS s.i440fx.confData ));

#ifdef DUMP_FULL_I440FX
  for (i=0; i<256; i++) {
    BX_DEBUG(( "i440fxArray%02x:0x%02x", i, BX_PCI_THIS s.i440fx.pci_conf[i] ));
    }
#else /* DUMP_FULL_I440FX */
  for (i=0x59; i<0x60; i++) {
    BX_DEBUG(( "i440fxArray%02x:0x%02x", i, BX_PCI_THIS s.i440fx.pci_conf[i] ));
    }
#endif /* DUMP_FULL_I440FX */
}

  bx_bool
bx_pci_c::register_pci_handlers( void *this_ptr, bx_pci_read_handler_t f1,
                                 bx_pci_write_handler_t f2, Bit8u *devfunc,
                                 const char *name, const char *descr)
{
  unsigned i, handle;

  if (strcmp(name, "pci") && strcmp(name, "pci2isa") && strcmp(name, "pci_ide")
      && (*devfunc == 0x00)) {
    for (i = 0; i < BX_N_PCI_SLOTS; i++) {
      if (bx_options.pcislot[i].Oused->get() &&
          !strcmp(name, bx_options.pcislot[i].Odevname->getptr())) {
        *devfunc = (i + 2) << 3;
        BX_PCI_THIS slot_used[i] = 1;
        BX_INFO(("PCI slot #%d used by plugin '%s'", i+1, name));
        break;
      }
    }
    if (*devfunc == 0x00) {
      BX_ERROR(("Plugin '%s' not connected to a PCI slot", name));
    }
  }
  /* check if device/function is available */
  if (BX_PCI_THIS pci_handler_id[*devfunc] == BX_MAX_PCI_DEVICES) {
    if (BX_PCI_THIS num_pci_handles >= BX_MAX_PCI_DEVICES) {
      BX_INFO(("too many PCI devices installed."));
      BX_PANIC(("  try increasing BX_MAX_PCI_DEVICES"));
      return false;
      }
    handle = BX_PCI_THIS num_pci_handles++;
    BX_PCI_THIS pci_handler[handle].read  = f1;
    BX_PCI_THIS pci_handler[handle].write = f2;
    BX_PCI_THIS pci_handler[handle].this_ptr = this_ptr;
    BX_PCI_THIS pci_handler_id[*devfunc] = handle;
    BX_INFO(("%s present at device %d, function %d", descr, *devfunc >> 3,
             *devfunc & 0x07));
    return true; // device/function mapped successfully
    }
  else {
    return false; // device/function not available, return false.
    }
}


  bx_bool
bx_pci_c::is_pci_device(const char *name)
{
  unsigned i;

  for (i = 0; i < BX_N_PCI_SLOTS; i++) {
    if (bx_options.pcislot[i].Oused->get() &&
        !strcmp(name, bx_options.pcislot[i].Odevname->getptr())) {
      return 1;
    }
  }
  return 0;
}

  bx_bool
bx_pci_c::pci_set_base_mem(void *this_ptr, memory_handler_t f1, memory_handler_t f2,
                           Bit32u *addr, Bit8u *pci_conf, unsigned size)
{
  Bit32u newbase;

  Bit32u oldbase = *addr;
  Bit32u mask = ~(size - 1);
  Bit8u pci_flags = pci_conf[0x00] & 0x0f;
  pci_conf[0x00] &= (mask & 0xf0);
  pci_conf[0x01] &= (mask >> 8) & 0xff;
  pci_conf[0x02] &= (mask >> 16) & 0xff;
  pci_conf[0x03] &= (mask >> 24) & 0xff;
  ReadHostDWordFromLittleEndian(pci_conf, newbase);
  pci_conf[0x00] |= pci_flags;
  if ((newbase != mask) && (newbase != oldbase)) { // skip PCI probe
    if (oldbase > 0) {
      DEV_unregister_memory_handlers(f1, f2, oldbase, oldbase + size - 1);
    }
    if (newbase > 0) {
      DEV_register_memory_handlers(f1, this_ptr, f2, this_ptr, newbase, newbase + size - 1);
    }
    *addr = newbase;
    return 1;
  }
  return 0;
}

  bx_bool
bx_pci_c::pci_set_base_io(void *this_ptr, bx_read_handler_t f1, bx_write_handler_t f2,
                          Bit32u *addr, Bit8u *pci_conf, unsigned size,
                          const Bit8u *iomask, const char *name)
{
  unsigned i;
  Bit32u newbase;

  Bit32u oldbase = *addr;
  Bit16u mask = ~(size - 1);
  Bit8u pci_flags = pci_conf[0x00] & 0x03;
  pci_conf[0x00] &= (mask & 0xfc);
  pci_conf[0x01] &= (mask >> 8);
  ReadHostDWordFromLittleEndian(pci_conf, newbase);
  pci_conf[0x00] |= pci_flags;
  if (((newbase & 0xfffc) != mask) && (newbase != oldbase)) { // skip PCI probe
    if (oldbase > 0) {
      for (i=0; i<size; i++) {
        if (iomask[i] > 0) {
          DEV_unregister_ioread_handler(this_ptr, f1, oldbase + i, iomask[i]);
          DEV_unregister_iowrite_handler(this_ptr, f2, oldbase + i, iomask[i]);
        }
      }
    }
    if (newbase > 0) {
      for (i=0; i<size; i++) {
        if (iomask[i] > 0) {
          DEV_register_ioread_handler(this_ptr, f1, newbase + i, name, iomask[i]);
          DEV_register_iowrite_handler(this_ptr, f2, newbase + i, name, iomask[i]);
        }
      }
    }
    *addr = newbase;
    return 1;
  }
  return 0;
}

#endif /* BX_SUPPORT_PCI */
