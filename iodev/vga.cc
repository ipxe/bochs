/////////////////////////////////////////////////////////////////////////
// $Id: vga.cc,v 1.89 2003/10/04 15:58:21 danielg4 Exp $
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


// Define BX_PLUGGABLE in files that can be compiled into plugins.  For
// platforms that require a special tag on exported symbols, BX_PLUGGABLE 
// is used to know when we are exporting symbols and when we are importing.
#define BX_PLUGGABLE

#include "bochs.h"

#define LOG_THIS theVga->

/* NOTES:
 * I take it data rotate is a true rotate with carry of bit 0 to bit 7.
 * support map mask (3c5 reg 02)
 */

/* Notes from cb
 *
 * It seems that the vga card should support multi bytes IO reads and write
 * From my tests, inw(port) return port+1 * 256 + port, except for port 0x3c9
 * (PEL data register, data cycling). More reverse engineering is needed.
 * This would fix the gentoo bug.
 */

// (mch)
#define VGA_TRACE_FEATURE

// Only reference the array if the tile numbers are within the bounds
// of the array.  If out of bounds, do nothing.
#define SET_TILE_UPDATED(xtile,ytile,value)                              \
  do {                                                                   \
    if (((xtile) < BX_NUM_X_TILES) && ((ytile) < BX_NUM_Y_TILES))        \
      BX_VGA_THIS s.vga_tile_updated[(xtile)][(ytile)] = value;          \
  } while (0)

// Only reference the array if the tile numbers are within the bounds
// of the array.  If out of bounds, return 0.
#define GET_TILE_UPDATED(xtile,ytile)                                    \
  ((((xtile) < BX_NUM_X_TILES) && ((ytile) < BX_NUM_Y_TILES))?           \
     BX_VGA_THIS s.vga_tile_updated[(xtile)][(ytile)]                    \
     : 0)

static const Bit8u ccdat[16][4] = {
  { 0x00, 0x00, 0x00, 0x00 },
  { 0xff, 0x00, 0x00, 0x00 },
  { 0x00, 0xff, 0x00, 0x00 },
  { 0xff, 0xff, 0x00, 0x00 },
  { 0x00, 0x00, 0xff, 0x00 },
  { 0xff, 0x00, 0xff, 0x00 },
  { 0x00, 0xff, 0xff, 0x00 },
  { 0xff, 0xff, 0xff, 0x00 },
  { 0x00, 0x00, 0x00, 0xff },
  { 0xff, 0x00, 0x00, 0xff },
  { 0x00, 0xff, 0x00, 0xff },
  { 0xff, 0xff, 0x00, 0xff },
  { 0x00, 0x00, 0xff, 0xff },
  { 0xff, 0x00, 0xff, 0xff },
  { 0x00, 0xff, 0xff, 0xff },
  { 0xff, 0xff, 0xff, 0xff },
};

bx_vga_c *theVga = NULL;

unsigned old_iHeight = 0, old_iWidth = 0, old_MSL = 0;

  int
libvga_LTX_plugin_init(plugin_t *plugin, plugintype_t type, int argc, char *argv[])
{
  theVga = new bx_vga_c ();
  bx_devices.pluginVgaDevice = theVga;
  BX_REGISTER_DEVICE_DEVMODEL(plugin, type, theVga, BX_PLUGIN_VGA);
  return(0); // Success
}

  void
libvga_LTX_plugin_fini(void)
{
}

bx_vga_c::bx_vga_c(void)
{
  put("VGA");
  s.vga_mem_updated = 0;
  s.x_tilesize = X_TILESIZE;
  s.y_tilesize = Y_TILESIZE;
  timer_id = BX_NULL_TIMER_HANDLE;
}


bx_vga_c::~bx_vga_c(void)
{
  // nothing for now
}


  void
bx_vga_c::init(void)
{
  unsigned i;
  unsigned x,y;

  unsigned addr;
  for (addr=0x03B4; addr<=0x03B5; addr++) {
    DEV_register_ioread_handler(this, read_handler, addr, "vga video", 1);
    DEV_register_iowrite_handler(this, write_handler, addr, "vga video", 3);
    }

  for (addr=0x03BA; addr<=0x03BA; addr++) {
    DEV_register_ioread_handler(this, read_handler, addr, "vga video", 1);
    DEV_register_iowrite_handler(this, write_handler, addr, "vga video", 3);
    }

  for (addr=0x03C0; addr<=0x03CF; addr++) {
    DEV_register_ioread_handler(this, read_handler, addr, "vga video", 1);
    DEV_register_iowrite_handler(this, write_handler, addr, "vga video", 3);
    }

  for (addr=0x03D4; addr<=0x03D5; addr++) {
    DEV_register_ioread_handler(this, read_handler, addr, "vga video", 1);
    DEV_register_iowrite_handler(this, write_handler, addr, "vga video", 3);
    }

  for (addr=0x03DA; addr<=0x03DA; addr++) {
    DEV_register_ioread_handler(this, read_handler, addr, "vga video", 1);
    DEV_register_iowrite_handler(this, write_handler, addr, "vga video", 3);
    }


  BX_VGA_THIS s.misc_output.color_emulation  = 1;
  BX_VGA_THIS s.misc_output.enable_ram  = 1;
  BX_VGA_THIS s.misc_output.clock_select     = 0;
  BX_VGA_THIS s.misc_output.select_high_bank = 0;
  BX_VGA_THIS s.misc_output.horiz_sync_pol   = 1;
  BX_VGA_THIS s.misc_output.vert_sync_pol    = 1;

  BX_VGA_THIS s.CRTC.address = 0;

  BX_VGA_THIS s.attribute_ctrl.mode_ctrl.graphics_alpha = 0;
  BX_VGA_THIS s.attribute_ctrl.mode_ctrl.display_type = 0;
  BX_VGA_THIS s.attribute_ctrl.mode_ctrl.enable_line_graphics = 1;
  BX_VGA_THIS s.attribute_ctrl.mode_ctrl.blink_intensity = 0;
  BX_VGA_THIS s.attribute_ctrl.mode_ctrl.pixel_panning_compat = 0;
  BX_VGA_THIS s.attribute_ctrl.mode_ctrl.pixel_clock_select = 0;
  BX_VGA_THIS s.attribute_ctrl.mode_ctrl.internal_palette_size = 0;

  BX_VGA_THIS s.line_offset=80;
  BX_VGA_THIS s.line_compare=1023;
  BX_VGA_THIS s.vertical_display_end=399;

  for (i=0; i<0x18; i++)
    BX_VGA_THIS s.CRTC.reg[i] = 0;
  BX_VGA_THIS s.CRTC.address = 0;

  BX_VGA_THIS s.attribute_ctrl.flip_flop = 0;
  BX_VGA_THIS s.attribute_ctrl.address = 0;
  BX_VGA_THIS s.attribute_ctrl.video_enabled = 1;
  for (i=0; i<16; i++)
    BX_VGA_THIS s.attribute_ctrl.palette_reg[i] = 0;
  BX_VGA_THIS s.attribute_ctrl.overscan_color = 0;
  BX_VGA_THIS s.attribute_ctrl.color_plane_enable = 0x0f;
  BX_VGA_THIS s.attribute_ctrl.horiz_pel_panning = 0;
  BX_VGA_THIS s.attribute_ctrl.color_select = 0;

  for (i=0; i<256; i++) {
    BX_VGA_THIS s.pel.data[i].red = 0;
    BX_VGA_THIS s.pel.data[i].green = 0;
    BX_VGA_THIS s.pel.data[i].blue = 0;
    }
  BX_VGA_THIS s.pel.write_data_register = 0;
  BX_VGA_THIS s.pel.write_data_cycle = 0;
  BX_VGA_THIS s.pel.read_data_register = 0;
  BX_VGA_THIS s.pel.read_data_cycle = 0;
  BX_VGA_THIS s.pel.dac_state = 0x01;
  BX_VGA_THIS s.pel.mask = 0xff;

  BX_VGA_THIS s.graphics_ctrl.index = 0;
  BX_VGA_THIS s.graphics_ctrl.set_reset = 0;
  BX_VGA_THIS s.graphics_ctrl.enable_set_reset = 0;
  BX_VGA_THIS s.graphics_ctrl.color_compare = 0;
  BX_VGA_THIS s.graphics_ctrl.data_rotate = 0;
  BX_VGA_THIS s.graphics_ctrl.raster_op    = 0;
  BX_VGA_THIS s.graphics_ctrl.read_map_select = 0;
  BX_VGA_THIS s.graphics_ctrl.write_mode = 0;
  BX_VGA_THIS s.graphics_ctrl.read_mode  = 0;
  BX_VGA_THIS s.graphics_ctrl.odd_even = 0;
  BX_VGA_THIS s.graphics_ctrl.chain_odd_even = 0;
  BX_VGA_THIS s.graphics_ctrl.shift_reg = 0;
  BX_VGA_THIS s.graphics_ctrl.graphics_alpha = 0;
  BX_VGA_THIS s.graphics_ctrl.memory_mapping = 2; // monochrome text mode
  BX_VGA_THIS s.graphics_ctrl.color_dont_care = 0;
  BX_VGA_THIS s.graphics_ctrl.bitmask = 0;
  for (i=0; i<4; i++) {
    BX_VGA_THIS s.graphics_ctrl.latch[i] = 0;
    }

  BX_VGA_THIS s.sequencer.index = 0;
  BX_VGA_THIS s.sequencer.map_mask = 0;
  for (i=0; i<4; i++) {
    BX_VGA_THIS s.sequencer.map_mask_bit[i] = 0;
    }
  BX_VGA_THIS s.sequencer.reset1 = 1;
  BX_VGA_THIS s.sequencer.reset2 = 1;
  BX_VGA_THIS s.sequencer.reg1 = 0;
  BX_VGA_THIS s.sequencer.char_map_select = 0;
  BX_VGA_THIS s.sequencer.extended_mem = 1; // display mem greater than 64K
  BX_VGA_THIS s.sequencer.odd_even = 1; // use sequential addressing mode
  BX_VGA_THIS s.sequencer.chain_four = 0; // use map mask & read map select

  memset(BX_VGA_THIS s.vga_memory, 0, sizeof(BX_VGA_THIS s.vga_memory));

  BX_VGA_THIS s.vga_mem_updated = 0;
  for (y=0; y<480/Y_TILESIZE; y++)
    for (x=0; x<640/X_TILESIZE; x++)
      SET_TILE_UPDATED (x, y, 0);

  {
  /* ??? should redo this to pass X args */
  char *argv[1] = { "bochs" };
  bx_gui->init(1, &argv[0], BX_VGA_THIS s.x_tilesize, BX_VGA_THIS s.y_tilesize);
  }

  BX_INFO(("interval=%u", bx_options.Ovga_update_interval->get ()));
  if (BX_VGA_THIS timer_id == BX_NULL_TIMER_HANDLE) {
    BX_VGA_THIS timer_id = bx_pc_system.register_timer(this, timer_handler,
       bx_options.Ovga_update_interval->get (), 1, 1, "vga");
  }

  /* video card with BIOS ROM */
  DEV_cmos_set_reg(0x14, (DEV_cmos_get_reg(0x14) & 0xcf) | 0x00); 

  BX_VGA_THIS s.charmap_address = 0;
  BX_VGA_THIS s.x_dotclockdiv2 = 0;
  BX_VGA_THIS s.y_doublescan = 0;

#if BX_SUPPORT_VBE  
  // The following is for the vbe display extension
  // FIXME: change 0xff80 & 0xff81 into some nice constants
  
  for (addr=VBE_DISPI_IOPORT_INDEX; addr<=VBE_DISPI_IOPORT_DATA; addr++) {
    DEV_register_ioread_handler(this, vbe_read_handler, addr, "vga video", 7);
    DEV_register_iowrite_handler(this, vbe_write_handler, addr, "vga video", 7);
  }    
  BX_VGA_THIS s.vbe_cur_dispi=VBE_DISPI_ID0;
  BX_VGA_THIS s.vbe_xres=640;
  BX_VGA_THIS s.vbe_yres=480;
  BX_VGA_THIS s.vbe_bpp=8;
  BX_VGA_THIS s.vbe_bank=0;
  BX_VGA_THIS s.vbe_enabled=0;
  BX_VGA_THIS s.vbe_curindex=0;
  BX_VGA_THIS s.vbe_offset_x=0;
  BX_VGA_THIS s.vbe_offset_y=0;
  BX_VGA_THIS s.vbe_virtual_xres=640;
  BX_VGA_THIS s.vbe_virtual_yres=480;
  BX_VGA_THIS s.vbe_bpp_multiplier=1;
  BX_VGA_THIS s.vbe_virtual_start=0;
  BX_VGA_THIS s.vbe_line_byte_width=640;
  BX_VGA_THIS s.vbe_lfb_enabled=0;

  
  BX_INFO(("VBE Bochs Display Extension Enabled"));
#endif  
}

  void
bx_vga_c::reset(unsigned type)
{
}


  void
bx_vga_c::determine_screen_dimensions(unsigned *piHeight, unsigned *piWidth)
{
  int ai[0x20];
  int i,h,v;
  for ( i = 0 ; i < 0x20 ; i++ )
   ai[i] = BX_VGA_THIS s.CRTC.reg[i];

  h = (ai[1] + 1) * 8;
  v = (ai[18] | ((ai[7] & 0x02) << 7) | ((ai[7] & 0x40) << 3)) + 1;

  /*
  switch ( ( BX_VGA_THIS s.misc_output.vert_sync_pol << 1) | BX_VGA_THIS s.misc_output.horiz_sync_pol )
   {
   case 0: *piHeight = 200; break;
   case 1: *piHeight = 400; break;
   case 2: *piHeight = 350; break;
   case 3: *piHeight = 480; break;
   }
  */

  if ( BX_VGA_THIS s.graphics_ctrl.shift_reg == 0 )
    {
    *piWidth = 640;
    *piHeight = 480;

    if ( BX_VGA_THIS s.CRTC.reg[6] == 0xBF )
      {
      if (BX_VGA_THIS s.CRTC.reg[23] == 0xA3 &&
         BX_VGA_THIS s.CRTC.reg[20] == 0x40 &&
         BX_VGA_THIS s.CRTC.reg[9] == 0x41)
        {
        *piWidth = 320;
        *piHeight = 240;
        }
      else {
        if (BX_VGA_THIS s.x_dotclockdiv2) h <<= 1;
        *piWidth = h;
        *piHeight = v;
        }
      }
    else if ((h >= 640) && (v >= 480)) {
      *piWidth = h;
      *piHeight = v;
      }
    }
  else if ( BX_VGA_THIS s.graphics_ctrl.shift_reg == 2 )
    {

    if ( BX_VGA_THIS s.sequencer.chain_four )
      {
      *piWidth = h;
      *piHeight = v;
      }
    else
      {
      *piWidth = h;
      *piHeight = v;
      }
    }
  else
    {
    if (BX_VGA_THIS s.x_dotclockdiv2) h <<= 1;
    *piWidth = h;
    *piHeight = v;
    }
}


  // static IO port read callback handler
  // redirects to non-static class handler to avoid virtual functions

  Bit32u
bx_vga_c::read_handler(void *this_ptr, Bit32u address, unsigned io_len)
{
#if !BX_USE_VGA_SMF
  bx_vga_c *class_ptr = (bx_vga_c *) this_ptr;

  return( class_ptr->read(address, io_len) );
}


  Bit32u
