/////////////////////////////////////////////////////////////////////////
// $Id: iodev.h,v 1.39 2004/01/15 02:08:35 danielg4 Exp $
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
//  I/O port handlers API Copyright (C) 2003 by Frank Cornelis
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



/* maximum number of emulated devices allowed.  floppy, vga, etc...
   you can increase this to anything below 256 since an 8-bit handle
   is used for each device */
#define BX_MAX_IO_DEVICES 30

/* the last device in the array is the "default" I/O device */
#define BX_DEFAULT_IO_DEVICE   (BX_MAX_IO_DEVICES-1)

/* number of IRQ lines supported.  In an ISA PC there are two
   PIC chips cascaded together.  each has 8 IRQ lines, so there
   should be 16 IRQ's total */
#define BX_MAX_IRQS 16
#define BX_NO_IRQ  -1


class bx_pit_c;
class bx_keyb_c;
class bx_ioapic_c;
class bx_g2h_c;
#if BX_IODEBUG_SUPPORT
class bx_iodebug_c;
#endif



typedef Bit32u (*bx_read_handler_t)(void *, Bit32u, unsigned);
typedef void   (*bx_write_handler_t)(void *, Bit32u, Bit32u, unsigned);


#if BX_USE_DEV_SMF
#  define BX_DEV_SMF  static
#  define BX_DEV_THIS bx_devices.
#else
#  define BX_DEV_SMF
#  define BX_DEV_THIS this->
#endif

//////////////////////////////////////////////////////////////////////
// bx_devmodel_c declaration
//////////////////////////////////////////////////////////////////////

// This class defines virtual methods that are common to all devices. 
// Child classes do not need to implement all of them, because in this 
// definition they are defined as empty, as opposed to being pure 
// virtual (= 0).
class BOCHSAPI bx_devmodel_c : public logfunctions {
  public:
  virtual ~bx_devmodel_c () {}
  virtual void init_mem(BX_MEM_C *) {}
  virtual void init(void) {}
  virtual void reset(unsigned type) {}
  virtual void device_load_state () {}
  virtual void device_save_state () {}
};

//////////////////////////////////////////////////////////////////////
// declare stubs for devices
//////////////////////////////////////////////////////////////////////

