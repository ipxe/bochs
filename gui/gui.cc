/////////////////////////////////////////////////////////////////////////
// $Id: gui.cc,v 1.26 2002/02/04 20:31:35 vruppert Exp $
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


#include <signal.h>
#include "bochs.h"
#include "gui/bitmaps/floppya.h"
#include "gui/bitmaps/floppyb.h"
#include "gui/bitmaps/mouse.h"
#include "gui/bitmaps/reset.h"
#include "gui/bitmaps/power.h"
#include "gui/bitmaps/snapshot.h"
#include "gui/bitmaps/configbutton.h"
#include "gui/bitmaps/cdromd.h"
#if BX_WITH_MACOS
#  include <Disks.h>
#endif




bx_gui_c   bx_gui;

#define BX_GUI_THIS bx_gui.
#define LOG_THIS BX_GUI_THIS

bx_gui_c::bx_gui_c(void)
{
  put("GUI"); // Init in specific_init
  settype(GUILOG);
}

  void
bx_gui_c::init(int argc, char **argv, unsigned tilewidth, unsigned tileheight)
{
  specific_init(&bx_gui, argc, argv, tilewidth, tileheight, BX_HEADER_BAR_Y);

  // Define some bitmaps to use in the headerbar
  BX_GUI_THIS floppyA_bmap_id = create_bitmap(bx_floppya_bmap,
                          BX_FLOPPYA_BMAP_X, BX_FLOPPYA_BMAP_Y);
  BX_GUI_THIS floppyA_eject_bmap_id = create_bitmap(bx_floppya_eject_bmap,
                          BX_FLOPPYA_BMAP_X, BX_FLOPPYA_BMAP_Y);
  BX_GUI_THIS floppyB_bmap_id = create_bitmap(bx_floppyb_bmap,
                          BX_FLOPPYB_BMAP_X, BX_FLOPPYB_BMAP_Y);
  BX_GUI_THIS floppyB_eject_bmap_id = create_bitmap(bx_floppyb_eject_bmap,
                          BX_FLOPPYB_BMAP_X, BX_FLOPPYB_BMAP_Y);
  BX_GUI_THIS cdromD_bmap_id = create_bitmap(bx_cdromd_bmap,
                          BX_CDROMD_BMAP_X, BX_CDROMD_BMAP_Y);
  BX_GUI_THIS cdromD_eject_bmap_id = create_bitmap(bx_cdromd_eject_bmap,
                          BX_CDROMD_BMAP_X, BX_CDROMD_BMAP_Y);
  BX_GUI_THIS mouse_bmap_id = create_bitmap(bx_mouse_bmap,
                          BX_MOUSE_BMAP_X, BX_MOUSE_BMAP_Y);
  BX_GUI_THIS nomouse_bmap_id = create_bitmap(bx_nomouse_bmap,
                          BX_MOUSE_BMAP_X, BX_MOUSE_BMAP_Y);


  BX_GUI_THIS power_bmap_id = create_bitmap(bx_power_bmap, BX_POWER_BMAP_X, BX_POWER_BMAP_Y);
  BX_GUI_THIS reset_bmap_id = create_bitmap(bx_reset_bmap, BX_RESET_BMAP_X, BX_RESET_BMAP_Y);
  BX_GUI_THIS snapshot_bmap_id = create_bitmap(bx_snapshot_bmap, BX_SNAPSHOT_BMAP_X, BX_SNAPSHOT_BMAP_Y);
  BX_GUI_THIS config_bmap_id = create_bitmap(bx_config_bmap, BX_SNAPSHOT_BMAP_X, BX_SNAPSHOT_BMAP_Y);


  // Add the initial bitmaps to the headerbar, and enable callback routine, for use
  // when that bitmap is clicked on

  // Floppy A:
  BX_GUI_THIS floppyA_status = bx_devices.floppy->get_media_status(0);
  if (BX_GUI_THIS floppyA_status)
    BX_GUI_THIS floppyA_hbar_id = headerbar_bitmap(BX_GUI_THIS floppyA_bmap_id,
                          BX_GRAVITY_LEFT, floppyA_handler);
  else
    BX_GUI_THIS floppyA_hbar_id = headerbar_bitmap(BX_GUI_THIS floppyA_eject_bmap_id,
                          BX_GRAVITY_LEFT, floppyA_handler);

  // Floppy B:
  BX_GUI_THIS floppyB_status = bx_devices.floppy->get_media_status(1);
  if (BX_GUI_THIS floppyB_status)
    BX_GUI_THIS floppyB_hbar_id = headerbar_bitmap(BX_GUI_THIS floppyB_bmap_id,
                          BX_GRAVITY_LEFT, floppyB_handler);
  else
    BX_GUI_THIS floppyB_hbar_id = headerbar_bitmap(BX_GUI_THIS floppyB_eject_bmap_id,
                          BX_GRAVITY_LEFT, floppyB_handler);

  // CDROM
  BX_GUI_THIS cdromD_status =
    bx_devices.hard_drive->get_cd_media_status()
    && bx_options.cdromd.Opresent->get ();
  if (BX_GUI_THIS cdromD_status)
    BX_GUI_THIS cdromD_hbar_id = headerbar_bitmap(BX_GUI_THIS cdromD_bmap_id,
                          BX_GRAVITY_LEFT, cdromD_handler);
  else
    BX_GUI_THIS cdromD_hbar_id = headerbar_bitmap(BX_GUI_THIS cdromD_eject_bmap_id,
                          BX_GRAVITY_LEFT, cdromD_handler);

  // Mouse button
  if (bx_options.Omouse_enabled->get ())
    BX_GUI_THIS mouse_hbar_id = headerbar_bitmap(BX_GUI_THIS mouse_bmap_id,
                          BX_GRAVITY_LEFT, toggle_mouse_enable);
  else
    BX_GUI_THIS mouse_hbar_id = headerbar_bitmap(BX_GUI_THIS nomouse_bmap_id,
                          BX_GRAVITY_LEFT, toggle_mouse_enable);

  // Power button
  BX_GUI_THIS power_hbar_id = headerbar_bitmap(BX_GUI_THIS power_bmap_id,
                          BX_GRAVITY_RIGHT, power_handler);
  // Reset button
  BX_GUI_THIS reset_hbar_id = headerbar_bitmap(BX_GUI_THIS reset_bmap_id,
                          BX_GRAVITY_RIGHT, reset_handler);
  // Snapshot button
  BX_GUI_THIS snapshot_hbar_id = headerbar_bitmap(BX_GUI_THIS snapshot_bmap_id,
                          BX_GRAVITY_RIGHT, snapshot_handler);
  // Configure button
  BX_GUI_THIS config_hbar_id = headerbar_bitmap(BX_GUI_THIS config_bmap_id,
                          BX_GRAVITY_RIGHT, config_handler);

  show_headerbar();
}