bx_vga_c::read(Bit32u address, unsigned io_len)
{
#else
  UNUSED(this_ptr);
#endif  // !BX_USE_VGA_SMF
  bx_bool  horiz_retrace = 0, vert_retrace = 0;
  Bit64u usec;
  Bit16u vertres;
  Bit8u retval;

#if defined(VGA_TRACE_FEATURE)
  Bit32u ret = 0;
#define RETURN(x) do { ret = (x); goto read_return; } while (0)
#else
#define RETURN return
#endif

#ifdef __OS2__
  if ( bx_options.videomode == BX_VIDEO_DIRECT )
     {
     return _inp(address);
     }
#endif

#if !defined(VGA_TRACE_FEATURE)
    BX_DEBUG(("io read from 0x%04x", (unsigned) address));
#endif

  if ( (address >= 0x03b0) && (address <= 0x03bf) &&
       (BX_VGA_THIS s.misc_output.color_emulation) ) {
	RETURN(0xff);
  }
  if ( (address >= 0x03d0) && (address <= 0x03df) &&
       (BX_VGA_THIS s.misc_output.color_emulation==0) ) {
	RETURN(0xff);
  }

  switch (address) {
    case 0x03ba: /* Input Status 1 (monochrome emulation modes) */
    case 0x03ca: /* Feature Control ??? */
    case 0x03da: /* Input Status 1 (color emulation modes) */
      // bit3: Vertical Retrace
      //       0 = display is in the display mode
      //       1 = display is in the vertical retrace mode
      // bit0: Display Enable
      //       0 = display is in the display mode
      //       1 = display is not in the display mode; either the
      //           horizontal or vertical retrace period is active

      // using 72 Hz vertical frequency
      usec = bx_pc_system.time_usec();
      switch ( ( BX_VGA_THIS s.misc_output.vert_sync_pol << 1) | BX_VGA_THIS s.misc_output.horiz_sync_pol )
      {
        case 0: vertres = 200; break;
        case 1: vertres = 400; break;
        case 2: vertres = 350; break;
        default: vertres = 480; break;
      }
      if ((usec % 13888) < 70) {
        vert_retrace = 1;
      }
      if ((usec % (13888 / vertres)) == 0) {
        horiz_retrace = 1;
      }

      retval = 0;
      if (horiz_retrace || vert_retrace)
        retval = 0x01;
      if (vert_retrace)
        retval |= 0x08;

      /* reading this port resets the flip-flop to address mode */
      BX_VGA_THIS s.attribute_ctrl.flip_flop = 0;
      RETURN(retval);
      break;


    case 0x03c0: /* */
      if (BX_VGA_THIS s.attribute_ctrl.flip_flop == 0) {
        //BX_INFO(("io read: 0x3c0: flip_flop = 0"));
        retval =
          (BX_VGA_THIS s.attribute_ctrl.video_enabled << 5) |
          BX_VGA_THIS s.attribute_ctrl.address;
        RETURN(retval);
        }
      else {
        BX_ERROR(("io read: 0x3c0: flip_flop != 0"));
        return(0);
        }
      break;

    case 0x03c1: /* */
      switch (BX_VGA_THIS s.attribute_ctrl.address) {
        case 0x00: case 0x01: case 0x02: case 0x03:
        case 0x04: case 0x05: case 0x06: case 0x07:
        case 0x08: case 0x09: case 0x0a: case 0x0b:
        case 0x0c: case 0x0d: case 0x0e: case 0x0f:
          retval = BX_VGA_THIS s.attribute_ctrl.palette_reg[BX_VGA_THIS s.attribute_ctrl.address];
	  RETURN(retval);
          break;
        case 0x10: /* mode control register */
          retval =
            (BX_VGA_THIS s.attribute_ctrl.mode_ctrl.graphics_alpha << 0) |
            (BX_VGA_THIS s.attribute_ctrl.mode_ctrl.display_type << 1) |
            (BX_VGA_THIS s.attribute_ctrl.mode_ctrl.enable_line_graphics << 2) |
            (BX_VGA_THIS s.attribute_ctrl.mode_ctrl.blink_intensity << 3) |
            (BX_VGA_THIS s.attribute_ctrl.mode_ctrl.pixel_panning_compat << 5) |
            (BX_VGA_THIS s.attribute_ctrl.mode_ctrl.pixel_clock_select << 6) |
            (BX_VGA_THIS s.attribute_ctrl.mode_ctrl.internal_palette_size << 7);
	  RETURN(retval);
          break;
	case 0x11: /* overscan color register */
	  RETURN(BX_VGA_THIS s.attribute_ctrl.overscan_color);
          break;
	case 0x12: /* color plane enable */
	  RETURN(BX_VGA_THIS s.attribute_ctrl.color_plane_enable);
          break;
        case 0x13: /* horizontal PEL panning register */
          RETURN(BX_VGA_THIS s.attribute_ctrl.horiz_pel_panning);
          break;
        case 0x14: /* color select register */
          RETURN(BX_VGA_THIS s.attribute_ctrl.color_select);
          break;
        default:
          BX_INFO(("io read: 0x3c1: unknown register 0x%02x",
            (unsigned) BX_VGA_THIS s.attribute_ctrl.address));
          RETURN(0);
        }
      break;

    case 0x03c2: /* Input Status 0 */
      BX_DEBUG(("io read 0x3c2: input status #0: ignoring"));
      RETURN(0);
      break;

    case 0x03c3: /* VGA Enable Register */
      RETURN(1);
      break;

    case 0x03c4: /* Sequencer Index Register */
      RETURN(BX_VGA_THIS s.sequencer.index);
      break;

    case 0x03c5: /* Sequencer Registers 00..04 */
      switch (BX_VGA_THIS s.sequencer.index) {
        case 0: /* sequencer: reset */
          BX_DEBUG(("io read 0x3c5: sequencer reset"));
          RETURN(BX_VGA_THIS s.sequencer.reset1 | (BX_VGA_THIS s.sequencer.reset2<<1));
          break;
        case 1: /* sequencer: clocking mode */
          BX_DEBUG(("io read 0x3c5: sequencer clocking mode"));
          RETURN(BX_VGA_THIS s.sequencer.reg1);
          break;
        case 2: /* sequencer: map mask register */
          RETURN(BX_VGA_THIS s.sequencer.map_mask);
          break;
        case 3: /* sequencer: character map select register */
          RETURN(BX_VGA_THIS s.sequencer.char_map_select);
          break;
        case 4: /* sequencer: memory mode register */
          retval =
            (BX_VGA_THIS s.sequencer.extended_mem   << 1) |
            (BX_VGA_THIS s.sequencer.odd_even       << 2) |
            (BX_VGA_THIS s.sequencer.chain_four     << 3);
          RETURN(retval);
          break;

        default:
          BX_DEBUG(("io read 0x3c5: index %u unhandled",
            (unsigned) BX_VGA_THIS s.sequencer.index));
          RETURN(0);
        }
      break;

    case 0x03c6: /* PEL mask ??? */
      RETURN(BX_VGA_THIS s.pel.mask);
      break;

    case 0x03c7: /* DAC state, read = 11b, write = 00b */
      RETURN(BX_VGA_THIS s.pel.dac_state);
      break;

    case 0x03c8: /* PEL address write mode */
      RETURN(BX_VGA_THIS s.pel.write_data_register);
      break;

    case 0x03c9: /* PEL Data Register, colors 00..FF */
      if (BX_VGA_THIS s.pel.dac_state == 0x03) {
        switch (BX_VGA_THIS s.pel.read_data_cycle) {
          case 0:
            retval = BX_VGA_THIS s.pel.data[BX_VGA_THIS s.pel.read_data_register].red;
            break;
          case 1:
            retval = BX_VGA_THIS s.pel.data[BX_VGA_THIS s.pel.read_data_register].green;
            break;
          case 2:
            retval = BX_VGA_THIS s.pel.data[BX_VGA_THIS s.pel.read_data_register].blue;
            break;
          default:
            retval = 0; // keep compiler happy
          }
        BX_VGA_THIS s.pel.read_data_cycle++;
        if (BX_VGA_THIS s.pel.read_data_cycle >= 3) {
          BX_VGA_THIS s.pel.read_data_cycle = 0;
          BX_VGA_THIS s.pel.read_data_register++;
          }
	}
      else {
        retval = 0x3f;
        }
      RETURN(retval);
      break;

    case 0x03cc: /* Miscellaneous Output / Graphics 1 Position ??? */
      retval =
        ((BX_VGA_THIS s.misc_output.color_emulation  & 0x01) << 0) |
        ((BX_VGA_THIS s.misc_output.enable_ram       & 0x01) << 1) |
        ((BX_VGA_THIS s.misc_output.clock_select     & 0x03) << 2) |
        ((BX_VGA_THIS s.misc_output.select_high_bank & 0x01) << 5) |
        ((BX_VGA_THIS s.misc_output.horiz_sync_pol   & 0x01) << 6) |
        ((BX_VGA_THIS s.misc_output.vert_sync_pol    & 0x01) << 7);
      RETURN(retval);
      break;

    case 0x03ce: /* Graphics Controller Index Register */
      RETURN(BX_VGA_THIS s.graphics_ctrl.index);
      break;

    case 0x03cd: /* ??? */
      BX_DEBUG(("io read from 03cd"));
      RETURN(0x00);
      break;

    case 0x03cf: /* Graphics Controller Registers 00..08 */
      switch (BX_VGA_THIS s.graphics_ctrl.index) {
        case 0: /* Set/Reset */
          RETURN(BX_VGA_THIS s.graphics_ctrl.set_reset);
          break;
        case 1: /* Enable Set/Reset */
          RETURN(BX_VGA_THIS s.graphics_ctrl.enable_set_reset);
          break;
        case 2: /* Color Compare */
          RETURN(BX_VGA_THIS s.graphics_ctrl.color_compare);
          break;
        case 3: /* Data Rotate */
          retval =
            ((BX_VGA_THIS s.graphics_ctrl.raster_op & 0x03) << 3) |
            ((BX_VGA_THIS s.graphics_ctrl.data_rotate & 0x07) << 0);
          RETURN(retval);
          break;
        case 4: /* Read Map Select */
          RETURN(BX_VGA_THIS s.graphics_ctrl.read_map_select);
          break;
        case 5: /* Mode */
          retval =
            ((BX_VGA_THIS s.graphics_ctrl.shift_reg & 0x03) << 5) |
            ((BX_VGA_THIS s.graphics_ctrl.odd_even & 0x01 ) << 4) |
            ((BX_VGA_THIS s.graphics_ctrl.read_mode & 0x01) << 3) |
            ((BX_VGA_THIS s.graphics_ctrl.write_mode & 0x03) << 0);

          if (BX_VGA_THIS s.graphics_ctrl.odd_even ||
              BX_VGA_THIS s.graphics_ctrl.shift_reg)
            BX_DEBUG(("io read 0x3cf: reg 05 = 0x%02x", (unsigned) retval));
          RETURN(retval);
          break;
        case 6: /* Miscellaneous */
          retval =
            ((BX_VGA_THIS s.graphics_ctrl.memory_mapping & 0x03 ) << 2) |
            ((BX_VGA_THIS s.graphics_ctrl.odd_even & 0x01) << 1) |
            ((BX_VGA_THIS s.graphics_ctrl.graphics_alpha & 0x01) << 0);
          RETURN(retval);
          break;
        case 7: /* Color Don't Care */
          RETURN(BX_VGA_THIS s.graphics_ctrl.color_dont_care);
          break;
        case 8: /* Bit Mask */
          RETURN(BX_VGA_THIS s.graphics_ctrl.bitmask);
          break;
        default:
          /* ??? */
          BX_DEBUG(("io read: 0x3cf: index %u unhandled",
            (unsigned) BX_VGA_THIS s.graphics_ctrl.index));
          RETURN(0);
        }
      break;

    case 0x03d4: /* CRTC Index Register (color emulation modes) */
      RETURN(BX_VGA_THIS s.CRTC.address);
      break;

    case 0x03b5: /* CRTC Registers (monochrome emulation modes) */
    case 0x03d5: /* CRTC Registers (color emulation modes) */
      if (BX_VGA_THIS s.CRTC.address > 0x18) {
        BX_DEBUG(("io read: invalid CRTC register 0x%02x",
          (unsigned) BX_VGA_THIS s.CRTC.address));
        RETURN(0);
      }
      RETURN(BX_VGA_THIS s.CRTC.reg[BX_VGA_THIS s.CRTC.address]);
      break;

    case 0x03b4: /* CRTC Index Register (monochrome emulation modes) */
    case 0x03cb: /* not sure but OpenBSD reads it a lot */
    default:
      BX_INFO(("io read from vga port 0x%02x", (unsigned) address));
      RETURN(0); /* keep compiler happy */
    }

#if defined(VGA_TRACE_FEATURE)
  read_return:
	BX_DEBUG(("8-bit read from %04x = %02x", (unsigned) address, ret));
  return ret;
#endif
}
#if defined(VGA_TRACE_FEATURE)
#undef RETURN
#endif

  // static IO port write callback handler
  // redirects to non-static class handler to avoid virtual functions

  void
bx_vga_c::write_handler(void *this_ptr, Bit32u address, Bit32u value, unsigned io_len)
{
#if !BX_USE_VGA_SMF
  bx_vga_c *class_ptr = (bx_vga_c *) this_ptr;

  class_ptr->write(address, value, io_len, 0);
#else
  UNUSED(this_ptr);
  theVga->write(address, value, io_len, 0);
#endif
}

  void
bx_vga_c::write_handler_no_log(void *this_ptr, Bit32u address, Bit32u value, unsigned io_len)
{
#if !BX_USE_VGA_SMF
  bx_vga_c *class_ptr = (bx_vga_c *) this_ptr;

  class_ptr->write(address, value, io_len, 1);
#else
  UNUSED(this_ptr);
  theVga->write(address, value, io_len, 1);
#endif
}

  void
