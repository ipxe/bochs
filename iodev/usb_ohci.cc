/////////////////////////////////////////////////////////////////////////
// $Id: usb_ohci.cc,v 1.6 2009/02/14 10:06:20 vruppert Exp $
/////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2009  Benjamin D Lunt (fys at frontiernet net)
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
/////////////////////////////////////////////////////////////////////////

// Experimental USB OHCI adapter

// Notes: See usb_common.cc

// Define BX_PLUGGABLE in files that can be compiled into plugins.  For
// platforms that require a special tag on exported symbols, BX_PLUGGABLE
// is used to know when we are exporting symbols and when we are importing.
#define BX_PLUGGABLE


// TODO:
//  if not Operational, stop the two timers to free cpu bandwidth,
//    then start them again when in Operational? (Maybe only if not
//    in operational for more than 10 frames.


#include "iodev.h"

#if BX_SUPPORT_PCI && BX_SUPPORT_USB_OHCI

#include "pci.h"
#include "usb_common.h"
#include "usb_hid.h"
#include "usb_msd.h"
#include "usb_ohci.h"

#define LOG_THIS theUSB_OHCI->

bx_usb_ohci_c* theUSB_OHCI = NULL;

const char *usb_ohci_port_name[] = {
  "HCRevision        ",
  "HCControl         ",
  "HCCommandStatus   ",
  "HCInterruptStatus ",
  "HCInterruptEnable ",
  "HCInterruptDisable",
  "HCHCCA            ",
  "HCPeriodCurrentED ",
  "HCControlHeadED   ",
  "HCControlCurrentED",
  "HCBulkHeadED      ",
  "HCBulkCurrentED   ",
  "HCDoneHead        ",
  "HCFmInterval      ",
  "HCFmRemaining     ",
  "HCFmNumber        ",
  "HCPeriodicStart   ",
  "HCLSThreshold     ",
  "HCRhDescriptorA   ",
  "HCRhDescriptorB   ",
  "HCRhStatus        ",
  "HCRhPortStatus0   ",
  "HCRhPortStatus1   ",
  "HCRhPortStatus2   ",
  "HCRhPortStatus3   ",
  "  **unknown**     "
};

int libusb_ohci_LTX_plugin_init(plugin_t *plugin, plugintype_t type, int argc, char *argv[])
{
  theUSB_OHCI = new bx_usb_ohci_c();
  bx_devices.pluginUSB_OHCI = theUSB_OHCI;
  BX_REGISTER_DEVICE_DEVMODEL(plugin, type, theUSB_OHCI, BX_PLUGIN_USB_OHCI);
  return 0; // Success
}

void libusb_ohci_LTX_plugin_fini(void)
{
  delete theUSB_OHCI;
}

bx_usb_ohci_c::bx_usb_ohci_c()
{
  put("OHCI");
  memset((void*)&hub, 0, sizeof(bx_usb_ohci_t));
  device_buffer = NULL;
}

bx_usb_ohci_c::~bx_usb_ohci_c()
{
  if (BX_OHCI_THIS device_buffer != NULL)
    delete [] BX_OHCI_THIS device_buffer;

  for (int j=0; j<USB_NUM_PORTS; j++) {
    if (BX_OHCI_THIS hub.usb_port[j].device != NULL) {
      delete BX_OHCI_THIS hub.usb_port[j].device;
    }
  }

  SIM->get_param_string(BXPN_OHCI_PORT1)->set_handler(NULL);
  SIM->get_param_string(BXPN_OHCI_PORT2)->set_handler(NULL);

  BX_DEBUG(("Exit"));
}

void bx_usb_ohci_c::init(void)
{
  BX_OHCI_THIS device_buffer = new Bit8u[65536];

  // Call our frame timer routine every 1mS (1,000uS)
  // Continuous and active
  BX_OHCI_THIS hub.frame_index =
                   bx_pc_system.register_timer(this, usb_frame_handler, 1000, 1,1, "ohci.frame_timer");

  // Call our interval timer routine every 1uS
  // Continuous and active
  BX_OHCI_THIS hub.interval_index =
                   bx_pc_system.register_timer(this, usb_interval_handler, 1, 1,1, "ohci.interval_timer");

  BX_OHCI_THIS hub.devfunc = 0x00;
  DEV_register_pci_handlers(this, &BX_OHCI_THIS hub.devfunc, BX_PLUGIN_USB_OHCI,
                            "Experimental USB OHCI");

  for (unsigned i=0; i<256; i++)
    BX_OHCI_THIS hub.pci_conf[i] = 0x0;

  BX_OHCI_THIS hub.base_addr = 0x0;
  BX_OHCI_THIS hub.ohci_done_count = 7;

  //FIXME: for now, we want a status bar // hub zero, port zero
  BX_OHCI_THIS hub.statusbar_id[0] = bx_gui->register_statusitem("OHCI");

  SIM->get_param_string(BXPN_OHCI_PORT1)->set_handler(usb_param_handler);
  SIM->get_param_string(BXPN_OHCI_PORT1)->set_runtime_param(1);
  SIM->get_param_string(BXPN_OHCI_PORT2)->set_handler(usb_param_handler);
  SIM->get_param_string(BXPN_OHCI_PORT2)->set_runtime_param(1);

  //HACK: Turn on debug messages from the start
  //BX_OHCI_THIS setonoff(LOGLEV_DEBUG, ACT_REPORT);

  BX_INFO(("USB OHCI initialized"));
}

void bx_usb_ohci_c::reset(unsigned type)
{
  unsigned i;

  if (type == BX_RESET_HARDWARE) {
    static const struct reset_vals_t {
      unsigned      addr;
      unsigned char val;
    } reset_vals[] = {
      { 0x00, 0xC1 }, { 0x01, 0x11 }, // 0x11C1 = vendor
      { 0x02, 0x03 }, { 0x03, 0x58 }, // 0x5803 = device
      { 0x04, 0x06 }, { 0x05, 0x00 }, // command_io
      { 0x06, 0x10 }, { 0x07, 0x02 }, // status (bit 4 = 1, has capabilities list.)
      { 0x08, 0x11 },                 // revision number
      { 0x09, 0x10 },                 // interface
      { 0x0a, 0x03 },                 // class_sub  USB Host Controller
      { 0x0b, 0x0c },                 // class_base Serial Bus Controller
      { 0x0D, 0x40 },                 // bus latency
      //{ 0x0e, 0x80 },                 // header_type_generic (multi-function device)
      { 0x0e, 0x00 },                 // header_type_generic

      // address space 0x10 - 0x13
      { 0x10, 0x00 }, { 0x11, 0x50 }, //
      { 0x12, 0x00 }, { 0x13, 0xE1 }, //

      { 0x2C, 0xC1 }, { 0x2D, 0x11 }, // subsystem vendor ID
      { 0x2E, 0x03 }, { 0x2F, 0x58 }, // subsystem ID

      { 0x34, 0x50 },                 // offset of capabilities list within configuration space

      { 0x3c, 0x0B },                 // IRQ
      { 0x3d, BX_PCI_INTD },          // INT
      { 0x3E, 0x03 },                 // minimum time bus master needs PCI bus ownership, in 250ns units
      { 0x3F, 0x56 },                 // maximum latency, in 250ns units (bus masters only) (read-only)

      // capabilities list:
      { 0x50, 0x01 },                 //
      { 0x51, 0x00 },                 //
      { 0x52, 0x02 },                 //
      { 0x53, 0x76 },                 //
      { 0x54, 0x00 },                 //
      { 0x55, 0x20 },                 //
      { 0x56, 0x00 },                 //
      { 0x57, 0x1F },                 //
    };
    for (i = 0; i < sizeof(reset_vals) / sizeof(*reset_vals); ++i) {
        BX_OHCI_THIS hub.pci_conf[reset_vals[i].addr] = reset_vals[i].val;
    }
  }

  BX_OHCI_THIS reset_hc();

  BX_OHCI_THIS mousedev = NULL;
  BX_OHCI_THIS keybdev = NULL;

  init_device(0, SIM->get_param_string(BXPN_OHCI_PORT1)->getptr());
  init_device(1, SIM->get_param_string(BXPN_OHCI_PORT2)->getptr());
}