#define STUBFUNC(dev,method) \
   pluginlog->panic("%s called in %s stub. you must not have loaded the %s plugin", #dev, #method, #dev )

class BOCHSAPI bx_keyb_stub_c : public bx_devmodel_c {
  public:
  virtual ~bx_keyb_stub_c () {}
  // stubs for bx_keyb_c methods
  virtual void mouse_motion(int delta_x, int delta_y, unsigned button_state) {
    STUBFUNC(keyboard, mouse_motion);
  }
  virtual void gen_scancode(Bit32u key) {
    STUBFUNC(keyboard, gen_scancode);
  }
  virtual void paste_bytes(Bit8u *data, Bit32s length) {
    STUBFUNC(keyboard, paste_bytes);
  }
  virtual void paste_delay_changed () {
    STUBFUNC(keyboard, paste_delay_changed);
  }
  virtual void mouse_enabled_changed(bool enabled) {
    STUBFUNC(keyboard, mouse_enabled_changed);
  }
};

class BOCHSAPI bx_hard_drive_stub_c : public bx_devmodel_c {
  public:
  virtual void   close_harddrive(void) {
    STUBFUNC(HD, close_harddrive);
  }
  virtual void   init() {
    STUBFUNC(HD, init);
  }
  virtual void   reset(unsigned type) {
    STUBFUNC(HD, reset);
  }
  virtual Bit32u   get_device_handle(Bit8u channel, Bit8u device) {
    STUBFUNC(HD, get_device_handle); return 0;
  }
  virtual Bit32u   get_first_cd_handle(void) {
    STUBFUNC(HD, get_first_cd_handle); return 0;
  }
  virtual unsigned get_cd_media_status(Bit32u handle) {
    STUBFUNC(HD, get_cd_media_status); return 0;
  }
  virtual unsigned set_cd_media_status(Bit32u handle, unsigned status) {
    STUBFUNC(HD, set_cd_media_status); return 0;
  }
  virtual Bit32u virt_read_handler(Bit32u address, unsigned io_len) 
  {
    STUBFUNC(HD, virt_read_handler); return 0;
  }
  virtual void   virt_write_handler(Bit32u address,
      Bit32u value, unsigned io_len) 
  {
    STUBFUNC(HD, virt_write_handler);
  }
};

class BOCHSAPI bx_floppy_stub_c : public bx_devmodel_c {
  public:
  virtual unsigned get_media_status(unsigned drive) {
    STUBFUNC(floppy,  get_media_status); return 0;
  }
  virtual unsigned set_media_status(unsigned drive, unsigned status) {
    STUBFUNC(floppy, set_media_status); return 0;
  }
};

class BOCHSAPI bx_cmos_stub_c : public bx_devmodel_c {
  public:
  virtual Bit32u get_reg(unsigned reg) {
    STUBFUNC(cmos, get_reg); return 0;
  }
  virtual void set_reg(unsigned reg, Bit32u val) {
    STUBFUNC(cmos, set_reg);
  }
  virtual time_t get_timeval() {
    // STUBFUNC(cmos, get_timeval); 
    return 0;
  }
  virtual void checksum_cmos(void) {
    STUBFUNC(cmos, checksum);
  }
};

class BOCHSAPI bx_dma_stub_c : public bx_devmodel_c {
  public:
  virtual unsigned registerDMA8Channel(
    unsigned channel,
    void (* dmaRead)(Bit8u *data_byte),
    void (* dmaWrite)(Bit8u *data_byte),
    const char *name
    ) {
    STUBFUNC(dma, registerDMA8Channel); return 0;
  }
  virtual unsigned registerDMA16Channel(
    unsigned channel,
    void (* dmaRead)(Bit16u *data_word),
    void (* dmaWrite)(Bit16u *data_word),   
    const char *name
    ) {
    STUBFUNC(dma, registerDMA16Channel); return 0;
  }
  virtual unsigned unregisterDMAChannel(unsigned channel) {
    STUBFUNC(dma, unregisterDMAChannel); return 0;
  }
  virtual unsigned get_TC(void) {
    STUBFUNC(dma, get_TC); return 0;
  }
  virtual void set_DRQ(unsigned channel, bx_bool val) {
    STUBFUNC(dma, set_DRQ);
  }
  virtual void raise_HLDA(void) {
    STUBFUNC(dma, raise_HLDA);
  }
};

class BOCHSAPI bx_pic_stub_c : public bx_devmodel_c {
  public:
  virtual void raise_irq(unsigned irq_no) {
    STUBFUNC(pic, raise_irq); 
  }
  virtual void lower_irq(unsigned irq_no) {
    STUBFUNC(pic, lower_irq); 
  }
  virtual Bit8u IAC(void) {
    STUBFUNC(pic, IAC); return 0;
  }
  virtual void show_pic_state(void) {
    STUBFUNC(pic, show_pic_state);
  }
};

class BOCHSAPI bx_vga_stub_c : public bx_devmodel_c {
  public:
  virtual void redraw_area(unsigned x0, unsigned y0, 
                           unsigned width, unsigned height) {
    STUBFUNC(vga, redraw_area);  
  }
  virtual Bit8u mem_read(Bit32u addr) {
    STUBFUNC(vga, mem_read);  return 0;
  }
  virtual void mem_write(Bit32u addr, Bit8u value) {
    STUBFUNC(vga, mem_write);
  }
  virtual void get_text_snapshot(Bit8u **text_snapshot, 
                                 unsigned *txHeight, unsigned *txWidth) {
    STUBFUNC(vga, get_text_snapshot); 
  }
  virtual void trigger_timer(void *this_ptr) {
    STUBFUNC(vga, trigger_timer); 
  }
  virtual void set_update_interval (unsigned interval) {
    STUBFUNC(vga, set_update_interval); 
  }
  virtual Bit8u get_actl_palette_idx(Bit8u index) {
    return 0;
  }
};

class BOCHSAPI bx_pci_stub_c : public bx_devmodel_c {
  public:
  virtual bx_bool register_pci_handlers(void *this_ptr,
                                        Bit32u (*bx_pci_read_handler)(void *, Bit8u, unsigned),
                                        void(*bx_pci_write_handler)(void *, Bit8u, Bit32u, unsigned),
                                        Bit8u devfunc, const char *name) {
    STUBFUNC(pci, register_pci_handlers); return 0;
  }
  virtual Bit8u find_free_devfunc() {
	STUBFUNC(pci, find_free_devfunc); return 0;
  }
  virtual Bit8u rd_memType (Bit32u addr) {
    return 0;
  }
  virtual Bit8u wr_memType (Bit32u addr) {
    return 0;
  }
  virtual void print_i440fx_state(void) {}
};

class BOCHSAPI bx_ne2k_stub_c : public bx_devmodel_c {
  public:
  virtual void print_info(FILE *file, int page, int reg, int nodups) {}
};

class BOCHSAPI bx_devices_c : public logfunctions {
public:
  bx_devices_c(void);
  ~bx_devices_c(void);
  // Register I/O addresses and IRQ lines. Initialize any internal
  // structures.  init() is called only once, even if the simulator
  // reboots or is restarted.
  void init(BX_MEM_C *);
  // Enter reset state in response to a reset condition.
  // The types of reset conditions are defined in bochs.h:
  // power-on, hardware, or software.
  void reset(unsigned type);
  BX_MEM_C *mem;  // address space associated with these devices
  bx_bool register_io_read_handler(void *this_ptr, bx_read_handler_t f, 
		  Bit32u addr, const char *name, Bit8u mask );
  bx_bool unregister_io_read_handler( void *this_ptr, bx_read_handler_t f,
                                        Bit32u addr, Bit8u mask );
  bx_bool register_io_write_handler(void *this_ptr, bx_write_handler_t f, Bit32u addr, const char *name, Bit8u mask );
  bx_bool unregister_io_write_handler( void *this_ptr, bx_write_handler_t f,
                                        Bit32u addr, Bit8u mask );
  bx_bool register_io_read_handler_range( void *this_ptr, bx_read_handler_t f,
		  Bit32u begin_addr, Bit32u end_addr, 
		  const char *name, Bit8u mask );
  bx_bool register_io_write_handler_range( void *this_ptr, bx_write_handler_t f,
		  Bit32u begin_addr, Bit32u end_addr, 
		  const char *name, Bit8u mask );
  bx_bool unregister_io_read_handler_range( void *this_ptr, bx_read_handler_t f,
                                        Bit32u begin, Bit32u end, Bit8u mask );
  bx_bool unregister_io_write_handler_range( void *this_ptr, bx_write_handler_t f,
                                        Bit32u begin, Bit32u end, Bit8u mask );
  bx_bool register_default_io_read_handler(void *this_ptr, bx_read_handler_t f, const char *name, Bit8u mask );
  bx_bool register_default_io_write_handler(void *this_ptr, bx_write_handler_t f, const char *name, Bit8u mask );
  bx_bool register_irq(unsigned irq, const char *name);
  bx_bool unregister_irq(unsigned irq, const char *name);
  void iodev_init(void);
  Bit32u inp(Bit16u addr, unsigned io_len) BX_CPP_AttrRegparmN(2);
  void   outp(Bit16u addr, Bit32u value, unsigned io_len) BX_CPP_AttrRegparmN(3);

  static void timer_handler(void *);
  void timer(void);

  bx_devmodel_c    *pluginBiosDevice;
  bx_ioapic_c      *ioapic;
  bx_pci_stub_c    *pluginPciBridge;
  bx_devmodel_c    *pluginPci2IsaBridge;
  bx_devmodel_c    *pluginPciVgaAdapter;
  bx_devmodel_c    *pluginPciDevAdapter;
  bx_devmodel_c    *pluginPciUSBAdapter;
  bx_devmodel_c	   *pluginPciPNicAdapter;
  bx_pit_c         *pit;
  bx_keyb_stub_c   *pluginKeyboard;
  bx_dma_stub_c    *pluginDmaDevice;
  bx_floppy_stub_c *pluginFloppyDevice;
  bx_cmos_stub_c   *pluginCmosDevice;
  bx_devmodel_c    *pluginSerialDevice;
  bx_devmodel_c    *pluginParallelDevice;
  bx_devmodel_c    *pluginUnmapped;
  bx_vga_stub_c    *pluginVgaDevice;
  bx_pic_stub_c    *pluginPicDevice;
  bx_hard_drive_stub_c *pluginHardDrive;
  bx_devmodel_c    *pluginSB16Device;
  bx_ne2k_stub_c   *pluginNE2kDevice;
  bx_g2h_c         *g2h;
  bx_devmodel_c    *pluginExtFpuIrq;
  bx_devmodel_c    *pluginGameport;
#if BX_IODEBUG_SUPPORT
  bx_iodebug_c	   *iodebug;
#endif

  // stub classes that the pointers (above) can point to until a plugin is
  // loaded
  bx_cmos_stub_c stubCmos;
  bx_keyb_stub_c stubKeyboard;
  bx_hard_drive_stub_c stubHardDrive;
  bx_dma_stub_c  stubDma;
  bx_pic_stub_c  stubPic;
  bx_floppy_stub_c  stubFloppy;
  bx_vga_stub_c  stubVga;
  bx_pci_stub_c  stubPci;
  bx_ne2k_stub_c stubNE2k;

  // Some info to pass to devices which can handled bulk IO.  This allows
  // the interface to remain the same for IO devices which can't handle
  // bulk IO.  We should probably implement special INPBulk() and OUTBulk()
  // functions which stick these values in the bx_devices_c class, and
  // then call the normal functions rather than having gross globals
  // variables.
  Bit32u   bulkIOHostAddr;
  unsigned bulkIOQuantumsRequested;
  unsigned bulkIOQuantumsTransferred;

private:

  struct io_handler_struct {
	struct io_handler_struct *next;
	struct io_handler_struct *prev;	
	void *funct; // C++ type checking is great, but annoying
	void *this_ptr;
	const char *handler_name;  // name of device
	int usage_count;
	Bit8u mask;          // io_len mask
  };
  struct io_handler_struct io_read_handlers;
  struct io_handler_struct io_write_handlers;
#define PORTS 0x10000
  struct io_handler_struct **read_port_to_handler;
  struct io_handler_struct **write_port_to_handler;
  
  // more for informative purposes, the names of the devices which
  // are use each of the IRQ 0..15 lines are stored here
  const char *irq_handler_name[BX_MAX_IRQS];

  static Bit32u read_handler(void *this_ptr, Bit32u address, unsigned io_len);
  static void   write_handler(void *this_ptr, Bit32u address, Bit32u value, unsigned io_len);
  BX_DEV_SMF Bit32u port92_read(Bit32u address, unsigned io_len);
  BX_DEV_SMF void   port92_write(Bit32u address, Bit32u value, unsigned io_len);

  static Bit32u default_read_handler(void *this_ptr, Bit32u address, unsigned io_len);
  static void   default_write_handler(void *this_ptr, Bit32u address, Bit32u value, unsigned io_len);

  int timer_handle;
  bx_bool is_serial_enabled ();
  bx_bool is_usb_enabled ();
  bx_bool is_parallel_enabled ();
  };



#if BX_PCI_SUPPORT
#include "iodev/pci.h"
#include "iodev/pci2isa.h"
#if BX_PCI_VGA_SUPPORT
#include "iodev/pcivga.h"
#endif
#if BX_PCI_DEV_SUPPORT
#include "iodev/pcidev.h"
#endif
#if BX_PCI_USB_SUPPORT
#include "iodev/pciusb.h"
#endif
#endif
#include "iodev/vga.h"
#if BX_SUPPORT_APIC
#  include "iodev/ioapic.h"
#endif
#include "iodev/biosdev.h"
#include "iodev/cmos.h"
#include "iodev/dma.h"
#include "iodev/floppy.h"
#include "iodev/harddrv.h"
#include "iodev/vmware3.h"
#if BX_IODEBUG_SUPPORT
#   include "iodev/iodebug.h"
#endif
#include "iodev/keyboard.h"
#include "iodev/parallel.h"
#include "iodev/pic.h"
#include "iodev/pit.h"
#include "iodev/pit_wrap.h"
#include "iodev/virt_timer.h"
#include "iodev/serial.h"
#if BX_SUPPORT_SB16
#  include "iodev/sb16.h"
#endif
#include "iodev/unmapped.h"
#include "iodev/eth.h"
#include "iodev/ne2k.h"
#if BX_PCI_PNIC_SUPPORT
#include "iodev/pcipnic.h"
#endif
#include "iodev/guest2host.h"
#include "iodev/slowdown_timer.h"
#include "iodev/extfpuirq.h"
#include "iodev/gameport.h"
