/////////////////////////////////////////////////////////////////////////
// $Id$
/////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2011-2015  The Bochs Project
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

// Common code for sound lowlevel modules

class bx_sound_lowlevel_c;
class bx_soundlow_waveout_c;

// Pseudo device that loads the lowlevel sound module
class bx_soundmod_ctl_c : public bx_soundmod_ctl_stub_c {
public:
  bx_soundmod_ctl_c();
  virtual ~bx_soundmod_ctl_c();
  virtual void init(void);
  virtual void* get_module();
  virtual void VOC_init_file(FILE *stream);
  virtual void VOC_write_block(FILE *stream, int block, Bit32u headerlen,
                               Bit8u header[], Bit32u datalen, Bit8u data[]);
private:
  bx_sound_lowlevel_c *soundmod;
  bx_soundlow_waveout_c *waveout;
};