void bx_usb_ohci_c::reset_hc()
{
  int i;

  // reset locals
  BX_OHCI_THIS hub.ohci_done_count = 7;

  // HcRevision
  BX_OHCI_THIS hub.op_regs.HcRevision.reserved =   0x000001;  // unknown why it is 1
  BX_OHCI_THIS hub.op_regs.HcRevision.rev      =       0x10;

  // HcControl
  BX_OHCI_THIS hub.op_regs.HcControl.reserved  =          0;
  BX_OHCI_THIS hub.op_regs.HcControl.rwe       =          0;
  BX_OHCI_THIS hub.op_regs.HcControl.rwc       =          0;
  BX_OHCI_THIS hub.op_regs.HcControl.ir        =          0;
  BX_OHCI_THIS hub.op_regs.HcControl.hcfs      =          0;
  BX_OHCI_THIS hub.op_regs.HcControl.ble       =          0;
  BX_OHCI_THIS hub.op_regs.HcControl.cle       =          0;
  BX_OHCI_THIS hub.op_regs.HcControl.ie        =          0;
  BX_OHCI_THIS hub.op_regs.HcControl.ple       =          0;
  BX_OHCI_THIS hub.op_regs.HcControl.cbsr      =          0;

  // HcCommandStatus
  BX_OHCI_THIS hub.op_regs.HcCommandStatus.reserved0 = 0x000000;
  BX_OHCI_THIS hub.op_regs.HcCommandStatus.soc       =        0;
  BX_OHCI_THIS hub.op_regs.HcCommandStatus.reserved1 = 0x000000;
  BX_OHCI_THIS hub.op_regs.HcCommandStatus.ocr       =        0;
  BX_OHCI_THIS hub.op_regs.HcCommandStatus.blf       =        0;
  BX_OHCI_THIS hub.op_regs.HcCommandStatus.clf       =        0;
  BX_OHCI_THIS hub.op_regs.HcCommandStatus.hcr       =        0;

  // HcInterruptStatus
  BX_OHCI_THIS hub.op_regs.HcInterruptStatus.zero     = 0;
  BX_OHCI_THIS hub.op_regs.HcInterruptStatus.oc       = 0;
  BX_OHCI_THIS hub.op_regs.HcInterruptStatus.reserved = 0;
  BX_OHCI_THIS hub.op_regs.HcInterruptStatus.rhsc     = 0;
  BX_OHCI_THIS hub.op_regs.HcInterruptStatus.fno      = 0;
  BX_OHCI_THIS hub.op_regs.HcInterruptStatus.ue       = 0;
  BX_OHCI_THIS hub.op_regs.HcInterruptStatus.rd       = 0;
  BX_OHCI_THIS hub.op_regs.HcInterruptStatus.sf       = 0;
  BX_OHCI_THIS hub.op_regs.HcInterruptStatus.wdh      = 0;
  BX_OHCI_THIS hub.op_regs.HcInterruptStatus.so       = 0;

  // HcInterruptEnable
  BX_OHCI_THIS hub.op_regs.HcInterruptEnable.mie      = 0;
  BX_OHCI_THIS hub.op_regs.HcInterruptEnable.oc       = 0;
  BX_OHCI_THIS hub.op_regs.HcInterruptEnable.reserved = 0;
  BX_OHCI_THIS hub.op_regs.HcInterruptEnable.rhsc     = 0;
  BX_OHCI_THIS hub.op_regs.HcInterruptEnable.fno      = 0;
  BX_OHCI_THIS hub.op_regs.HcInterruptEnable.ue       = 0;
  BX_OHCI_THIS hub.op_regs.HcInterruptEnable.rd       = 0;
  BX_OHCI_THIS hub.op_regs.HcInterruptEnable.sf       = 0;
  BX_OHCI_THIS hub.op_regs.HcInterruptEnable.wdh      = 0;
  BX_OHCI_THIS hub.op_regs.HcInterruptEnable.so       = 0;

  // HcHCCA
  BX_OHCI_THIS hub.op_regs.HcHCCA.hcca = 0x000000;
  BX_OHCI_THIS hub.op_regs.HcHCCA.zero = 0x00;

  // HcPeriodCurrentED
  BX_OHCI_THIS hub.op_regs.HcPeriodCurrentED.pced   = 0x0000000;
  BX_OHCI_THIS hub.op_regs.HcPeriodCurrentED.zero   = 0;

  // HcControlHeadED
  BX_OHCI_THIS hub.op_regs.HcControlHeadED.ched     = 0x0000000;
  BX_OHCI_THIS hub.op_regs.HcControlHeadED.zero     = 0;

  // HcControlCurrentED
  BX_OHCI_THIS hub.op_regs.HcControlCurrentED.cced  = 0x0000000;
  BX_OHCI_THIS hub.op_regs.HcControlCurrentED.zero  = 0;

  // HcBulkHeadED
  BX_OHCI_THIS hub.op_regs.HcBulkHeadED.bhed     = 0x0000000;
  BX_OHCI_THIS hub.op_regs.HcBulkHeadED.zero     = 0;

  // HcBulkCurrentED
  BX_OHCI_THIS hub.op_regs.HcBulkCurrentED.bced  = 0x0000000;
  BX_OHCI_THIS hub.op_regs.HcBulkCurrentED.zero  = 0;

  // HcDoneHead
  BX_OHCI_THIS hub.op_regs.HcDoneHead.dh    = 0x00000000;
  BX_OHCI_THIS hub.op_regs.HcDoneHead.zero  = 0;

  // HcFmInterval
  BX_OHCI_THIS hub.op_regs.HcFmInterval.fit      =      0;
  BX_OHCI_THIS hub.op_regs.HcFmInterval.fsmps    =      0;
  BX_OHCI_THIS hub.op_regs.HcFmInterval.reserved =      0;
  BX_OHCI_THIS hub.op_regs.HcFmInterval.fi       = 0x2EDF;

  // HcFmRemaining
  BX_OHCI_THIS hub.op_regs.HcFmRemaining.frt      =      0;
  BX_OHCI_THIS hub.op_regs.HcFmRemaining.reserved =      0;
  BX_OHCI_THIS hub.op_regs.HcFmRemaining.fr       = 0x0000;

  // HcFmNumber
  BX_OHCI_THIS hub.op_regs.HcFmNumber.reserved    =      0;
  BX_OHCI_THIS hub.op_regs.HcFmNumber.fn          =      0;

  // HcPeriodicStart
  BX_OHCI_THIS hub.op_regs.HcPeriodicStart.reserved  =      0;
  BX_OHCI_THIS hub.op_regs.HcPeriodicStart.ps        =      0;

  // HcLSThreshold
  BX_OHCI_THIS hub.op_regs.HcLSThreshold.reserved    =      0;
  BX_OHCI_THIS hub.op_regs.HcLSThreshold.lst         = 0x0628;

  // HcRhDescriptorA
  BX_OHCI_THIS hub.op_regs.HcRhDescriptorA.potpgt   = 0x10;
  BX_OHCI_THIS hub.op_regs.HcRhDescriptorA.reserved =    0;
  BX_OHCI_THIS hub.op_regs.HcRhDescriptorA.nocp     =    0;
  BX_OHCI_THIS hub.op_regs.HcRhDescriptorA.ocpm     =    1;
  BX_OHCI_THIS hub.op_regs.HcRhDescriptorA.dt       =    0;
  BX_OHCI_THIS hub.op_regs.HcRhDescriptorA.nps      =    0;
  BX_OHCI_THIS hub.op_regs.HcRhDescriptorA.psm      =    1;
  BX_OHCI_THIS hub.op_regs.HcRhDescriptorA.ndp      =    USB_NUM_PORTS;

  // HcRhDescriptorB
  BX_OHCI_THIS hub.op_regs.HcRhDescriptorB.ppcm     = 0x0000 | ((USB_NUM_PORTS == 1) ? 2 : 0) | ((USB_NUM_PORTS == 2) ? 4 : 0);
  BX_OHCI_THIS hub.op_regs.HcRhDescriptorB.dr       = 0x0000;

  // HcRhStatus
  BX_OHCI_THIS hub.op_regs.HcRhStatus.crwe      = 0;
  BX_OHCI_THIS hub.op_regs.HcRhStatus.reserved0 = 0;
  BX_OHCI_THIS hub.op_regs.HcRhStatus.ocic      = 0;
  BX_OHCI_THIS hub.op_regs.HcRhStatus.lpsc      = 0;
  BX_OHCI_THIS hub.op_regs.HcRhStatus.drwe      = 0;
  BX_OHCI_THIS hub.op_regs.HcRhStatus.reserved1 = 0;
  BX_OHCI_THIS hub.op_regs.HcRhStatus.oci       = 0;
  BX_OHCI_THIS hub.op_regs.HcRhStatus.lps       = 0;

  // HcRhPortStatus[x]
  for (i=0; i<USB_NUM_PORTS; i++) {
    reset_port(i);
    if (BX_OHCI_THIS hub.usb_port[i].device != NULL) {
      delete BX_OHCI_THIS hub.usb_port[i].device;
      BX_OHCI_THIS hub.usb_port[i].device = NULL;
    }
  }

  BX_OHCI_THIS mousedev = NULL;
  BX_OHCI_THIS keybdev = NULL;

  init_device(0, SIM->get_param_string(BXPN_OHCI_PORT1)->getptr());
  init_device(1, SIM->get_param_string(BXPN_OHCI_PORT2)->getptr());
}

void bx_usb_ohci_c::reset_port(int p)
{
  BX_OHCI_THIS hub.op_regs.HcRhPortStatus[p].reserved0 = 0;
  BX_OHCI_THIS hub.op_regs.HcRhPortStatus[p].prsc      = 0;
  BX_OHCI_THIS hub.op_regs.HcRhPortStatus[p].ocic      = 0;
  BX_OHCI_THIS hub.op_regs.HcRhPortStatus[p].pssc      = 0;
  BX_OHCI_THIS hub.op_regs.HcRhPortStatus[p].pesc      = 0;
  BX_OHCI_THIS hub.op_regs.HcRhPortStatus[p].csc       = 0;
  BX_OHCI_THIS hub.op_regs.HcRhPortStatus[p].reserved1 = 0;
  BX_OHCI_THIS hub.op_regs.HcRhPortStatus[p].lsda      = 0;
  BX_OHCI_THIS hub.op_regs.HcRhPortStatus[p].pps       = 0;
  BX_OHCI_THIS hub.op_regs.HcRhPortStatus[p].reserved2 = 0;
  BX_OHCI_THIS hub.op_regs.HcRhPortStatus[p].prs       = 0;
  BX_OHCI_THIS hub.op_regs.HcRhPortStatus[p].poci      = 0;
  BX_OHCI_THIS hub.op_regs.HcRhPortStatus[p].pss       = 0;
  BX_OHCI_THIS hub.op_regs.HcRhPortStatus[p].pes       = 0;
  BX_OHCI_THIS hub.op_regs.HcRhPortStatus[p].ccs       = 0;
}

void bx_usb_ohci_c::register_state(void)
{
  unsigned i;
  char portnum[8];
  bx_list_c *hub, *port;

  // TODO: save/restore support
  bx_list_c *list = new bx_list_c(SIM->get_bochs_root(), "usb_ohci", "USB OHCI State");
  hub = new bx_list_c(list, "hub", 4);
  for (i=0; i<USB_NUM_PORTS; i++) {
    sprintf(portnum, "port%d", i+1);
    port = new bx_list_c(hub, portnum, 2);
    // empty list for USB device state
    new bx_list_c(port, "device", 20);
  }
  register_pci_state(hub, BX_OHCI_THIS hub.pci_conf);
}

void bx_usb_ohci_c::after_restore_state(void)
{
  if (DEV_pci_set_base_mem(BX_OHCI_THIS_PTR, read_handler, write_handler,
                         &BX_OHCI_THIS hub.base_addr,
                         &BX_OHCI_THIS hub.pci_conf[0x10],
                         4096))  {
     BX_INFO(("new base address: 0x%04x", BX_OHCI_THIS hub.base_addr));
  }
  for (int j=0; j<USB_NUM_PORTS; j++) {
    if (BX_OHCI_THIS hub.usb_port[j].device != NULL) {
      BX_OHCI_THIS hub.usb_port[j].device->after_restore_state();
    }
  }
}

