/*
 *  plex86: run multiple x86 operating systems concurrently
 *  Copyright (C) 1999-2000  The plex86 developers team
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */

#ifndef __PLUGIN_H
#define __PLUGIN_H

#ifdef __cplusplus
extern "C" {
#endif

#define BX_PLUGIN_UNMAPPED "UNMAPPED"
#define BX_PLUGIN_BIOSDEV  "BIOSDEV"
#define BX_PLUGIN_CMOS     "CMOS RAM"
#define BX_PLUGIN_VGA      "VGA"
#define BX_PLUGIN_FLOPPY   "FLOPPY"
#define BX_PLUGIN_PARALLEL "PARALLEL"
#define BX_PLUGIN_SERIAL   "SERIAL"

#define BX_REGISTER_DEVICE pluginRegisterDevice

#if BX_PLUGINS

#define BX_INIT_DEVICES() {bx_devices.init(BX_MEM(0)); bx_init_plugins();}
#define BX_RESET_DEVICES(type) {bx_devices.reset(type); bx_reset_plugins(type);}

#define BX_REGISTER_IOREAD_HANDLER(a,b,c,d,e,f)  pluginRegisterIOReadHandler(b,c,d,e,f)
#define BX_REGISTER_IOWRITE_HANDLER(a,b,c,d,e,f) pluginRegisterIOWriteHandler(b,c,d,e,f)
#define BX_REGISTER_DEFAULT_IOREAD_HANDLER(a,b,c,d,e) pluginRegisterDefaultIOReadHandler(b,c,d,e)
#define BX_REGISTER_DEFAULT_IOWRITE_HANDLER(a,b,c,d,e) pluginRegisterDefaultIOWriteHandler(b,c,d,e)

#define BX_REGISTER_IRQ(a,b,c) pluginRegisterIRQ(b,c)
#define BX_UNREGISTER_IRQ(a,b) pluginUnregisterIRQ(b)

#define BX_GET_CMOS_REG(a,b) pluginGetCMOSReg(b)
#define BX_SET_CMOS_REG(a,b,c) pluginSetCMOSReg(b,c)
#define BX_CMOS_CHECKSUM(a) pluginCMOSChecksum()
#define BX_GET_CMOS_TIMEVAL(a) pluginGetCMOSTimeval()

#define BX_VGA_MEM_READ(addr) pluginVGAMemRead(addr)
#define BX_VGA_MEM_WRITE(addr, val) pluginVGAMemWrite(addr, val)
#define BX_VGA_REDRAW_AREA(left, top, right, bottom) pluginVGARedrawArea(left, top, right, bottom)
#define BX_VGA_GET_TEXT_SNAPSHOT(rawsnap, height, width) pluginVGAGetTextSnapshot(rawsnap, height, width)
#define BX_VGA_REFRESH() pluginVGARefresh ()
#define BX_VGA_SET_UPDATE_INTERVAL(val) pluginVGASetUpdateInterval(val)

#define BX_FLOPPY_GET_MEDIA_STATUS(drive) pluginFloppyGetMediaStatus(drive)
#define BX_FLOPPY_SET_MEDIA_STATUS(drive, status) pluginFloppySetMediaStatus(drive, status)

#define BX_FLOPPY_PRESENT() (pluginDevicePresent(BX_PLUGIN_FLOPPY))

#define BX_HD_READ_HANDLER(a, b, c) \
    (bx_devices.hard_drive->read_handler(bx_devices.hard_drive, b, c))
#define BX_HD_WRITE_HANDLER(a, b, c, d) \
    (bx_devices.hard_drive->write_handler(bx_devices.hard_drive, b, c, d))

#else

#define BX_INIT_DEVICES() {bx_devices.init(BX_MEM(0)); }
#define BX_RESET_DEVICES(type) {bx_devices.reset(type); }

#define BX_REGISTER_IOREAD_HANDLER(a,b,c,d,e,f) a devices->register_io_read_handler(b,c,d,e)
#define BX_REGISTER_IOWRITE_HANDLER(a,b,c,d,e,f) a devices->register_io_write_handler(b,c,d,e)
#define BX_REGISTER_DEFAULT_IOREAD_HANDLER(a,b,c,d,e) a devices->register_default_io_read_handler(b,c,d)
#define BX_REGISTER_DEFAULT_IOWRITE_HANDLER(a,b,c,d,e) a devices->register_default_io_write_handler(b,c,d)
#define BX_REGISTER_IRQ(a,b,c) a devices->register_irq(b,c)
#define BX_UNREGISTER_IRQ(a,b) a devices->unregister_irq(b)

#define BX_GET_CMOS_REG(a,b) (a devices->cmos->s.reg[b])
#define BX_SET_CMOS_REG(a,b,c) a devices->cmos->s.reg[b]=c
#define BX_CMOS_CHECKSUM(a) a devices->cmos->checksum_cmos()
#define BX_GET_CMOS_TIMEVAL(a) a devices->cmos->s.timeval

#define BX_VGA_MEM_READ(addr) (bx_devices.vga->mem_read(addr))
#define BX_VGA_MEM_WRITE(addr, val) bx_devices.vga->mem_write(addr, val)
#define BX_VGA_REDRAW_AREA(left, top, right, bottom) \
    (bx_vga.redraw_area(left, top, right, bottom))
#define BX_VGA_GET_TEXT_SNAPSHOT(rawsnap, height, width) \
    (bx_vga.get_text_snapshot(rawsnap, height, width))
#define BX_VGA_REFRESH() bx_vga.timer_handler (&bx_vga)
#define BX_VGA_SET_UPDATE_INTERVAL(val) bx_vga.set_update_interval(val)

#define BX_FLOPPY_GET_MEDIA_STATUS(drive) \
    (bx_devices.floppy->get_media_status(drive))
#define BX_FLOPPY_SET_MEDIA_STATUS(drive, status) \
    (bx_devices.floppy->set_media_status(drive, status))

#define BX_FLOPPY_PRESENT() (bx_devices.floppy)

#define BX_HD_READ_HANDLER(a, b, c) \
    (a devices->hard_drive->read_handler(a devices->hard_drive, b, c))
#define BX_HD_WRITE_HANDLER(a, b, c, d) \
    (a devices->hard_drive->write_handler(a devices->hard_drive, b, c, d))

#endif // #if BX_PLUGINS


// FIXME Do we really need pluginRegisterTimer ?
#define BX_REGISTER_TIMER(a,b,c,d,e,f) bx_pc_system.register_timer(a,b,c,d,e,f)

#define BX_PIC_LOWER_IRQ(a,b)  pluginLowerIRQ(b)
#define BX_PIC_RAISE_IRQ(a,b)  pluginRaiseIRQ(b)


#if BX_PLUGINS
	/*
#define BX_EVENT_MOUSE_MOTION(dx, dy, state) \
    ((*pluginMouseMotion)(dx, dy, state))
#define BX_EVENT_GEN_SCANCODE(scancode) \
    ((*pluginGenScancode)(scancode))
	*/

#else

#endif

#define BX_EVENT_MOUSE_MOTION(dx, dy, state) \
    (bx_devices.keyboard->mouse_motion(dx, dy, state))
#define BX_EVENT_GEN_SCANCODE(scancode) \
    (bx_devices.keyboard->gen_scancode(scancode))
#define BX_EVENT_PUT_SCANCODE(scancode, count) \
    (bx_devices.keyboard->put_scancode(scancode, count))
#define BX_KBD_PASTE_BYTES(bytes, count) \
    (bx_devices.keyboard->paste_bytes(bytes, count))

#define BX_HD_GET_FIRST_CD_HANDLE() \
    (bx_devices.hard_drive->get_first_cd_handle())
#define BX_CD_GET_MEDIA_STATUS(handle) \
    (bx_devices.hard_drive->get_cd_media_status(handle))
#define BX_CD_SET_MEDIA_STATUS(handle, status) \
    (bx_devices.hard_drive->set_cd_media_status(handle, status))


#include <dlfcn.h>

typedef Bit32u (*ioReadHandler_t)(void *, Bit32u, unsigned);
typedef void   (*ioWriteHandler_t)(void *, Bit32u, Bit32u, unsigned);

#define MAX_ARGC 10

typedef struct _plugin_t
{
    int  initialized;
    void *handle;
    int  argc;
    char *name, *args, *argv[MAX_ARGC];
    int  (*plugin_init)(struct _plugin_t *plugin, int argc, char *argv[]);
    void (*plugin_fini)(void);

    struct _plugin_t *next;
} plugin_t;

extern plugin_t *plugins;

typedef struct _device_t
{
    const char *name;
    void (*device_init_mem)(BX_MEM_C *);
    void (*device_init_dev)(bx_devices_c *);
    void (*device_reset)(unsigned);
    void (*device_load_state)();
    void (*device_save_state)();

    struct _device_t *next;
} device_t;

extern device_t *devices;

void plugin_startup (void);
void plugin_load (char *name, char *args);
plugin_t *plugin_unload (plugin_t *plugin);
void plugin_init_all (void);
void plugin_fini_all (void);

/* === Device Stuff === */
typedef void (*deviceInitMem_t)(BX_MEM_C *);
typedef void (*deviceInitDev_t)(bx_devices_c *);
typedef void (*deviceReset_t)(unsigned);
typedef void (*deviceLoad_t)(void);
typedef void (*deviceSave_t)(void);

void pluginRegisterDevice(deviceInitMem_t init_mem, deviceInitDev_t init_dev,
                          deviceReset_t reset, deviceLoad_t load, 
                          deviceSave_t save, char *name);
Boolean pluginDevicePresent(char *name);

/* === IO port stuff === */
extern int (*pluginRegisterIOReadHandler)(void *thisPtr, ioReadHandler_t callback,
                                unsigned base, const char *name, unsigned len);
extern int (*pluginRegisterIOWriteHandler)(void *thisPtr, ioWriteHandler_t callback,
                                 unsigned base, const char *name, unsigned len);
extern int (*pluginRegisterDefaultIOReadHandler)(void *thisPtr, ioReadHandler_t callback,
                                const char *name, unsigned len);
extern int (*pluginRegisterDefaultIOWriteHandler)(void *thisPtr, ioWriteHandler_t callback,
                                 const char *name, unsigned len);

/* === IRQ stuff === */
extern void (*pluginRegisterIRQ)(unsigned irq, const char *name);
extern void (*pluginUnregisterIRQ)(unsigned irq, const char *name);
extern void (*pluginRaiseIRQ)(unsigned irq);
extern void (*pluginLowerIRQ)(unsigned irq);


/* === CMOS query/set stuff === */

/* These are function pointers.  At startup time, they are set to
 * builtin default handlers which will cause an abort if called.
 * A CMOS plugin should set these pointers to real handler functions.
 */
extern Bit32u (* pluginGetCMOSReg)(unsigned reg);
extern void   (* pluginSetCMOSReg)(unsigned reg, Bit32u val);
extern void   (* pluginCMOSChecksum)(void);
extern time_t (* pluginGetCMOSTimeval)(void);


/* === A20 enable line stuff === */
extern unsigned (*pluginGetA20E)(void);
extern void     (*pluginSetA20E)(unsigned val);


/* === Keyboard/Mouse input stuff === */
extern void (* pluginMouseMotion)(int d_x, int d_y, unsigned button_state);
extern void (* pluginGenScancode)(Bit32u scancode);


/* === DMA stuff === */
extern unsigned (* pluginRegisterDMAChannel)(
  unsigned channel,
  void (* dma_read)(Bit8u *data_byte),
  void (* dma_write)(Bit8u *data_byte)
  );
extern void     (* pluginDMASetDRQ)(unsigned channel, unsigned val);
extern unsigned (* pluginDMAGetTC)(void);

/* === Hard drive / floppy port sharing hack === */
extern Bit32u (* pluginHDReadHandler)(Bit32u address,
    unsigned io_len);
extern void   (* pluginHDWriteHandler)(Bit32u address,
    Bit32u value, unsigned io_len);

/* === Floppy stuff ===*/
extern unsigned (* pluginFloppyGetMediaStatus)(unsigned drive);
extern unsigned (* pluginFloppySetMediaStatus)(unsigned drive, unsigned status);

/* === VGA stuff === */
extern void (* pluginVGARedrawArea)(unsigned x0, unsigned y0,
                 unsigned width, unsigned height);
extern Bit8u (* pluginVGAMemRead)(Bit32u addr);
extern void  (* pluginVGAMemWrite)(Bit32u addr, Bit8u value);
extern void  (* pluginVGAGetTextSnapshot)(Bit8u **text_snapshot, 
		          unsigned *txHeight, unsigned *txWidth);
extern void  (* pluginVGARefresh)(void);
extern void  (* pluginVGASetUpdateInterval)(unsigned);

/* === Timer stuff === */
extern int      (*pluginRegisterTimer)(void *this_ptr, void (*funct)(void *),
                             Bit32u useconds, Boolean continuous,
                             Boolean active, const char *name);

extern void     (*pluginActivateTimer)(unsigned id, Bit32u usec, Boolean continuous);
extern void     (*pluginDeactivateTimer)(unsigned id);

/* === HRQ stuff === */
extern void     (*pluginSetHRQ)(unsigned val);
extern void     (*pluginSetHRQHackCallback)( void (*callback)(void) );

/* === Reset stuff === */
extern void     (*pluginResetSignal)(unsigned sig);

void plugin_abort (void);

// called from bochs main (hack)
extern int bx_load_plugins ();
extern void bx_init_plugins (void);
extern void bx_reset_plugins (unsigned);

// every plugin must define these, within the extern"C" block, so that
// a non-mangled function symbol is available in the shared library.
void plugin_fini(void);
int plugin_init(plugin_t *plugin, int argc, char *argv[]);


#ifdef __cplusplus
};
#endif

#endif /* __PLUGIN_H */
