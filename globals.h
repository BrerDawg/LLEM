/*
Copyright (C) 2018 BrerDawg

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

//globals.h
//v1.01			
 
#ifndef globals_h
#define globals_h


#define _FILE_OFFSET_BITS 64			//large file handling, must be before all #include...
//#define _LARGE_FILES

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <locale.h>
#include <string>
#include <vector>
#include <wchar.h>
#include <algorithm>
#include <math.h>

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Enumerations.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/Fl_Text_Editor.H>
#include <FL/Fl_JPEG_Image.H>
#include <FL/Fl_File_Input.H>
#include <FL/Fl_Input_Choice.H>

#include "pref.h"
#include "GCProfile.h"
#include "GCLed.h"

//linux code
#ifndef compile_for_windows

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
//#include <X11/Xaw/Form.h>							//64bit
//#include <X11/Xaw/Command.h>						//64bit

#include <dirent.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <syslog.h>		//MakeIniPathFilename(..) needs this
#endif


//windows code
#ifdef compile_for_windows
#include <windows.h>
#include <process.h>
#include <winnls.h>
#include <share.h>
#include <fcntl.h>
#include <io.h>
#include <iostream>
#include <fstream>
#include <conio.h>

#define WC_ERR_INVALID_CHARS 0x0080		//missing from gcc's winnls.h
#endif


#define cn_grph_buf_size 4096			//make this >= 'framecnt'


using namespace std;

#define cnsAppName "LLEM"
#define cnsAppWndName "LLEM - Lunar Lander Excursion Module"

#define cns_build_date build_date       //this holds the build date e.g: 2016 Mar 23, obtained automatically by a shell cmd and sets '-Dbuild_date' option in Makefile, you could manually use e.g: -Dbuild_date="\"2016 Mar 23\""

struct colref
{
int r, g, b;
};

static colref col_blk =   { 0,    0,    0 	};
static colref col_bkgd =   { 64,    64,    64 	};
static colref col_yel = 	{ 255,   255,   0 	};
static colref col_red =	{ 255,   0,     0	};
static colref col_mag =	{ 255,   0,     255	};
static colref col_grey = { 200,   200,   200	};
static colref col_wht =	{ 255,   255,   255	};






struct st_fluidfl_extract			//refer: 'fluidfl_extract_dimensions()', 'fluidfl_find_dimensions()', handy for getting gui ctrl dimensions, see desc at function: 'fluidfl_extract_dimensions()'
{
Fl_Widget *wdg;
string label;
int x;
int y;
int w;
int h;
};



struct st_envlp_table_tag
{
float amp;
float tim;
float delta_amp;
int step_cnt;
int step_cur;
};


struct st_envlp_tag
{
bool running;
int cnt;
int idx;
float cur_amp;

st_envlp_table_tag tbl[10];


//float amp;
//float tim;
//float delta_amp;
//int step_cnt;
//int step_cur;
};







//for fluid.fl
class fluid_wnd : public Fl_Double_Window
{
private:

public:

public:
fluid_wnd( int xx, int yy, int wid, int hei, const char *label );

private:
};






//for fluid.fl
class audio_wnd : public Fl_Double_Window
{
private:

public:

public:
audio_wnd( int xx, int yy, int wid, int hei, const char *label );

private:
};





#endif