void bx_usb_ohci_c::init_device(Bit8u port, const char *devname)
{
  usbdev_type type = USB_DEV_TYPE_NONE;
  char pname[BX_PATHNAME_LEN];

  if (!strlen(devname) || !strcmp(devname, "none")) return;

  if (!strcmp(devname, "mouse")) {
    type = USB_DEV_TYPE_MOUSE;
    BX_OHCI_THIS hub.usb_port[port].device = new usb_hid_device_c(type);
    if (BX_OHCI_THIS mousedev == NULL) {
      BX_OHCI_THIS mousedev = (usb_hid_device_c*)BX_OHCI_THIS hub.usb_port[port].device;
    }
  } else if (!strcmp(devname, "tablet")) {
    type = USB_DEV_TYPE_TABLET;
    BX_OHCI_THIS hub.usb_port[port].device = new usb_hid_device_c(type);
    if (BX_OHCI_THIS mousedev == NULL) {
      BX_OHCI_THIS mousedev = (usb_hid_device_c*)BX_OHCI_THIS hub.usb_port[port].device;
    }
  } else if (!strcmp(devname, "keypad")) {
    type = USB_DEV_TYPE_KEYPAD;
    BX_OHCI_THIS hub.usb_port[port].device = new usb_hid_device_c(type);
    if (BX_OHCI_THIS keybdev == NULL) {
      BX_OHCI_THIS keybdev = (usb_hid_device_c*)BX_OHCI_THIS hub.usb_port[port].device;
    }
  } else if (!strncmp(devname, "disk", 4)) {
    if ((strlen(devname) > 5) && (devname[4] == ':')) {
      type = USB_DEV_TYPE_DISK;
      BX_OHCI_THIS hub.usb_port[port].device = new usb_msd_device_c();
    } else {
      BX_PANIC(("USB device 'disk' needs a filename separated with a colon"));
      return;
    }
  } else {
    BX_PANIC(("unknown USB device: %s", devname));
    return;
  }
  sprintf(pname, "usb_ohci.hub.port%d.device", port+1);
  bx_list_c *devlist = (bx_list_c*)SIM->get_param(pname, SIM->get_bochs_root());
  BX_OHCI_THIS hub.usb_port[port].device->register_state(devlist);
  usb_set_connect_status(port, type, 1);
}

void bx_usb_ohci_c::set_irq_level(const bx_bool level)
{
  DEV_pci_set_irq(BX_OHCI_THIS hub.devfunc, BX_OHCI_THIS hub.pci_conf[0x3d], level);
}

void bx_usb_ohci_c::set_irq_state()
{
  bx_bool level = 0;

  if (BX_OHCI_THIS hub.op_regs.HcInterruptEnable.mie && (
    (BX_OHCI_THIS hub.op_regs.HcInterruptStatus.so && BX_OHCI_THIS hub.op_regs.HcInterruptEnable.so) ||
    (BX_OHCI_THIS hub.op_regs.HcInterruptStatus.wdh && BX_OHCI_THIS hub.op_regs.HcInterruptEnable.wdh) ||
    (BX_OHCI_THIS hub.op_regs.HcInterruptStatus.sf && BX_OHCI_THIS hub.op_regs.HcInterruptEnable.sf) ||
    (BX_OHCI_THIS hub.op_regs.HcInterruptStatus.rd && BX_OHCI_THIS hub.op_regs.HcInterruptEnable.rd) ||
    (BX_OHCI_THIS hub.op_regs.HcInterruptStatus.ue && BX_OHCI_THIS hub.op_regs.HcInterruptEnable.ue) ||
    (BX_OHCI_THIS hub.op_regs.HcInterruptStatus.fno && BX_OHCI_THIS hub.op_regs.HcInterruptEnable.fno) ||
    (BX_OHCI_THIS hub.op_regs.HcInterruptStatus.rhsc && BX_OHCI_THIS hub.op_regs.HcInterruptEnable.rhsc) ||
    (BX_OHCI_THIS hub.op_regs.HcInterruptStatus.oc && BX_OHCI_THIS hub.op_regs.HcInterruptEnable.oc))) {
      level = 1;
      BX_DEBUG(("Interrupt Fired."));
  }

  set_irq_level(level);
}

bx_bool bx_usb_ohci_c::read_handler(bx_phy_address addr, unsigned len, void *data, void *param)
{
  Bit32u val = 0x0;
  int p = 0;

  if (len != 4) {
    BX_INFO(("Read at 0x%08X with len != 4 (%i)", addr, len));
    return 1;
  }

  Bit32u  offset = addr - BX_OHCI_THIS hub.base_addr;
  switch (offset) {
    case 0x00: // HcRevision
      val =   BX_OHCI_THIS hub.op_regs.HcRevision.reserved << 8
            | BX_OHCI_THIS hub.op_regs.HcRevision.rev;
      break;

    case 0x04: // HcControl
      val =   (BX_OHCI_THIS hub.op_regs.HcControl.reserved     << 11)
            | (BX_OHCI_THIS hub.op_regs.HcControl.rwe      ? 1 << 10 : 0)
            | (BX_OHCI_THIS hub.op_regs.HcControl.rwc      ? 1 << 9 : 0)
            | (BX_OHCI_THIS hub.op_regs.HcControl.ir       ? 1 << 8 : 0)
            | (BX_OHCI_THIS hub.op_regs.HcControl.hcfs         << 6)
            | (BX_OHCI_THIS hub.op_regs.HcControl.ble      ? 1 << 5 : 0)
            | (BX_OHCI_THIS hub.op_regs.HcControl.cle      ? 1 << 4 : 0)
            | (BX_OHCI_THIS hub.op_regs.HcControl.ie       ? 1 << 3 : 0)
            | (BX_OHCI_THIS hub.op_regs.HcControl.ple      ? 1 << 2 : 0)
            | (BX_OHCI_THIS hub.op_regs.HcControl.cbsr         << 0);
      break;

    case 0x08: // HcCommandStatus
      val =   (BX_OHCI_THIS hub.op_regs.HcCommandStatus.reserved0     << 18)
            | (BX_OHCI_THIS hub.op_regs.HcCommandStatus.soc           << 16)
            | (BX_OHCI_THIS hub.op_regs.HcCommandStatus.reserved1     << 4)
            | (BX_OHCI_THIS hub.op_regs.HcCommandStatus.ocr       ? 1 << 3 : 0)
            | (BX_OHCI_THIS hub.op_regs.HcCommandStatus.blf       ? 1 << 2 : 0)
            | (BX_OHCI_THIS hub.op_regs.HcCommandStatus.clf       ? 1 << 1 : 0)
            | (BX_OHCI_THIS hub.op_regs.HcCommandStatus.hcr       ? 1 << 0 : 0);
      break;

    case 0x0C: // HcInterruptStatus
      val =   (BX_OHCI_THIS hub.op_regs.HcInterruptStatus.zero     ? 1 << 31 : 0)
            | (BX_OHCI_THIS hub.op_regs.HcInterruptStatus.oc       ? 1 << 30 : 0)
            | (BX_OHCI_THIS hub.op_regs.HcInterruptStatus.reserved     << 7)
            | (BX_OHCI_THIS hub.op_regs.HcInterruptStatus.rhsc     ? 1 << 6 : 0)
            | (BX_OHCI_THIS hub.op_regs.HcInterruptStatus.fno      ? 1 << 5 : 0)
            | (BX_OHCI_THIS hub.op_regs.HcInterruptStatus.ue       ? 1 << 4 : 0)
            | (BX_OHCI_THIS hub.op_regs.HcInterruptStatus.rd       ? 1 << 3 : 0)
            | (BX_OHCI_THIS hub.op_regs.HcInterruptStatus.sf       ? 1 << 2 : 0)
            | (BX_OHCI_THIS hub.op_regs.HcInterruptStatus.wdh      ? 1 << 1 : 0)
            | (BX_OHCI_THIS hub.op_regs.HcInterruptStatus.so       ? 1 << 0 : 0);
      break;

    case 0x10: // HcInterruptEnable
    case 0x14: // HcInterruptDisable (reading this one returns that one)
      val =   (BX_OHCI_THIS hub.op_regs.HcInterruptEnable.mie      ? 1 << 31 : 0)
            | (BX_OHCI_THIS hub.op_regs.HcInterruptEnable.oc       ? 1 << 30 : 0)
            | (BX_OHCI_THIS hub.op_regs.HcInterruptEnable.reserved     << 7)
            | (BX_OHCI_THIS hub.op_regs.HcInterruptEnable.rhsc     ? 1 << 6 : 0)
            | (BX_OHCI_THIS hub.op_regs.HcInterruptEnable.fno      ? 1 << 5 : 0)
            | (BX_OHCI_THIS hub.op_regs.HcInterruptEnable.ue       ? 1 << 4 : 0)
            | (BX_OHCI_THIS hub.op_regs.HcInterruptEnable.rd       ? 1 << 3 : 0)
            | (BX_OHCI_THIS hub.op_regs.HcInterruptEnable.sf       ? 1 << 2 : 0)
            | (BX_OHCI_THIS hub.op_regs.HcInterruptEnable.wdh      ? 1 << 1 : 0)
            | (BX_OHCI_THIS hub.op_regs.HcInterruptEnable.so       ? 1 << 0 : 0);
      break;

    case 0x18: // HcHCCA
      val =   (BX_OHCI_THIS hub.op_regs.HcHCCA.hcca << 8)
            | (BX_OHCI_THIS hub.op_regs.HcHCCA.zero);
      break;

    case 0x1C: // HcPeriodCurrentED
      val =   (BX_OHCI_THIS hub.op_regs.HcPeriodCurrentED.pced << 4)
            | (BX_OHCI_THIS hub.op_regs.HcPeriodCurrentED.zero << 0);
      break;

    case 0x20: // HcControlHeadED
      val =   (BX_OHCI_THIS hub.op_regs.HcControlHeadED.ched << 4)
            | (BX_OHCI_THIS hub.op_regs.HcControlHeadED.zero << 0);
      break;

    case 0x24: // HcControlCurrentED
      val =   (BX_OHCI_THIS hub.op_regs.HcControlCurrentED.cced << 4)
            | (BX_OHCI_THIS hub.op_regs.HcControlCurrentED.zero << 0);
      break;

    case 0x28: // HcBulkHeadED
      val =   (BX_OHCI_THIS hub.op_regs.HcBulkHeadED.bhed << 4)
            | (BX_OHCI_THIS hub.op_regs.HcBulkHeadED.zero << 0);
      break;

    case 0x2C: // HcBulkCurrentED
      val =   (BX_OHCI_THIS hub.op_regs.HcBulkCurrentED.bced << 4)
            | (BX_OHCI_THIS hub.op_regs.HcBulkCurrentED.zero << 0);
      break;

    case 0x30: // HcDoneHead
      val =   (BX_OHCI_THIS hub.op_regs.HcDoneHead.dh   << 4)
            | (BX_OHCI_THIS hub.op_regs.HcDoneHead.zero << 0);
      break;

    case 0x34: // HcFmInterval
      val =   (BX_OHCI_THIS hub.op_regs.HcFmInterval.fit      ? 1 << 31 : 0)
            | (BX_OHCI_THIS hub.op_regs.HcFmInterval.fsmps        << 16)
            | (BX_OHCI_THIS hub.op_regs.HcFmInterval.reserved     << 14)
            | (BX_OHCI_THIS hub.op_regs.HcFmInterval.fi           << 0);
      break;

    case 0x38: // HcFmRemaining
      val =   (BX_OHCI_THIS hub.op_regs.HcFmRemaining.frt      ? 1 << 31 : 0)
            | (BX_OHCI_THIS hub.op_regs.HcFmRemaining.reserved     << 14)
            | (BX_OHCI_THIS hub.op_regs.HcFmRemaining.fr           << 0);
      break;

    case 0x3C: // HcFmNumber
      val =   (BX_OHCI_THIS hub.op_regs.HcFmNumber.reserved << 16)
            | (BX_OHCI_THIS hub.op_regs.HcFmNumber.fn       << 0);
      break;

    case 0x40: // HcPeriodicStart
      val =   (BX_OHCI_THIS hub.op_regs.HcPeriodicStart.reserved << 14)
            | (BX_OHCI_THIS hub.op_regs.HcPeriodicStart.ps       << 0);
      break;

    case 0x44: // HcLSThreshold
      val =   (BX_OHCI_THIS hub.op_regs.HcLSThreshold.reserved << 12)
            | (BX_OHCI_THIS hub.op_regs.HcLSThreshold.lst      << 0);
      break;

    case 0x48: // HcRhDescriptorA
      val =   (BX_OHCI_THIS hub.op_regs.HcRhDescriptorA.potpgt       << 24)
            | (BX_OHCI_THIS hub.op_regs.HcRhDescriptorA.reserved     << 13)
            | (BX_OHCI_THIS hub.op_regs.HcRhDescriptorA.nocp     ? 1 << 12 : 0)
            | (BX_OHCI_THIS hub.op_regs.HcRhDescriptorA.ocpm     ? 1 << 11 : 0)
            | 0 //BX_OHCI_THIS hub.op_regs.HcRhDescriptorA.dt       << 10
            | (BX_OHCI_THIS hub.op_regs.HcRhDescriptorA.nps      ? 1 <<  9 : 0)
            | (BX_OHCI_THIS hub.op_regs.HcRhDescriptorA.psm      ? 1 <<  8 : 0)
            | (BX_OHCI_THIS hub.op_regs.HcRhDescriptorA.ndp          <<  0);
      break;

    case 0x4C: // HcRhDescriptorB
      val =   (BX_OHCI_THIS hub.op_regs.HcRhDescriptorB.ppcm << 16)
            | (BX_OHCI_THIS hub.op_regs.HcRhDescriptorB.dr   << 0);
      break;

    case 0x50: // HcRhStatus
      val =   (BX_OHCI_THIS hub.op_regs.HcRhStatus.crwe      ? 1 << 31 : 0)
            | (BX_OHCI_THIS hub.op_regs.HcRhStatus.reserved0     << 18)
            | (BX_OHCI_THIS hub.op_regs.HcRhStatus.ocic      ? 1 << 17 : 0)
            | 0 //BX_OHCI_THIS hub.op_regs.HcRhStatus.lpsc      << 16
            | (BX_OHCI_THIS hub.op_regs.HcRhStatus.drwe      ? 1 << 15 : 0)
            | (BX_OHCI_THIS hub.op_regs.HcRhStatus.reserved1     <<  2)
            | (BX_OHCI_THIS hub.op_regs.HcRhStatus.oci       ? 1 <<  1 : 0)
            | (BX_OHCI_THIS hub.op_regs.HcRhStatus.lps       ? 1 <<  0 : 0);
      break;

    case 0x60: // HcRhPortStatus[3]
#if (USB_NUM_PORTS < 4)
      val = 0;
      break;
#else
      p = 3;
#endif
    case 0x5C: // HcRhPortStatus[2]
#if (USB_NUM_PORTS < 3)
      val = 0;
      break;
#else
      p = 2;
#endif
    case 0x58: // HcRhPortStatus[1]
#if (USB_NUM_PORTS < 2)
      val = 0;
      break;
#else
      p = 1;
#endif
    case 0x54: // HcRhPortStatus[0]
      if (BX_OHCI_THIS hub.op_regs.HcRhPortStatus[p].pps == 1) {
        p = (offset - 0x54) >> 2;
        val =   (BX_OHCI_THIS hub.op_regs.HcRhPortStatus[p].reserved0  << 21)
              | (BX_OHCI_THIS hub.op_regs.HcRhPortStatus[p].prsc      ? (1 << 20) : 0)
              | (BX_OHCI_THIS hub.op_regs.HcRhPortStatus[p].ocic      ? (1 << 19) : 0)
              | (BX_OHCI_THIS hub.op_regs.HcRhPortStatus[p].pssc      ? (1 << 18) : 0)
              | (BX_OHCI_THIS hub.op_regs.HcRhPortStatus[p].pesc      ? (1 << 17) : 0)
              | (BX_OHCI_THIS hub.op_regs.HcRhPortStatus[p].csc       ? (1 << 16) : 0)
              | (BX_OHCI_THIS hub.op_regs.HcRhPortStatus[p].reserved1     << 10)
              | (BX_OHCI_THIS hub.op_regs.HcRhPortStatus[p].lsda      ? (1 <<  9) : 0)
              | (BX_OHCI_THIS hub.op_regs.HcRhPortStatus[p].pps       ? (1 <<  8) : 0)
              | (BX_OHCI_THIS hub.op_regs.HcRhPortStatus[p].reserved2     <<  5)
              | (BX_OHCI_THIS hub.op_regs.HcRhPortStatus[p].prs       ? (1 <<  4) : 0)
              | (BX_OHCI_THIS hub.op_regs.HcRhPortStatus[p].poci      ? (1 <<  3) : 0)
              | (BX_OHCI_THIS hub.op_regs.HcRhPortStatus[p].pss       ? (1 <<  2) : 0)
              | (BX_OHCI_THIS hub.op_regs.HcRhPortStatus[p].pes       ? (1 <<  1) : 0)
              | (BX_OHCI_THIS hub.op_regs.HcRhPortStatus[p].ccs       ? (1 <<  0) : 0);
      } else
        val = 0;
      break;

    default:
      BX_ERROR(("unsupported read from address=0x%08X!", addr));
      break;
  }

  int name = offset >> 2;
  if (name > (0x60 >> 2))
    name = 25;
//  BX_INFO(("register read from address 0x%04X (%s):  0x%08X (len=%i)", (unsigned) addr, usb_ohci_port_name[name], (Bit32u) val, len));
  *((Bit32u *) data) = val;

  return 1;
}

