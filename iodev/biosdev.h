
// $Id: biosdev.h,v 1.3.6.3 2003-04-04 03:46:06 slechta Exp $
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


#define BX_BIOS_MESSAGE_SIZE 80


#if BX_USE_BIOS_SMF
#  define BX_BIOS_SMF  static
#  define BX_BIOS_THIS_PTR theBiosDevice->
#  define BX_BIOS_THIS theBiosDevice
#else
#  define BX_BIOS_SMF
#  define BX_BIOS_THIS_PTR this->
#  define BX_BIOS_THIS this
#endif


class bx_biosdev_c : public bx_devmodel_c {
public:
  bx_biosdev_c(void);
  ~bx_biosdev_c(void);

  virtual void init(void);
  virtual void register_state(bx_param_c *list_p);
  virtual void reset (unsigned type);

private:

  static void   write_handler(void *this_ptr, Bit32u address, Bit32u value, unsigned io_len);
#if !BX_USE_BIOS_SMF
  void   write(Bit32u address, Bit32u value, unsigned io_len);
#endif

  struct s_t {
    Bit8u bios_message[BX_BIOS_MESSAGE_SIZE];
    unsigned int bios_message_i;

    Bit8u vgabios_message[BX_BIOS_MESSAGE_SIZE];
    unsigned int vgabios_message_i;
    } s;  // state information

  };
