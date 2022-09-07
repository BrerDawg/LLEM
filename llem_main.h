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

//llem_main.h
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
#include <unistd.h>						//for 'readlink()'

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

#include "globals.h"
#include "pref.h"
#include "GCProfile.h"
#include "GCLed.h"
//#include "fluid.h"
//#include "my_input_wheel.h"
#include "mgraph.h"
#include "gc_rtaudio.h"
#include "rt_code.h"
#include "filter_code.h"
//#include "gc_srateconv.h"
//#include "audio_formats.h"
//#include "fft_code.h"
#include "antialias_code.h"
#include "llem_code.h"
#include "freeverb_code.h"




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


using namespace std;


#define cnFontEditor 4
#define cnSizeEditor 12

#define cnGap 2
#define cnCslHei 270

#define cnsLogName "log.txt"						//log filename
#define cnsLogSwapName "log_swap.txt"	//log swap file for culling older entries at begining of log file

//linux code
#ifndef compile_for_windows
#define LLU "llu"
#endif


//windows code
#ifdef compile_for_windows
#define LLU "I64u"
#endif


#define cns_help_filename "help.txt"

//linux code
#ifndef compile_for_windows
#define cns_open_editor "open_editor.sh"		// !! don't use white spaces
#endif

//windows code
#ifdef compile_for_windows
#define cns_open_editor "open_editor.bat"		// !! don't use white spaces
#endif







//use this class as it trys to hold un-maximised window size values by detecting a
//large window resize that is close to sreen resolution, see dble_wnd::resise()
class dble_wnd : public Fl_Double_Window
{
private:										//private var
bool ctrl_key, shift_key;
bool left_button;
bool right_button;
bool middle_button;
int mousewheel;



public:											//public var
bool dble_wnd_verbose;
int maximize_boundary_x, maximize_boundary_y, maximize_boundary_w, maximize_boundary_h;     //see dble_wnd::resize()
int restore_size_x, restore_size_y, restore_size_w, restore_size_h;                         //see dble_wnd::resize()

public:											//public functions
dble_wnd( int xx, int yy, int wid, int hei, const char *label = 0 );
~dble_wnd();

private:										//private functions
int handle( int e );
void resize( int xx, int yy, int ww, int hh );

};












class mywnd : public dble_wnd
{
private:										//private var
int *buf;
int ctrl_key;
int left_button;
string dropped_str;

int mousewheel;
Fl_Box *bx_image;
Fl_JPEG_Image *jpg;


public:											//public var
int a_public_var;

public:											//public functions
mywnd( int xx, int yy, int wid, int hei, const char *label );
~mywnd();
void init();

private:										//private functions
void draw();
int handle( int );
void setcolref( colref col );

};