void
bx_gui_c::update_floppy_status_buttons (void) {
  BX_GUI_THIS floppyA_status = 
    bx_devices.floppy->get_media_status (0)
    && bx_options.floppya.Oinitial_status->get ();
  BX_GUI_THIS floppyB_status = 
      bx_devices.floppy->get_media_status (1)
      && bx_options.floppyb.Oinitial_status->get ();
  BX_GUI_THIS cdromD_status =
    bx_devices.hard_drive->get_cd_media_status()
    && bx_options.cdromd.Opresent->get ();
  if (BX_GUI_THIS floppyA_status)
    replace_bitmap(BX_GUI_THIS floppyA_hbar_id, BX_GUI_THIS floppyA_bmap_id);
  else {
#if BX_WITH_MACOS
    // If we are using the Mac floppy driver, eject the disk
    // from the floppy drive.  This doesn't work in MacOS X.
    if (!strcmp(bx_options.floppya.Opath->get (), SuperDrive))
      DiskEject(1);
#endif
    replace_bitmap(BX_GUI_THIS floppyA_hbar_id, BX_GUI_THIS floppyA_eject_bmap_id);
    }
  if (BX_GUI_THIS floppyB_status)
    replace_bitmap(BX_GUI_THIS floppyB_hbar_id, BX_GUI_THIS floppyB_bmap_id);
  else {
#if BX_WITH_MACOS
    // If we are using the Mac floppy driver, eject the disk
    // from the floppy drive.  This doesn't work in MacOS X.
    if (!strcmp(bx_options.floppyb.Opath->get (), SuperDrive))
      DiskEject(1);
#endif
    replace_bitmap(BX_GUI_THIS floppyB_hbar_id, BX_GUI_THIS floppyB_eject_bmap_id);
    }
  if (BX_GUI_THIS cdromD_status)
    replace_bitmap(BX_GUI_THIS cdromD_hbar_id, BX_GUI_THIS cdromD_bmap_id);
  else {
    replace_bitmap(BX_GUI_THIS cdromD_hbar_id, BX_GUI_THIS cdromD_eject_bmap_id);
    }
}

  void