bx_bool bx_usb_ohci_c::write_handler(bx_phy_address addr, unsigned len, void *data, void *param)
{
  Bit32u value = *((Bit32u *) data);
  Bit32u  offset = addr - BX_OHCI_THIS hub.base_addr;
  int p;

  int name = offset >> 2;
  if (name > (0x60 >> 2))
    name = 25;
//  BX_INFO(("register write to  address 0x%04X (%s):  0x%08X (len=%i)", (unsigned) addr, usb_ohci_port_name[name], (unsigned) value, len));

  if (len != 4) {
    BX_INFO(("Write at 0x%08X with len != 4 (%i)", addr, len));
    return 1;
  }

  switch (offset) {
    case 0x00: // HcRevision
      BX_ERROR(("Write to HcRevision ignored"));
      break;

    case 0x04: // HcControl
      if (value & 0xFFFFF800)
        BX_ERROR(("Write to reserved field in HcControl"));
      BX_OHCI_THIS hub.op_regs.HcControl.rwe      = (value & (1<<10)) ? 1 : 0;
      BX_OHCI_THIS hub.op_regs.HcControl.rwc      = (value & (1<< 9)) ? 1 : 0;
      BX_OHCI_THIS hub.op_regs.HcControl.ir       = (value & (1<< 8)) ? 1 : 0;
      BX_OHCI_THIS hub.op_regs.HcControl.hcfs     = (value & (3<< 6)) >>  6;
      BX_OHCI_THIS hub.op_regs.HcControl.ble      = (value & (1<< 5)) ? 1 : 0;
      BX_OHCI_THIS hub.op_regs.HcControl.cle      = (value & (1<< 4)) ? 1 : 0;
      BX_OHCI_THIS hub.op_regs.HcControl.ie       = (value & (1<< 3)) ? 1 : 0;
      BX_OHCI_THIS hub.op_regs.HcControl.ple      = (value & (1<< 2)) ? 1 : 0;
      BX_OHCI_THIS hub.op_regs.HcControl.cbsr     = (value & (3<< 0)) >>  0;
      if (BX_OHCI_THIS hub.op_regs.HcControl.hcfs == 0x02) {
        BX_OHCI_THIS hub.op_regs.HcFmRemaining.fr = BX_OHCI_THIS hub.op_regs.HcFmInterval.fi;
        BX_OHCI_THIS hub.op_regs.HcFmRemaining.frt = 0;
      }
      break;

    case 0x08: // HcCommandStatus
      if (value & 0xFFFCFFF0)
        BX_ERROR(("Write to a reserved field in HcCommandStatus"));
      if (value & (3<<16))
        BX_ERROR(("Write to R/O field: HcCommandStatus.soc"));
      if (value & (1<< 3)) BX_OHCI_THIS hub.op_regs.HcCommandStatus.ocr = 1;
      if (value & (1<< 2)) BX_OHCI_THIS hub.op_regs.HcCommandStatus.blf = 1;
      if (value & (1<< 1)) BX_OHCI_THIS hub.op_regs.HcCommandStatus.clf = 1;
      if (value & (1<< 0)) {
        BX_OHCI_THIS hub.op_regs.HcCommandStatus.hcr = 1;
        BX_OHCI_THIS reset_hc();
        BX_OHCI_THIS hub.op_regs.HcControl.hcfs = 3;      // suspend state
        for (unsigned i=0; i<USB_NUM_PORTS; i++)
          if (BX_OHCI_THIS hub.op_regs.HcRhPortStatus[i].ccs && (BX_OHCI_THIS hub.usb_port[i].device != NULL))
            BX_OHCI_THIS usb_send_msg(BX_OHCI_THIS hub.usb_port[i].device, USB_MSG_RESET);
      }
      break;

    case 0x0C: // HcInterruptStatus /// all are WC
      if (value & 0xBFFFFF80)
        BX_DEBUG(("Write to a reserved field in HcInterruptStatus"));
      if (value & (1<<30)) BX_OHCI_THIS hub.op_regs.HcInterruptStatus.oc   = 0;
      if (value & (1<< 6)) BX_OHCI_THIS hub.op_regs.HcInterruptStatus.rhsc = 0;
      if (value & (1<< 5)) BX_OHCI_THIS hub.op_regs.HcInterruptStatus.fno  = 0;
      if (value & (1<< 4)) BX_OHCI_THIS hub.op_regs.HcInterruptStatus.ue   = 0;
      if (value & (1<< 3)) BX_OHCI_THIS hub.op_regs.HcInterruptStatus.rd   = 0;
      if (value & (1<< 2)) BX_OHCI_THIS hub.op_regs.HcInterruptStatus.sf   = 0;
      if (value & (1<< 1)) BX_OHCI_THIS hub.op_regs.HcInterruptStatus.wdh  = 0;
      if (value & (1<< 0)) BX_OHCI_THIS hub.op_regs.HcInterruptStatus.so   = 0;
      break;

    case 0x10: // HcInterruptEnable
      if (value & 0x3FFFFF80)
        BX_ERROR(("Write to a reserved field in HcInterruptEnable"));
      if (value & (1<<31)) BX_OHCI_THIS hub.op_regs.HcInterruptEnable.mie  = 1;
      if (value & (1<<30)) BX_OHCI_THIS hub.op_regs.HcInterruptEnable.oc   = 1;
      if (value & (1<< 6)) BX_OHCI_THIS hub.op_regs.HcInterruptEnable.rhsc = 1;
      if (value & (1<< 5)) BX_OHCI_THIS hub.op_regs.HcInterruptEnable.fno  = 1;
      if (value & (1<< 4)) BX_OHCI_THIS hub.op_regs.HcInterruptEnable.ue   = 1;
      if (value & (1<< 3)) BX_OHCI_THIS hub.op_regs.HcInterruptEnable.rd   = 1;
      if (value & (1<< 2)) BX_OHCI_THIS hub.op_regs.HcInterruptEnable.sf   = 1;
      if (value & (1<< 1)) BX_OHCI_THIS hub.op_regs.HcInterruptEnable.wdh  = 1;
      if (value & (1<< 0)) BX_OHCI_THIS hub.op_regs.HcInterruptEnable.so   = 1;
      set_irq_state();
      break;

    case 0x14: // HcInterruptDisable
      if (value & 0x3FFFFF80)
        BX_ERROR(("Write to a reserved field in HcInterruptDisable"));
      if (value & (1<<31)) BX_OHCI_THIS hub.op_regs.HcInterruptEnable.mie  = 0;
      if (value & (1<<30)) BX_OHCI_THIS hub.op_regs.HcInterruptEnable.oc   = 0;
      if (value & (1<< 6)) BX_OHCI_THIS hub.op_regs.HcInterruptEnable.rhsc = 0;
      if (value & (1<< 5)) BX_OHCI_THIS hub.op_regs.HcInterruptEnable.fno  = 0;
      if (value & (1<< 4)) BX_OHCI_THIS hub.op_regs.HcInterruptEnable.ue   = 0;
      if (value & (1<< 3)) BX_OHCI_THIS hub.op_regs.HcInterruptEnable.rd   = 0;
      if (value & (1<< 2)) BX_OHCI_THIS hub.op_regs.HcInterruptEnable.sf   = 0;
      if (value & (1<< 1)) BX_OHCI_THIS hub.op_regs.HcInterruptEnable.wdh  = 0;
      if (value & (1<< 0)) BX_OHCI_THIS hub.op_regs.HcInterruptEnable.so   = 0;
      break;

    case 0x18: // HcHCCA
      // the HCD can write 0xFFFFFFFF to this register to see what the alignement is
      //  by reading back the amount and seeing how many lower bits are clear.
      if ((value & 0x000000FF) && (value != 0xFFFFFFFF))
        BX_ERROR(("Write to lower byte of HcHCCA non zero."));
      BX_OHCI_THIS hub.op_regs.HcHCCA.hcca = (value & 0xFFFFFF00) >> 8;
      break;

    case 0x1C: // HcPeriodCurrentED
      BX_ERROR(("Write to HcPeriodCurrentED not allowed."));
      break;

    case 0x20: // HcControlHeadED
      if (value & 0x0000000F)
        BX_ERROR(("Write to lower nibble of HcControlHeadED non zero."));
      BX_OHCI_THIS hub.op_regs.HcControlHeadED.ched = (value & 0xFFFFFFF0) >> 4;
      break;

    case 0x24: // HcControlCurrentED
      if (value & 0x0000000F)
        BX_ERROR(("Write to lower nibble of HcControlCurrentED non zero."));
      BX_OHCI_THIS hub.op_regs.HcControlCurrentED.cced = (value & 0xFFFFFFF0) >> 4;
      break;

    case 0x28: // HcBulkHeadED
      if (value & 0x0000000F)
        BX_ERROR(("Write to lower nibble of HcBulkHeadED non zero."));
      BX_OHCI_THIS hub.op_regs.HcBulkHeadED.bhed = (value & 0xFFFFFFF0) >> 4;
      break;

    case 0x2C: // HcBulkCurrentED
      if (value & 0x0000000F)
        BX_ERROR(("Write to lower nibble of HcBulkCurrentED non zero."));
      BX_OHCI_THIS hub.op_regs.HcBulkCurrentED.bced = (value & 0xFFFFFFF0) >> 4;
      break;

    case 0x30: // HcDoneHead
      BX_ERROR(("Write to HcDoneHead not allowed."));
      break;

    case 0x34: // HcFmInterval
      if (value & 0x0000C000)
        BX_ERROR(("Write to a reserved field in HcFmInterval."));
      BX_OHCI_THIS hub.op_regs.HcFmInterval.fit      = (value & (1<<31)) ? 1 : 0;
      BX_OHCI_THIS hub.op_regs.HcFmInterval.fsmps    = (value & 0x7FFF0000) >> 16;
      BX_OHCI_THIS hub.op_regs.HcFmInterval.fi       = (value & 0x00003FFF) >> 0;
      break;

    case 0x38: // HcFmRemaining
      BX_ERROR(("Write to HcFmRemaining not allowed."));
      break;

    case 0x3C: // HcFmNumber
      BX_ERROR(("Write to HcFmNumber not allowed."));
      break;

    case 0x40: // HcPeriodicStart
      if (value & 0xFFFFC000)
        BX_ERROR(("Write to a reserved field in HcPeriodicStart."));
      BX_OHCI_THIS hub.op_regs.HcPeriodicStart.ps       = (value & 0x00003FFF);
      break;

    case 0x44: // HcLSThreshold
      BX_ERROR(("Write to HcLSThreshold not allowed."));
      break;

    case 0x48: // HcRhDescriptorA
      if (value & 0x00FFE000)
        BX_ERROR(("Write to a reserved field in HcRhDescriptorA."));
      if (value & 0x000000FF)
        BX_ERROR(("Write to HcRhDescriptorA.ndp not allowed."));
      if (value & (1<<10))
        BX_ERROR(("Write to HcRhDescriptorA.dt not allowed."));
      BX_OHCI_THIS hub.op_regs.HcRhDescriptorA.potpgt   = (value & 0xFF000000) >> 24;
      BX_OHCI_THIS hub.op_regs.HcRhDescriptorA.nocp     = (value & (1<<12)) ? 1 : 0;
      BX_OHCI_THIS hub.op_regs.HcRhDescriptorA.ocpm     = (value & (1<<11)) ? 1 : 0;
      BX_OHCI_THIS hub.op_regs.HcRhDescriptorA.nps      = (value & (1<< 9)) ? 1 : 0;
      BX_OHCI_THIS hub.op_regs.HcRhDescriptorA.psm      = (value & (1<< 8)) ? 1 : 0;
      if (BX_OHCI_THIS hub.op_regs.HcRhDescriptorA.psm == 0) {

        BX_INFO(("Ben: BX_OHCI_THIS hub.op_regs.HcRhDescriptorA.psm == 0"));
        // all ports have power, etc.
        // BX_USB_OHCI_THIS hub.op_regs.HcRhPortStatus[p].pps = 1
        //  Call a routine to set each ports dword (LS, Connected, etc.)

      } else {

        BX_INFO(("Ben: BX_OHCI_THIS hub.op_regs.HcRhDescriptorA.psm == 1"));
        // only ports with bit set in rhstatus have power, etc.
        //  Call a routine to set each ports dword (LS, Connected, etc.)

      }
      break;

    case 0x4C: // HcRhDescriptorB
      BX_OHCI_THIS hub.op_regs.HcRhDescriptorB.ppcm = (value & 0xFFFF0000) >> 16;
      BX_OHCI_THIS hub.op_regs.HcRhDescriptorB.dr   = (value & 0x0000FFFF) >>  0;
      break;

    case 0x50: { // HcRhStatus
      if (value & 0x7FFC7FFC)
        BX_ERROR(("Write to a reserved field in HcRhStatus."));
      if (value & (1<<1))
        BX_ERROR(("Write to HcRhStatus.oci not allowed."));
      // which one of these two takes presidence?
      if (value & (1<<31)) BX_OHCI_THIS hub.op_regs.HcRhStatus.drwe = 0;
      if (value & (1<<15)) BX_OHCI_THIS hub.op_regs.HcRhStatus.drwe = 1;

      if (value & (1<<17)) BX_OHCI_THIS hub.op_regs.HcRhStatus.ocic = 1;
      if (value & (1<<16)) {
        if (BX_OHCI_THIS hub.op_regs.HcRhDescriptorA.psm == 0) {
          for (p=0; p<USB_NUM_PORTS; p++)
            BX_OHCI_THIS hub.op_regs.HcRhPortStatus[p].pps = 1;
        } else {
          for (p=0; p<USB_NUM_PORTS; p++)
            if ((BX_OHCI_THIS hub.op_regs.HcRhDescriptorB.ppcm & (1<<p)) == 0)
              BX_OHCI_THIS hub.op_regs.HcRhPortStatus[p].pps = 1;
        }
      }
      if (value & (1<<0)) {
        if (BX_OHCI_THIS hub.op_regs.HcRhDescriptorA.psm == 0) {
          for (p=0; p<USB_NUM_PORTS; p++)
            BX_OHCI_THIS hub.op_regs.HcRhPortStatus[p].pps = 0;
        } else {
          for (p=0; p<USB_NUM_PORTS; p++)
            if (!(BX_OHCI_THIS hub.op_regs.HcRhDescriptorB.ppcm & (1<<p)))
              BX_OHCI_THIS hub.op_regs.HcRhPortStatus[p].pps = 0;
        }
      }
      break;
    }

    case 0x60: // HcRhPortStatus[3]
#if (USB_NUM_PORTS < 4)
      break;
#endif
    case 0x5C: // HcRhPortStatus[2]
#if (USB_NUM_PORTS < 3)
      break;
#endif
    case 0x58: // HcRhPortStatus[1]
#if (USB_NUM_PORTS < 2)
      break;
#endif
    case 0x54: { // HcRhPortStatus[0]
      p = (offset - 0x54) >> 2;
      if (value & 0xFFE0FCE0)
        BX_ERROR(("Write to a reserved field in HcRhPortStatus[p]."));
      if (value & (1<<0))
        BX_OHCI_THIS hub.op_regs.HcRhPortStatus[p].pes = 0;
      if (value & (1<<1)) {
        if (BX_OHCI_THIS hub.op_regs.HcRhPortStatus[p].ccs == 0)
          BX_OHCI_THIS hub.op_regs.HcRhPortStatus[p].csc = 1;
        else
          BX_OHCI_THIS hub.op_regs.HcRhPortStatus[p].pes = 1;
      }
      if (value & (1<<2)) {
        if (BX_OHCI_THIS hub.op_regs.HcRhPortStatus[p].ccs == 0)
          BX_OHCI_THIS hub.op_regs.HcRhPortStatus[p].csc = 1;
        else
          BX_OHCI_THIS hub.op_regs.HcRhPortStatus[p].pss = 1;
      }
//      if (value & (1<<3))
//        if (BX_OHCI_THIS hub.op_regs.HcRhPortStatus[p].pss)
//          ; // do a resume (or test this in the timer code and do the resume there)
      if (value & (1<<4)) {
        if (BX_OHCI_THIS hub.op_regs.HcRhPortStatus[p].ccs == 0)
          BX_OHCI_THIS hub.op_regs.HcRhPortStatus[p].csc = 1;
        else {
          reset_port(p);
          BX_OHCI_THIS hub.op_regs.HcRhPortStatus[p].pps = 1;
          BX_OHCI_THIS hub.op_regs.HcRhPortStatus[p].pes = 1;
          BX_OHCI_THIS hub.op_regs.HcRhPortStatus[p].prsc = 1;
          // are we are currently connected/disconnected
          if (BX_OHCI_THIS hub.usb_port[p].device != NULL) {
            BX_OHCI_THIS hub.op_regs.HcRhPortStatus[p].lsda =
              (BX_OHCI_THIS hub.usb_port[p].device->get_speed() == USB_SPEED_LOW);
            usb_set_connect_status(p, BX_OHCI_THIS hub.usb_port[p].device->get_type(), 1);
            BX_OHCI_THIS usb_send_msg(BX_OHCI_THIS hub.usb_port[p].device, USB_MSG_RESET);
          }
          BX_OHCI_THIS hub.op_regs.HcInterruptStatus.rhsc = 1;
          set_irq_state();
        }
      }
      if (value & (1<<8))
        BX_OHCI_THIS hub.op_regs.HcRhPortStatus[p].pps = 1;
      if (value & (1<<9))
        BX_OHCI_THIS hub.op_regs.HcRhPortStatus[p].pps = 0;
      if (value & (1<<16))
        BX_OHCI_THIS hub.op_regs.HcRhPortStatus[p].csc = (value & ((1<<4) | (1<<1) | (1<<2))) ? 1 : 0;
      if (value & (1<<17))
        BX_OHCI_THIS hub.op_regs.HcRhPortStatus[p].pesc = 0;
      if (value & (1<<18))
        BX_OHCI_THIS hub.op_regs.HcRhPortStatus[p].pssc = 0;
      if (value & (1<<19))
        BX_OHCI_THIS hub.op_regs.HcRhPortStatus[p].ocic = 0;
      if (value & (1<<20))
        BX_OHCI_THIS hub.op_regs.HcRhPortStatus[p].prsc = 0;
      break;
    }

    default:
      BX_ERROR(("unsupported write to address=0x%08X, val = 0x%08X!", addr, value));
      break;
  }

  return 1;
}