bx_vga_c::write(Bit32u address, Bit32u value, unsigned io_len, bx_bool no_log)
{
  unsigned i;
  Bit8u charmap1, charmap2, prev_memory_mapping;
  bx_bool prev_video_enabled, prev_line_graphics, prev_int_pal_size;
  bx_bool prev_graphics_alpha, prev_chain_odd_even;
  bx_bool needs_update = 0;

#if defined(VGA_TRACE_FEATURE)
  if (!no_log)
	switch (io_len) {
	      case 1:
		    BX_DEBUG(("8-bit write to %04x = %02x", (unsigned)address, (unsigned)value));
		    break;
	      case 2:
		    BX_DEBUG(("16-bit write to %04x = %04x", (unsigned)address, (unsigned)value));
		    break;
	      default:
		    BX_PANIC(("Weird VGA write size"));
	}
#else
  if (io_len == 1) {
    BX_DEBUG(("io write to 0x%04x = 0x%02x", (unsigned) address,
      (unsigned) value));
  }
#endif

  if (io_len == 2) {
#if BX_USE_VGA_SMF
    bx_vga_c::write_handler_no_log(0, address, value & 0xff, 1);
    bx_vga_c::write_handler_no_log(0, address+1, (value >> 8) & 0xff, 1);
#else
    bx_vga_c::write(address, value & 0xff, 1, 1);
    bx_vga_c::write(address+1, (value >> 8) & 0xff, 1, 1);
#endif
    return;
    }

#ifdef __OS2__
  if ( bx_options.videomode == BX_VIDEO_DIRECT )
     {
     _outp(address,value);
     return;
     }
#endif

  if ( (address >= 0x03b0) && (address <= 0x03bf) &&
       (BX_VGA_THIS s.misc_output.color_emulation) )
    return;
  if ( (address >= 0x03d0) && (address <= 0x03df) &&
       (BX_VGA_THIS s.misc_output.color_emulation==0) )
    return;

  switch (address) {
    case 0x03b4: /* CRTC Index Register (monochrome emulation modes) */
      BX_VGA_THIS s.CRTC.address = value & 0x7f;
      if (BX_VGA_THIS s.CRTC.address > 0x18)
        BX_DEBUG(("write: invalid CRTC register 0x%02x selected",
          (unsigned) BX_VGA_THIS s.CRTC.address));
      break;

    case 0x03b5: /* CRTC Registers (monochrome emulation modes) */
      if (BX_VGA_THIS s.CRTC.address > 0x18) {
        BX_DEBUG(("write: invalid CRTC register 0x%02x ignored",
          (unsigned) BX_VGA_THIS s.CRTC.address));
        return;
      }
      BX_VGA_THIS s.CRTC.reg[BX_VGA_THIS s.CRTC.address] = value;
#if !defined(VGA_TRACE_FEATURE)
      BX_DEBUG(("mono CRTC Reg[%u] = %02x",
          (unsigned) BX_VGA_THIS s.CRTC.address, (unsigned) value));
#endif
      break;

    case 0x03ba: /* Feature Control (monochrome emulation modes) */
#if !defined(VGA_TRACE_FEATURE)
      BX_DEBUG(("io write 3ba: feature control: ignoring"));
#endif
      break;

    case 0x03c0: /* Attribute Controller */
      if (BX_VGA_THIS s.attribute_ctrl.flip_flop == 0) { /* address mode */
        prev_video_enabled = BX_VGA_THIS s.attribute_ctrl.video_enabled;
        BX_VGA_THIS s.attribute_ctrl.video_enabled = (value >> 5) & 0x01;
#if !defined(VGA_TRACE_FEATURE)
        BX_DEBUG(("io write 3c0: video_enabled = %u",
                  (unsigned) BX_VGA_THIS s.attribute_ctrl.video_enabled));
#endif
        if (BX_VGA_THIS s.attribute_ctrl.video_enabled == 0)
          bx_gui->clear_screen();
        else if (!prev_video_enabled) {
#if !defined(VGA_TRACE_FEATURE)
          BX_DEBUG(("found enable transition"));
#endif
          needs_update = 1;
        }
        value &= 0x1f; /* address = bits 0..4 */
        BX_VGA_THIS s.attribute_ctrl.address = value;
        switch (value) {
          case 0x00: case 0x01: case 0x02: case 0x03:
          case 0x04: case 0x05: case 0x06: case 0x07:
          case 0x08: case 0x09: case 0x0a: case 0x0b:
          case 0x0c: case 0x0d: case 0x0e: case 0x0f:
            break;

          default:
            BX_DEBUG(("io write 3c0: address mode reg=%u",
              (unsigned) value));
          }
        }
      else { /* data-write mode */
        switch (BX_VGA_THIS s.attribute_ctrl.address) {
          case 0x00: case 0x01: case 0x02: case 0x03:
          case 0x04: case 0x05: case 0x06: case 0x07:
          case 0x08: case 0x09: case 0x0a: case 0x0b:
          case 0x0c: case 0x0d: case 0x0e: case 0x0f:
            if (value != BX_VGA_THIS s.attribute_ctrl.palette_reg[BX_VGA_THIS s.attribute_ctrl.address]) {
              BX_VGA_THIS s.attribute_ctrl.palette_reg[BX_VGA_THIS s.attribute_ctrl.address] =
                value;
              needs_update = 1;
            }
            break;
          case 0x10: // mode control register
            prev_line_graphics = BX_VGA_THIS s.attribute_ctrl.mode_ctrl.enable_line_graphics;
            prev_int_pal_size = BX_VGA_THIS s.attribute_ctrl.mode_ctrl.internal_palette_size;
            BX_VGA_THIS s.attribute_ctrl.mode_ctrl.graphics_alpha =
              (value >> 0) & 0x01;
            BX_VGA_THIS s.attribute_ctrl.mode_ctrl.display_type =
              (value >> 1) & 0x01;
            BX_VGA_THIS s.attribute_ctrl.mode_ctrl.enable_line_graphics =
              (value >> 2) & 0x01;
            BX_VGA_THIS s.attribute_ctrl.mode_ctrl.blink_intensity =
              (value >> 3) & 0x01;
            BX_VGA_THIS s.attribute_ctrl.mode_ctrl.pixel_panning_compat =
              (value >> 5) & 0x01;
            BX_VGA_THIS s.attribute_ctrl.mode_ctrl.pixel_clock_select =
              (value >> 6) & 0x01;
            BX_VGA_THIS s.attribute_ctrl.mode_ctrl.internal_palette_size =
              (value >> 7) & 0x01;
            if (((value >> 2) & 0x01) != prev_line_graphics) {
              bx_gui->set_text_charmap(
                & BX_VGA_THIS s.vga_memory[0x20000 + BX_VGA_THIS s.charmap_address]);
              BX_VGA_THIS s.vga_mem_updated = 1;
            }
            if (((value >> 7) & 0x01) != prev_int_pal_size) {
              needs_update = 1;
            }
#if !defined(VGA_TRACE_FEATURE)
            BX_DEBUG(("io write 3c0: mode control: %02x h",
                (unsigned) value));
#endif
            break;
          case 0x11: // Overscan Color Register
            BX_VGA_THIS s.attribute_ctrl.overscan_color = (value & 0x3f);
#if !defined(VGA_TRACE_FEATURE)
            BX_DEBUG(("io write 3c0: overscan color = %02x",
                        (unsigned) value));
#endif
            break;
          case 0x12: // Color Plane Enable Register
            BX_VGA_THIS s.attribute_ctrl.color_plane_enable = (value & 0x0f);
            needs_update = 1;
#if !defined(VGA_TRACE_FEATURE)
            BX_DEBUG(("io write 3c0: color plane enable = %02x",
                        (unsigned) value));
#endif
            break;
          case 0x13: // Horizontal Pixel Panning Register
            BX_VGA_THIS s.attribute_ctrl.horiz_pel_panning = (value & 0x0f);
            needs_update = 1;
#if !defined(VGA_TRACE_FEATURE)
            BX_DEBUG(("io write 3c0: horiz pel panning = %02x",
                        (unsigned) value));
#endif
            break;
          case 0x14: // Color Select Register
            BX_VGA_THIS s.attribute_ctrl.color_select = (value & 0x0f);
            needs_update = 1;
#if !defined(VGA_TRACE_FEATURE)
            BX_DEBUG(("io write 3c0: color select = %02x",
                        (unsigned) BX_VGA_THIS s.attribute_ctrl.color_select));
#endif
            break;
          default:
            BX_DEBUG(("io write 3c0: data-write mode %02x h",
              (unsigned) BX_VGA_THIS s.attribute_ctrl.address));
          }
        }
      BX_VGA_THIS s.attribute_ctrl.flip_flop = !BX_VGA_THIS s.attribute_ctrl.flip_flop;
      break;

    case 0x03c2: // Miscellaneous Output Register
      BX_VGA_THIS s.misc_output.color_emulation  = (value >> 0) & 0x01;
      BX_VGA_THIS s.misc_output.enable_ram       = (value >> 1) & 0x01;
      BX_VGA_THIS s.misc_output.clock_select     = (value >> 2) & 0x03;
      BX_VGA_THIS s.misc_output.select_high_bank = (value >> 5) & 0x01;
      BX_VGA_THIS s.misc_output.horiz_sync_pol   = (value >> 6) & 0x01;
      BX_VGA_THIS s.misc_output.vert_sync_pol    = (value >> 7) & 0x01;
#if !defined(VGA_TRACE_FEATURE)
        BX_DEBUG(("io write 3c2:"));
        BX_DEBUG(("  color_emulation (attempted) = %u",
                  (value >> 0) & 0x01));
        BX_DEBUG(("  enable_ram = %u",
                  (unsigned) BX_VGA_THIS s.misc_output.enable_ram));
        BX_DEBUG(("  clock_select = %u",
                  (unsigned) BX_VGA_THIS s.misc_output.clock_select));
        BX_DEBUG(("  select_high_bank = %u",
                  (unsigned) BX_VGA_THIS s.misc_output.select_high_bank));
        BX_DEBUG(("  horiz_sync_pol = %u",
                  (unsigned) BX_VGA_THIS s.misc_output.horiz_sync_pol));
        BX_DEBUG(("  vert_sync_pol = %u",
                  (unsigned) BX_VGA_THIS s.misc_output.vert_sync_pol));
#endif
      break;

    case 0x03c3: // VGA enable
      // bit0: enables VGA display if set
#if !defined(VGA_TRACE_FEATURE)
      BX_DEBUG(("io write 3c3: (ignoring) VGA enable = %u",
                  (unsigned) (value & 0x01) ));
#endif
      break;

    case 0x03c4: /* Sequencer Index Register */
      if (value > 4) {
        BX_DEBUG(("io write 3c4: value > 4"));
        }
      BX_VGA_THIS s.sequencer.index = value;
      break;

    case 0x03c5: /* Sequencer Registers 00..04 */
      switch (BX_VGA_THIS s.sequencer.index) {
        case 0: /* sequencer: reset */
#if !defined(VGA_TRACE_FEATURE)
          BX_DEBUG(("write 0x3c5: sequencer reset: value=0x%02x",
                      (unsigned) value));
#endif
          if (BX_VGA_THIS s.sequencer.reset1 && ((value & 0x01) == 0)) {
            BX_VGA_THIS s.sequencer.char_map_select = 0;
            BX_VGA_THIS s.charmap_address = 0;
            bx_gui->set_text_charmap(
              & BX_VGA_THIS s.vga_memory[0x20000 + BX_VGA_THIS s.charmap_address]);
            BX_VGA_THIS s.vga_mem_updated = 1;
          }
          BX_VGA_THIS s.sequencer.reset1 = (value >> 0) & 0x01;
          BX_VGA_THIS s.sequencer.reset2 = (value >> 1) & 0x01;
          break;
        case 1: /* sequencer: clocking mode */
#if !defined(VGA_TRACE_FEATURE)
          BX_DEBUG(("io write 3c5=%02x: clocking mode reg: ignoring",
                      (unsigned) value));
#endif
          BX_VGA_THIS s.sequencer.reg1 = value & 0x3f;
          BX_VGA_THIS s.x_dotclockdiv2 = ((value & 0x08) > 0);
          break;
        case 2: /* sequencer: map mask register */
          BX_VGA_THIS s.sequencer.map_mask = (value & 0x0f);
          for (i=0; i<4; i++)
            BX_VGA_THIS s.sequencer.map_mask_bit[i] = (value >> i) & 0x01;
          break;
        case 3: /* sequencer: character map select register */
          BX_VGA_THIS s.sequencer.char_map_select = value;
          charmap1 = value & 0x13;
          if (charmap1 > 3) charmap1 = (charmap1 & 3) + 4;
          charmap2 = (value & 0x2C) >> 2;
          if (charmap2 > 3) charmap2 = (charmap2 & 3) + 4;
	  if (BX_VGA_THIS s.CRTC.reg[0x09] > 0) {
            BX_VGA_THIS s.charmap_address = (charmap1 << 13);
            bx_gui->set_text_charmap(
              & BX_VGA_THIS s.vga_memory[0x20000 + BX_VGA_THIS s.charmap_address]);
            BX_VGA_THIS s.vga_mem_updated = 1;
            }
          if (charmap2 != charmap1)
            BX_INFO(("char map select: #2=%d (unused)", charmap2));
          break;
        case 4: /* sequencer: memory mode register */
          BX_VGA_THIS s.sequencer.extended_mem   = (value >> 1) & 0x01;
          BX_VGA_THIS s.sequencer.odd_even       = (value >> 2) & 0x01;
          BX_VGA_THIS s.sequencer.chain_four     = (value >> 3) & 0x01;

#if !defined(VGA_TRACE_FEATURE)
          BX_DEBUG(("io write 3c5: index 4:"));
          BX_DEBUG(("  extended_mem %u",
              (unsigned) BX_VGA_THIS s.sequencer.extended_mem));
          BX_DEBUG(("  odd_even %u",
              (unsigned) BX_VGA_THIS s.sequencer.odd_even));
          BX_DEBUG(("  chain_four %u",
              (unsigned) BX_VGA_THIS s.sequencer.chain_four));
#endif
          break;
        default:
          BX_DEBUG(("io write 3c5: index %u unhandled",
            (unsigned) BX_VGA_THIS s.sequencer.index));
        }
      break;

    case 0x03c6: /* PEL mask */
      BX_VGA_THIS s.pel.mask = value;
      if (BX_VGA_THIS s.pel.mask != 0xff)
        BX_DEBUG(("io write 3c6: PEL mask=0x%02x != 0xFF", value));
      // BX_VGA_THIS s.pel.mask should be and'd with final value before
      // indexing into color register BX_VGA_THIS s.pel.data[]
      break;

    case 0x03c7: // PEL address, read mode
      BX_VGA_THIS s.pel.read_data_register = value;
      BX_VGA_THIS s.pel.read_data_cycle    = 0;
      BX_VGA_THIS s.pel.dac_state = 0x03;
      break;

    case 0x03c8: /* PEL address write mode */
      BX_VGA_THIS s.pel.write_data_register = value;
      BX_VGA_THIS s.pel.write_data_cycle    = 0;
      BX_VGA_THIS s.pel.dac_state = 0x00;
      break;

    case 0x03c9: /* PEL Data Register, colors 00..FF */
      switch (BX_VGA_THIS s.pel.write_data_cycle) {
        case 0:
          BX_VGA_THIS s.pel.data[BX_VGA_THIS s.pel.write_data_register].red = value;
          break;
        case 1:
          BX_VGA_THIS s.pel.data[BX_VGA_THIS s.pel.write_data_register].green = value;
          break;
        case 2:
          BX_VGA_THIS s.pel.data[BX_VGA_THIS s.pel.write_data_register].blue = value;

          needs_update |= bx_gui->palette_change(BX_VGA_THIS s.pel.write_data_register,
            BX_VGA_THIS s.pel.data[BX_VGA_THIS s.pel.write_data_register].red<<2,
            BX_VGA_THIS s.pel.data[BX_VGA_THIS s.pel.write_data_register].green<<2,
            BX_VGA_THIS s.pel.data[BX_VGA_THIS s.pel.write_data_register].blue<<2);
          break;
        }

      BX_VGA_THIS s.pel.write_data_cycle++;
      if (BX_VGA_THIS s.pel.write_data_cycle >= 3) {
        //BX_INFO(("BX_VGA_THIS s.pel.data[%u] {r=%u, g=%u, b=%u}",
        //  (unsigned) BX_VGA_THIS s.pel.write_data_register,
        //  (unsigned) BX_VGA_THIS s.pel.data[BX_VGA_THIS s.pel.write_data_register].red,
        //  (unsigned) BX_VGA_THIS s.pel.data[BX_VGA_THIS s.pel.write_data_register].green,
        //  (unsigned) BX_VGA_THIS s.pel.data[BX_VGA_THIS s.pel.write_data_register].blue);
        BX_VGA_THIS s.pel.write_data_cycle = 0;
        BX_VGA_THIS s.pel.write_data_register++;
        }
      break;

    case 0x03ca: /* Graphics 2 Position (EGA) */
      // ignore, EGA only???
      break;

    case 0x03cc: /* Graphics 1 Position (EGA) */
      // ignore, EGA only???
      break;

    case 0x03ce: /* Graphics Controller Index Register */
      if (value > 0x08) /* ??? */
        BX_DEBUG(("io write: 3ce: value > 8"));
      BX_VGA_THIS s.graphics_ctrl.index = value;
      break;

    case 0x03cd: /* ??? */
      BX_DEBUG(("io write to 03cd = %02x", (unsigned) value));
      break;

    case 0x03cf: /* Graphics Controller Registers 00..08 */
      switch (BX_VGA_THIS s.graphics_ctrl.index) {
        case 0: /* Set/Reset */
          BX_VGA_THIS s.graphics_ctrl.set_reset = value & 0x0f;
          break;
        case 1: /* Enable Set/Reset */
          BX_VGA_THIS s.graphics_ctrl.enable_set_reset = value & 0x0f;
          break;
        case 2: /* Color Compare */
          BX_VGA_THIS s.graphics_ctrl.color_compare = value & 0x0f;
          break;
        case 3: /* Data Rotate */
          BX_VGA_THIS s.graphics_ctrl.data_rotate = value & 0x07;
          /* ??? is this bits 3..4 or 4..5 */
          BX_VGA_THIS s.graphics_ctrl.raster_op    = (value >> 3) & 0x03; /* ??? */
          break;
        case 4: /* Read Map Select */
          BX_VGA_THIS s.graphics_ctrl.read_map_select = value & 0x03;
#if !defined(VGA_TRACE_FEATURE)
          BX_DEBUG(("io write to 03cf = %02x (RMS)", (unsigned) value));
#endif
          break;
        case 5: /* Mode */
          BX_VGA_THIS s.graphics_ctrl.write_mode        = value & 0x03;
          BX_VGA_THIS s.graphics_ctrl.read_mode         = (value >> 3) & 0x01;
          BX_VGA_THIS s.graphics_ctrl.odd_even   = (value >> 4) & 0x01;
          BX_VGA_THIS s.graphics_ctrl.shift_reg         = (value >> 5) & 0x03;

          if (BX_VGA_THIS s.graphics_ctrl.odd_even)
            BX_DEBUG(("io write: 3cf: reg 05: value = %02xh",
              (unsigned) value));
          if (BX_VGA_THIS s.graphics_ctrl.shift_reg)
            BX_DEBUG(("io write: 3cf: reg 05: value = %02xh",
              (unsigned) value));
          break;
        case 6: /* Miscellaneous */
          prev_graphics_alpha = BX_VGA_THIS s.graphics_ctrl.graphics_alpha;
          prev_chain_odd_even = BX_VGA_THIS s.graphics_ctrl.chain_odd_even;
          prev_memory_mapping = BX_VGA_THIS s.graphics_ctrl.memory_mapping;

          BX_VGA_THIS s.graphics_ctrl.graphics_alpha = value & 0x01;
          BX_VGA_THIS s.graphics_ctrl.chain_odd_even = (value >> 1) & 0x01;
          BX_VGA_THIS s.graphics_ctrl.memory_mapping = (value >> 2) & 0x03;
#if !defined(VGA_TRACE_FEATURE)
          BX_DEBUG(("memory_mapping set to %u",
              (unsigned) BX_VGA_THIS s.graphics_ctrl.memory_mapping));
          BX_DEBUG(("graphics mode set to %u",
              (unsigned) BX_VGA_THIS s.graphics_ctrl.graphics_alpha));
          BX_DEBUG(("odd_even mode set to %u",
              (unsigned) BX_VGA_THIS s.graphics_ctrl.odd_even));
          BX_DEBUG(("io write: 3cf: reg 06: value = %02xh",
                (unsigned) value));
#endif
          if (prev_memory_mapping != BX_VGA_THIS s.graphics_ctrl.memory_mapping)
            needs_update = 1;
          if (prev_graphics_alpha != BX_VGA_THIS s.graphics_ctrl.graphics_alpha) {
            needs_update = 1;
            old_iHeight = 0;
          }
          break;
        case 7: /* Color Don't Care */
          BX_VGA_THIS s.graphics_ctrl.color_dont_care = value & 0x0f;
          break;
        case 8: /* Bit Mask */
          BX_VGA_THIS s.graphics_ctrl.bitmask = value;
          break;
        default:
          /* ??? */
          BX_DEBUG(("io write: 3cf: index %u unhandled",
            (unsigned) BX_VGA_THIS s.graphics_ctrl.index));
        }
      break;

    case 0x03d4: /* CRTC Index Register (color emulation modes) */
      BX_VGA_THIS s.CRTC.address = value & 0x7f;
      if (BX_VGA_THIS s.CRTC.address > 0x18)
        BX_DEBUG(("write: invalid CRTC register 0x%02x selected",
          (unsigned) BX_VGA_THIS s.CRTC.address));
      break;

    case 0x03d5: /* CRTC Registers (color emulation modes) */
      if (BX_VGA_THIS s.CRTC.address > 0x18) {
        BX_DEBUG(("write: invalid CRTC register 0x%02x ignored",
          (unsigned) BX_VGA_THIS s.CRTC.address));
        return;
      }
      if (value != BX_VGA_THIS s.CRTC.reg[BX_VGA_THIS s.CRTC.address]) {
        BX_VGA_THIS s.CRTC.reg[BX_VGA_THIS s.CRTC.address] = value;
        switch (BX_VGA_THIS s.CRTC.address) {
          case 0x07:
            BX_VGA_THIS s.vertical_display_end &= 0xff;
            if (BX_VGA_THIS s.CRTC.reg[0x07] & 0x02) BX_VGA_THIS s.vertical_display_end |= 0x100;
            if (BX_VGA_THIS s.CRTC.reg[0x07] & 0x40) BX_VGA_THIS s.vertical_display_end |= 0x200;
            BX_VGA_THIS s.line_compare &= 0x2ff;
            if (BX_VGA_THIS s.CRTC.reg[0x07] & 0x10) BX_VGA_THIS s.line_compare |= 0x100;
            needs_update = 1;
            break;
          case 0x08:
            // Vertical pel panning change
            needs_update = 1;
            break;
          case 0x09:
            BX_VGA_THIS s.y_doublescan = ((value & 0x9f) > 0);
            BX_VGA_THIS s.line_compare &= 0x1ff;
            if (BX_VGA_THIS s.CRTC.reg[0x09] & 0x40) BX_VGA_THIS s.line_compare |= 0x200;
            needs_update = 1;
            break;
          case 0x0A:
          case 0x0B:
          case 0x0E:
          case 0x0F:
            // Cursor size / location change
            BX_VGA_THIS s.vga_mem_updated = 1;
            break;
          case 0x0C:
          case 0x0D:
            // Start address change
            if (BX_VGA_THIS s.graphics_ctrl.graphics_alpha) {
              needs_update = 1;
            } else {
              BX_VGA_THIS s.vga_mem_updated = 1;
            }
            break;
          case 0x12:
            BX_VGA_THIS s.vertical_display_end &= 0x300;
            BX_VGA_THIS s.vertical_display_end |= BX_VGA_THIS s.CRTC.reg[0x12];
            break;
          case 0x13:
          case 0x14:
          case 0x17:
            // Line offset change
            BX_VGA_THIS s.line_offset = BX_VGA_THIS s.CRTC.reg[0x13] << 1;
            if (BX_VGA_THIS s.CRTC.reg[0x14] & 0x40) BX_VGA_THIS s.line_offset <<= 2;
            else if ((BX_VGA_THIS s.CRTC.reg[0x17] & 0x40) == 0) BX_VGA_THIS s.line_offset <<= 1;
            needs_update = 1;
            break;
          case 0x18:
            BX_VGA_THIS s.line_compare &= 0x300;
            BX_VGA_THIS s.line_compare |= BX_VGA_THIS s.CRTC.reg[0x18];
            needs_update = 1;
            break;
        }

      }
      break;

    case 0x03da: /* Feature Control (color emulation modes) */
      BX_DEBUG(("io write: 3da: ignoring: feature ctrl & vert sync"));
      break;

    case 0x03c1: /* */
    default:
      BX_ERROR(("unsupported io write to port 0x%04x, val=0x%02x",
        (unsigned) address, (unsigned) value));
    }
  if (needs_update) {
    BX_VGA_THIS s.vga_mem_updated = 1;
    // Mark all video as updated so the changes will go through
    if ((BX_VGA_THIS s.graphics_ctrl.graphics_alpha)
#if BX_SUPPORT_VBE  
        || (BX_VGA_THIS s.vbe_enabled)
#endif
       ) {
      for (unsigned xti = 0; xti < BX_NUM_X_TILES; xti++) {
        for (unsigned yti = 0; yti < BX_NUM_Y_TILES; yti++) {
          SET_TILE_UPDATED (xti, yti, 1);
        }
      }
    } else {
      memset(BX_VGA_THIS s.text_snapshot, 0,
             sizeof(BX_VGA_THIS s.text_snapshot));
    }
  }
}

