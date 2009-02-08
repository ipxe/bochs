/////////////////////////////////////////////////////////////////////////
// $Id: pcivga.cc,v 1.24 2009/02/08 09:05:52 vruppert Exp $
/////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2002,2003 Mike Nordell
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
//  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA

//
// Experimental PCI VGA adapter
//

// Note: This "driver" was created for the SOLE PURPOSE of getting BeOS
// to boot. It currently does NOTHING more than presenting a generic VGA
// device on the PCI bus. ALL gfx in/out-put is still handled by the VGA code.
// Furthermore, almost all of the PCI registers are currently acting like RAM.


// Define BX_PLUGGABLE in files that can be compiled into plugins.  For
// platforms that require a special tag on exported symbols, BX_PLUGGABLE
// is used to know when we are exporting symbols and when we are importing.
#define BX_PLUGGABLE

#include "iodev.h"

#if BX_SUPPORT_PCI && BX_SUPPORT_PCIVGA

#include "pci.h"
#include "vga.h"
#include "pcivga.h"

#define LOG_THIS thePciVgaAdapter->

bx_pcivga_c* thePciVgaAdapter = 0;

int libpcivga_LTX_plugin_init(plugin_t *plugin, plugintype_t type, int argc, char *argv[])
{
  thePciVgaAdapter = new bx_pcivga_c();
  BX_REGISTER_DEVICE_DEVMODEL(plugin, type, thePciVgaAdapter, BX_PLUGIN_PCIVGA);
  return 0; // Success
}

void libpcivga_LTX_plugin_fini(void)
{
  delete thePciVgaAdapter;
}

bx_pcivga_c::bx_pcivga_c()
{
  put("PCIVGA");
}

bx_pcivga_c::~bx_pcivga_c()
{
  BX_DEBUG(("Exit"));
}

void bx_pcivga_c::init(void)
{
  // called once when bochs initializes

  Bit8u devfunc = 0x00;
  unsigned i;

  DEV_register_pci_handlers(this, &devfunc, BX_PLUGIN_PCIVGA,
                            "Experimental PCI VGA");

  for (i=0; i<256; i++) {
    BX_PCIVGA_THIS s.pci_conf[i] = 0x0;
  }

  // readonly registers
  static const struct init_vals_t {
    unsigned      addr;
    unsigned char val;
  } init_vals[] = {
    // Note that the values for vendor and device id are selected at random!
    // There might actually be "real" values for "experimental" vendor and
    // device that should be used!
    { 0x00, 0x34 }, { 0x01, 0x12 }, // 0x1234 - experimental vendor
    { 0x02, 0x11 }, { 0x03, 0x11 }, // 0x1111 - experimental device
    { 0x0a, 0x00 },                 // class_sub  VGA controller
    { 0x0b, 0x03 },                 // class_base display
    { 0x0e, 0x00 }                  // header_type_generic
  };
  for (i = 0; i < sizeof(init_vals) / sizeof(*init_vals); ++i) {
    BX_PCIVGA_THIS s.pci_conf[init_vals[i].addr] = init_vals[i].val;
  }
}

void bx_pcivga_c::reset(unsigned type)
{
  static const struct reset_vals_t {
    unsigned      addr;
    unsigned char val;
  } reset_vals[] = {
      { 0x04, 0x03 }, { 0x05, 0x00 }, // command_io + command_mem
      { 0x06, 0x00 }, { 0x07, 0x02 }  // status_devsel_medium
  };
  for (unsigned i = 0; i < sizeof(reset_vals) / sizeof(*reset_vals); ++i) {
    BX_PCIVGA_THIS s.pci_conf[reset_vals[i].addr] = reset_vals[i].val;
  }
  // FIXME: LFB address should be changeable
  WriteHostDWordToLittleEndian(&BX_PCIVGA_THIS s.pci_conf[0x10],
                               VBE_DISPI_LFB_PHYSICAL_ADDRESS | 0x08);
}

void bx_pcivga_c::register_state(void)
{
  bx_list_c *list = new bx_list_c(SIM->get_bochs_root(), "pcivga", "PCI VGA Adapter State", 1);
  register_pci_state(list, BX_PCIVGA_THIS s.pci_conf);
}