void bx_usb_ohci_c::usb_interval_handler(void *this_ptr)
{
  bx_usb_ohci_c *class_ptr = (bx_usb_ohci_c *) this_ptr;
  class_ptr->usb_interval_timer();
}

// Called once every 1uS
void bx_usb_ohci_c::usb_interval_timer(void)
{
  // if remaining > 0, decrement it
  //(0x2EDF+1) / 12 = 1000
  if (BX_OHCI_THIS hub.op_regs.HcFmRemaining.fr > 0)
    BX_OHCI_THIS hub.op_regs.HcFmRemaining.fr =- 12;
}

void bx_usb_ohci_c::usb_frame_handler(void *this_ptr)
{
  bx_usb_ohci_c *class_ptr = (bx_usb_ohci_c *) this_ptr;
  class_ptr->usb_frame_timer();
}

// Called once every 1mS
void bx_usb_ohci_c::usb_frame_timer(void)
{
  struct OHCI_ED cur_ed;
  Bit32u address, ed_address;
  Bit16u zero = 0;

  if (BX_OHCI_THIS hub.op_regs.HcControl.hcfs == 2) {
    // set remaining to the interval amount.
    BX_OHCI_THIS hub.op_regs.HcFmRemaining.fr = BX_OHCI_THIS hub.op_regs.HcFmInterval.fi;
    BX_OHCI_THIS hub.op_regs.HcFmRemaining.frt = BX_OHCI_THIS hub.op_regs.HcFmInterval.fit;

    // The Frame Number Register is incremented
    //  every time bit 15 is changed (at 0x8000 or 0x0000), fno is fired.
    BX_OHCI_THIS hub.op_regs.HcFmNumber.fn++;
    DEV_MEM_WRITE_PHYSICAL((BX_OHCI_THIS hub.op_regs.HcHCCA.hcca << 8) + 0x80, 2, (Bit8u *) &BX_OHCI_THIS hub.op_regs.HcFmNumber.fn);
    DEV_MEM_WRITE_PHYSICAL((BX_OHCI_THIS hub.op_regs.HcHCCA.hcca << 8) + 0x82, 2, (Bit8u *) &zero);
    if ((BX_OHCI_THIS hub.op_regs.HcFmNumber.fn == 0x8000) || (BX_OHCI_THIS hub.op_regs.HcFmNumber.fn == 0x0000)) {
      BX_OHCI_THIS hub.op_regs.HcInterruptStatus.fno = 1;
      set_irq_state();
    }

    //
    BX_OHCI_THIS hub.op_regs.HcInterruptStatus.sf = 1;
    set_irq_state();

    // if interrupt delay (done_count) == 0, and status.wdh == 0, then update the donehead fields.
    BX_DEBUG(("done_count = %i, status.wdh = %i", BX_OHCI_THIS hub.ohci_done_count, BX_OHCI_THIS hub.op_regs.HcInterruptStatus.wdh));
    if ((BX_OHCI_THIS hub.ohci_done_count == 0) && (BX_OHCI_THIS hub.op_regs.HcInterruptStatus.wdh == 0)) {
      Bit32u temp = BX_OHCI_THIS hub.op_regs.HcDoneHead.dh << 4;
      if (BX_OHCI_THIS hub.op_regs.HcInterruptStatus.oc || BX_OHCI_THIS hub.op_regs.HcInterruptStatus.rhsc ||
          BX_OHCI_THIS hub.op_regs.HcInterruptStatus.fno || BX_OHCI_THIS hub.op_regs.HcInterruptStatus.ue ||
          BX_OHCI_THIS hub.op_regs.HcInterruptStatus.rd || BX_OHCI_THIS hub.op_regs.HcInterruptStatus.sf ||
          BX_OHCI_THIS hub.op_regs.HcInterruptStatus.so)
        temp |= 1;
      BX_DEBUG(("Updating the hcca.DoneHead field to 0x%08X and setting the wdh flag", temp));
      DEV_MEM_WRITE_PHYSICAL((BX_OHCI_THIS hub.op_regs.HcHCCA.hcca << 8) + 0x84, 4, (Bit8u *) &temp);
      BX_OHCI_THIS hub.op_regs.HcDoneHead.dh = 0;
      BX_OHCI_THIS hub.ohci_done_count = 7;
      BX_OHCI_THIS hub.op_regs.HcInterruptStatus.wdh = 1;
      set_irq_state();
    }

    // if (6 >= done_count > 0) then decrement done_count
    if ((BX_OHCI_THIS hub.ohci_done_count != 7) && (BX_OHCI_THIS hub.ohci_done_count > 0))
      BX_OHCI_THIS hub.ohci_done_count--;

    // TODO:  Rather than just comparing .fr to <8000 here, and <4000 below, see the highlighted
    //   statement on page 45.

    // if the control list is enabled *and* the control list filled bit is set, do a control list ED
    if (BX_OHCI_THIS hub.op_regs.HcControl.cle && BX_OHCI_THIS hub.op_regs.HcCommandStatus.clf) {
      BX_OHCI_THIS hub.op_regs.HcCommandStatus.clf = 0;
      ed_address = (BX_OHCI_THIS hub.op_regs.HcControlCurrentED.cced << 4);
      while (ed_address) {
        DEV_MEM_READ_PHYSICAL(ed_address,      4, (Bit8u*) &cur_ed.dword0);
        DEV_MEM_READ_PHYSICAL(ed_address +  4, 4, (Bit8u*) &cur_ed.dword1);
        DEV_MEM_READ_PHYSICAL(ed_address +  8, 4, (Bit8u*) &cur_ed.dword2);
        DEV_MEM_READ_PHYSICAL(ed_address + 12, 4, (Bit8u*) &cur_ed.dword3);
        if (process_ed(&cur_ed, ed_address, 1))
          BX_OHCI_THIS hub.op_regs.HcCommandStatus.clf = 1;
        BX_OHCI_THIS hub.op_regs.HcControlCurrentED.cced = ED_GET_NEXTED(&cur_ed) >> 4;
        if (BX_OHCI_THIS hub.op_regs.HcFmRemaining.fr < 8000)
          goto do_bulk_eds;
        ed_address = ED_GET_NEXTED(&cur_ed);
      }
      BX_OHCI_THIS hub.op_regs.HcControlCurrentED.cced = BX_OHCI_THIS hub.op_regs.HcControlHeadED.ched;
    }

do_bulk_eds:
    // if the bulk list is enabled *and* the bulk list filled bit is set, do a bulk list ED
    if (BX_OHCI_THIS hub.op_regs.HcControl.ble && BX_OHCI_THIS hub.op_regs.HcCommandStatus.blf) {
      BX_OHCI_THIS hub.op_regs.HcCommandStatus.blf = 0;
      ed_address = (BX_OHCI_THIS hub.op_regs.HcBulkCurrentED.bced << 4);
      while (ed_address) {
        DEV_MEM_READ_PHYSICAL(ed_address,      4, (Bit8u*) &cur_ed.dword0);
        DEV_MEM_READ_PHYSICAL(ed_address +  4, 4, (Bit8u*) &cur_ed.dword1);
        DEV_MEM_READ_PHYSICAL(ed_address +  8, 4, (Bit8u*) &cur_ed.dword2);
        DEV_MEM_READ_PHYSICAL(ed_address + 12, 4, (Bit8u*) &cur_ed.dword3);
        if (process_ed(&cur_ed, ed_address, 1))
          BX_OHCI_THIS hub.op_regs.HcCommandStatus.blf = 1;
        BX_OHCI_THIS hub.op_regs.HcBulkCurrentED.bced = ED_GET_NEXTED(&cur_ed) >> 4;
        if (BX_OHCI_THIS hub.op_regs.HcFmRemaining.fr < 4000)
          goto do_iso_eds;
        ed_address = ED_GET_NEXTED(&cur_ed);
      }
      BX_OHCI_THIS hub.op_regs.HcBulkCurrentED.bced = BX_OHCI_THIS hub.op_regs.HcBulkHeadED.bhed;
    }

do_iso_eds:
    // do the ED's in the interrupt table
    address = (BX_OHCI_THIS hub.op_regs.HcHCCA.hcca << 8) + ((BX_OHCI_THIS hub.op_regs.HcFmNumber.fn & 0x1F) * 4);
    DEV_MEM_READ_PHYSICAL(address, 4, (Bit8u*) &ed_address);
    while (ed_address) {
      DEV_MEM_READ_PHYSICAL(ed_address,      4, (Bit8u*) &cur_ed.dword0);
      DEV_MEM_READ_PHYSICAL(ed_address +  4, 4, (Bit8u*) &cur_ed.dword1);
      DEV_MEM_READ_PHYSICAL(ed_address +  8, 4, (Bit8u*) &cur_ed.dword2);
      DEV_MEM_READ_PHYSICAL(ed_address + 12, 4, (Bit8u*) &cur_ed.dword3);
      process_ed(&cur_ed, ed_address, BX_OHCI_THIS hub.op_regs.HcControl.ple);
      ed_address = ED_GET_NEXTED(&cur_ed);
    }

  }  // end run schedule
}