void 
bx_vga_c::set_update_interval (unsigned interval)
{
  BX_INFO (("Changing timer interval to %d\n", interval));
  BX_VGA_THIS timer_handler (theVga);
  bx_pc_system.activate_timer (BX_VGA_THIS timer_id, interval, 1);
}

  void
bx_vga_c::trigger_timer(void *this_ptr)
{
  timer_handler(this_ptr);
}

  void
bx_vga_c::timer_handler(void *this_ptr)
{
#if !BX_USE_VGA_SMF
  
  bx_vga_c *class_ptr = (bx_vga_c *) this_ptr;

  class_ptr->timer();
}

  void
bx_vga_c::timer(void)
{
#else
  UNUSED(this_ptr);
#endif

  update();
  bx_gui->flush();

}


  void
bx_vga_c::update(void)
{
  unsigned iHeight, iWidth;

  /* no screen update necessary */
  if (BX_VGA_THIS s.vga_mem_updated==0)
    return;

  /* skip screen update when the sequencer is in reset mode or video is disabled */
  if (!BX_VGA_THIS s.sequencer.reset1 || !BX_VGA_THIS s.sequencer.reset2
      || !BX_VGA_THIS s.attribute_ctrl.video_enabled)
    return;

  /* skip screen update if the vertical retrace is in progress
     (using 72 Hz vertical frequency) */
  if ((bx_pc_system.time_usec() % 13888) < 70)
    return;

#if BX_SUPPORT_VBE  
  if ((BX_VGA_THIS s.vbe_enabled) && (BX_VGA_THIS s.vbe_bpp != VBE_DISPI_BPP_4))
  {
    // specific VBE code display update code
    // this is partly copied/modified from the 320x200x8 update more below
    unsigned xc, yc, xti, yti;
    unsigned r;
    unsigned long pixely, bmp_ofs_y, tile_ofs_y;

    if (BX_VGA_THIS s.vbe_bpp == VBE_DISPI_BPP_32)
    {
      Bit32u *vidmem = (Bit32u *)(&BX_VGA_THIS s.vbe_memory[BX_VGA_THIS s.vbe_virtual_start]);
      Bit32u *tile   = (Bit32u *)(BX_VGA_THIS s.tile);
      Bit16u width  =  BX_VGA_THIS s.vbe_virtual_xres;

      Bit32u *vidptr, *tileptr;

      iWidth=BX_VGA_THIS s.vbe_xres;
      iHeight=BX_VGA_THIS s.vbe_yres;

      for (yc=0, yti = 0; yc<iHeight; yc+=Y_TILESIZE, yti++)
      {
        for (xc=0, xti = 0; xc<iWidth; xc+=X_TILESIZE, xti++)
        {
          if (GET_TILE_UPDATED (xti, yti))
          {
            for (r=0; r<Y_TILESIZE; r++)
            {
              pixely    = yc + r;
              // calc offsets into video and tile memory
              bmp_ofs_y = pixely*width;
              tile_ofs_y = r*X_TILESIZE;
              // get offsets so that we do less calc in the inner loop
              vidptr = &vidmem[bmp_ofs_y+xc];
              tileptr = &tile[tile_ofs_y];
              memmove(tileptr, vidptr, X_TILESIZE<<2);
            }
            SET_TILE_UPDATED (xti, yti, 0);
            bx_gui->graphics_tile_update(BX_VGA_THIS s.tile, xc, yc);
          }
        }
      }
    }
    else if (BX_VGA_THIS s.vbe_bpp == VBE_DISPI_BPP_24)
    {
      Bit8u *vidmem = &BX_VGA_THIS s.vbe_memory[BX_VGA_THIS s.vbe_virtual_start];
      Bit8u *tile   =  BX_VGA_THIS s.tile;
      Bit16u width  =  BX_VGA_THIS s.vbe_virtual_xres*3;

      Bit8u *vidptr, *tileptr;

      iWidth=BX_VGA_THIS s.vbe_xres;
      iHeight=BX_VGA_THIS s.vbe_yres;

      for (yc=0, yti = 0; yc<iHeight; yc+=Y_TILESIZE, yti++)
      {
        for (xc=0, xti = 0; xc<iWidth; xc+=X_TILESIZE, xti++)
        {
          if (GET_TILE_UPDATED (xti, yti))
          {
            for (r=0; r<Y_TILESIZE; r++)
            {
              pixely    = yc + r;
              // calc offsets into video and tile memory
              bmp_ofs_y = pixely*width;
              tile_ofs_y = r*X_TILESIZE*3;
              // get offsets so that we do less calc in the inner loop
              vidptr = &vidmem[bmp_ofs_y+xc*3];
              tileptr = &tile[tile_ofs_y];
              memmove(tileptr, vidptr, X_TILESIZE*3);
            }
            SET_TILE_UPDATED (xti, yti, 0);
            bx_gui->graphics_tile_update(BX_VGA_THIS s.tile, xc, yc);
          }
        }
      }
    }
    else if ((BX_VGA_THIS s.vbe_bpp == VBE_DISPI_BPP_15) ||
             (BX_VGA_THIS s.vbe_bpp == VBE_DISPI_BPP_16))
    {
      Bit16u *vidmem = (Bit16u *)(&BX_VGA_THIS s.vbe_memory[BX_VGA_THIS s.vbe_virtual_start]);
      Bit16u *tile   = (Bit16u *)(BX_VGA_THIS s.tile);
      Bit16u width  =  BX_VGA_THIS s.vbe_virtual_xres;

      Bit16u *vidptr, *tileptr;

      iWidth=BX_VGA_THIS s.vbe_xres;
      iHeight=BX_VGA_THIS s.vbe_yres;

      for (yc=0, yti = 0; yc<iHeight; yc+=Y_TILESIZE, yti++)
      {
        for (xc=0, xti = 0; xc<iWidth; xc+=X_TILESIZE, xti++)
        {
          if (GET_TILE_UPDATED (xti, yti))
          {
            for (r=0; r<Y_TILESIZE; r++)
            {
              pixely    = yc + r;
              // calc offsets into video and tile memory
              bmp_ofs_y = pixely*width;
              tile_ofs_y = r*X_TILESIZE;
              // get offsets so that we do less calc in the inner loop
              vidptr = &vidmem[bmp_ofs_y+xc];
              tileptr = &tile[tile_ofs_y];
              memmove(tileptr, vidptr, X_TILESIZE<<1);
            }
            SET_TILE_UPDATED (xti, yti, 0);
            bx_gui->graphics_tile_update(BX_VGA_THIS s.tile, xc, yc);
          }
        }
      }
    }
    else /* Update 8bpp mode */
    {
      Bit8u *vidmem = &BX_VGA_THIS s.vbe_memory[BX_VGA_THIS s.vbe_virtual_start]; 
      Bit8u *tile   =  BX_VGA_THIS s.tile;
      Bit16u width  =  BX_VGA_THIS s.vbe_virtual_xres;

      Bit8u *vidptr, *tileptr;

      iWidth=BX_VGA_THIS s.vbe_xres;
      iHeight=BX_VGA_THIS s.vbe_yres;

      for (yc=0, yti = 0; yc<iHeight; yc+=Y_TILESIZE, yti++)
      {
        for (xc=0, xti = 0; xc<iWidth; xc+=X_TILESIZE, xti++)
        {
          // If the tile has not been updated, copy it into the tile buffer for update  
          if (GET_TILE_UPDATED (xti, yti)) {
            for (r=0; r<Y_TILESIZE; r++) {
              // actual video y coord is tile_y + y
              pixely = yc + r;
              // calc offsets into video and tile memory
              bmp_ofs_y = pixely*width;
              tile_ofs_y = r*X_TILESIZE;
              // get offsets so that we do less calc in the inner loop
              vidptr = &vidmem[bmp_ofs_y+xc];
              tileptr = &tile[tile_ofs_y];
              memmove(tileptr, vidptr, X_TILESIZE);
            }
            SET_TILE_UPDATED (xti, yti, 0);
            bx_gui->graphics_tile_update(BX_VGA_THIS s.tile, xc, yc);
          }
        }
      }
    }

    old_iWidth = iWidth;
    old_iHeight = iHeight;
    BX_VGA_THIS s.vga_mem_updated = 0;
    // after a vbe display update, don't try to do any 'normal vga' updates anymore
    return;
  }
#endif  
  // fields that effect the way video memory is serialized into screen output:
  // GRAPHICS CONTROLLER:
  //   BX_VGA_THIS s.graphics_ctrl.shift_reg:
  //     0: output data in standard VGA format or CGA-compatible 640x200 2 color
  //        graphics mode (mode 6)
  //     1: output data in CGA-compatible 320x200 4 color graphics mode
  //        (modes 4 & 5)
  //     2: output data 8 bits at a time from the 4 bit planes
  //        (mode 13 and variants like modeX)

  // if (BX_VGA_THIS s.vga_mem_updated==0 || BX_VGA_THIS s.attribute_ctrl.video_enabled == 0)

  if (BX_VGA_THIS s.graphics_ctrl.graphics_alpha) {
    Bit8u color;
    unsigned bit_no, r, c, x, y;
    unsigned long byte_offset, start_addr;
    unsigned xc, yc, xti, yti;

    start_addr = (BX_VGA_THIS s.CRTC.reg[0x0c] << 8) | BX_VGA_THIS s.CRTC.reg[0x0d];

//BX_DEBUG(("update: shiftreg=%u, chain4=%u, mapping=%u",
//  (unsigned) BX_VGA_THIS s.graphics_ctrl.shift_reg,
//  (unsigned) BX_VGA_THIS s.sequencer.chain_four,
//  (unsigned) BX_VGA_THIS s.graphics_ctrl.memory_mapping);

    switch ( BX_VGA_THIS s.graphics_ctrl.shift_reg ) {

      case 0:
        Bit8u attribute, palette_reg_val, DAC_regno;
        unsigned long line_compare;

        determine_screen_dimensions(&iHeight, &iWidth);
        if( (iWidth != old_iWidth) || (iHeight != old_iHeight) ) {
          bx_gui->dimension_update(iWidth, iHeight);
          old_iWidth = iWidth;
          old_iHeight = iHeight;
        }

        if (BX_VGA_THIS s.graphics_ctrl.memory_mapping == 3) { // CGA 640x200x2

          for (yc=0, yti=0; yc<iHeight; yc+=Y_TILESIZE, yti++) {
            for (xc=0, xti=0; xc<iWidth; xc+=X_TILESIZE, xti++) {
              if (GET_TILE_UPDATED (xti, yti)) {
                for (r=0; r<Y_TILESIZE; r++) {
                  y = yc + r;
                  if (BX_VGA_THIS s.y_doublescan) y >>= 1;
                  for (c=0; c<X_TILESIZE; c++) {

                    x = xc + c;
                    /* 0 or 0x2000 */
                    byte_offset = start_addr + ((y & 1) << 13);
                    /* to the start of the line */
                    byte_offset += (320 / 4) * (y / 2);
                    /* to the byte start */
                    byte_offset += (x / 8);

                    bit_no = 7 - (x % 8);
                    palette_reg_val = (((BX_VGA_THIS s.vga_memory[byte_offset]) >> bit_no) & 1);
                    DAC_regno = BX_VGA_THIS s.attribute_ctrl.palette_reg[palette_reg_val];
                    BX_VGA_THIS s.tile[r*X_TILESIZE + c] = DAC_regno;
                  }
                }
                SET_TILE_UPDATED (xti, yti, 0);
                bx_gui->graphics_tile_update(BX_VGA_THIS s.tile, xc, yc);
              }
            }
          }
        } else { // output data in serial fashion with each display plane
                 // output on its associated serial output.  Standard EGA/VGA format

          line_compare = BX_VGA_THIS s.line_compare;
          if (BX_VGA_THIS s.y_doublescan) line_compare >>= 1;

          for (yc=0, yti=0; yc<iHeight; yc+=Y_TILESIZE, yti++) {
            for (xc=0, xti=0; xc<iWidth; xc+=X_TILESIZE, xti++) {
              if (GET_TILE_UPDATED (xti, yti)) {
                for (r=0; r<Y_TILESIZE; r++) {
                  y = yc + r;
                  if (BX_VGA_THIS s.y_doublescan) y >>= 1;
                  for (c=0; c<X_TILESIZE; c++) {
                    x = xc + c;
                    if (BX_VGA_THIS s.x_dotclockdiv2) x >>= 1;
                    bit_no = 7 - (x % 8);
                    if (y > line_compare) {
                      byte_offset = x / 8 +
                        ((y - line_compare - 1) * BX_VGA_THIS s.line_offset);
                    } else {
                      byte_offset = start_addr + x / 8 +
                        (y * BX_VGA_THIS s.line_offset);
                    }
                    attribute =
                      (((BX_VGA_THIS s.vga_memory[0*65536 + byte_offset] >> bit_no) & 0x01) << 0) |
                      (((BX_VGA_THIS s.vga_memory[1*65536 + byte_offset] >> bit_no) & 0x01) << 1) |
                      (((BX_VGA_THIS s.vga_memory[2*65536 + byte_offset] >> bit_no) & 0x01) << 2) |
                      (((BX_VGA_THIS s.vga_memory[3*65536 + byte_offset] >> bit_no) & 0x01) << 3);

                    attribute &= BX_VGA_THIS s.attribute_ctrl.color_plane_enable;
                    // undocumented feature ???: colors 0..7 high intensity, colors 8..15 blinking
                    // using low/high intensity. Blinking is not implemented yet.
                    if (BX_VGA_THIS s.attribute_ctrl.mode_ctrl.blink_intensity) attribute ^= 0x08;
                    palette_reg_val = BX_VGA_THIS s.attribute_ctrl.palette_reg[attribute];
                    if (BX_VGA_THIS s.attribute_ctrl.mode_ctrl.internal_palette_size) {
                      // use 4 lower bits from palette register
                      // use 4 higher bits from color select register
                      // 16 banks of 16-color registers
                      DAC_regno = (palette_reg_val & 0x0f) |
                                  (BX_VGA_THIS s.attribute_ctrl.color_select << 4);
                      }
                    else {
                      // use 6 lower bits from palette register
                      // use 2 higher bits from color select register
                      // 4 banks of 64-color registers
                      DAC_regno = (palette_reg_val & 0x3f) |
                                  ((BX_VGA_THIS s.attribute_ctrl.color_select & 0x0c) << 4);
                      }
                    // DAC_regno &= video DAC mask register ???

                    BX_VGA_THIS s.tile[r*X_TILESIZE + c] = DAC_regno;
                    }
                  }
                SET_TILE_UPDATED (xti, yti, 0);
                bx_gui->graphics_tile_update(BX_VGA_THIS s.tile, xc, yc);
                }
              }
            }
          }
        break; // case 0

      case 1: // output the data in a CGA-compatible 320x200 4 color graphics
              // mode.  (modes 4 & 5)

        /* CGA 320x200x4 start */
        determine_screen_dimensions(&iHeight, &iWidth);
	if( (iWidth != old_iWidth) || (iHeight != old_iHeight) )
	{
	  bx_gui->dimension_update(iWidth, iHeight);
	  old_iWidth = iWidth;
	  old_iHeight = iHeight;
	}

        for (yc=0, yti=0; yc<iHeight; yc+=Y_TILESIZE, yti++) {
          for (xc=0, xti=0; xc<iWidth; xc+=X_TILESIZE, xti++) {
            if (GET_TILE_UPDATED (xti, yti)) {
              for (r=0; r<Y_TILESIZE; r++) {
                y = yc + r;
                if (BX_VGA_THIS s.y_doublescan) y >>= 1;
                for (c=0; c<X_TILESIZE; c++) {

                  x = xc + c;
                  if (BX_VGA_THIS s.x_dotclockdiv2) x >>= 1;
                  /* 0 or 0x2000 */
                  byte_offset = start_addr + ((y & 1) << 13);
                  /* to the start of the line */
                  byte_offset += (320 / 4) * (y / 2);
                  /* to the byte start */
                  byte_offset += (x / 4);

                  attribute = 6 - 2*(x % 4);
                  palette_reg_val = (BX_VGA_THIS s.vga_memory[byte_offset]) >> attribute;
                  palette_reg_val &= 3;
                  DAC_regno = BX_VGA_THIS s.attribute_ctrl.palette_reg[palette_reg_val];
                  BX_VGA_THIS s.tile[r*X_TILESIZE + c] = DAC_regno;
                }
              }
              SET_TILE_UPDATED (xti, yti, 0);
              bx_gui->graphics_tile_update(BX_VGA_THIS s.tile, xc, yc);
            }
          }
        }
        /* CGA 320x200x4 end */

        break; // case 1

      case 2: // output the data eight bits at a time from the 4 bit plane
              // (format for VGA mode 13 hex)
        determine_screen_dimensions(&iHeight, &iWidth);

        if ( BX_VGA_THIS s.sequencer.chain_four ) {
          unsigned long pixely, pixelx, plane;
          // bx_vga_dump_status();

          if (BX_VGA_THIS s.misc_output.select_high_bank != 1)
            BX_PANIC(("update: select_high_bank != 1"));

	  if( (iHeight != old_iHeight) || (iWidth != old_iWidth) )
	  {
	    bx_gui->dimension_update(iWidth, iHeight);
	    old_iHeight = iHeight;
	    old_iWidth = iWidth;
	  }

          for (yc=0, yti=0; yc<iHeight; yc+=Y_TILESIZE, yti++) {
            for (xc=0, xti=0; xc<iWidth; xc+=X_TILESIZE, xti++) {
              if (GET_TILE_UPDATED (xti, yti)) {
                for (r=0; r<Y_TILESIZE; r++) {
                  pixely = yc + r;
                  if (BX_VGA_THIS s.y_doublescan) pixely >>= 1;
                  for (c=0; c<X_TILESIZE; c++) {
                    pixelx = (xc + c) >> 1;
                    plane  = (pixelx % 4);
                    byte_offset = start_addr + (plane * 65536) +
                                  (pixely * BX_VGA_THIS s.line_offset) + (pixelx & ~0x03);
                    color = BX_VGA_THIS s.vga_memory[byte_offset];
                    BX_VGA_THIS s.tile[r*X_TILESIZE + c] = color;
                    }
                  }
                SET_TILE_UPDATED (xti, yti, 0);
                bx_gui->graphics_tile_update(BX_VGA_THIS s.tile, xc, yc);
                }
              }
            }
          }

        else { // chain_four == 0, modeX
          unsigned long pixely, pixelx, plane;

	  if( (iWidth != old_iWidth) || (iHeight != old_iHeight) )
	  {
	    bx_gui->dimension_update(iWidth, iHeight);
	    old_iWidth = iWidth;
	    old_iHeight = iHeight;
	  }

          for (yc=0, yti=0; yc<iHeight; yc+=Y_TILESIZE, yti++) {
            for (xc=0, xti=0; xc<iWidth; xc+=X_TILESIZE, xti++) {
              if (GET_TILE_UPDATED (xti, yti)) {
                for (r=0; r<Y_TILESIZE; r++) {
                  pixely = yc + r;
                  if (BX_VGA_THIS s.y_doublescan) pixely >>= 1;
                  for (c=0; c<X_TILESIZE; c++) {
                    pixelx = (xc + c) >> 1;
                    plane  = (pixelx % 4);
                    byte_offset = (plane * 65536) +
                                  (pixely * BX_VGA_THIS s.line_offset)
                                  + (pixelx >> 2);
                    color = BX_VGA_THIS s.vga_memory[start_addr + byte_offset];
                    BX_VGA_THIS s.tile[r*X_TILESIZE + c] = color;
                    }
                  }
                SET_TILE_UPDATED (xti, yti, 0);
                bx_gui->graphics_tile_update(BX_VGA_THIS s.tile, xc, yc);
                }
	      }
	    }
          }
        break; // case 2

      default:
        BX_PANIC(("update: shift_reg == %u", (unsigned)
          BX_VGA_THIS s.graphics_ctrl.shift_reg ));
      }

    BX_VGA_THIS s.vga_mem_updated = 0;
    return;
    }

  else { // text mode
    unsigned long start_address;
    unsigned long cursor_address, cursor_x, cursor_y;
    bx_vga_tminfo_t tm_info;


    tm_info.cs_start = BX_VGA_THIS s.CRTC.reg[0x0a] & 0x3f;
    tm_info.cs_end = BX_VGA_THIS s.CRTC.reg[0x0b] & 0x1f;
    tm_info.line_offset = BX_VGA_THIS s.CRTC.reg[0x13] << 2;
    tm_info.line_compare = BX_VGA_THIS s.line_compare;
    tm_info.h_panning = BX_VGA_THIS s.attribute_ctrl.horiz_pel_panning & 0x0f;
    tm_info.v_panning = BX_VGA_THIS s.CRTC.reg[0x08] & 0x1f;
    tm_info.line_graphics = BX_VGA_THIS s.attribute_ctrl.mode_ctrl.enable_line_graphics;
    if ((BX_VGA_THIS s.sequencer.reg1 & 0x01) == 0) {
      if (tm_info.h_panning == 8)
        tm_info.h_panning = 0;
      else
        tm_info.h_panning++;
    }

    switch (BX_VGA_THIS s.graphics_ctrl.memory_mapping) {
      case 0: // 128K @ A0000
      case 1: // 64K @ A0000
	iWidth = 8*80;	// TODO: should use font size
	iHeight = 16*25;
	if( (iWidth != old_iWidth) || (iHeight != old_iHeight) )
	{
	  bx_gui->dimension_update(iWidth, iHeight, 16, 8);
	  old_iWidth = iWidth;
	  old_iHeight = iHeight;
	}
//BX_DEBUG(("update(): case 2"));
        /* pass old text snapshot & new VGA memory contents */
        start_address = 0x0;
        cursor_address = 2*((BX_VGA_THIS s.CRTC.reg[0x0e] << 8) |
                          BX_VGA_THIS s.CRTC.reg[0x0f]);
        if (cursor_address < start_address) {
          cursor_x = 0xffff;
          cursor_y = 0xffff;
          }
        else {
          cursor_x = ((cursor_address - start_address)/2) % 80;
          cursor_y = ((cursor_address - start_address)/2) / 80;
          }
        bx_gui->text_update(BX_VGA_THIS s.text_snapshot,
                          &BX_VGA_THIS s.vga_memory[start_address],
                           cursor_x, cursor_y, tm_info, 25);
        // screen updated, copy new VGA memory contents into text snapshot
        memcpy(BX_VGA_THIS s.text_snapshot,
              &BX_VGA_THIS s.vga_memory[start_address],
               2*80*25);
        BX_VGA_THIS s.vga_mem_updated = 0;
        break;

      case 2: // B0000 .. B7FFF
	iWidth = 8*80;	// TODO: should use font size
	iHeight = 16*25;
	if( (iWidth != old_iWidth) || (iHeight != old_iHeight) )
	{
	  bx_gui->dimension_update(iWidth, iHeight, 16, 8);
	  old_iWidth = iWidth;
	  old_iHeight = iHeight;
	}
//BX_DEBUG(("update(): case 2"));
        /* pass old text snapshot & new VGA memory contents */
        start_address = 2*((BX_VGA_THIS s.CRTC.reg[12] << 8) + BX_VGA_THIS s.CRTC.reg[13]);
        cursor_address = 2*((BX_VGA_THIS s.CRTC.reg[0x0e] << 8) |
                          BX_VGA_THIS s.CRTC.reg[0x0f]);
        if (cursor_address < start_address) {
          cursor_x = 0xffff;
          cursor_y = 0xffff;
          }
        else {
          cursor_x = ((cursor_address - start_address)/2) % 80;
          cursor_y = ((cursor_address - start_address)/2) / 80;
          }
        bx_gui->text_update(BX_VGA_THIS s.text_snapshot,
                          &BX_VGA_THIS s.vga_memory[start_address],
                           cursor_x, cursor_y, tm_info, 25);
        // screen updated, copy new VGA memory contents into text snapshot
        memcpy(BX_VGA_THIS s.text_snapshot,
              &BX_VGA_THIS s.vga_memory[start_address],
               2*80*25);
        BX_VGA_THIS s.vga_mem_updated = 0;
        break;

      case 3: // B8000 .. BFFFF
        unsigned VDE, MSL, rows, cWidth;

        // Verticle Display End: find out how many lines are displayed
        VDE = BX_VGA_THIS s.vertical_display_end;
        // Maximum Scan Line: height of character cell
        MSL = BX_VGA_THIS s.CRTC.reg[0x09] & 0x1f;
        if (MSL == 0) {
          BX_ERROR(("character height = 1, skipping text update"));
          return;
        }
        if ((MSL == 1) && (BX_VGA_THIS s.CRTC.reg[0x06] == 100)) {
          // emulated CGA graphics mode 160x100x16 colors
          MSL = 3;
          rows = 100;
          cWidth = 8;
          iWidth = cWidth * BX_VGA_THIS s.CRTC.reg[1];
          iHeight = 400;
        } else {
          rows = (VDE+1)/(MSL+1);
          if (rows > BX_MAX_TEXT_LINES)
            BX_PANIC(("text rows>%d: %d",BX_MAX_TEXT_LINES,rows));
          cWidth = ((BX_VGA_THIS s.sequencer.reg1 & 0x01) == 1) ? 8 : 9;
          iWidth = cWidth * (BX_VGA_THIS s.CRTC.reg[1] + 1);
          iHeight = VDE+1;
        }
	if( (iWidth != old_iWidth) || (iHeight != old_iHeight) || (MSL != old_MSL) )
	{
	  bx_gui->dimension_update(iWidth, iHeight, MSL+1, cWidth);
	  old_iWidth = iWidth;
	  old_iHeight = iHeight;
	  old_MSL = MSL;
	}
        // pass old text snapshot & new VGA memory contents
        start_address = 2*((BX_VGA_THIS s.CRTC.reg[12] << 8) + BX_VGA_THIS s.CRTC.reg[13]);
        cursor_address = 2*((BX_VGA_THIS s.CRTC.reg[0x0e] << 8) |
                          BX_VGA_THIS s.CRTC.reg[0x0f]);
        if (cursor_address < start_address) {
          cursor_x = 0xffff;
          cursor_y = 0xffff;
          }
        else {
          cursor_x = ((cursor_address - start_address)/2) % (iWidth/cWidth);
          cursor_y = ((cursor_address - start_address)/2) / (iWidth/cWidth);
          }
        bx_gui->text_update(BX_VGA_THIS s.text_snapshot,
                          &BX_VGA_THIS s.vga_memory[start_address],
                           cursor_x, cursor_y, tm_info, rows);
        // screen updated, copy new VGA memory contents into text snapshot
        memcpy(BX_VGA_THIS s.text_snapshot,
              &BX_VGA_THIS s.vga_memory[start_address],
               2*80*rows);
        BX_VGA_THIS s.vga_mem_updated = 0;
        break;
      default:
        BX_DEBUG(("update(): color text mode: mem map is %u",
                 (unsigned) BX_VGA_THIS s.graphics_ctrl.memory_mapping));
      }
    }
}


  Bit8u