// pci configuration space read callback handler
Bit32u bx_pcivga_c::pci_read_handler(Bit8u address, unsigned io_len)
{
  Bit32u value = 0;

  if (io_len > 4 || io_len == 0) {
    BX_DEBUG(("Experimental PCIVGA read register 0x%02x, len=%u !",
             (unsigned) address, (unsigned) io_len));
    return 0xffffffff;
  }

  const char* pszName = "                  ";
  switch (address) {
    case 0x00: if (io_len == 2) {
                 pszName = "(vendor id)       ";
               } else if (io_len == 4) {
                 pszName = "(vendor + device) ";
               }
      break;
    case 0x04: if (io_len == 2) {
                 pszName = "(command)         ";
               } else if (io_len == 4) {
                 pszName = "(command+status)  ";
               }
      break;
    case 0x08: if (io_len == 1) {
                 pszName = "(revision id)     ";
               } else if (io_len == 4) {
                 pszName = "(rev.+class code) ";
               }
      break;
    case 0x0c: pszName = "(cache line size) "; break;
    case 0x10: pszName = "(base address #0) "; break;
    case 0x28: pszName = "(cardbus cis)     "; break;
    case 0x2c: pszName = "(subsys. vendor+) "; break;
    case 0x30: pszName = "(rom base)        "; break;
    case 0x3c: pszName = "(interrupt line+) "; break;
    case 0x3d: pszName = "(interrupt pin)   "; break;
  }

  // This odd code is to display only what bytes actually were read.
  char szTmp[9];
  char szTmp2[3];
  szTmp[0] = '\0';
  szTmp2[0] = '\0';
  for (unsigned i=0; i<io_len; i++) {
    value |= (BX_PCIVGA_THIS s.pci_conf[address+i] << (i*8));

    sprintf(szTmp2, "%02x", (BX_PCIVGA_THIS s.pci_conf[address+i]));
    strrev(szTmp2);
    strcat(szTmp, szTmp2);
  }
  strrev(szTmp);
  BX_DEBUG(("Experimental PCIVGA  read register 0x%02x %svalue 0x%s",
            address, pszName, szTmp));
  return value;
}


// static pci configuration space write callback handler
void bx_pcivga_c::pci_write_handler(Bit8u address, Bit32u value, unsigned io_len)
{
  unsigned i;
  unsigned write_addr;
  Bit8u new_value, old_value;
  Bit32u mask, value32;
  bx_bool baseaddr_change = 0;

  if ((address >= 0x14) && (address < 0x34))
    return;
  if (io_len <= 4) {
    // This odd code is to display only what bytes actually were written.
    char szTmp[9];
    char szTmp2[3];
    szTmp[0] = '\0';
    szTmp2[0] = '\0';
    for (i = 0; i < io_len; i++) {
      write_addr = address + i;
      old_value = BX_PCIVGA_THIS s.pci_conf[write_addr];
      new_value = (Bit8u)(value & 0xff);
      switch (write_addr) {
        case 0x04: // disallowing write to command
        case 0x06: // disallowing write to status lo-byte (is that expected?)
          new_value = old_value;
          strcpy(szTmp2, "..");
          break;
        case 0x10: // base address #0
          new_value = (new_value & 0xf0) | (old_value & 0x0f);
        case 0x11: case 0x12: case 0x13:
          baseaddr_change |= (old_value != new_value);
        default:
          sprintf(szTmp2, "%02x", new_value);
      }
      BX_PCIVGA_THIS s.pci_conf[write_addr] = new_value;
      value >>= 8;
      strrev(szTmp2);
      strcat(szTmp, szTmp2);
    }
    if (baseaddr_change) {
      // FIXME: implement changeable LFB address
      ReadHostDWordFromLittleEndian(&BX_PCIVGA_THIS s.pci_conf[0x10], value32);
      mask = ~((VBE_DISPI_TOTAL_VIDEO_MEMORY_MB << 20) - 1);
      value32 &= mask;
      WriteHostDWordToLittleEndian(&BX_PCIVGA_THIS s.pci_conf[0x10], value32);
    }
    strrev(szTmp);
    BX_DEBUG(("Experimental PCIVGA write register 0x%02x value 0x%s", address, szTmp));
  }
}

#endif // BX_SUPPORT_PCI && BX_SUPPORT_PCIVGA