//  see http://www.koders.com/c/fid100DD4B4D99FF9CC7179538BCE26685B577697C4.aspx for more

bx_bool bx_usb_ohci_c::process_ed(struct OHCI_ED *ed, const Bit32u ed_address, const bx_bool enabled)
{
  bx_bool fnd = 0;
  struct OHCI_TD cur_td;

  if (!ED_GET_H(ed) && !ED_GET_K(ed) && (ED_GET_HEADP(ed) != ED_GET_TAILP(ed))) {
    // if the isochronous is enabled and ed is a isochronous, do TD
    if (ED_GET_F(ed) && BX_OHCI_THIS hub.op_regs.HcControl.ie) {
      // load and do a isochronous TD list
      BX_INFO(("Ben: Found a valid ED that points to an isochronous TD"));
      // we currently ignore ISO TD's
    }
    if (!ED_GET_F(ed) && enabled) {
      BX_INFO(("Ben: Found a valid ED that points to an control/bulk/int TD"));
      while (ED_GET_HEADP(ed) && (ED_GET_HEADP(ed) != ED_GET_TAILP(ed))) {
        DEV_MEM_READ_PHYSICAL(ED_GET_HEADP(ed),      4, (Bit8u*) &cur_td.dword0);
        DEV_MEM_READ_PHYSICAL(ED_GET_HEADP(ed) +  4, 4, (Bit8u*) &cur_td.dword1);
        DEV_MEM_READ_PHYSICAL(ED_GET_HEADP(ed) +  8, 4, (Bit8u*) &cur_td.dword2);
        DEV_MEM_READ_PHYSICAL(ED_GET_HEADP(ed) + 12, 4, (Bit8u*) &cur_td.dword3);
        BX_INFO(("TD: 0x%08X", ED_GET_HEADP(ed)));
        if (process_td(&cur_td, ed)) {
          const Bit32u temp = ED_GET_HEADP(ed);
          ED_SET_HEADP(ed, TD_GET_NEXTTD(&cur_td));
          TD_SET_NEXTTD(&cur_td, BX_OHCI_THIS hub.op_regs.HcDoneHead.dh << 4);
          BX_OHCI_THIS hub.op_regs.HcDoneHead.dh = temp >> 4;
          DEV_MEM_WRITE_PHYSICAL(temp,      4, (Bit8u*) &cur_td.dword0);
          DEV_MEM_WRITE_PHYSICAL(temp +  4, 4, (Bit8u*) &cur_td.dword1);
          DEV_MEM_WRITE_PHYSICAL(temp +  8, 4, (Bit8u*) &cur_td.dword2);
          DEV_MEM_WRITE_PHYSICAL(temp + 12, 4, (Bit8u*) &cur_td.dword3);
          if (TD_GET_DI(&cur_td) < BX_OHCI_THIS hub.ohci_done_count)
            BX_OHCI_THIS hub.ohci_done_count = TD_GET_DI(&cur_td);
          fnd = 1;
        } else
          break;
      }
    }
    DEV_MEM_WRITE_PHYSICAL(ed_address,      4, (Bit8u*) &ed->dword0);
    DEV_MEM_WRITE_PHYSICAL(ed_address +  4, 4, (Bit8u*) &ed->dword1);
    DEV_MEM_WRITE_PHYSICAL(ed_address +  8, 4, (Bit8u*) &ed->dword2);
    DEV_MEM_WRITE_PHYSICAL(ed_address + 12, 4, (Bit8u*) &ed->dword3);
  }

  return fnd;
}