bx_vga_c::mem_read(Bit32u addr)
{
  Bit32u offset;

#if BX_SUPPORT_VBE  
  // if in a vbe enabled mode, read from the vbe_memory
  if ((BX_VGA_THIS s.vbe_enabled) && (BX_VGA_THIS s.vbe_bpp != VBE_DISPI_BPP_4))
  {
        return vbe_mem_read(addr);
  }
#endif  

#if defined(VGA_TRACE_FEATURE)
//	BX_DEBUG(("8-bit memory read from %08x", addr));
#endif

#ifdef __OS2__

#if BX_PLUGINS
#error Fix the code for plugins
#endif

  if ( bx_options.videomode == BX_VIDEO_DIRECT )
     {
     char value;

     value = devices->mem->video[addr-0xA0000];

     return value;
     }
#endif

  switch (BX_VGA_THIS s.graphics_ctrl.memory_mapping) {
    case 1: // 0xA0000 .. 0xAFFFF
      if (addr > 0xAFFFF) return 0xff;
      offset = addr - 0xA0000;
      break;
    case 2: // 0xB0000 .. 0xB7FFF
      if ((addr < 0xB0000) || (addr > 0xB7FFF)) return 0xff;
      return BX_VGA_THIS s.vga_memory[addr - 0xB0000];
      break;
    case 3: // 0xB8000 .. 0xBFFFF
      if (addr < 0xB8000) return 0xff;
      return BX_VGA_THIS s.vga_memory[addr - 0xB8000];
      break;
    default: // 0xA0000 .. 0xBFFFF
      return BX_VGA_THIS s.vga_memory[addr - 0xA0000];
    }

  // addr between 0xA0000 and 0xAFFFF
  if ( BX_VGA_THIS s.sequencer.chain_four ) {

    // Mode 13h: 320 x 200 256 color mode: chained pixel representation
    return BX_VGA_THIS s.vga_memory[(offset & ~0x03) + (offset % 4)*65536];
    }

  /* addr between 0xA0000 and 0xAFFFF */
  switch (BX_VGA_THIS s.graphics_ctrl.read_mode) {
    case 0: /* read mode 0 */
      BX_VGA_THIS s.graphics_ctrl.latch[0] = BX_VGA_THIS s.vga_memory[          offset];
      BX_VGA_THIS s.graphics_ctrl.latch[1] = BX_VGA_THIS s.vga_memory[1*65536 + offset];
      BX_VGA_THIS s.graphics_ctrl.latch[2] = BX_VGA_THIS s.vga_memory[2*65536 + offset];
      BX_VGA_THIS s.graphics_ctrl.latch[3] = BX_VGA_THIS s.vga_memory[3*65536 + offset];
      return(BX_VGA_THIS s.graphics_ctrl.latch[BX_VGA_THIS s.graphics_ctrl.read_map_select]);
      break;

    case 1: /* read mode 1 */
      {
      Bit8u color_compare, color_dont_care;
      Bit8u latch0, latch1, latch2, latch3, retval;

      color_compare   = BX_VGA_THIS s.graphics_ctrl.color_compare & 0x0f;
      color_dont_care = BX_VGA_THIS s.graphics_ctrl.color_dont_care & 0x0f;
      latch0 = BX_VGA_THIS s.graphics_ctrl.latch[0] = BX_VGA_THIS s.vga_memory[          offset];
      latch1 = BX_VGA_THIS s.graphics_ctrl.latch[1] = BX_VGA_THIS s.vga_memory[1*65536 + offset];
      latch2 = BX_VGA_THIS s.graphics_ctrl.latch[2] = BX_VGA_THIS s.vga_memory[2*65536 + offset];
      latch3 = BX_VGA_THIS s.graphics_ctrl.latch[3] = BX_VGA_THIS s.vga_memory[3*65536 + offset];

      latch0 ^= ccdat[color_compare][0];
      latch1 ^= ccdat[color_compare][1];
      latch2 ^= ccdat[color_compare][2];
      latch3 ^= ccdat[color_compare][3];

      latch0 &= ccdat[color_dont_care][0];
      latch1 &= ccdat[color_dont_care][1];
      latch2 &= ccdat[color_dont_care][2];
      latch3 &= ccdat[color_dont_care][3];

      retval = ~(latch0 | latch1 | latch2 | latch3);

      return retval;
      }
      break;
    default:
      return 0;
    }
}

  void
