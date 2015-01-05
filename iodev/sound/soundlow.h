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

// this is the size of a sound packet used for sending and receiving
// it should not be too large to avoid lag, and not too
// small to avoid unnecessary overhead.
#define BX_SOUNDLOW_WAVEPACKETSIZE  8192

// Definitions for the output functions
#define BX_SOUNDLOW_OK   0
#define BX_SOUNDLOW_ERR  1

// Lowlvel sound modules
#define BX_SOUNDLOW_DUMMY   0
#define BX_SOUNDLOW_LINUX   1
#define BX_SOUNDLOW_OSX     2
#define BX_SOUNDLOW_WIN     3
#define BX_SOUNDLOW_SDL     4
#define BX_SOUNDLOW_ALSA    5

typedef Bit32u (*sound_record_handler_t)(void *arg, Bit32u len);
typedef Bit32u (*get_wavedata_cb_t)(void *arg, Bit16u rate, Bit8u *buffer, Bit32u len);

// The class with the input/output functions
class bx_sound_lowlevel_c : public logfunctions {
public:

  /*
  These functions are the sound lowlevel functions, sending
  the music or sending/receiving sound to/from the OS specific driver.
  They are in a different file (soundxxx.cc) because they are
  non-portable, while everything in the soundcard code is portable
  */

  bx_sound_lowlevel_c();
  virtual ~bx_sound_lowlevel_c();

  virtual int get_type() {return BX_SOUNDLOW_DUMMY;}

  virtual int waveready();
  virtual int midiready();

  virtual int openmidioutput(const char *mididev);
  virtual int sendmidicommand(int delta, int command, int length, Bit8u data[]);
  virtual int closemidioutput();

  virtual int openwaveoutput(const char *wavedev);
  virtual int startwaveplayback(int frequency, int bits, bx_bool stereo, int format);
  virtual int sendwavepacket(int length, Bit8u data[]);
  virtual int stopwaveplayback();
  virtual int closewaveoutput();

  virtual int openwaveinput(const char *wavedev, sound_record_handler_t rh);
  virtual int startwaverecord(int frequency, int bits, bx_bool stereo, int format);
  virtual int getwavepacket(int length, Bit8u data[]);
  virtual int stopwaverecord();
  virtual int closewaveinput();

  static void record_timer_handler(void *);
  void record_timer(void);

  virtual bx_bool set_wavedata_callback(void *, get_wavedata_cb_t wd_cb) {return 0;}
protected:
  int record_timer_index;
  int record_packet_size;
  sound_record_handler_t record_handler;
};
