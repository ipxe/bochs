/////////////////////////////////////////////////////////////////////////
// $Id$
/////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2015  The Bochs Project
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
//
/////////////////////////////////////////////////////////////////////////

// Support for sound output to files (based on SB16 code)


class bx_soundlow_waveout_file_c : public bx_soundlow_waveout_c {
public:
  bx_soundlow_waveout_file_c();
  virtual ~bx_soundlow_waveout_file_c();

  virtual int openwaveoutput(const char *wavedev);
  virtual int set_pcm_params(bx_pcm_param_t *param);
  virtual int output(int length, Bit8u data[]);
private:
  FILE *wavefile;
};

class bx_soundlow_midiout_file_c : public bx_soundlow_midiout_c {
public:
  bx_soundlow_midiout_file_c();
  virtual ~bx_soundlow_midiout_file_c();

  virtual int openmidioutput(const char *mididev);
  virtual int sendmidicommand(int delta, int command, int length, Bit8u data[]);

private:
  void writedeltatime(Bit32u deltatime);

  FILE *midifile;
};

class bx_sound_file_c : public bx_sound_lowlevel_c {
public:
  bx_sound_file_c();
  virtual ~bx_sound_file_c() {}

  virtual bx_soundlow_waveout_c* get_waveout();
  virtual bx_soundlow_midiout_c* get_midiout();
};
