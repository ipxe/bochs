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

// Common sound module code and dummy sound lowlevel functions

// Define BX_PLUGGABLE in files that can be compiled into plugins.  For
// platforms that require a special tag on exported symbols, BX_PLUGGABLE
// is used to know when we are exporting symbols and when we are importing.
#define BX_PLUGGABLE

#include "iodev.h"

#if BX_SUPPORT_SOUNDLOW

#include "soundmod.h"
#include "soundlow.h"
#include "soundlnx.h"
#include "soundosx.h"
#include "soundwin.h"
#include "soundsdl.h"
#include "soundalsa.h"
#include "soundfile.h"

#if BX_WITH_SDL || BX_WITH_SDL2
#include <SDL.h>
#endif

#define LOG_THIS theSoundModCtl->

bx_soundmod_ctl_c* theSoundModCtl = NULL;

int CDECL libsoundmod_LTX_plugin_init(plugin_t *plugin, plugintype_t type, int argc, char *argv[])
{
  if (type == PLUGTYPE_CORE) {
    theSoundModCtl = new bx_soundmod_ctl_c;
    bx_devices.pluginSoundModCtl = theSoundModCtl;
    return 0; // Success
  } else {
    return -1;
  }
}

void CDECL libsoundmod_LTX_plugin_fini(void)
{
  delete theSoundModCtl;
}

bx_soundmod_ctl_c::bx_soundmod_ctl_c()
{
  put("soundctl", "SNDCTL");
  n_sound_drivers = 0;
  waveout = NULL;
}

bx_soundmod_ctl_c::~bx_soundmod_ctl_c()
{
  for (unsigned i = 0; i < n_sound_drivers; i++) {
    delete soundmod[i].module;
  }
}

void bx_soundmod_ctl_c::init()
{
  int ret;
  const char *driver = SIM->get_param_string(BXPN_SOUND_DRIVER)->getptr();
  const char *pwaveout = SIM->get_param_string(BXPN_SOUND_WAVEOUT)->getptr();
  const char *wavein = SIM->get_param_string(BXPN_SOUND_WAVEIN)->getptr();

  if (get_driver(driver) != NULL) {
    if (!strlen(wavein)) {
      SIM->get_param_string(BXPN_SOUND_WAVEIN)->set(pwaveout);
    }
    waveout = soundmod[0].module->get_waveout();
    if (waveout != NULL) {
      ret = waveout->openwaveoutput(pwaveout);
      if (ret != BX_SOUNDLOW_OK) {
        BX_PANIC(("Could not open wave output device"));
      }
    } else {
      BX_PANIC(("no waveout support in sound driver '%s'", driver));
    }
  }
}

bx_sound_lowlevel_c* bx_soundmod_ctl_c::get_driver(const char *name)
{
  static const char default_name[] = BX_SOUND_LOWLEVEL_NAME;
  bx_sound_lowlevel_c *driver = NULL;
  unsigned i;

  if (strcmp(name, "default") == 0) {
    name = default_name;
  }
  for (i = 0; i < n_sound_drivers; i++) {
    if (!strcmp(name, soundmod[i].name)) {
      return soundmod[i].module;
    }
  }
  if (i == BX_MAX_SOUND_DRIVERS) {
    BX_PANIC(("Too many sound drivers!"));
    return NULL;
  }
  if (strcmp(name, "dummy") == 0) {
    driver = new bx_sound_lowlevel_c();
  } else if (!strcmp(name, "file")) {
    driver = new bx_sound_file_c();
#if BX_HAVE_ALSASOUND
  } else if (!strcmp(name, "alsa")) {
    driver = new bx_sound_alsa_c();
#endif
#if BX_WITH_SDL || BX_WITH_SDL2
  } else if (!strcmp(name, "sdl")) {
    driver = new bx_sound_sdl_c();
#endif
#if (defined(linux) || defined(__FreeBSD__) || defined(__FreeBSD_kernel__))
  } else if (!strcmp(name, "oss")) {
    driver = new bx_sound_oss_c();
#endif
#if defined(macintosh)
  } else if (!strcmp(name, "osx")) {
    driver = new bx_sound_osx_c();
#endif
#if defined(WIN32)
  } else if (!strcmp(name, "win")) {
    driver = new bx_sound_windows_c();
#endif
  } else {
    BX_PANIC(("unknown lowlevel sound driver '%s'", name));
  }
  if (driver != NULL) {
    BX_INFO(("Installed sound driver '%s' at index #%d", name, i));
    strcpy(soundmod[i].name, name);
    soundmod[i].module = driver;
    n_sound_drivers++;
  }
  return driver;
}

bx_soundlow_waveout_c* bx_soundmod_ctl_c::get_waveout(const char *driver)
{
  bx_sound_lowlevel_c *module = NULL;

  if (!strcmp(driver, "default")) {
    module = soundmod[0].module;
  } else {
    module = get_driver(driver);
  }
  if (module != NULL) {
    return module->get_waveout();
  } else {
    return NULL;
  }
}

bx_soundlow_wavein_c* bx_soundmod_ctl_c::get_wavein(const char *driver)
{
  bx_sound_lowlevel_c *module = NULL;

  if (!strcmp(driver, "default")) {
    module = soundmod[0].module;
  } else {
    module = get_driver(driver);
  }
  if (module != NULL) {
    return module->get_wavein();
  } else {
    return NULL;
  }
}

bx_soundlow_midiout_c* bx_soundmod_ctl_c::get_midiout(const char *driver)
{
  bx_sound_lowlevel_c *module = NULL;

  if (!strcmp(driver, "default")) {
    module = soundmod[0].module;
  } else {
    module = get_driver(driver);
  }
  if (module != NULL) {
    return module->get_midiout();
  } else {
    return NULL;
  }
}

/* Handlers for the VOC file output */

// Write the header of the VOC file.

void bx_soundmod_ctl_c::VOC_init_file(FILE *stream)
{
  struct {
    char id[20];
    Bit16u headerlen;  // All in LITTLE Endian!
    Bit16u version;
    Bit16u chksum;
  } vocheader =
    { "Creative Voice File",
#ifdef BX_LITTLE_ENDIAN
      0x1a, 0x0114, 0x111f };
#else
      0x1a00, 0x1401, 0x1f11 };
#endif

  vocheader.id[19] = 26;    // Replace string end with 26

  fwrite(&vocheader, 1, sizeof vocheader, stream);
}

// write one block to the VOC file
void bx_soundmod_ctl_c::VOC_write_block(FILE *stream, int block,
                           Bit32u headerlen, Bit8u header[],
                           Bit32u datalen, Bit8u data[])
{
  Bit32u i;

  if (block > 9) {
      BX_ERROR(("VOC Block %d not recognized, ignored.", block));
      return;
  }

  fputc(block, stream);

  i = headerlen + datalen;
#ifdef BX_LITTLE_ENDIAN
  fwrite(&i, 1, 3, stream);       // write the length in 24-bit little endian
#else
  Bit8u lengthbytes[3];
  lengthbytes[0] = i & 0xff; i >>= 8;
  lengthbytes[1] = i & 0xff; i >>= 8;
  lengthbytes[2] = i & 0xff;
  fwrite(lengthbytes, 1, 3, stream);
#endif
  BX_DEBUG(("Voc block %d; Headerlen %d; Datalen %d",
            block, headerlen, datalen));
  if (headerlen > 0)
    fwrite(header, 1, headerlen, stream);
  if (datalen > 0)
    fwrite(data, 1, datalen, stream);
}

#endif
