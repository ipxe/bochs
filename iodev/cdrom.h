/////////////////////////////////////////////////////////////////////////
// $Id: cdrom.h,v 1.13 2003/08/19 00:37:03 cbothamy Exp $
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


// Header file for low-level OS specific CDROM emulation


class cdrom_interface : public logfunctions {
public:
  cdrom_interface(char *dev);
  ~cdrom_interface(void);
  void init(void);

  // Load CD-ROM. Returns false if CD is not ready.
  bx_bool insert_cdrom(char *dev = NULL);

  // Logically eject the CD.
  void eject_cdrom();

  // Read CD TOC. Returns false if start track is out of bounds.
  bx_bool read_toc(uint8* buf, int* length, bx_bool msf, int start_track);

  // Return CD-ROM capacity (in 2048 byte frames)
  uint32 capacity();

  // Read a single block from the CD
  void read_block(uint8* buf, int lba) BX_CPP_AttrRegparmN(2);

  // Start (spin up) the CD.
  int start_cdrom();

private:
  int fd;
  char *path;

  int using_file;
#ifdef WIN32
  HANDLE hFile;
  int hid;
  int tid;
  int lun;
#endif
  };

