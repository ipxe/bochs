////////////////////////////////////////////////////////////////////////
// $Id$
/////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2018  The Bochs Project
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


#ifndef BX_DISPLAY_DDC_H
#define BX_DISPLAY_DDC_H

class bx_ddc_c : public logfunctions {
public:
  bx_ddc_c();
  virtual ~bx_ddc_c();

  Bit8u read(void);
  void write(bx_bool dck, bx_bool dda);

private:

  void combine_signals(void);

  struct {
    bx_bool DCKhost;
    bx_bool DDAhost;
    bx_bool DCKmon;
    bx_bool DDAmon;
    Bit8u   retval;
  } s;  // state information
};

#endif
