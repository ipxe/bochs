/////////////////////////////////////////////////////////////////////////
// $Id$
/////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2011-2017  The Bochs Project
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

// Sound driver loader code

#include "iodev.h"

#if BX_SUPPORT_SOUNDLOW

#include "soundmod.h"
#include "soundlow.h"

#if BX_WITH_SDL || BX_WITH_SDL2
#include <SDL.h>
#endif

#define LOG_THIS bx_soundmod_ctl.

bx_soundmod_ctl_c bx_soundmod_ctl;


bx_soundmod_ctl_c::bx_soundmod_ctl_c()
{
  put("soundctl", "SNDCTL");
  n_sound_drivers = 0;
  soundmod[0].module = NULL;
  waveout = NULL;
}

void bx_soundmod_ctl_c::init()
{
  const char *pwaveout = SIM->get_param_string(BXPN_SOUND_WAVEOUT)->getptr();
  const char *pwavein = SIM->get_param_string(BXPN_SOUND_WAVEIN)->getptr();
  int ret;

  waveout = get_waveout(0);
  if (waveout != NULL) {
    if (!strlen(pwavein)) {
      SIM->get_param_string(BXPN_SOUND_WAVEIN)->set(pwaveout);
    }
    ret = waveout->openwaveoutput(pwaveout);
    if (ret != BX_SOUNDLOW_OK) {
      BX_PANIC(("Could not open wave output device"));
    }
  } else {
    BX_PANIC(("no waveout support present"));
  }
}

void bx_soundmod_ctl_c::exit()
{
  unsigned i, driver_id;

  while (n_sound_drivers > 0) {
    i = --n_sound_drivers;
    driver_id = soundmod[i].drv_id;
    if (driver_id == BX_SOUNDDRV_DUMMY) {
      delete soundmod[i].module;
    } else {
      PLUG_unload_sound_plugin(sound_driver_names[driver_id]);
    }
  }
}

bx_bool bx_soundmod_ctl_c::register_driver(bx_sound_lowlevel_c *module, int driver_id)
{
  unsigned i = n_sound_drivers;

  if (i == BX_MAX_SOUND_DRIVERS) {
    BX_PANIC(("Too many sound drivers!"));
    return 0;
  }
  if (module != NULL) {
    BX_INFO(("Installed sound driver '%s' at index #%d",
             sound_driver_names[driver_id], i));
    soundmod[i].drv_id = driver_id;
    soundmod[i].module = module;
    n_sound_drivers++;
    return 1;
  }
  return 0;
}

bx_sound_lowlevel_c* bx_soundmod_ctl_c::get_driver(int driver_id)
{
  unsigned i, loaded = 0;

  do {
    for (i = 0; i < n_sound_drivers; i++) {
      if (driver_id == soundmod[i].drv_id) {
        return soundmod[i].module;
      }
    }
    if (loaded) return NULL;
    if (i == BX_MAX_SOUND_DRIVERS) {
      BX_PANIC(("Too many sound drivers!"));
      return NULL;
    }
    if (driver_id == BX_SOUNDDRV_DUMMY) {
      bx_sound_lowlevel_c *driver = new bx_sound_dummy_c();
      register_driver(driver, driver_id);
    } else {
      PLUG_load_sound_plugin(sound_driver_names[driver_id]);
    }
    loaded = 1;
  } while (1);
  return NULL;
}

bx_soundlow_waveout_c* bx_soundmod_ctl_c::get_waveout(bx_bool using_file)
{
  bx_sound_lowlevel_c *module = NULL;

  if (!using_file) {
    int driver_id = SIM->get_param_enum(BXPN_SOUND_WAVEOUT_DRV)->get();
    module = get_driver(driver_id);
  } else {
    module = get_driver(BX_SOUNDDRV_FILE);
  }
  if (module != NULL) {
    return module->get_waveout();
  } else {
    return NULL;
  }
}

bx_soundlow_wavein_c* bx_soundmod_ctl_c::get_wavein()
{
  bx_sound_lowlevel_c *module = NULL;
  bx_soundlow_wavein_c *wavein = NULL;

  int driver_id = SIM->get_param_enum(BXPN_SOUND_WAVEIN_DRV)->get();
  module = get_driver(driver_id);
  if (module != NULL) {
    wavein = module->get_wavein();
    if (wavein == NULL) {
      BX_ERROR(("sound service 'wavein' not available - using dummy driver"));
      module = get_driver(BX_SOUNDDRV_DUMMY);
      if (module != NULL) {
        wavein = module->get_wavein();
      }
    }
  }
  return wavein;
}

bx_soundlow_midiout_c* bx_soundmod_ctl_c::get_midiout(bx_bool using_file)
{
  bx_sound_lowlevel_c *module = NULL;
  bx_soundlow_midiout_c *midiout = NULL;

  if (!using_file) {
    int driver_id = SIM->get_param_enum(BXPN_SOUND_MIDIOUT_DRV)->get();
    module = get_driver(driver_id);
  } else {
    module = get_driver(BX_SOUNDDRV_FILE);
  }
  if (module != NULL) {
    midiout = module->get_midiout();
    if (midiout == NULL) {
      BX_ERROR(("sound service 'midiout' not available - using dummy driver"));
      module = get_driver(BX_SOUNDDRV_DUMMY);
      if (module != NULL) {
        midiout = module->get_midiout();
      }
    }
  }
  return midiout;
}

#endif