bx_bool bx_usb_ohci_c::process_td(struct OHCI_TD *td, struct OHCI_ED *ed)
{
  usb_device_c *dev = NULL;
  unsigned i, pid = 0, len = 0;
  int ret = 0;

  // The td->cc field should be 111x if it hasn't been processed yet.
  if (TD_GET_CC(td) < NotAccessed) {
    BX_ERROR(("Found TD with CC value not 111x"));
    return 0;
  }

  if (ED_GET_D(ed) == 1)
    pid = USB_TOKEN_OUT;
  else if (ED_GET_D(ed) == 2)
    pid = USB_TOKEN_IN;
  else {
    if (TD_GET_DP(td) == 0)
      pid = USB_TOKEN_SETUP;
    else if (TD_GET_DP(td) == 1)
      pid = USB_TOKEN_OUT;
    else if (TD_GET_DP(td) == 2)
      pid = USB_TOKEN_IN;
  }

  // find the device
  for (i=0; i<USB_NUM_PORTS; i++) {
    if (BX_OHCI_THIS hub.usb_port[i].device != NULL) {
      if (BX_OHCI_THIS hub.usb_port[i].device->get_connected()) {
        if (BX_OHCI_THIS hub.usb_port[i].device->get_address() == ED_GET_FA(ed)) {
          dev = BX_OHCI_THIS hub.usb_port[i].device;
          break;
        }
      }
    }
  }
  if (dev == NULL) {
    if ((pid == USB_TOKEN_OUT) && (ED_GET_MPS(ed) == 0x7FF) && (ED_GET_FA(ed) == 0)) {
      // This is the "keep awake" packet that Windows sends once a schedule cycle.
      // For now, let it pass through to the code below.
    } else {
      TD_SET_CC(td, DeviceNotResponding);
      TD_SET_EC(td, 3);
      BX_PANIC(("Device not found for addr: %i", ED_GET_FA(ed)));
      return 1;  // device not found
    }
  }

  // calculate the length of the packet
  if (TD_GET_CBP(td) && TD_GET_BE(td)) {
    if ((TD_GET_CBP(td) & 0xFFFFF000) != (TD_GET_BE(td) & 0xFFFFF000))
      len = (TD_GET_BE(td) & 0xFFF) + 0x1001 - (TD_GET_CBP(td) & 0xFFF);
    else
      len = (TD_GET_BE(td) - TD_GET_CBP(td)) + 1;
  } else
    len = 0;

  if (dev != NULL) {
    BX_OHCI_THIS usb_packet.pid = pid;
    BX_OHCI_THIS usb_packet.devaddr = ED_GET_FA(ed);
    BX_OHCI_THIS usb_packet.devep = ED_GET_EN(ed);
    BX_OHCI_THIS usb_packet.data = BX_OHCI_THIS device_buffer;
    switch (pid) {
      case USB_TOKEN_SETUP:
      case USB_TOKEN_OUT:
        BX_OHCI_THIS usb_packet.len = (len <= ED_GET_MPS(ed)) ? len : ED_GET_MPS(ed);
        break;
      case USB_TOKEN_IN:
        BX_OHCI_THIS usb_packet.len = len;
        break;
    }

    BX_INFO(("    pid = %i  addr = %i   endpnt = %i    len = %i  mps = %i (0x%08X   0x%08X)", pid, ED_GET_FA(ed), ED_GET_EN(ed), len, ED_GET_MPS(ed), TD_GET_CBP(td), TD_GET_BE(td)));
    BX_INFO(("    td->t = %i  ed->c = %i  td->di = %i  td->r = %i", TD_GET_T(td), ED_GET_C(ed), TD_GET_DI(td), TD_GET_R(td)));

    switch (pid) {
      case USB_TOKEN_SETUP:
        if (len > 0)
          DEV_MEM_READ_PHYSICAL_BLOCK(TD_GET_CBP(td), len, device_buffer);
        // TODO: This is a hack.  dev->handle_packet() should return the amount of bytes
        //  it received, not the amount it anticipates on receiving/sending in the next packet.
        if ((ret = dev->handle_packet(&BX_OHCI_THIS usb_packet)) >= 0)
          ret = 8;
        break;
      case USB_TOKEN_OUT:
        if (len > 0)
          DEV_MEM_READ_PHYSICAL_BLOCK(TD_GET_CBP(td), len, device_buffer);
        ret = dev->handle_packet(&BX_OHCI_THIS usb_packet);
        break;
      case USB_TOKEN_IN:
        ret = dev->handle_packet(&BX_OHCI_THIS usb_packet);
        if (ret >= 0) {
          //if (ret > (int) ED_GET_MPS(ed))
          //  ret = USB_RET_BABBLE;
          if (ret > 0)
            DEV_MEM_WRITE_PHYSICAL_BLOCK(TD_GET_CBP(td), ret, device_buffer);
        } else
          ret = 0;
        BX_INFO(("  %02X %02X %02X %02X %02X %02X %02X %02X", device_buffer[0], device_buffer[1], device_buffer[2],
          device_buffer[3], device_buffer[4], device_buffer[5], device_buffer[6], device_buffer[7]));
        break;
      default:
        TD_SET_CC(td, UnexpectedPID);
        TD_SET_EC(td, 3);
        return 1;
    }

    if ((ret == (int)len) || ((pid == USB_TOKEN_IN) && (ret >= 0) && TD_GET_R(td))) {
      if (ret == (int)len)
        TD_SET_CBP(td, 0);
      else {
        TD_SET_CBP(td, TD_GET_CBP(td) + ret);
        if (((TD_GET_CBP(td) & 0x0FFF) + ret) > 0x0FFF) {
          TD_SET_CBP(td, TD_GET_CBP(td) & 0x0FFF);
          TD_SET_CBP(td, TD_GET_CBP(td) | (TD_GET_BE(td) & ~0x0FFF));
        }
      }
      if (TD_GET_T(td) & 2)
        TD_SET_T(td, TD_GET_T(td) ^ 1);
      ED_SET_C(ed, (TD_GET_T(td) & 1));
      TD_SET_CC(td, NoError);
      TD_SET_EC(td, 0);
    } else {
      if (ret >= 0)
        TD_SET_CC(td, DataUnderrun);
      else {
        switch (ret) {
          case USB_RET_NODEV:  // (-1)
            TD_SET_CC(td, DeviceNotResponding);
            break;
          case USB_RET_NAK:    // (-2)
            TD_SET_CC(td, Stall);
            break;
          case USB_RET_STALL:  // (-3)
            TD_SET_CC(td, Stall);
            break;
          case USB_RET_BABBLE:  // (-4)
            TD_SET_CC(td, BufferOverrun);
            break;
          case USB_RET_ASYNC:  // (-5)
            TD_SET_CC(td, BufferOverrun);
            break;
          default:
            BX_ERROR(("Unknown error returned: %i", ret));
            break;
        }
      }
      TD_SET_EC(td, 3);
      ED_SET_H(ed, 1);
    }

    BX_INFO((" td->cbp = 0x%08X   ret = %i  len = %i  td->cc = %i   td->ec = %i  ed->h = %i", TD_GET_CBP(td), ret, len, TD_GET_CC(td), TD_GET_EC(td), ED_GET_H(ed)));
    BX_INFO(("    td->t = %i  ed->c = %i", TD_GET_T(td), ED_GET_C(ed)));

    return 1;
  }

  return 0;
}

