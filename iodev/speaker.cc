/////////////////////////////////////////////////////////////////////////
// $Id$
/////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2003       David N. Welton <davidw@dedasys.com>.
//  Copyright (C) 2003-2013  The Bochs Project
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

#define BX_PLUGGABLE

#include "iodev.h"
#include "speaker.h"

#if BX_SUPPORT_SOUNDLOW
#include "sound/soundmod.h"
#endif

#ifdef __linux__
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/kd.h>
#endif

#define LOG_THIS theSpeaker->

bx_speaker_c *theSpeaker= NULL;

int libspeaker_LTX_plugin_init(plugin_t *plugin, plugintype_t type, int argc, char *argv[])
{
  theSpeaker = new bx_speaker_c();
  bx_devices.pluginSpeaker = theSpeaker;
  BX_REGISTER_DEVICE_DEVMODEL(plugin, type, theSpeaker, BX_PLUGIN_SPEAKER);
  return(0); // Success
}

void libspeaker_LTX_plugin_fini(void)
{
  bx_devices.pluginSpeaker = &bx_devices.stubSpeaker;
  delete theSpeaker;
}

bx_speaker_c::bx_speaker_c()
{
  put("speaker", "SPEAK");

  beep_frequency = 0.0; // Off
#ifdef __linux__
  consolefd = -1;
#endif
}

bx_speaker_c::~bx_speaker_c()
{
  beep_off();
#ifdef __linux__
  if (consolefd >= 0) {
    close(consolefd);
  }
#endif
  BX_DEBUG(("Exit"));
}

void bx_speaker_c::init(void)
{
  outputinit = 0;
#if defined(WIN32)
  useBeep = FALSE;
#endif
}

void bx_speaker_c::reset(unsigned type)
{
  if (!outputinit) {
    outputinit = 1;
#if BX_SUPPORT_SOUNDLOW
    if (DEV_soundmod_beep_off()) {
      BX_INFO(("Using lowlevel sound support for output"));
      return;
    }
#endif
#ifdef __linux__
    consolefd = open("/dev/console", O_WRONLY);
    if (consolefd != -1) {
      BX_INFO(("Using /dev/console for output"));
    } else {
      BX_ERROR(("Failed to open /dev/console: %s", strerror(errno)));
      BX_ERROR(("Deactivating beep on console"));
    }
#elif defined(WIN32)
    BX_INFO(("Using system beep for output"));
    useBeep = TRUE;
#endif
  }

  beep_off();
}

void bx_speaker_c::beep_on(float frequency)
{
#if defined(WIN32)
  if (useBeep && (beep_frequency != 0.0)) {
    beep_off();
  }
#endif
  beep_frequency = frequency;

#if BX_SUPPORT_SOUNDLOW
  if (DEV_soundmod_beep_on(frequency))
    return;
#endif
#ifdef __linux__
  if (consolefd != -1) {
    this->info("pc speaker on with frequency %f", frequency);
    ioctl(consolefd, KIOCSOUND, (int)(clock_tick_rate/frequency));
  }
#elif defined(WIN32)
  if (useBeep) {
    usec_start = bx_pc_system.time_usec();
  }
#endif

  // give the gui a chance to signal beep on
  bx_gui->beep_on(frequency);
}

#if defined(WIN32)

static struct {
  DWORD frequency;
  DWORD msec;
} beep_info;

DWORD WINAPI BeepThread(LPVOID)
{
  static BOOL threadActive = FALSE;

  while (threadActive) Sleep(10);
  threadActive = TRUE;
  Beep(beep_info.frequency, beep_info.msec);
  threadActive = FALSE;
  return 0;
}

#endif

void bx_speaker_c::beep_off()
{
#if BX_SUPPORT_SOUNDLOW
  if (DEV_soundmod_beep_off())
    return;
#endif
  if (beep_frequency != 0.0) {
#ifdef __linux__
    if (consolefd != -1) {
      ioctl(consolefd, KIOCSOUND, 0);
    }
#elif defined(WIN32)
    // FIXME: sound should start at beep_on() and end here
    if (useBeep) {
      DWORD threadID;
      beep_info.msec = (DWORD)((bx_pc_system.time_usec() - usec_start) / 1000);
      beep_info.frequency = (DWORD)beep_frequency;
      CreateThread(NULL, 0, BeepThread, NULL, 0, &threadID);
    }
#endif

    // give the gui a chance to signal beep off
    bx_gui->beep_off();

    beep_frequency = 0.0;
  }
}