bx_gui_c::floppyA_handler(void)
{
  BX_GUI_THIS floppyA_status = !BX_GUI_THIS floppyA_status;
  bx_devices.floppy->set_media_status(0, BX_GUI_THIS floppyA_status);
  BX_GUI_THIS update_floppy_status_buttons ();
}

  void
bx_gui_c::floppyB_handler(void)
{
  BX_GUI_THIS floppyB_status = !BX_GUI_THIS floppyB_status;
  bx_devices.floppy->set_media_status(1, BX_GUI_THIS floppyB_status);
  BX_GUI_THIS update_floppy_status_buttons ();
}

  void
bx_gui_c::cdromD_handler(void)
{
  BX_GUI_THIS cdromD_status =
    bx_devices.hard_drive->set_cd_media_status(!BX_GUI_THIS cdromD_status);
  BX_GUI_THIS update_floppy_status_buttons ();
}

  void
bx_gui_c::reset_handler(void)
{
  BX_INFO(( "system RESET callback." ));
  bx_pc_system.ResetSignal( PCS_SET ); /* XXX is this right? */
  for (int i=0; i<BX_SMP_PROCESSORS; i++)
      BX_CPU(i)->reset(BX_RESET_HARDWARE);
}

  void
bx_gui_c::power_handler(void)
{
  // the user pressed power button, so there's no doubt they want bochs
  // to quit.  Change panics to fatal for the GUI and then do a panic.
  LOG_THIS setonoff(LOGLEV_PANIC, ACT_FATAL);
  BX_PANIC (("POWER button turned off."));
  // shouldn't reach this point, but if you do, QUIT!!!
  fprintf (stderr, "Bochs is exiting because you pressed the power button.\n");
  ::exit (1);
}

  void
bx_gui_c::snapshot_handler(void)
{
  Bit8u* text_snapshot = NULL;
  unsigned line_addr, txHeight, txWidth;
  FILE *OUTPUT;

  bx_vga.get_text_snapshot(&text_snapshot, &txHeight, &txWidth);
  if (txHeight > 0) {
    OUTPUT = fopen("snapshot.txt", "w");
    for (unsigned i=0; i<txHeight; i++) {
      line_addr = i * txWidth * 2;
      for (unsigned j=0; j<(txWidth*2); j+=2) {
        if (OUTPUT) fputc(text_snapshot[line_addr+j], OUTPUT);
      }
      fprintf(OUTPUT, "\n");
    }
    fclose(OUTPUT);
  } else {
    BX_INFO(( "# SNAPSHOT callback (graphics mode unimplemented)." ));
  }
}

  void
bx_gui_c::config_handler(void)
{
#if BX_USE_CONTROL_PANEL
  bx_control_panel (BX_CPANEL_RUNTIME);
#else
  BX_INFO(( "# CONFIG callback (unimplemented)." ));
#endif
}

  void
bx_gui_c::toggle_mouse_enable(void)
{
  int old = bx_options.Omouse_enabled->get ();
  BX_DEBUG (("toggle mouse_enabled, now %d", !old));
  bx_options.Omouse_enabled->set (!old);
}

  void
bx_gui_c::mouse_enabled_changed (Boolean val)
{
  // This is only called when SIM->get_init_done is 1.  Note that VAL
  // is the new value of mouse_enabled, which may not match the old
  // value which is still in bx_options.Omouse_enabled->get ().
  BX_DEBUG (("replacing the mouse bitmaps"));
  if (val)
    replace_bitmap(BX_GUI_THIS mouse_hbar_id, BX_GUI_THIS mouse_bmap_id);
  else
    replace_bitmap(BX_GUI_THIS mouse_hbar_id, BX_GUI_THIS nomouse_bmap_id);
  // give the GUI a chance to respond to the event.  Most guis will hide
  // the native mouse cursor and do something to trap the mouse inside the
  // bochs VGA display window.
  mouse_enabled_changed_specific (val);
}

void 
bx_gui_c::init_signal_handlers ()
{
#if BX_GUI_SIGHANDLER
  Bit32u mask = get_sighandler_mask ();
  for (Bit32u sig=0; sig<32; sig++)
  {
    if (mask & (1<<sig))
      signal (sig, bx_signal_handler);
  }
#endif
}