bx_vga_c::mem_write(Bit32u addr, Bit8u value)
{
  Bit32u offset;
  Bit8u new_val[4];
  unsigned start_addr;

#if BX_SUPPORT_VBE
  // if in a vbe enabled mode, write to the vbe_memory
  if ((BX_VGA_THIS s.vbe_enabled) && (BX_VGA_THIS s.vbe_bpp != VBE_DISPI_BPP_4))
  {
        vbe_mem_write(addr,value);
        return;
  }
#endif

#if defined(VGA_TRACE_FEATURE)
//	BX_DEBUG(("8-bit memory write to %08x = %02x", addr, value));
#endif

#ifdef __OS2__

#if BX_PLUGINS
#error Fix the code for plugins
#endif

  if ( bx_options.videomode == BX_VIDEO_DIRECT )
    {
    devices->mem->video[addr-0xA0000] = value;

    return;
    }
#endif

  switch (BX_VGA_THIS s.graphics_ctrl.memory_mapping) {
    case 1: // 0xA0000 .. 0xAFFFF
      if (addr > 0xAFFFF) return;
      offset = addr - 0xA0000;
      break;
    case 2: // 0xB0000 .. 0xB7FFF
      if ((addr < 0xB0000) || (addr > 0xB7FFF)) return;
      offset = addr - 0xB0000;
      break;
    case 3: // 0xB8000 .. 0xBFFFF
      if (addr < 0xB8000) return;
      offset = addr - 0xB8000;
      break;
    default: // 0xA0000 .. 0xBFFFF
      offset = addr - 0xA0000;
    }

  start_addr = (BX_VGA_THIS s.CRTC.reg[0x0c] << 8) | BX_VGA_THIS s.CRTC.reg[0x0d];

  if (BX_VGA_THIS s.graphics_ctrl.graphics_alpha) {
    if (BX_VGA_THIS s.graphics_ctrl.memory_mapping == 3) { // 0xB8000 .. 0xBFFFF
      unsigned x_tileno, x_tileno2, y_tileno;

      /* CGA 320x200x4 / 640x200x2 start */
      BX_VGA_THIS s.vga_memory[offset] = value;
      offset -= start_addr;
      if (offset>=0x2000) {
        y_tileno = offset - 0x2000;
        y_tileno /= (320/4);
        y_tileno <<= 1; //2 * y_tileno;
        y_tileno++;
        x_tileno = (offset - 0x2000) % (320/4);
        x_tileno <<= 2; //*= 4;
      } else {
        y_tileno = offset / (320/4);
        y_tileno <<= 1; //2 * y_tileno;
        x_tileno = offset % (320/4);
        x_tileno <<= 2; //*=4;
      }
      x_tileno2=x_tileno;
      if (BX_VGA_THIS s.graphics_ctrl.shift_reg==0) {
        x_tileno*=2;
        x_tileno2+=7;
      } else {
        x_tileno2+=3;
      }
      if (BX_VGA_THIS s.x_dotclockdiv2) {
        x_tileno/=(X_TILESIZE/2);
        x_tileno2/=(X_TILESIZE/2);
      } else {
        x_tileno/=X_TILESIZE;
        x_tileno2/=X_TILESIZE;
      }
      if (BX_VGA_THIS s.y_doublescan) {
        y_tileno/=(Y_TILESIZE/2);
      } else {
        y_tileno/=Y_TILESIZE;
      }
      BX_VGA_THIS s.vga_mem_updated = 1;
      SET_TILE_UPDATED (x_tileno, y_tileno, 1);
      if (x_tileno2!=x_tileno) {
        SET_TILE_UPDATED (x_tileno2, y_tileno, 1);
      }
      return;
      /* CGA 320x200x4 / 640x200x2 end */
      }
    else if (BX_VGA_THIS s.graphics_ctrl.memory_mapping != 1) {

      BX_PANIC(("mem_write: graphics: mapping = %u",
               (unsigned) BX_VGA_THIS s.graphics_ctrl.memory_mapping));
      return;
      }

    if ( BX_VGA_THIS s.sequencer.chain_four ) {
      unsigned x_tileno, y_tileno;

      // 320 x 200 256 color mode: chained pixel representation
      BX_VGA_THIS s.vga_memory[(offset & ~0x03) + (offset % 4)*65536] = value;
      offset -= start_addr;
      x_tileno = (offset % BX_VGA_THIS s.line_offset) / (X_TILESIZE/2);
      if (BX_VGA_THIS s.y_doublescan) {
        y_tileno = (offset / BX_VGA_THIS s.line_offset) / (Y_TILESIZE/2);
      } else {
        y_tileno = (offset / BX_VGA_THIS s.line_offset) / Y_TILESIZE;
      }
      BX_VGA_THIS s.vga_mem_updated = 1;
      SET_TILE_UPDATED (x_tileno, y_tileno, 1);
      return;
      }

    }

  /* addr between 0xA0000 and 0xAFFFF */
  switch (BX_VGA_THIS s.graphics_ctrl.write_mode) {
    unsigned i;

    case 0: /* write mode 0 */
      {
        const Bit8u bitmask = BX_VGA_THIS s.graphics_ctrl.bitmask;
        const Bit8u set_reset = BX_VGA_THIS s.graphics_ctrl.set_reset;
        const Bit8u enable_set_reset = BX_VGA_THIS s.graphics_ctrl.enable_set_reset;
        /* perform rotate on CPU data in case its needed */
        if (BX_VGA_THIS s.graphics_ctrl.data_rotate) {
          value = (value >> BX_VGA_THIS s.graphics_ctrl.data_rotate) |
                  (value << (8 - BX_VGA_THIS s.graphics_ctrl.data_rotate));
        }
        new_val[0] = BX_VGA_THIS s.graphics_ctrl.latch[0] & ~bitmask;
        new_val[1] = BX_VGA_THIS s.graphics_ctrl.latch[1] & ~bitmask;
        new_val[2] = BX_VGA_THIS s.graphics_ctrl.latch[2] & ~bitmask;
        new_val[3] = BX_VGA_THIS s.graphics_ctrl.latch[3] & ~bitmask;
        switch (BX_VGA_THIS s.graphics_ctrl.raster_op) {
          case 0: // replace
            new_val[0] |= ((enable_set_reset & 1)
                           ? ((set_reset & 1) ? bitmask : 0)
                           : (value & bitmask));
            new_val[1] |= ((enable_set_reset & 2)
                           ? ((set_reset & 2) ? bitmask : 0)
                           : (value & bitmask));
            new_val[2] |= ((enable_set_reset & 4)
                           ? ((set_reset & 4) ? bitmask : 0)
                           : (value & bitmask));
            new_val[3] |= ((enable_set_reset & 8)
                           ? ((set_reset & 8) ? bitmask : 0)
                           : (value & bitmask));
            break;
          case 1: // AND
            new_val[0] |= ((enable_set_reset & 1)
                           ? ((set_reset & 1)
                              ? (BX_VGA_THIS s.graphics_ctrl.latch[0] & bitmask)
                              : 0)
                           : (value & BX_VGA_THIS s.graphics_ctrl.latch[0]) & bitmask);
            new_val[1] |= ((enable_set_reset & 2)
                           ? ((set_reset & 2)
                              ? (BX_VGA_THIS s.graphics_ctrl.latch[1] & bitmask)
                              : 0)
                           : (value & BX_VGA_THIS s.graphics_ctrl.latch[1]) & bitmask);
            new_val[2] |= ((enable_set_reset & 4)
                           ? ((set_reset & 4)
                              ? (BX_VGA_THIS s.graphics_ctrl.latch[2] & bitmask)
                              : 0)
                           : (value & BX_VGA_THIS s.graphics_ctrl.latch[2]) & bitmask);
            new_val[3] |= ((enable_set_reset & 8)
                           ? ((set_reset & 8)
                              ? (BX_VGA_THIS s.graphics_ctrl.latch[3] & bitmask)
                              : 0)
                           : (value & BX_VGA_THIS s.graphics_ctrl.latch[3]) & bitmask);
            break;
          case 2: // OR
            new_val[0]
              |= ((enable_set_reset & 1)
                  ? ((set_reset & 1)
                     ? bitmask
                     : (BX_VGA_THIS s.graphics_ctrl.latch[0] & bitmask))
                  : ((value | BX_VGA_THIS s.graphics_ctrl.latch[0]) & bitmask));
            new_val[1]
              |= ((enable_set_reset & 2)
                  ? ((set_reset & 2)
                     ? bitmask
                     : (BX_VGA_THIS s.graphics_ctrl.latch[1] & bitmask))
                  : ((value | BX_VGA_THIS s.graphics_ctrl.latch[1]) & bitmask));
            new_val[2]
              |= ((enable_set_reset & 4)
                  ? ((set_reset & 4)
                     ? bitmask
                     : (BX_VGA_THIS s.graphics_ctrl.latch[2] & bitmask))
                  : ((value | BX_VGA_THIS s.graphics_ctrl.latch[2]) & bitmask));
            new_val[3]
              |= ((enable_set_reset & 8)
                  ? ((set_reset & 8)
                     ? bitmask
                     : (BX_VGA_THIS s.graphics_ctrl.latch[3] & bitmask))
                  : ((value | BX_VGA_THIS s.graphics_ctrl.latch[3]) & bitmask));
            break;
          case 3: // XOR
            new_val[0]
              |= ((enable_set_reset & 1)
                 ? ((set_reset & 1)
                    ? (~BX_VGA_THIS s.graphics_ctrl.latch[0] & bitmask)
                    : (BX_VGA_THIS s.graphics_ctrl.latch[0] & bitmask))
                 : (value ^ BX_VGA_THIS s.graphics_ctrl.latch[0]) & bitmask);
            new_val[1]
              |= ((enable_set_reset & 2)
                 ? ((set_reset & 2)
                    ? (~BX_VGA_THIS s.graphics_ctrl.latch[1] & bitmask)
                    : (BX_VGA_THIS s.graphics_ctrl.latch[1] & bitmask))
                 : (value ^ BX_VGA_THIS s.graphics_ctrl.latch[1]) & bitmask);
            new_val[2]
              |= ((enable_set_reset & 4)
                 ? ((set_reset & 4)
                    ? (~BX_VGA_THIS s.graphics_ctrl.latch[2] & bitmask)
                    : (BX_VGA_THIS s.graphics_ctrl.latch[2] & bitmask))
                 : (value ^ BX_VGA_THIS s.graphics_ctrl.latch[2]) & bitmask);
            new_val[3]
              |= ((enable_set_reset & 8)
                 ? ((set_reset & 8)
                    ? (~BX_VGA_THIS s.graphics_ctrl.latch[3] & bitmask)
                    : (BX_VGA_THIS s.graphics_ctrl.latch[3] & bitmask))
                 : (value ^ BX_VGA_THIS s.graphics_ctrl.latch[3]) & bitmask);
            break;
          default:
            BX_PANIC(("vga_mem_write: write mode 0: op = %u",
                      (unsigned) BX_VGA_THIS s.graphics_ctrl.raster_op));
        }
      }
      break;

    case 1: /* write mode 1 */
      for (i=0; i<4; i++ ) {
        new_val[i] = BX_VGA_THIS s.graphics_ctrl.latch[i];
        }
      break;

    case 2: /* write mode 2 */
      {
        const Bit8u bitmask = BX_VGA_THIS s.graphics_ctrl.bitmask;

        new_val[0] = BX_VGA_THIS s.graphics_ctrl.latch[0] & ~bitmask;
        new_val[1] = BX_VGA_THIS s.graphics_ctrl.latch[1] & ~bitmask;
        new_val[2] = BX_VGA_THIS s.graphics_ctrl.latch[2] & ~bitmask;
        new_val[3] = BX_VGA_THIS s.graphics_ctrl.latch[3] & ~bitmask;
        switch (BX_VGA_THIS s.graphics_ctrl.raster_op) {
          case 0: // write
            new_val[0] |= (value & 1) ? bitmask : 0;
            new_val[1] |= (value & 2) ? bitmask : 0;
            new_val[2] |= (value & 4) ? bitmask : 0;
            new_val[3] |= (value & 8) ? bitmask : 0;
            break;
          case 1: // AND
            new_val[0] |= (value & 1)
              ? (BX_VGA_THIS s.graphics_ctrl.latch[0] & bitmask)
              : 0;
            new_val[1] |= (value & 2)
              ? (BX_VGA_THIS s.graphics_ctrl.latch[1] & bitmask)
              : 0;
            new_val[2] |= (value & 4)
              ? (BX_VGA_THIS s.graphics_ctrl.latch[2] & bitmask)
              : 0;
            new_val[3] |= (value & 8)
              ? (BX_VGA_THIS s.graphics_ctrl.latch[3] & bitmask)
              : 0;
            break;
          case 2: // OR
            new_val[0] |= (value & 1)
              ? bitmask
              : (BX_VGA_THIS s.graphics_ctrl.latch[0] & bitmask);
            new_val[1] |= (value & 2)
              ? bitmask
              : (BX_VGA_THIS s.graphics_ctrl.latch[1] & bitmask);
            new_val[2] |= (value & 4)
              ? bitmask
              : (BX_VGA_THIS s.graphics_ctrl.latch[2] & bitmask);
            new_val[3] |= (value & 8)
              ? bitmask
              : (BX_VGA_THIS s.graphics_ctrl.latch[3] & bitmask);
            break;
          case 3: // XOR
            new_val[0] |= (value & 1)
              ? (~BX_VGA_THIS s.graphics_ctrl.latch[0] & bitmask)
              : (BX_VGA_THIS s.graphics_ctrl.latch[0] & bitmask);
            new_val[1] |= (value & 2)
              ? (~BX_VGA_THIS s.graphics_ctrl.latch[1] & bitmask)
              : (BX_VGA_THIS s.graphics_ctrl.latch[1] & bitmask);
            new_val[2] |= (value & 4)
              ? (~BX_VGA_THIS s.graphics_ctrl.latch[2] & bitmask)
              : (BX_VGA_THIS s.graphics_ctrl.latch[2] & bitmask);
            new_val[3] |= (value & 8)
              ? (~BX_VGA_THIS s.graphics_ctrl.latch[3] & bitmask)
              : (BX_VGA_THIS s.graphics_ctrl.latch[3] & bitmask);
            break;
        }
      }
      break;

    case 3: /* write mode 3 */
      {
        const Bit8u bitmask = BX_VGA_THIS s.graphics_ctrl.bitmask & value;
        const Bit8u set_reset = BX_VGA_THIS s.graphics_ctrl.set_reset;

        /* perform rotate on CPU data */
        if (BX_VGA_THIS s.graphics_ctrl.data_rotate) {
          value = (value >> BX_VGA_THIS s.graphics_ctrl.data_rotate) |
                  (value << (8 - BX_VGA_THIS s.graphics_ctrl.data_rotate));
        }
        new_val[0] = BX_VGA_THIS s.graphics_ctrl.latch[0] & ~bitmask;
        new_val[1] = BX_VGA_THIS s.graphics_ctrl.latch[1] & ~bitmask;
        new_val[2] = BX_VGA_THIS s.graphics_ctrl.latch[2] & ~bitmask;
        new_val[3] = BX_VGA_THIS s.graphics_ctrl.latch[3] & ~bitmask;

        value &= bitmask;

        switch (BX_VGA_THIS s.graphics_ctrl.raster_op) {
          case 0: // write
            new_val[0] |= (set_reset & 1) ? value : 0;
            new_val[1] |= (set_reset & 2) ? value : 0;
            new_val[2] |= (set_reset & 4) ? value : 0;
            new_val[3] |= (set_reset & 8) ? value : 0;
            break;
          case 1: // AND
            new_val[0] |= ((set_reset & 1) ? value : 0)
              & BX_VGA_THIS s.graphics_ctrl.latch[0];
            new_val[1] |= ((set_reset & 2) ? value : 0)
              & BX_VGA_THIS s.graphics_ctrl.latch[1];
            new_val[2] |= ((set_reset & 4) ? value : 0)
              & BX_VGA_THIS s.graphics_ctrl.latch[2];
            new_val[3] |= ((set_reset & 8) ? value : 0)
              & BX_VGA_THIS s.graphics_ctrl.latch[3];
            break;
          case 2: // OR
            new_val[0] |= ((set_reset & 1) ? value : 0)
              | BX_VGA_THIS s.graphics_ctrl.latch[0];
            new_val[1] |= ((set_reset & 2) ? value : 0)
              | BX_VGA_THIS s.graphics_ctrl.latch[1];
            new_val[2] |= ((set_reset & 4) ? value : 0)
              | BX_VGA_THIS s.graphics_ctrl.latch[2];
            new_val[3] |= ((set_reset & 8) ? value : 0)
              | BX_VGA_THIS s.graphics_ctrl.latch[3];
            break;
          case 3: // XOR
            new_val[0] |= ((set_reset & 1) ? value : 0)
              ^ BX_VGA_THIS s.graphics_ctrl.latch[0];
            new_val[1] |= ((set_reset & 2) ? value : 0)
              ^ BX_VGA_THIS s.graphics_ctrl.latch[1];
            new_val[2] |= ((set_reset & 4) ? value : 0)
              ^ BX_VGA_THIS s.graphics_ctrl.latch[2];
            new_val[3] |= ((set_reset & 8) ? value : 0)
              ^ BX_VGA_THIS s.graphics_ctrl.latch[3];
            break;
        }
      }
      break;

    default:
      BX_PANIC(("vga_mem_write: write mode %u ?",
        (unsigned) BX_VGA_THIS s.graphics_ctrl.write_mode));
  }

  if (BX_VGA_THIS s.sequencer.map_mask & 0x0f) {
    BX_VGA_THIS s.vga_mem_updated = 1;
    if (BX_VGA_THIS s.sequencer.map_mask_bit[0])
      BX_VGA_THIS s.vga_memory[0*65536 + offset] = new_val[0];
    if (BX_VGA_THIS s.sequencer.map_mask_bit[1])
      BX_VGA_THIS s.vga_memory[1*65536 + offset] = new_val[1];
    if (BX_VGA_THIS s.sequencer.map_mask_bit[2]) {
      if ((!BX_VGA_THIS s.graphics_ctrl.graphics_alpha) &&
          ((offset & 0xe000) == BX_VGA_THIS s.charmap_address)) {
        bx_gui->set_text_charbyte((offset & 0x1fff), new_val[2]);
        }
      BX_VGA_THIS s.vga_memory[2*65536 + offset] = new_val[2];
      }
    if (BX_VGA_THIS s.sequencer.map_mask_bit[3])
      BX_VGA_THIS s.vga_memory[3*65536 + offset] = new_val[3];

    unsigned x_tileno, y_tileno;

    if (BX_VGA_THIS s.graphics_ctrl.shift_reg == 2) {
      offset -= start_addr;
      x_tileno = (offset % BX_VGA_THIS s.line_offset) * 4 / (X_TILESIZE / 2);
      if (BX_VGA_THIS s.y_doublescan) {
        y_tileno = (offset / BX_VGA_THIS s.line_offset) / (Y_TILESIZE / 2);
      } else {
        y_tileno = (offset / BX_VGA_THIS s.line_offset) / Y_TILESIZE;
      }
      SET_TILE_UPDATED (x_tileno, y_tileno, 1);
    } else {
      if (BX_VGA_THIS s.line_compare < BX_VGA_THIS s.vertical_display_end) {
        if (BX_VGA_THIS s.x_dotclockdiv2) {
          x_tileno = (offset % BX_VGA_THIS s.line_offset) / (X_TILESIZE / 16);
        } else {
          x_tileno = (offset % BX_VGA_THIS s.line_offset) / (X_TILESIZE / 8);
        }
        if (BX_VGA_THIS s.y_doublescan) {
          y_tileno = ((offset / BX_VGA_THIS s.line_offset) * 2 + BX_VGA_THIS s.line_compare + 1) / Y_TILESIZE;
        } else {
          y_tileno = ((offset / BX_VGA_THIS s.line_offset) + BX_VGA_THIS s.line_compare + 1) / Y_TILESIZE;
        }
        SET_TILE_UPDATED (x_tileno, y_tileno, 1);
      }
      if (offset >= start_addr) {
        offset -= start_addr;
        if (BX_VGA_THIS s.x_dotclockdiv2) {
          x_tileno = (offset % BX_VGA_THIS s.line_offset) / (X_TILESIZE / 16);
        } else {
          x_tileno = (offset % BX_VGA_THIS s.line_offset) / (X_TILESIZE / 8);
        }
        if (BX_VGA_THIS s.y_doublescan) {
          y_tileno = (offset / BX_VGA_THIS s.line_offset) / (Y_TILESIZE / 2);
        } else {
          y_tileno = (offset / BX_VGA_THIS s.line_offset) / Y_TILESIZE;
        }
        SET_TILE_UPDATED (x_tileno, y_tileno, 1);
      }
    }
  }
}

  void
