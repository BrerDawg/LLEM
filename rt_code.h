/*
Copyright (C) 2019 BrerDawg

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/


//rt_code.h

//v1.01





#ifndef rt_code_h
#define rt_code_h

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <locale.h>
#include <string>
#include <vector>
#include <wchar.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>

#include "globals.h"
#include "GCProfile.h"
#include "gc_rtaudio.h"
#include "filter_code.h"
#include "freeverb_code.h"
//#include "gc_srateconv.h"

//linux code
#ifndef compile_for_windows
#define LLU "llu"
#endif


//windows code
#ifdef compile_for_windows
#define LLU "I64u"
#endif




using namespace std;



extern int cb_audio_proc_rtaudio( void *bf_out, void *bf_in, int frames, double streamTime, RtAudioStreamStatus status, void *arg_in );




#endif