// pci configuration space read callback handler
Bit32u bx_usb_ohci_c::pci_read_handler(Bit8u address, unsigned io_len)
{
  Bit32u value = 0;

  if (io_len > 4 || io_len == 0) {
    BX_ERROR(("Experimental USB OHCI read register 0x%02x, len=%u !",
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
    case 0x20: pszName = "(base address)    "; break;
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
    value |= (BX_OHCI_THIS hub.pci_conf[address+i] << (i*8));
    sprintf(szTmp2, "%02x", (BX_OHCI_THIS hub.pci_conf[address+i]));
    strrev(szTmp2);
    strcat(szTmp, szTmp2);
  }
  strrev(szTmp);
  BX_DEBUG(("USB OHCI read  register 0x%02x %svalue 0x%s", address, pszName, szTmp));
  return value;
}


// pci configuration space write callback handler
void bx_usb_ohci_c::pci_write_handler(Bit8u address, Bit32u value, unsigned io_len)
{
  Bit8u value8, oldval;
  bx_bool baseaddr_change = 0;

  if (((address >= 0x14) && (address <= 0x34)))
    return;

  // This odd code is to display only what bytes actually were written.
  char szTmp[9];
  char szTmp2[3];
  szTmp[0] = '\0';
  szTmp2[0] = '\0';
  if (io_len <= 4) {
    for (unsigned i=0; i<io_len; i++) {
      value8 = (value >> (i*8)) & 0xFF;
      oldval = BX_OHCI_THIS hub.pci_conf[address+i];
      switch (address+i) {
        case 0x04:
          value8 &= 0x06; // (bit 0 is read only for this card) (we don't allow port IO)
          BX_OHCI_THIS hub.pci_conf[address+i] = value8;
          sprintf(szTmp2, "%02x", value8);
          break;
        case 0x3d: //
        case 0x3e: //
        case 0x3f: //
        case 0x05: // disallowing write to command hi-byte
        case 0x06: // disallowing write to status lo-byte (is that expected?)
          strcpy(szTmp2, "..");
          break;
        case 0x3c:
          if (value8 != oldval) {
            BX_INFO(("new irq line = %d", value8));
            BX_OHCI_THIS hub.pci_conf[address+i] = value8;
          }
          sprintf(szTmp2, "%02x", value8);
          break;
        case 0x10:  // low 12 bits of BAR are R/O
          value8 = 0x00;
        case 0x11:  // low 12 bits of BAR are R/O
          value8 &= 0xF0;
        case 0x12:
        case 0x13:
          baseaddr_change |= (value8 != oldval);
        default:
          BX_OHCI_THIS hub.pci_conf[address+i] = value8;
          sprintf(szTmp2, "%02x", value8);
      }
      strrev(szTmp2);
      strcat(szTmp, szTmp2);
    }
    if (baseaddr_change) {
      if (DEV_pci_set_base_mem(BX_OHCI_THIS_PTR, read_handler, write_handler,
                       &BX_OHCI_THIS hub.base_addr,
                       &BX_OHCI_THIS hub.pci_conf[0x10],
                       4096)) {
         BX_INFO(("new base address: 0x%04x", BX_OHCI_THIS hub.base_addr));
      }
    }
  }
  strrev(szTmp);
  BX_DEBUG(("USB OHCI write register 0x%02x                   value 0x%s", address, szTmp));
}

bx_bool bx_usb_ohci_c::usb_mouse_enabled_changed(bx_bool enabled)
{
  if (BX_OHCI_THIS mousedev != NULL) {
    if (enabled) mousedev->handle_reset();
    return 1;
  }
  return 0;
}

void bx_usb_ohci_c::usb_set_connect_status(Bit8u port, int type, bx_bool connected)
{
  char pname[BX_PATHNAME_LEN];
  char fname[BX_PATHNAME_LEN];

  if (BX_OHCI_THIS hub.usb_port[port].device != NULL) {
    if (BX_OHCI_THIS hub.usb_port[port].device->get_type() == type) {
      if (connected) {
        BX_OHCI_THIS hub.op_regs.HcRhPortStatus[port].lsda =
          (BX_OHCI_THIS hub.usb_port[port].device->get_speed() == USB_SPEED_LOW);
        BX_OHCI_THIS hub.op_regs.HcRhPortStatus[port].ccs = 1;
        //BX_OHCI_THIS hub.op_regs.HcRhPortStatus[port].csc = 1;
        //BX_OHCI_THIS hub.op_regs.HcRhPortStatus[port].pesc = 1;
        if ((type == USB_DEV_TYPE_DISK) &&
            (!BX_OHCI_THIS hub.usb_port[port].device->get_connected())) {
          if (port == 0) {
            strcpy(pname, BXPN_OHCI_PORT1);
          } else {
            strcpy(pname, BXPN_OHCI_PORT2);
          }
          strcpy(fname, SIM->get_param_string(pname)->getptr() + 5);
          if (!((usb_msd_device_c*)BX_OHCI_THIS hub.usb_port[port].device)->init(fname)) {
            usb_set_connect_status(port, USB_DEV_TYPE_DISK, 0);
          } else {
            BX_INFO(("HD on USB port #%d: '%s'", port+1, fname));
          }
        }
      } else { // not connected
        BX_OHCI_THIS hub.op_regs.HcRhPortStatus[port].ccs = 0;
        //BX_OHCI_THIS hub.op_regs.HcRhPortStatus[port].csc = 1;
        BX_OHCI_THIS hub.op_regs.HcRhPortStatus[port].pes = 0;
        //BX_OHCI_THIS hub.op_regs.HcRhPortStatus[port].pesc = 1;
        BX_OHCI_THIS hub.op_regs.HcRhPortStatus[port].lsda = 0;
        if ((type == USB_DEV_TYPE_MOUSE) ||
            (type == USB_DEV_TYPE_TABLET)) {
          if (BX_OHCI_THIS hub.usb_port[port].device == BX_OHCI_THIS mousedev) {
            BX_OHCI_THIS mousedev = NULL;
          }
        } else if (type == USB_DEV_TYPE_KEYPAD) {
          if (BX_OHCI_THIS hub.usb_port[port].device == BX_OHCI_THIS keybdev) {
            BX_OHCI_THIS keybdev = NULL;
          }
        }
        if (BX_OHCI_THIS hub.usb_port[port].device != NULL) {
          delete BX_OHCI_THIS hub.usb_port[port].device;
          BX_OHCI_THIS hub.usb_port[port].device = NULL;
          sprintf(pname, "usb_ohci.hub.port%d.device", port+1);
          bx_list_c *devlist = (bx_list_c*)SIM->get_param(pname, SIM->get_bochs_root());
          devlist->clear();
        }
      }
    }
    // we changed the value of the port, so show it
    BX_OHCI_THIS hub.op_regs.HcInterruptStatus.rhsc = 1;
    set_irq_state();
  }
}

bx_bool bx_usb_ohci_c::usb_mouse_enq(int delta_x, int delta_y, int delta_z, unsigned button_state)
{
  if (BX_OHCI_THIS mousedev != NULL) {
    mousedev->mouse_enq(delta_x, delta_y, delta_z, button_state);
    return 1;
  }
  return 0;
}

bx_bool bx_usb_ohci_c::usb_key_enq(Bit8u *scan_code)
{
  if (BX_OHCI_THIS keybdev != NULL) {
    return keybdev->key_enq(scan_code);
  }
  return 0;
}

// Send an internal message to a USB device
void bx_usb_ohci_c::usb_send_msg(usb_device_c *dev, int msg)
{
  USBPacket p;
  memset(&p, 0, sizeof(p));
  p.pid = msg;
  dev->handle_packet(&p);
}

// USB runtime parameter handler
const char *bx_usb_ohci_c::usb_param_handler(bx_param_string_c *param, int set,
                                           const char *oldval, const char *val, int maxlen)
{
  usbdev_type type = USB_DEV_TYPE_NONE;

  // handler for USB runtime parameters
  if (set) {
    char pname[BX_PATHNAME_LEN];
    param->get_param_path(pname, BX_PATHNAME_LEN);
    if (!strcmp(pname, BXPN_OHCI_PORT1)) {
      BX_INFO(("USB port #1 experimental device change"));
      if (!strcmp(val, "none") && BX_OHCI_THIS hub.op_regs.HcRhPortStatus[0].ccs) {
        if (BX_OHCI_THIS hub.usb_port[0].device != NULL) {
          type = BX_OHCI_THIS hub.usb_port[0].device->get_type();
        }
        usb_set_connect_status(0, type, 0);
      } else if (strcmp(val, "none") && !BX_OHCI_THIS hub.op_regs.HcRhPortStatus[0].ccs) {
        init_device(0, val);
      }
    } else if (!strcmp(pname, BXPN_OHCI_PORT2)) {
      BX_INFO(("USB port #2 experimental device change"));
      if (!strcmp(val, "none") && BX_OHCI_THIS hub.op_regs.HcRhPortStatus[1].ccs) {
        if (BX_OHCI_THIS hub.usb_port[1].device != NULL) {
          type = BX_OHCI_THIS hub.usb_port[1].device->get_type();
        }
        usb_set_connect_status(1, type, 0);
      } else if (strcmp(val, "none") && !BX_OHCI_THIS hub.op_regs.HcRhPortStatus[1].ccs) {
        init_device(1, val);
      }
    } else {
      BX_PANIC(("usb_param_handler called with unexpected parameter '%s'", pname));
    }
  }
  return val;
}

#endif // BX_SUPPORT_PCI && BX_SUPPORT_USB_OHCI