bx_vga_c::get_text_snapshot(Bit8u **text_snapshot, unsigned *txHeight,
                                                   unsigned *txWidth)
{
  unsigned VDE, MSL;

  if (!BX_VGA_THIS s.graphics_ctrl.graphics_alpha) {
    *text_snapshot = &BX_VGA_THIS s.text_snapshot[0];
    VDE = BX_VGA_THIS s.vertical_display_end;
    MSL = BX_VGA_THIS s.CRTC.reg[0x09] & 0x1f;
    *txHeight = (VDE+1)/(MSL+1);
    *txWidth = BX_VGA_THIS s.CRTC.reg[1] + 1;
  } else {
    *txHeight = 0;
    *txWidth = 0;
  }
}

  Bit8u
bx_vga_c::get_actl_palette_idx(Bit8u index)
{
  return BX_VGA_THIS s.attribute_ctrl.palette_reg[index];
}

  void
bx_vga_c::dump_status(void)
{
  BX_INFO(("s.misc_output.color_emulation = %u",
            (unsigned) BX_VGA_THIS s.misc_output.color_emulation));
  BX_INFO(("s.misc_output.enable_ram = %u",
            (unsigned) BX_VGA_THIS s.misc_output.enable_ram));
  BX_INFO(("s.misc_output.clock_select = %u",
            (unsigned) BX_VGA_THIS s.misc_output.clock_select));
  if (BX_VGA_THIS s.misc_output.clock_select == 0)
    BX_INFO(("  25Mhz 640 horiz pixel clock"));
  else
    BX_INFO(("  28Mhz 720 horiz pixel clock"));
  BX_INFO(("s.misc_output.select_high_bank = %u",
            (unsigned) BX_VGA_THIS s.misc_output.select_high_bank));
  BX_INFO(("s.misc_output.horiz_sync_pol = %u",
            (unsigned) BX_VGA_THIS s.misc_output.horiz_sync_pol));
  BX_INFO(("s.misc_output.vert_sync_pol = %u",
            (unsigned) BX_VGA_THIS s.misc_output.vert_sync_pol));
  switch ( (BX_VGA_THIS s.misc_output.vert_sync_pol << 1) |
           BX_VGA_THIS s.misc_output.horiz_sync_pol ) {
    case 0: BX_INFO(("  (reserved")); break;
    case 1: BX_INFO(("  400 lines")); break;
    case 2: BX_INFO(("  350 lines")); break;
    case 3: BX_INFO(("  480 lines")); break;
    default: BX_INFO(("  ???"));
    }

  BX_INFO(("s.graphics_ctrl.odd_even = %u",
            (unsigned) BX_VGA_THIS s.graphics_ctrl.odd_even));
  BX_INFO(("s.graphics_ctrl.chain_odd_even = %u",
            (unsigned) BX_VGA_THIS s.graphics_ctrl.chain_odd_even));
  BX_INFO(("s.graphics_ctrl.shift_reg = %u",
            (unsigned) BX_VGA_THIS s.graphics_ctrl.shift_reg));
  BX_INFO(("s.graphics_ctrl.graphics_alpha = %u",
            (unsigned) BX_VGA_THIS s.graphics_ctrl.graphics_alpha));
  BX_INFO(("s.graphics_ctrl.memory_mapping = %u",
            (unsigned) BX_VGA_THIS s.graphics_ctrl.memory_mapping));
  switch (BX_VGA_THIS s.graphics_ctrl.memory_mapping) {
    case 0: BX_INFO(("  A0000-BFFFF")); break;
    case 1: BX_INFO(("  A0000-AFFFF")); break;
    case 2: BX_INFO(("  B0000-B7FFF")); break;
    case 3: BX_INFO(("  B8000-BFFFF")); break;
    default: BX_INFO(("  ???"));
    }

  BX_INFO(("s.sequencer.extended_mem = %u",
            (unsigned) BX_VGA_THIS s.sequencer.extended_mem));
  BX_INFO(("s.sequencer.odd_even = %u (inverted)",
            (unsigned) BX_VGA_THIS s.sequencer.odd_even));
  BX_INFO(("s.sequencer.chain_four = %u",
            (unsigned) BX_VGA_THIS s.sequencer.chain_four));

  BX_INFO(("s.attribute_ctrl.video_enabled = %u",
            (unsigned) BX_VGA_THIS s.attribute_ctrl.video_enabled));
  BX_INFO(("s.attribute_ctrl.mode_ctrl.graphics_alpha = %u",
            (unsigned) BX_VGA_THIS s.attribute_ctrl.mode_ctrl.graphics_alpha));
  BX_INFO(("s.attribute_ctrl.mode_ctrl.display_type = %u",
            (unsigned) BX_VGA_THIS s.attribute_ctrl.mode_ctrl.display_type));
  BX_INFO(("s.attribute_ctrl.mode_ctrl.internal_palette_size = %u",
            (unsigned) BX_VGA_THIS s.attribute_ctrl.mode_ctrl.internal_palette_size));
  BX_INFO(("s.attribute_ctrl.mode_ctrl.pixel_clock_select = %u",
            (unsigned) BX_VGA_THIS s.attribute_ctrl.mode_ctrl.pixel_clock_select));
}


  void
