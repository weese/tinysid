/*
 *  prefs.cpp - Preferences items
 *
 *  SIDPlayer (C) Copyright 1996-2004 Christian Bauer
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "prefs.h"


// List of preferences items
prefs_desc common_prefs_items[] = {
    {"victype", TYPE_STRING, false,     "type number of VIC-II to emulate (6569, 6567 or 6567R5)", NULL},
    {"sidtype", TYPE_STRING, false,     "type number of SID to emulate (6581 or 8580)", NULL},
    {"samplerate", TYPE_INT32, false,   "output sample rate in Hz", NULL},
    {"audio16bit", TYPE_BOOLEAN, false, "16-bit audio output", NULL},
    {"stereo", TYPE_BOOLEAN, false,     "stereo audio output", NULL},
    {"filters", TYPE_BOOLEAN, false,    "emulate SID filters", NULL},
    {"dualsid", TYPE_BOOLEAN, false,    "emulate dual SID chips", NULL},
    {"audioeffect", TYPE_INT32, false,  "audio effect type (0 = none, 1 = reverb, 2 = spatial)", NULL},
    {"revdelay", TYPE_INT32, false,     "effect delay in ms", NULL},
    {"revfeedback", TYPE_INT32, false,  "effect feedback (0..256 = 0..100%)", NULL},
    {"volume", TYPE_INT32, false,       "master volume (0..256 = 0..100%)", NULL},
    {"v1volume", TYPE_INT32, false,     "volume voice 1 (0..256 = 0..100%)", NULL},
    {"v2volume", TYPE_INT32, false,     "volume voice 2 (0..256 = 0..100%)", NULL},
    {"v3volume", TYPE_INT32, false,     "volume voice 3 (0..256 = 0..100%)", NULL},
    {"v4volume", TYPE_INT32, false,     "volume sampled voice (0..256 = 0..100%)", NULL},
    {"v1pan", TYPE_INT32, false,        "panning voice 1 (-256..256 = left..right)", NULL},
    {"v2pan", TYPE_INT32, false,        "panning voice 2 (-256..256 = left..right)", NULL},
    {"v3pan", TYPE_INT32, false,        "panning voice 3 (-256..256 = left..right)", NULL},
    {"v4pan", TYPE_INT32, false,        "panning sampled voice (-256..256 = left..right)", NULL},
    {"dualsep", TYPE_INT32, false,      "dual SID stereo separation (0..256 = 0..100%)", NULL},
    {"speed", TYPE_INT32, false,        "replay speed adjustment (percent)", NULL},
    {NULL, TYPE_END, false, NULL, NULL}    // End of list
};


/*
 *  Set default values for preferences items
 */

void AddPrefsDefaults()
{
    PrefsAddString("victype", "6569");
    PrefsAddString("sidtype", "6581");
    PrefsAddInt32("samplerate", 44100);
    PrefsAddBool("audio16bit", true);
    PrefsAddBool("stereo", true);
    PrefsAddBool("filters", false);
    PrefsAddBool("dualsid", false);
    PrefsAddInt32("audioeffect", 2);
    PrefsAddInt32("revdelay", 125);
    PrefsAddInt32("revfeedback", 0x50);
    PrefsAddInt32("volume", 0x100);
    PrefsAddInt32("v1volume", 0x100);
    PrefsAddInt32("v1pan", -0x40);
    PrefsAddInt32("v2volume", 0x100);
    PrefsAddInt32("v2pan", 0);
    PrefsAddInt32("v3volume", 0x100);
    PrefsAddInt32("v3pan", 0x40);
    PrefsAddInt32("v4volume", 0x100);
    PrefsAddInt32("v4pan", 0);
    PrefsAddInt32("dualsep", 0x80);
    PrefsAddInt32("speed", 100);
}
