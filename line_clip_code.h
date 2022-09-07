/*
Copyright (C) 2021 BrerDawg

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




#ifndef line_clip_code_h
#define line_clip_code_h


//line_clip_code.h
//v1.01			

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

//#include <FL/glut.H>	// changed for fltk
//#include <FL/glu.h>     // added for fltk

#include "globals.h"
#include "GCProfile.h"
//#include "my_input_wheel.h"
#include "filter_code.h"
#include "mgraph.h"

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



//clip_vportleft = -w()/2 + border;						//upward is positive, for: 'line_clip()', 'clip_poly()' algorithm
//clip_vportright = + w()/2 - border;
//clip_vporttop = h()/2 - border;					//NOTE: 'clip_vporttop' here is in an increasing direction up the screen
//clip_vportbot = -h()/2 +border;




enum en_lineclip_tag								//for cohen-sutherland line clip algorithm
{
en_lineclip_top = 0x1,								//must be binary for anding logic
en_lineclip_bot = 0x2, 
en_lineclip_right = 0x4,
en_lineclip_left = 0x8,
};





using namespace std;


class line_clip
{
private:


public:
int clip_vportleft;						//upward is positive, for: 'line_clip()', 'clip_poly()' algorithm
int clip_vportright;
int clip_vporttop;						//NOTE: 'clip_vporttop' here is in an increasing direction up the screen
int clip_vportbot;


public:
line_clip();
bool line_clip_int( int &x1_in, int &y1_in, int &x2_in, int &y2_in );


private:
int compute_outcode_int( int xx, int yy );

};







#endif