bx_vga_c::redraw_area(unsigned x0, unsigned y0, unsigned width,
                      unsigned height)
{
  unsigned xi, yi, x1, y1, xmax, ymax;

  BX_VGA_THIS s.vga_mem_updated = 1;

#if BX_SUPPORT_VBE
  if (BX_VGA_THIS s.graphics_ctrl.graphics_alpha || BX_VGA_THIS s.vbe_enabled) {
#else
  if (BX_VGA_THIS s.graphics_ctrl.graphics_alpha) {
#endif
    // graphics mode
    x1 = x0 + width  - 1;
    y1 = y0 + height - 1;

    xmax = old_iWidth;
    ymax = old_iHeight;
#if BX_SUPPORT_VBE
    if (BX_VGA_THIS s.vbe_enabled) {
      xmax = BX_VGA_THIS s.vbe_xres;
      ymax = BX_VGA_THIS s.vbe_yres;
    }
#endif
    for (yi=0; yi<ymax; yi+=Y_TILESIZE) {
      for (xi=0; xi<xmax; xi+=X_TILESIZE) {
        // is redraw rectangle outside x boundaries of this tile?
        if (x1 < xi) continue;
        if (x0 > (xi+X_TILESIZE-1)) continue;

        // is redraw rectangle outside y boundaries of this tile?
        if (y1 < yi) continue;
        if (y0 > (yi+Y_TILESIZE-1)) continue;
	unsigned xti = xi/X_TILESIZE;
	unsigned yti = yi/Y_TILESIZE;
        SET_TILE_UPDATED (xti, yti, 1);
        }
      }
    }
  else {
    // text mode
    memset(BX_VGA_THIS s.text_snapshot, 0,
           sizeof(BX_VGA_THIS s.text_snapshot));
    }
}


#if BX_SUPPORT_VBE
  Bit8u  BX_CPP_AttrRegparmN(1)
bx_vga_c::vbe_mem_read(Bit32u addr)
{
  Bit32u offset;

  if (addr >= VBE_DISPI_LFB_PHYSICAL_ADDRESS)
  {
    // LFB read
    offset = addr - VBE_DISPI_LFB_PHYSICAL_ADDRESS;
  }
  else
  {
    // banked mode read
    offset = BX_VGA_THIS s.vbe_bank*65536 + addr - 0xA0000;
  }

  // check for out of memory read
  if (offset > VBE_DISPI_TOTAL_VIDEO_MEMORY_BYTES)
    return 0;

  return (BX_VGA_THIS s.vbe_memory[offset]);
}

  void BX_CPP_AttrRegparmN(2)
bx_vga_c::vbe_mem_write(Bit32u addr, Bit8u value)
{
  Bit32u offset;
  unsigned x_tileno, y_tileno;

  if (BX_VGA_THIS s.vbe_lfb_enabled)
  {
    if (addr >= VBE_DISPI_LFB_PHYSICAL_ADDRESS)
    {
      // LFB write
      offset = addr - VBE_DISPI_LFB_PHYSICAL_ADDRESS;
    }
    else
    {
      // banked mode write while in LFB mode -> ignore
      return;
    }
  }
  else
  {
    if (addr < VBE_DISPI_LFB_PHYSICAL_ADDRESS)
    {
      // banked mode write
      offset = (BX_VGA_THIS s.vbe_bank*65536) + (addr - 0xA0000);
    }
    else
    {
      // LFB write while in banked mode -> ignore
      return;
    }
  }

  // check for out of memory write
  if (offset < VBE_DISPI_TOTAL_VIDEO_MEMORY_BYTES)
  {
    BX_VGA_THIS s.vbe_memory[offset]=value;
  }
  else
  {
    // make sure we don't flood the logfile
    static int count=0;
    if (count<100)
    {
      count ++;
      BX_INFO(("VBE_mem_write out of video memory write at %x",offset));
    }
  }

  offset-=BX_VGA_THIS s.vbe_virtual_start;

  // only update the UI when writing 'onscreen'
  if (offset < BX_VGA_THIS s.vbe_visable_screen_size)
  {
    y_tileno = ((offset / BX_VGA_THIS s.vbe_bpp_multiplier) / BX_VGA_THIS s.vbe_virtual_xres) / Y_TILESIZE;
    x_tileno = ((offset / BX_VGA_THIS s.vbe_bpp_multiplier) % BX_VGA_THIS s.vbe_virtual_xres) / X_TILESIZE;

    if ((y_tileno < BX_NUM_Y_TILES) && (x_tileno < BX_NUM_X_TILES))
    {
      BX_VGA_THIS s.vga_mem_updated = 1;
      SET_TILE_UPDATED (x_tileno, y_tileno, 1);
    }
  }  
}

  Bit32u
bx_vga_c::vbe_read_handler(void *this_ptr, Bit32u address, unsigned io_len)
{
#if !BX_USE_VGA_SMF
  bx_vga_c *class_ptr = (bx_vga_c *) this_ptr;

  return( class_ptr->vbe_read(address, io_len) );
}


  Bit32u
bx_vga_c::vbe_read(Bit32u address, unsigned io_len)
{
#else
  UNUSED(this_ptr);
#endif  // !BX_USE_VGA_SMF

//  BX_INFO(("VBE_read %x (len %x)", address, io_len));

  if (address==VBE_DISPI_IOPORT_INDEX)
  {
    // index register
    return (Bit32u) BX_VGA_THIS s.vbe_curindex;
  }
  else
  {
    // data register read
    
    switch (BX_VGA_THIS s.vbe_curindex)
    {
      case VBE_DISPI_INDEX_ID: // Display Interface ID check
      {
        return BX_VGA_THIS s.vbe_cur_dispi;
      } break;
      
      case VBE_DISPI_INDEX_XRES: // x resolution
      {
        return BX_VGA_THIS s.vbe_xres;
      } break;

      case VBE_DISPI_INDEX_YRES: // y resolution
      {
        return BX_VGA_THIS s.vbe_yres;
      } break;
      
      case VBE_DISPI_INDEX_BPP: // bpp
      {
        return BX_VGA_THIS s.vbe_bpp;
      } break;

      case VBE_DISPI_INDEX_ENABLE: // vbe enabled
      {
        return BX_VGA_THIS s.vbe_enabled;
      } break;
      
      case VBE_DISPI_INDEX_BANK: // current bank
      {
        return BX_VGA_THIS s.vbe_bank;
      } break;

      case VBE_DISPI_INDEX_X_OFFSET:
      {
      	return BX_VGA_THIS s.vbe_offset_x;
      } break;

      case VBE_DISPI_INDEX_Y_OFFSET:
      {
      	return BX_VGA_THIS s.vbe_offset_y;
      } break;

      case VBE_DISPI_INDEX_VIRT_WIDTH:
      {
      	return BX_VGA_THIS s.vbe_virtual_xres;
      	
      } break;
      
      case VBE_DISPI_INDEX_VIRT_HEIGHT:
      {
      	return BX_VGA_THIS s.vbe_virtual_yres;      	
      } break;
        

      default:
      {
        BX_PANIC(("VBE unknown data read index 0x%x",BX_VGA_THIS s.vbe_curindex));
      } break;
    }      
  }
  BX_PANIC(("VBE_read shouldn't reach this"));
  return 0; /* keep compiler happy */
}

  void
bx_vga_c::vbe_write_handler(void *this_ptr, Bit32u address, Bit32u value, unsigned io_len)
{
#if !BX_USE_VGA_SMF
  bx_vga_c *class_ptr = (bx_vga_c *) this_ptr;

  class_ptr->vbe_write(address, value, io_len);
}

  Bit32u
bx_vga_c::vbe_write(Bit32u address, Bit32u value, unsigned io_len)
{ 
#else
  UNUSED(this_ptr);
#endif  

//  BX_INFO(("VBE_write %x = %x (len %x)", address, value, io_len));
  
  switch(address)
  {
    // index register    
    case VBE_DISPI_IOPORT_INDEX:

      BX_VGA_THIS s.vbe_curindex = (Bit16u) value;
      break;

    // data register
    // FIXME: maybe do some 'sanity' checks on received data?
    case VBE_DISPI_IOPORT_DATA:
      switch (BX_VGA_THIS s.vbe_curindex)
      {
        case VBE_DISPI_INDEX_ID: // Display Interface ID check
        {
          if ( (value == VBE_DISPI_ID0) ||
               (value == VBE_DISPI_ID1) ||
               (value == VBE_DISPI_ID2) )
          {
            // allow backwards compatible with previous dispi bioses
            BX_VGA_THIS s.vbe_cur_dispi=value;
          }
          else
          {
            BX_PANIC(("VBE unknown Display Interface %x",value));
          }

          // make sure we don't flood the logfile
          static int count=0;
          if (count < 100)
          {
            count++;
            BX_INFO(("VBE known Display Interface %x",value));
          }
        } break;

        case VBE_DISPI_INDEX_XRES: // set xres
        {
          // check that we don't set xres during vbe enabled
          if (!BX_VGA_THIS s.vbe_enabled)
          {
            // check for within max xres range
            if (value <= VBE_DISPI_MAX_XRES)
            {
              BX_VGA_THIS s.vbe_xres=(Bit16u) value;
              BX_INFO(("VBE set xres (%d)",value));
            }
            else
            {
              BX_INFO(("VBE set xres more then max xres (%d)",value));
            }
          }
          else
          {
            BX_INFO(("VBE set xres during vbe enabled!"));
          }
        } break;

        case VBE_DISPI_INDEX_YRES: // set yres
        {
          // check that we don't set yres during vbe enabled
          if (!BX_VGA_THIS s.vbe_enabled)
          {
            // check for within max yres range
            if (value <= VBE_DISPI_MAX_YRES)
            {
              BX_VGA_THIS s.vbe_yres=(Bit16u) value;
              BX_INFO(("VBE set yres (%d)",value));
            }
            else
            {
              BX_INFO(("VBE set yres more then max yres (%d)",value));
            }
          }
          else
          {
            BX_INFO(("VBE set yres during vbe enabled!"));
          }
        } break;

        case VBE_DISPI_INDEX_BPP: // set bpp
        {
          // check that we don't set bpp during vbe enabled
          if (!BX_VGA_THIS s.vbe_enabled)
          {
            // for backward compatiblity
            if (value == 0) value = VBE_DISPI_BPP_8;
            // check for correct bpp range
            if ((value == VBE_DISPI_BPP_4) || (value == VBE_DISPI_BPP_8) || (value == VBE_DISPI_BPP_15) ||
                (value == VBE_DISPI_BPP_16) || (value == VBE_DISPI_BPP_24) || (value == VBE_DISPI_BPP_32))
            {
              BX_VGA_THIS s.vbe_bpp=(Bit16u) value;
              BX_INFO(("VBE set bpp (%d)",value));
            }
            else
            {
              BX_INFO(("VBE set bpp with unknown bpp (%d)",value));
            }
          }
          else
          {
            BX_INFO(("VBE set bpp during vbe enabled!"));
          }
        } break;

        case VBE_DISPI_INDEX_BANK: // set bank
        {
          value=value & 0xff ; // FIXME lobyte = vbe bank A?

          // check for max bank nr
          if (value < (VBE_DISPI_TOTAL_VIDEO_MEMORY_KB /64))
          {
            if (!BX_VGA_THIS s.vbe_lfb_enabled)
            {
              BX_DEBUG(("VBE set bank to %d", value));
              BX_VGA_THIS s.vbe_bank=value;
            }
            else
            {
              BX_ERROR(("VBE set bank in LFB mode ignored"));
            }
          }
          else
          {
            BX_INFO(("VBE set invalid bank (%d)",value));
          }
        } break;

        case VBE_DISPI_INDEX_ENABLE: // enable video
        {
          if (value & VBE_DISPI_ENABLED)
          {
            unsigned depth=0;

            // setup virtual resolution to be the same as current reso      
            BX_VGA_THIS s.vbe_virtual_yres=BX_VGA_THIS s.vbe_yres;
            BX_VGA_THIS s.vbe_virtual_xres=BX_VGA_THIS s.vbe_xres;

            // reset offset
            BX_VGA_THIS s.vbe_offset_x=0;
            BX_VGA_THIS s.vbe_offset_y=0;
            BX_VGA_THIS s.vbe_virtual_start=0;

            switch((BX_VGA_THIS s.vbe_bpp))
            {
              // Default pixel sizes
              case VBE_DISPI_BPP_8: 
                BX_VGA_THIS s.vbe_bpp_multiplier = 1;
                BX_VGA_THIS s.vbe_line_byte_width = BX_VGA_THIS s.vbe_virtual_xres;
                BX_VGA_THIS s.vbe_visable_screen_size = ((BX_VGA_THIS s.vbe_xres) * (BX_VGA_THIS s.vbe_yres));
                depth=8;
                break;

              case VBE_DISPI_BPP_4: 
                BX_VGA_THIS s.vbe_bpp_multiplier = 1;
                BX_VGA_THIS s.vbe_line_byte_width = BX_VGA_THIS s.vbe_virtual_xres;
                BX_VGA_THIS s.vbe_visable_screen_size = ((BX_VGA_THIS s.vbe_xres) * (BX_VGA_THIS s.vbe_yres));
                depth=4;
                break;

              case VBE_DISPI_BPP_15:
                BX_VGA_THIS s.vbe_bpp_multiplier = 2; 
                BX_VGA_THIS s.vbe_line_byte_width = BX_VGA_THIS s.vbe_virtual_xres * 2;
                BX_VGA_THIS s.vbe_visable_screen_size = ((BX_VGA_THIS s.vbe_xres) * (BX_VGA_THIS s.vbe_yres)) * 2;
                depth=15;
                break;        	          

              case VBE_DISPI_BPP_16:
                BX_VGA_THIS s.vbe_bpp_multiplier = 2; 
                BX_VGA_THIS s.vbe_line_byte_width = BX_VGA_THIS s.vbe_virtual_xres * 2;
                BX_VGA_THIS s.vbe_visable_screen_size = ((BX_VGA_THIS s.vbe_xres) * (BX_VGA_THIS s.vbe_yres)) * 2;
                depth=16;
                break;        	          

              case VBE_DISPI_BPP_24: 
                BX_VGA_THIS s.vbe_bpp_multiplier = 3; 
                BX_VGA_THIS s.vbe_line_byte_width = BX_VGA_THIS s.vbe_virtual_xres * 3;
                BX_VGA_THIS s.vbe_visable_screen_size = ((BX_VGA_THIS s.vbe_xres) * (BX_VGA_THIS s.vbe_yres)) * 3;
                depth=24;
                break;        	          

              case VBE_DISPI_BPP_32: 
                BX_VGA_THIS s.vbe_bpp_multiplier = 4; 
                BX_VGA_THIS s.vbe_line_byte_width = BX_VGA_THIS s.vbe_virtual_xres << 2;
                BX_VGA_THIS s.vbe_visable_screen_size = ((BX_VGA_THIS s.vbe_xres) * (BX_VGA_THIS s.vbe_yres)) << 2;
                depth=32;
                break;        	          
            }

            BX_INFO(("VBE enabling x %d, y %d, bpp %d, %u bytes visible", BX_VGA_THIS s.vbe_xres, BX_VGA_THIS s.vbe_yres, BX_VGA_THIS s.vbe_bpp, BX_VGA_THIS s.vbe_visable_screen_size));

            if (depth > 4)
            {
              BX_VGA_THIS s.vbe_lfb_enabled=(bx_bool)(value & VBE_DISPI_LFB_ENABLED);
              if ((value & VBE_DISPI_NOCLEARMEM) == 0)
              {
                memset(BX_VGA_THIS s.vbe_memory, 0, BX_VGA_THIS s.vbe_visable_screen_size);
              }
              bx_gui->dimension_update(BX_VGA_THIS s.vbe_xres, BX_VGA_THIS s.vbe_yres, 0, 0, depth);
              // some test applications expect these standard VGA settings
              BX_VGA_THIS s.CRTC.reg[9] = 0x00;
              BX_VGA_THIS s.attribute_ctrl.mode_ctrl.graphics_alpha = 1;
              BX_VGA_THIS s.graphics_ctrl.memory_mapping = 1;
              BX_VGA_THIS s.CRTC.reg[1] = (BX_VGA_THIS s.vbe_xres / 8) - 1;
              BX_VGA_THIS s.CRTC.reg[19] = BX_VGA_THIS s.vbe_xres >> 2;
              BX_VGA_THIS s.attribute_ctrl.mode_ctrl.pixel_clock_select = 1;
              BX_VGA_THIS s.CRTC.reg[18] = (BX_VGA_THIS s.vbe_yres - 1) & 0xff;
              BX_VGA_THIS s.CRTC.reg[7] &= ~0x42;
              if ((BX_VGA_THIS s.vbe_yres - 1) & 0x0100) {
                BX_VGA_THIS s.CRTC.reg[7] |= 0x02;
              }
              if ((BX_VGA_THIS s.vbe_yres - 1) & 0x0200) {
                BX_VGA_THIS s.CRTC.reg[7] |= 0x40;
              }
            }
          }
          else
          {
            BX_INFO(("VBE disabling"));
            BX_VGA_THIS s.vbe_lfb_enabled=0;
          }     
          BX_VGA_THIS s.vbe_enabled=(bx_bool)(value & VBE_DISPI_ENABLED);
        } break;

        case VBE_DISPI_INDEX_X_OFFSET:
        {
          // BX_INFO(("VBE offset x %x",value));
          BX_VGA_THIS s.vbe_offset_x=(Bit16u)value;

          BX_VGA_THIS s.vbe_virtual_start = ((BX_VGA_THIS s.vbe_offset_y) * (BX_VGA_THIS s.vbe_line_byte_width)) +
                                             ((BX_VGA_THIS s.vbe_offset_x) * (BX_VGA_THIS s.vbe_bpp_multiplier));

          BX_VGA_THIS s.vga_mem_updated = 1;
          for (unsigned xti = 0; xti < BX_NUM_X_TILES; xti++) {
            for (unsigned yti = 0; yti < BX_NUM_Y_TILES; yti++) {
              SET_TILE_UPDATED (xti, yti, 1);
            }
          }
        } break;

        case VBE_DISPI_INDEX_Y_OFFSET:
        {
          // BX_INFO(("VBE offset y %x",value));
          BX_VGA_THIS s.vbe_offset_y=(Bit16u)value;
          BX_VGA_THIS s.vbe_virtual_start = ((BX_VGA_THIS s.vbe_offset_y) * (BX_VGA_THIS s.vbe_line_byte_width)) +
                                             ((BX_VGA_THIS s.vbe_offset_x) * (BX_VGA_THIS s.vbe_bpp_multiplier));

          BX_VGA_THIS s.vga_mem_updated = 1;
          for (unsigned xti = 0; xti < BX_NUM_X_TILES; xti++) {
            for (unsigned yti = 0; yti < BX_NUM_Y_TILES; yti++) {
              SET_TILE_UPDATED (xti, yti, 1);
            }
          }
        } break;

        case VBE_DISPI_INDEX_VIRT_WIDTH:
        {
          BX_INFO(("VBE requested virtual width %d",value));

          // calculate virtual width & height dimensions
          // req:
          //   virt_width > xres
          //   virt_height >=yres
          //   virt_width*virt_height < MAX_VIDEO_MEMORY

          // basicly 2 situations

          // situation 1: 
          //   MAX_VIDEO_MEMORY / virt_width >= yres
          //        adjust result height
          //   else
          //        adjust result width based upon virt_height=yres
          Bit16u new_width=value;
          Bit16u new_height=(sizeof(BX_VGA_THIS s.vbe_memory) / BX_VGA_THIS s.vbe_bpp_multiplier) / new_width;
          if (new_height >=BX_VGA_THIS s.vbe_yres)
          {
            // we have a decent virtual width & new_height
            BX_INFO(("VBE decent virtual height %d",new_height));
          }
          else
          {
            // no decent virtual height: adjust width & height
            new_height=BX_VGA_THIS s.vbe_yres;
            new_width=(sizeof(BX_VGA_THIS s.vbe_memory) / BX_VGA_THIS s.vbe_bpp_multiplier) / new_height;

            BX_INFO(("VBE recalc virtual width %d height %d",new_width, new_height));
          }

          BX_VGA_THIS s.vbe_virtual_xres=new_width;
          BX_VGA_THIS s.vbe_virtual_yres=new_height;
          BX_VGA_THIS s.vbe_visable_screen_size = (new_width * (BX_VGA_THIS s.vbe_yres)) * BX_VGA_THIS s.vbe_bpp_multiplier;

        } break;
	/*      
        case VBE_DISPI_INDEX_VIRT_HEIGHT:
        {
          BX_INFO(("VBE virtual height %x",value));

        } break;
	*/  
        default:
        {
          BX_PANIC(("VBE unknown data write index 0x%x",BX_VGA_THIS s.vbe_curindex));
        } break;      
      }        
      break;
	  
  } // end switch address
}

#endif
