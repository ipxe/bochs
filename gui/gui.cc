//  Copyright (C) 2001  MandrakeSoft S.A.
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
#ifdef macintosh
#  include <Disks.h>
#endif




bx_gui_c   bx_gui;

#define BX_GUI_THIS bx_gui.
#define LOG_THIS BX_GUI_THIS

bx_gui_c::bx_gui_c(void)
{
  setprefix("[GUI ]"); // Init in specific_init
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
  BX_GUI_THIS mouse_bmap_id = create_bitmap(bx_mouse_bmap,
                          BX_MOUSE_BMAP_X, BX_MOUSE_BMAP_Y);
  BX_GUI_THIS nomouse_bmap_id = create_bitmap(bx_nomouse_bmap,
                          BX_MOUSE_BMAP_X, BX_MOUSE_BMAP_Y);


  BX_GUI_THIS power_bmap_id = create_bitmap(bx_power_bmap, BX_POWER_BMAP_X, BX_POWER_BMAP_Y);
  BX_GUI_THIS reset_bmap_id = create_bitmap(bx_reset_bmap, BX_RESET_BMAP_X, BX_RESET_BMAP_Y);
  BX_GUI_THIS snapshot_bmap_id = create_bitmap(bx_snapshot_bmap, BX_SNAPSHOT_BMAP_X, BX_SNAPSHOT_BMAP_Y);


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

  // Mouse button
  BX_GUI_THIS mouse_status = gui_get_mouse_enable();
  if (BX_GUI_THIS mouse_status)
    BX_GUI_THIS mouse_hbar_id = headerbar_bitmap(BX_GUI_THIS mouse_bmap_id,
                          BX_GRAVITY_LEFT, mouse_handler);
  else
    BX_GUI_THIS mouse_hbar_id = headerbar_bitmap(BX_GUI_THIS nomouse_bmap_id,
                          BX_GRAVITY_LEFT, mouse_handler);

  // Power button
  BX_GUI_THIS power_hbar_id = headerbar_bitmap(BX_GUI_THIS power_bmap_id,
                          BX_GRAVITY_RIGHT, power_handler);
  // Reset button
  BX_GUI_THIS reset_hbar_id = headerbar_bitmap(BX_GUI_THIS reset_bmap_id,
                          BX_GRAVITY_RIGHT, reset_handler);
  // Snapshot button
  BX_GUI_THIS snapshot_hbar_id = headerbar_bitmap(BX_GUI_THIS snapshot_bmap_id,
                          BX_GRAVITY_RIGHT, snapshot_handler);

  show_headerbar();
}


  void
bx_gui_c::floppyA_handler(void)
{
  unsigned new_status;

  new_status = bx_devices.floppy->set_media_status(0, !BX_GUI_THIS floppyA_status);
  if (new_status == BX_GUI_THIS floppyA_status)
    return;  // no change made

  BX_GUI_THIS floppyA_status = new_status;
  if (BX_GUI_THIS floppyA_status)
    replace_bitmap(BX_GUI_THIS floppyA_hbar_id, BX_GUI_THIS floppyA_bmap_id);
  else {
#ifdef macintosh
    // If we are using the Mac floppy driver, eject the disk
    // from the floppy drive
    if (!strcmp(bx_options.floppya.path, SuperDrive))
      DiskEject(1);
#endif
    replace_bitmap(BX_GUI_THIS floppyA_hbar_id, BX_GUI_THIS floppyA_eject_bmap_id);
    }
}

  void
bx_gui_c::floppyB_handler(void)
{
  unsigned new_status;

  new_status = bx_devices.floppy->set_media_status(1, !BX_GUI_THIS floppyB_status);
  if (new_status == BX_GUI_THIS floppyB_status)
    return;  // no change made

  BX_GUI_THIS floppyB_status = new_status;
  if (BX_GUI_THIS floppyB_status)
    replace_bitmap(BX_GUI_THIS floppyB_hbar_id, BX_GUI_THIS floppyB_bmap_id);
  else
    replace_bitmap(BX_GUI_THIS floppyB_hbar_id, BX_GUI_THIS floppyB_eject_bmap_id);
}

  void
bx_gui_c::reset_handler(void)
{
  BX_PANIC(( "RESET button was pressed.\n" ));
}

  void
bx_gui_c::power_handler(void)
{
  BX_PANIC(("POWER button turned off.\n"));
}

  void
bx_gui_c::snapshot_handler(void)
{
  BX_INFO(( "# SNAPSHOT callback (unimplemented).\n" ));
}

  void
bx_gui_c::mouse_handler(void)
{
  BX_GUI_THIS mouse_status = ! BX_GUI_THIS mouse_status;

  if (BX_GUI_THIS mouse_status)
    replace_bitmap(BX_GUI_THIS mouse_hbar_id, BX_GUI_THIS mouse_bmap_id);
  else
    replace_bitmap(BX_GUI_THIS mouse_hbar_id, BX_GUI_THIS nomouse_bmap_id);

  gui_set_mouse_enable(BX_GUI_THIS mouse_status);
}

  Boolean
bx_gui_c::gui_get_mouse_enable(void)
{
  return(bx_options.mouse_enabled);
}

  void
bx_gui_c::gui_set_mouse_enable(Boolean val)
{
  bx_options.mouse_enabled = val;
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
