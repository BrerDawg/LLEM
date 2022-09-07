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

//llem_main.cpp


//v1.01		2021-dec-05			//





//mingw needs library -lmsvcp60 for mcrtomb type calls:
//from Makefile.win that dev-c++ uses, these params were used to compile and link:
//LIBS =  -L"C:/Dev-Cpp/lib" -lmsvcp60 -lfltk_images -lfltk_jpeg -mwindows -lfltk -lole32 -luuid -lcomctl32 -lwsock32 -lm 
//INCS =  -I"C:/Dev-Cpp/include" 
//CXXINCS =  -I"C:/Dev-Cpp/lib/gcc/mingw32/3.4.2/include"  -I"C:/Dev-Cpp/include/c++/3.4.2/backward"  -I"C:/Dev-Cpp/include/c++/3.4.2/mingw32"  -I"C:/Dev-Cpp/include/c++/3.4.2"  -I"C:/Dev-Cpp/include" 
//CXXFLAGS = $(CXXINCS)  
//CFLAGS = $(INCS) -DWIN32 -mms-bitfields 

//#define compile_for_windows	//!!!!!!!!!!! uncomment for a dos/windows compile

//use -mconsole in make's LIBS shown above for windows to use the dos console, rather a attaching a console

#include "llem_main.h"
#include "iconapp.xpm"



//asm("int3");						//useful to trigger debugger






//global objs
//dble_wnd *wndMain;
llem_wnd *wnd_llem = 0;

Fl_RGB_Image *theapp_icon0;

PrefWnd* pref_wnd2=0;



//global vars
int mains_font;
int mains_fontsize;

string csIniFilename;
int iBorderWidth;
int iBorderHeight;
int gi0,gi1,gi2,gi3,gi4;
double gd0,gd1,gd2,gd3,gd4;
string app_path;						//path this app resides on
string dir_seperator="\\";				//assume dos\windows folder directory seperator
int mode;
int font_num = cnFontEditor;
int font_size = cnSizeEditor;
unsigned long long int ns_tim_start1;	//a ns timer start val
double perf_period;						//used for windows timings in timing_start( ), timing_stop( )
string fav_editor;

int pref_invert_mousewheel = 0;
int pref_allow_bounce_collision_det = 0;
int pref_audio_bounce_gain = 30;
int pref_audio_explosion_gain = 80;
int pref_audio_bass_boost = 1;
int pref_audio_gain = 100;
int pref_audio_reverb_on = 1;
int pref_audio_reverb_room_size = 30;
int pref_audio_reverb_damping = 70;
int pref_audio_reverb_width = 100;
int pref_audio_reverb_dry_wet = 30;
int pref_development_mode = 0;
int pref_audio_fryingpan_plosive_gain = 100;
int pref_distortion_gain = 100;

int pref_expertise0 = 0;			//refer: 'landing_rating_speed_limit[]'
int pref_expertise1 = 1;
int pref_expertise2 = 0;
int pref_expertise3 = 0;
int pref_expertise4 = 0;


bool mute = 0;



int iAppExistDontRun = 1;	//if set, app probably exist and user doesn't want to run another
							//set it to assume we don't want to run another incase user closes
							//wnd without pressing either 'Run Anyway' or 'Don't Run, Exit' buttons




string sdebug;							//holds all cslpf output 
bool my_verbose = 1;


float timer_period = 0.02f;

mystr tim1;
rtaud rta;																//rtaudio
st_rtaud_arg_tag st_rta_arg;											//this is used in audio proc callback to work out chan in/out counts

int srate = 48000;
int framecnt = 1024;

float time_per_sample;
bool audio_started = 0;
int audio_source = 2;

float audio_gain = 2.0f;
float audio_thrust_gain = 0.0f;
float audio_thrust_gain_slewed = audio_thrust_gain;						//slew rate limited, adj'd by rt audio callback to match 'audio_thrust_gain'
//float audio_thrust_gain_slewed_zeroed = audio_thrust_gain_slewed;
float audio_explosion_gain = 1.5f;

float filt_freq0 = 1000;
float filt_q0 = 2;
float play_speed = 0.5;

st_iir_2nd_order_tag iir0;												//thrust filter
st_iir_2nd_order_tag iir1;												//
st_iir_2nd_order_tag iir2;												//thrust subsonic filter
st_iir_2nd_order_tag iir20;												//explosion filter
st_iir_2nd_order_tag iir30;												//explosion filter
st_iir_2nd_order_tag iir40;												//frying pan noise src filter

float gain_iir0 = 1.0f;
float gain_iir1 = 1.0f;
float gain_iir2 = 1.0f;
float gain_iir20 = 1.0f;
float gain_iir30 = 1.0f;
float gain_iir40 = 1.0f;


float *revb_bf0 = 0;
float *revb_bf1 = 0;
float *revb_bf10 = 0;
float *revb_bf11 = 0;
int revb_size0;
int revb_size1;
int revb_size10;
int revb_size11;
int revb_rd0 = 1;
int revb_wr0 = 0;
int revb_rd1 = 1;
int revb_wr1 = 0;

int revb_rd10 = 1;
int revb_wr10 = 0;
int revb_rd11 = 1;
int revb_wr11 = 0;

string slast_wave_fname;
string slast_ship_fname;
string slast_moonscape_fname;
string sship_state_fname;

audio_formats af0;
st_audio_formats_tag saf0;
bool aud_loaded = 0;
uint64_t audptr = 0;
double dfract_audptr = 0;

int iflag_nan = 0;				//rt code debug


st_envlp_tag st_envlp0[10];		//used for explosion sound
st_envlp_tag st_envlp1[10];
st_envlp_tag st_envlp2[10];		//used for bounce sound

string slast_highscore_name;

 
//function prototypes
void LoadSettings(string csIniFilename);
void SaveSettings(string csIniFilename);
int CheckInstanceExists(string csAppName);
void open_editor( string fname );
int RunShell( string sin );




//callbacks
void cslpf( const char *fmt,... );
void cb_wndmain( Fl_Callback *, void* v);
void cb_btAbout(Fl_Widget *, void *);
void cb_btOpen(Fl_Widget *, void *);
void cb_btSave(Fl_Widget *, void *);
void cb_btQuit(Fl_Widget *, void *);
void cb_timer1(void *);
void cb_pref2(Fl_Widget*w, void* v);
void cb_show_mywnd(Fl_Widget*w, void* v);
//void cb_font_pref(Fl_Widget*w, void* v);
void cb_open_folder(Fl_Widget *, void *);
void cb_open_file(Fl_Widget *, void *);
void log_callback( string s );

void cb_user2( void *o, int row, int ctrl );

void cb_edit_undo(Fl_Widget *, void *);
void cb_edit_redo(Fl_Widget *, void *);
void cb_edit_cut(Fl_Widget *, void *);
void cb_edit_copy(Fl_Widget *, void *);
void cb_edit_paste(Fl_Widget *, void *);

bool start_audio();
void stop_audio();
bool open_audio_file( string sfname );
void graph_update( vector<float> &v0, vector<float> &v1, vector<float> &v2 );

void create_filter_iir( en_filter_pass_type_tag filt_type, float filt_freq_in, float filt_q_in, st_iir_2nd_order_tag &iir );


bool grph_bf_loaded = 0;												//set when audio proc has loaded data

float grph_bf0[cn_grph_buf_size];
float grph_bf1[cn_grph_buf_size];
float grph_bf2[cn_grph_buf_size];


freeverb_reverb *fvb = 0;











//make sure 'picked' is not the same string as 'pathfilename'
//string 'picked' is loaded with selected path and filename, on 'OK' 
//on 'Cancel' string 'picked' is set to a null string
//returns 1 if 'OK', else 0
//set 'type' to Fl_File_Chooser::CREATE to allow a new filename to be entered

//linux code
#ifndef compile_for_windows
bool my_file_chooser( string &picked, const char* title, const char* pat, const char* start_path_filename, int type, Fl_Font fnt = -1, int fntsze = -1 )
{
picked = "";

//show file chooser
Fl_File_Chooser fc	( 	 start_path_filename,		// directory
						 pat,                       // filter
						 Fl_File_Chooser::SINGLE | type,   // chooser type
						 title						// title
					);

if ( fnt != -1 )fc.textfont( fnt );
if ( fntsze != -1 )fc.textsize( fntsze );

fc.show();

while( fc.shown() )
	{
	Fl::wait();
	}


if( fc.value() == 0 ) return 0;


picked = fc.value();

//windows code
//#ifdef compile_for_windows
//make the slash suit Windows OS
//mystr m1;
//m1 = fc.value();
//m1.FindReplace( picked, "/", "\\",0);
//#endif


return 1;
}
#endif












//windows code
#ifdef compile_for_windows
bool my_file_chooser( string &picked, const char* title, const char* pat, const char* start_path_filename, int type, Fl_Font fnt = -1, int fntsze = -1 )
{
OPENFILENAME ofn;
char szFile[ 8192 ];
string fname;

mystr m1;

m1 = start_path_filename;

m1.ExtractFilename( dir_seperator[ 0 ],  fname );		 //remove path from filename

strncpy( szFile, fname.c_str(), sizeof( szFile ) );		//put supplied fname as default

memset( &ofn, 0, sizeof( ofn ) );

ofn.lStructSize = sizeof ( ofn );
ofn.hwndOwner = NULL ;
ofn.lpstrFile = szFile ;
//ofn.lpstrFile[ 0 ] = '\0';
ofn.nMaxFile = sizeof( szFile );
ofn.lpstrFilter = "All\0*.*\0Text\0*.TXT\0";
ofn.nFilterIndex = 1;
ofn.lpstrFileTitle = 0;
ofn.nMaxFileTitle = 0;
ofn.lpstrInitialDir = start_path_filename ;
ofn.lpstrTitle = title;
ofn.Flags = OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR ;

if( type == Fl_File_Chooser::CREATE )
	{
	if( GetSaveFileName( &ofn ) )
		{
		picked = szFile;
		return 1;
		}
	}
else{
	if( GetOpenFileName( &ofn ) )
		{
		picked = szFile;
		return 1;
		}
	}
return 0;
}
#endif




















//linux code
#ifndef compile_for_windows
//make sure 'picked' is not the same string as 'pathfilename'
//string 'picked' is loaded with selected dir, on 'OK'
//on 'Cancel' string 'picked' is set to a null string
//returns 1 if 'OK', else 0
//set 'type' to  Fl_File_Chooser::CREATE to allow a new directory to be entered
bool my_dir_chooser( string &picked, const char* title, const char* pat, const char* start_path_filename, int type, Fl_Font fnt = -1, int fntsze = -1 )
{
picked = "";
mystr m1;

//show file chooser
Fl_File_Chooser fc	( 	 start_path_filename,				// directory
						 pat,                     		  	// filter
						 Fl_File_Chooser::DIRECTORY | type,	// chooser type
						 title								// title
					);

if ( fnt != -1 )fc.textfont( fnt );
if ( fntsze != -1 )fc.textsize( fntsze );

fc.show();

while( fc.shown() )
	{
	Fl::wait();
	}


if( fc.value() == 0 ) return 0;


picked = fc.value();

//make the slash suit Windows OS
//m1 = fc.value();
//m1.FindReplace( picked, "/", "\\",0);



return 1;
}
#endif












//windows code
#ifdef compile_for_windows



int CALLBACK BrowseCallbackProc( HWND hwnd, UINT uMsg, LPARAM lp, LPARAM pData ) 
{
TCHAR szDir[MAX_PATH];

switch( uMsg ) 
	{
	case BFFM_INITIALIZED: 
	if ( GetCurrentDirectory( sizeof(szDir) / sizeof(TCHAR), szDir ) )
		{
         // WParam is TRUE since you are passing a path.
         // It would be FALSE if you were passing a pidl.
		SendMessage(hwnd, BFFM_SETSELECTION, TRUE, (LPARAM)szDir);
		}
	break;

	case BFFM_SELCHANGED: 
	// Set the status window to the currently selected path.
	if (SHGetPathFromIDList((LPITEMIDLIST) lp ,szDir))
		{
		SendMessage(hwnd,BFFM_SETSTATUSTEXT,0,(LPARAM)szDir);
		}
	break;
   }
return 0;
}








/*

//this returns the PIDL for a particular folder, can be use with GetFolderSelection(..) code further below,
//if used in 'bi.pidlRoot', it only shows folders below the 'path', you can't seem to navigate upward past 'path'
LPITEMIDLIST get_pidl_from_path( string path )
{
LPITEMIDLIST  pidl = 0;
LPSHELLFOLDER pDesktopFolder;
char		szPath[MAX_PATH];
OLECHAR		olePath[MAX_PATH];
char		szDisplayName[MAX_PATH];
ULONG		chEaten;
ULONG		dwAttributes;
HRESULT		hr;



// 
// Get a pointer to the Desktop's IShellFolder interface.
// 
if ( SUCCEEDED( SHGetDesktopFolder( &pDesktopFolder ) ) )
	{
	// 
	// IShellFolder::ParseDisplayName requires the file name be in
	// Unicode.
	// 
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, path.c_str(), -1, olePath, MAX_PATH );

	// 
	// Convert the path to an ITEMIDLIST.
	// 
	hr = pDesktopFolder->ParseDisplayName( NULL, NULL, olePath, &chEaten, &pidl, &dwAttributes );

	if ( FAILED( hr ) )
		{
		pidl = 0;
		printf( "grrr() - failed\n" );
        // Handle error.
		}

	// 
	// pidl now contains a pointer to an ITEMIDLIST for .\readme.txt.
	// This ITEMIDLIST needs to be freed using the IMalloc allocator
	// returned from SHGetMalloc().
	// 

	//release the desktop folder object
	pDesktopFolder->Release();
	}

return 	pidl;
}

*/












//Displays a directory selection dialog. CoInitialize must be called before calling this function.
//szBuf must be MAX_PATH characters in length. hWnd may be NULL.
//Note: the get_pidl_from_path(..) call is commented out
BOOL GetFolderSelection( HWND hWnd, LPTSTR szBuf, LPCTSTR szTitle, string starting_path )
{
LPITEMIDLIST pidl     = NULL;
BROWSEINFO   bi       = { 0 };
BOOL         bResult  = FALSE;

bi.hwndOwner      = hWnd;
bi.pszDisplayName = szBuf;
bi.pidlRoot       = 0;//  get_pidl_from_path( starting_path );		//get_pidl_from_path(..) is not used as it only shows folders below the path, you can't navigate upward past 'pidlRoot'
bi.lpszTitle      = szTitle;											
bi.ulFlags        = BIF_RETURNONLYFSDIRS | BIF_USENEWUI;
bi.lpfn = BrowseCallbackProc;


if ((pidl = SHBrowseForFolder(&bi)) != NULL)
	{
	bResult = SHGetPathFromIDList(pidl, szBuf);
	CoTaskMemFree(pidl);
	}

return bResult;
}











bool my_dir_chooser( string &picked, const char* title, const char* pat, const char* start_path_filename, int type, Fl_Font fnt = -1, int fntsze = -1 )
{
OPENFILENAME ofn;
char szFile[ 8192 ];



TCHAR szFolder[ MAX_PATH ];
CoInitialize( NULL );

if( GetFolderSelection( NULL, szFolder, title, start_path_filename ))
	{
	printf( "The folder selected was %s\n", szFolder );
	picked = szFolder;
	return 1;
	}

return 0;



/*
memset( &ofn, 0, sizeof( ofn ) );

ofn.lStructSize = sizeof ( ofn );
ofn.hwndOwner = NULL ;
ofn.lpstrFile = szFile ;
ofn.lpstrFile[ 0 ] = '\0';
ofn.nMaxFile = sizeof( szFile );
ofn.lpstrFilter = "All\0*.*\0Text\0*.TXT\0";
ofn.nFilterIndex =1;
ofn.lpstrFileTitle = 0;
ofn.nMaxFileTitle = 0;
ofn.lpstrInitialDir = start_path_filename ;
ofn.lpstrTitle = title;
ofn.Flags = OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST ;
GetOpenFileName( &ofn );

picked = szFile;
*/

}
#endif















void cb_file_open( Fl_Widget *w, void* v )
{
string s1, s2;

//linux code
#ifndef compile_for_windows
s1 = "/mnt/home/PuppyLinux/MyPrj/Skeleton Unicode fltk - George";
#endif

//windows code
#ifdef compile_for_windows
s1 = "c:\\gc\\MyPrj\\Skeleton Unicode fltk - George";
#endif

/*
if( my_file_chooser( s2, "Open File", "*", s1.c_str(), 0, font_num, font_size ) )
	{
	u_wnd->fi_filename->value( s2.c_str() );
	}


u_wnd->fc = new ucde_file_chooser( s1.c_str(), 0, FL_SINGLE, "Open a file" );
u_wnd->fc->textfont( font_num );
u_wnd->fc->textsize( font_size );
u_wnd->fc->callback( cb_ucde_file_chooser, 0 );

//fc->redraw();
u_wnd->fc->show();

while( u_wnd->fc->shown() )
	{
	Fl::wait();
	}
GCCol.o:  GCCol.h

u_wnd->fi_filename->value( u_wnd->fc->value() );
//u_wnd->fi_filename->value("Help!");
*/

//fc->hide();
//fc->show();
//fc->fileName->show();

}





















void cb_dir_open( Fl_Widget *w, void* v )
{
string s1, s2;
//linux code
#ifndef compile_for_windows
s1 = "/mnt/home/PuppyLinux/MyPrj/Skeleton Unicode fltk - George";
#endif

//windows code
#ifdef compile_for_windows
s1 = "c:\\gc\\MyPrj\\Skeleton Unicode fltk - George";
#endif

/*
if( my_dir_chooser( s2, "Open Dir", "*", s1.c_str(), 0, font_num, font_size ) )
	{
	u_wnd->fi_filename->value( s2.c_str() );
	}
*/
return;

char *ptr;

ptr = fl_dir_chooser( "Open Dir", s1.c_str() );
/*
if( ptr )
	{
	u_wnd->fi_filename->value( ptr );
	}
*/
}












/*
bool check_file_exists( string fname )
{
FILE *fp;

mystr m1;

FILE* wcfopen( wstring &wstr );





//linux code
#ifndef compile_for_windows
//cslpf("src file exists: %s \n", fname.c_str() );

fp = fopen( fname.c_str() ,"rb" );		 //open file
#endif



//windows code
#ifdef compile_for_windows
wstring ws1;
mystr m1 = fname;

m1.mbcstr_wcstr( ws1 );	//convert utf8 string to windows wchar string array

fp = _wfsopen( ws1.c_str(), L"rb", _SH_DENYNO );
#endif


if( fp == 0 )
	{
	return 0;
	}


fclose( fp );
return 1;
}

*/














//----------------------------------------------------------




mywnd::mywnd( int xx, int yy, int wid, int hei, const char *label ) : dble_wnd( xx, yy, wid ,hei, label )
{
buf = 0;
jpg = 0;

init();

jpg = new Fl_JPEG_Image( "earthmap.jpg" );      // load jpeg image into ram
printf("Image dimensions w=%d, h=%d, depth=%d\n", jpg->w(), jpg->h(), jpg->d() );

bx_image = new Fl_Box( 50, h() - 50, jpg->w(), jpg->h() );


bx_image->image( jpg );

}





mywnd::~mywnd()
{
if ( buf != 0 ) delete buf;
printf(" \nFreeing memory...\n ");

if ( jpg != 0 ) delete jpg;

}





void mywnd::init()
{
ctrl_key = 0;
mousewheel = 0;

if ( buf != 0 ) delete buf;

buf = new int [ 100 ];

printf(" \nAllocating memory...\n ");
}





void mywnd::setcolref( colref col )
{
fl_color( col.r , col.g , col.b );
}







void mywnd::draw()
{
int rght,bot;
string s1;


Fl_Double_Window::draw();


//clear wnd
setcolref( col_bkgd );
fl_rectf( 0 , 0 , w() , h() - 50 );

setcolref( col_yel );

strpf( s1, "Dropped something here: %s", dropped_str.c_str() );
fl_draw( s1.c_str(), 5, 14 );


if ( ctrl_key == 1 ) s1 ="ControlKey: down";
else s1 ="ControlKey: up";
fl_draw( s1.c_str(), 5, 42 );


if ( left_button == 1 ) s1 ="LeftButton: down";
else s1 ="LeftButton: up";
setcolref( col_yel );
fl_draw( s1.c_str(), 5, 56 );

strpf( s1,"MouseWheel Val = %d" , mousewheel );
fl_draw( s1.c_str(), 5, 70 );

strpf( s1,"You should see a map below... ");
fl_draw( s1.c_str(), 5, 98 );

setcolref( col_mag );
fl_line( 50, 70, 80 , h() );


//bx_image->draw( 0, 0 );

}









//int bDonePaste = 0;		//need this to be 0 to make dragging from web browser in linux work


int mywnd::handle( int e )
{
bool need_redraw = 0;
bool dont_pass_on = 0;


#ifdef compile_for_windows 
bDonePaste = 1;					//does not seem to be required for Windows, so nullify by setting to 1
#endif

if ( e == FL_PASTE )	//needed below code because on drag release the first FL_PASTE call does not have valid text as yet,
	{					//possibly because of a delay in X windows, so have used Fl:paste(..) to send another paste event and 
//	if( bDonePaste == 0)	//this seems to work ok (does not seem to happen in Windows)
	if( 0 )
		{
		Fl::paste( *this, 0 );					//passing second var as 0 uses currently selected text, (not prev cut text)
//		printf("\nDropped1\n" );
//		bDonePaste = 1;
		}
	else{
//		bDonePaste = 0;
		string s = Fl::event_text();
		int len = Fl::event_length();
		printf("\nDropped Len=%d, Str=%s\n", len, s.c_str() );
		if( len )								//anything dropped/pasted?
			{
			dropped_str = s;
			need_redraw = 1;
			}
		}
//	return 1;
	dont_pass_on = 1;
	}


if (e == FL_DND_DRAG)
	{
	printf("\nDrag\n");
	dont_pass_on = 1;
	}

if (e == FL_DND_ENTER)
	{
	printf("\nDrag Enter\n");
	dont_pass_on = 1;
	}

if (e == FL_DND_RELEASE)
	{
	printf("\nDrag Release\n");
	dont_pass_on = 1;
	}




if ( e == FL_PUSH )
	{
	if(Fl::event_button()==1)
		{
		left_button = 1;
		need_redraw = 1;
		}
	dont_pass_on = 1;
	}

if ( e == FL_RELEASE )
	{
	
	if(Fl::event_button()==1)
		{
		left_button = 0;
		need_redraw = 1;
		}
	dont_pass_on = 1;
	}


if ( ( e == FL_KEYDOWN ) || ( e == FL_SHORTCUT ) )					//key pressed?
	{
	int key = Fl::event_key();
	
	if( ( key == FL_Control_L ) || (  key == FL_Control_R ) ) ctrl_key = 1;
	
	need_redraw = 1;
	dont_pass_on = 1;
	}


if ( e == FL_KEYUP )												//key release?
	{
	int key = Fl::event_key();
	
	if( ( key == FL_Control_L ) || ( key == FL_Control_R ) ) ctrl_key = 0;

	need_redraw = 1;
	dont_pass_on = 1;
	}


if ( e == FL_MOUSEWHEEL )
	{
	mousewheel = Fl::event_dy();
	
	need_redraw = 1;
	dont_pass_on = 1;
	}
	
if ( need_redraw ) redraw();

if( dont_pass_on ) return 1;

return Fl_Double_Window::handle(e);
}


//----------------------------------------------------------














//----------------------------------------------------------

dble_wnd::dble_wnd( int xx, int yy, int wid, int hei, const char *label ) : Fl_Double_Window( xx, yy, wid ,hei, label )
{
ctrl_key = shift_key = 0;

dble_wnd_verbose = 0;
maximize_boundary_x = 5;                //assume this window's title bar and borders, see dble_wnd::resize()
maximize_boundary_y = 20;
maximize_boundary_w = 10;
maximize_boundary_h = 25;

}







dble_wnd::~dble_wnd()
{

}












//try to determine if window maximize was pressed, store previous size so we can save final un-maximized values,
//makes assumption on when a window has been maximized using window title bar, border widths and actual screen resolution,
//you need to change maximize_boundary_x, maximize_boundary_y, maximize_boundary_w, maximize_boundary_h, if your window title bar/borders are different

void dble_wnd::resize( int xx, int yy, int ww, int hh )
{

//assuming screen: 0, 0, 1920, 1080
//got this resize when maximize button was pressed: 5, y: 20, w: 1910, h: 1055
if( dble_wnd_verbose ) printf( "dble_wnd::resize() - x: %d, y: %d, w: %d, h: %d\n", xx, yy, ww, hh );


int xxx, yyy, www, hhh;

Fl::screen_xywh( xxx, yyy, www, hhh );
if( dble_wnd_verbose ) printf( "dble_wnd::resize() - screen_xywh() is x: %d, y: %d, w: %d, h: %d\n", xxx, yyy, www, hhh );

if( xx >= xxx + maximize_boundary_x )                                 //detect a possible the maximize button press happened
    {
    if( yy >= ( yyy + maximize_boundary_y ) )
        {
        if( ww >= ( www - maximize_boundary_w ) )
            {
            if( hh >= ( hhh - maximize_boundary_h ) )
                {
                if( dble_wnd_verbose ) printf( "dble_wnd::resize() - possibly maximize was pressed, storing original pos/size for later\n" );

                goto probably_maximized;
                }
            }
        }
   
    }


restore_size_x = xx;                    
restore_size_y = yy;
restore_size_w = ww;
restore_size_h = hh;

Fl_Double_Window::resize( xx, yy, ww, hh );

if( dble_wnd_verbose ) printf( "dble_wnd::resize() - actually resizing to x: %d, y: %d, w: %d, h: %d\n", xx, yy, www, hh );


probably_maximized:

return;
}






int dble_wnd::handle( int e )
{
string s1;
bool need_redraw = 0;
bool dont_pass_on = 0;


if ( e == FL_LEAVE )
	{
//	take_focus();
//if( pref_main_wnd_auto_take_focus ) Fl::focus( this );
//	Fl::focus( this->parent() );
	dont_pass_on = 0;
	}

if ( e == FL_KEYDOWN )						//key press?
	{
	int key = Fl::event_key();

	printf( "dble_wnd::handle() - key: 0x%02x\n", key );
		
	if( key == FL_Enter )		//is it CR ?
		{
		}
	need_redraw = 1;
    dont_pass_on = 1;
	}


if ( e == FL_KEYUP )												//key release?
	{
	int key = Fl::event_key();
	
	if( ( key == FL_Control_L ) || ( key == FL_Control_R ) ) ctrl_key = 0;
	if( ( key == FL_Shift_L ) || (  key == FL_Shift_R ) ) shift_key = 0;

	need_redraw = 1;
    dont_pass_on = 0;
	}


if ( e == FL_MOUSEWHEEL )
	{
	mousewheel = Fl::event_dy();
//	printf( "dble_wnd::handle() - mousewheel: %d\n", mousewheel );

	need_redraw = 1;
    dont_pass_on = 0;
	}


if ( need_redraw ) redraw();

if( dont_pass_on ) return 1;

return Fl_Double_Window::handle(e);

}
//----------------------------------------------------------













 







void cb_edit_undo(Fl_Widget *, void *)
{
printf( "cb_edit_undo()\n" );
}



void cb_edit_redo(Fl_Widget *, void *)
{
printf( "cb_edit_redo()\n" );
}




void cb_edit_cut(Fl_Widget *, void *)
{
//Fl_Text_Editor::kf_cut( 0, calc_wnd->te_tally );

printf( "cb_edit_cut()\n" );
}



void cb_edit_copy(Fl_Widget *, void *)
{
//Fl_Text_Editor::kf_copy( 0, calc_wnd->te_tally );

printf( "cb_edit_copy()\n" );
}


void cb_edit_paste(Fl_Widget *, void *)
{
//Fl_Text_Editor::kf_paste( 0, calc_wnd->te_tally );

printf( "cb_edit_paste()\n" );
}








void cb_pref2(Fl_Widget*w, void* v)
{
if( pref_wnd2 ) pref_wnd2->Show(1);
}










void make_pref2_wnd()
{
sControl sc;

if(pref_wnd2==0)
	{
	pref_wnd2 = new PrefWnd( wnd_llem->x() + 20, wnd_llem->y() + 20, 730, 510,"Preferences","Settings","PrefWnd2Pos");
	}
else{
	pref_wnd2->Show(1);
	return;
	}


// -- dont need to do the below manual default load as "ClearToDefCtrl()" does this for you --

pref_wnd2->ClearToDefCtrl();			//this will clear sc struct to safe default values
pref_wnd2->AddControl();
pref_wnd2->CreateRow(10);			//specify optional row height

/*
pref_wnd->ClearToDefCtrl();			//this will clear sc struct to safe default values

pref_wnd2->sc.type=cnMenuChoicePref;
pref_wnd2->sc.x=120;
pref_wnd2->sc.y=0;
pref_wnd2->sc.w=150;
pref_wnd2->sc.h=20;
pref_wnd2->sc.label="Initial Execution:";
pref_wnd2->sc.label_type = FL_NORMAL_LABEL;      //label effects such a FL_EMBOSSED_LABEL, FL_ENGRAVED_LABEL, FL_SHADOW_LABEL
pref_wnd2->sc.label_align = FL_ALIGN_LEFT;
pref_wnd2->sc.tooltip="";						//tool tip
pref_wnd2->sc.options="&None,&Step into main(...),&Run";	//menu button drop down options
pref_wnd2->sc.labelfont=-1;						//-1 means use fltk default
pref_wnd2->sc.labelsize=-1;						//-1 means use fltk default
pref_wnd2->sc.textfont=-1;//fl_font();
pref_wnd2->sc.textsize=-1;//fl_size();
pref_wnd2->sc.section="MyPref";
pref_wnd2->sc.key="InitExec";
pref_wnd2->sc.keypostfix=-1;					//ini file Key post fix
pref_wnd2->sc.def="0";							//default to use if ini value not avail
pref_wnd2->sc.iretval=&gi0;						//address of int to be modified, -1 means none
pref_wnd2->sc.dretval=(double*)-1;				//address of double to be modified, -1 means none
pref_wnd2->sc.sretval=(string*)-1;				//address of string to be modified, -1 means none
pref_wnd2->sc.cb = cb_user2;					//address of a callback if any, 0 means none
pref_wnd2->sc.dynamic = 1;						//allow immediate dynamic change of user var
pref_wnd2->sc.uniq_id = 0;                      //allows identification of an actual control, rather that using its row and column values, don't use 0xffffffff
pref_wnd2->AddControl();

pref_wnd2->CreateRow(25);			//specify optional row height
*/



pref_wnd2->ClearToDefCtrl();			//this will clear sc struct to safe default values

pref_wnd2->sc.type=cnCheckPref;
pref_wnd2->sc.x=200;
pref_wnd2->sc.y=0;
pref_wnd2->sc.w=18;
pref_wnd2->sc.h=18;
pref_wnd2->sc.label="Invert Mousewheel";
pref_wnd2->sc.label_type = FL_NORMAL_LABEL;      //label effects such a FL_EMBOSSED_LABEL, FL_ENGRAVED_LABEL, FL_SHADOW_LABEL
pref_wnd2->sc.label_align = FL_ALIGN_LEFT;
pref_wnd2->sc.tooltip="invert mousewheel thrust control";						//tool tip
pref_wnd2->sc.options="";						//menu button drop down options
pref_wnd2->sc.labelfont=4;						//-1 means use fltk default
pref_wnd2->sc.labelsize=10;						//-1 means use fltk default
pref_wnd2->sc.textfont=4;
pref_wnd2->sc.textsize=10;
pref_wnd2->sc.section="Settings";
pref_wnd2->sc.key="pref_invert_mousewheel";
pref_wnd2->sc.keypostfix=-1;					//ini file Key post fix
pref_wnd2->sc.def="0";							//default to use if ini value not avail
pref_wnd2->sc.iretval=&pref_invert_mousewheel;						//address of int to be modified, -1 means none
pref_wnd2->sc.dretval=(double*)-1;				//address of double to be modified, -1 means none
pref_wnd2->sc.sretval=(string*)-1;				//address of string to be modified, -1 means none
pref_wnd2->sc.cb = cb_user2;					//address of a callback if any, 0 means none
pref_wnd2->sc.dynamic = 1;						//allow immediate dynamic change of user var
pref_wnd2->sc.uniq_id = 0;                      //allows identification of an actual control, rather that using its row and column values, don't use 0xffffffff
pref_wnd2->AddControl();

pref_wnd2->CreateRow(20);					//specify optional row height




pref_wnd2->ClearToDefCtrl();			//this will clear sc struct to safe default values

pref_wnd2->sc.type=cnCheckPref;
pref_wnd2->sc.x=200;
pref_wnd2->sc.y=0;
pref_wnd2->sc.w=18;
pref_wnd2->sc.h=18;
pref_wnd2->sc.label="Allow Lateral Bounce Collisions";
pref_wnd2->sc.label_type = FL_NORMAL_LABEL;      //label effects such a FL_EMBOSSED_LABEL, FL_ENGRAVED_LABEL, FL_SHADOW_LABEL
pref_wnd2->sc.label_align = FL_ALIGN_LEFT;
pref_wnd2->sc.tooltip="allow lateral landing bounce which may result in collisions or skidding off a landing zone";						//tool tip
pref_wnd2->sc.options="";						//menu button drop down options
pref_wnd2->sc.labelfont=4;						//-1 means use fltk default
pref_wnd2->sc.labelsize=10;						//-1 means use fltk default
pref_wnd2->sc.textfont=4;
pref_wnd2->sc.textsize=10;
pref_wnd2->sc.section="Settings";
pref_wnd2->sc.key="pref_allow_bounce_collision_det";
pref_wnd2->sc.keypostfix=-1;					//ini file Key post fix
pref_wnd2->sc.def="1";							//default to use if ini value not avail
pref_wnd2->sc.iretval=&pref_allow_bounce_collision_det;						//address of int to be modified, -1 means none
pref_wnd2->sc.dretval=(double*)-1;				//address of double to be modified, -1 means none
pref_wnd2->sc.sretval=(string*)-1;				//address of string to be modified, -1 means none
pref_wnd2->sc.cb = cb_user2;					//address of a callback if any, 0 means none
pref_wnd2->sc.dynamic = 1;						//allow immediate dynamic change of user var
pref_wnd2->sc.uniq_id = 1;                      //allows identification of an actual control, rather that using its row and column values, don't use 0xffffffff
pref_wnd2->AddControl();

pref_wnd2->CreateRow(20);					//specify optional row height





pref_wnd2->ClearToDefCtrl();					//this will clear sc struct to safe default values

pref_wnd2->sc.type=cnInputIntPref;
pref_wnd2->sc.x= 200;
pref_wnd2->sc.y=0;
pref_wnd2->sc.w=30;
pref_wnd2->sc.h=18;
pref_wnd2->sc.label="Bounce Audio Gain";
pref_wnd2->sc.label_type = FL_NORMAL_LABEL;      //label effects such a FL_EMBOSSED_LABEL, FL_ENGRAVED_LABEL, FL_SHADOW_LABEL
pref_wnd2->sc.label_align = FL_ALIGN_LEFT;
pref_wnd2->sc.tooltip="landing bounce audio gain, enter a percentage value between 0-->100";						//tool tip
pref_wnd2->sc.options="";						//menu button drop down options
pref_wnd2->sc.labelfont=4;						//-1 means use fltk default
pref_wnd2->sc.labelsize=10;						//-1 means use fltk default
pref_wnd2->sc.textfont=4;
pref_wnd2->sc.textsize=10;
pref_wnd2->sc.section="Settings";
pref_wnd2->sc.key="pref_audio_bounce_gain";
pref_wnd2->sc.keypostfix=-1;					//ini file Key post fix
pref_wnd2->sc.def="30";							//default to use if ini value not avail
pref_wnd2->sc.iretval=&pref_audio_bounce_gain; //address of int to be modified, -1 means none
pref_wnd2->sc.dretval=(double*)-1;				//address of double to be modified, -1 means none
pref_wnd2->sc.sretval=(string*)-1;				//address of string to be modified, -1 means none
pref_wnd2->sc.cb = cb_user2;					//address of a callback if any, 0 means none
pref_wnd2->sc.dynamic = 1;						//allow immediate dynamic change of user var
pref_wnd2->sc.uniq_id = 2;                      //allows identification of an actual control, rather that using its row and column values, don't use 0xffffffff
pref_wnd2->AddControl();

pref_wnd2->CreateRow(25);						//specify optional row height






pref_wnd2->ClearToDefCtrl();					//this will clear sc struct to safe default values

pref_wnd2->sc.type=cnInputIntPref;
pref_wnd2->sc.x= 200;
pref_wnd2->sc.y=0;
pref_wnd2->sc.w=30;
pref_wnd2->sc.h=18;
pref_wnd2->sc.label="Explosion Audio Gain";
pref_wnd2->sc.label_type = FL_NORMAL_LABEL;      //label effects such a FL_EMBOSSED_LABEL, FL_ENGRAVED_LABEL, FL_SHADOW_LABEL
pref_wnd2->sc.label_align = FL_ALIGN_LEFT;
pref_wnd2->sc.tooltip="explosion audio gain, enter a percentage value between 0-->150";						//tool tip
pref_wnd2->sc.options="";						//menu button drop down options
pref_wnd2->sc.labelfont=4;						//-1 means use fltk default
pref_wnd2->sc.labelsize=10;						//-1 means use fltk default
pref_wnd2->sc.textfont=4;
pref_wnd2->sc.textsize=10;
pref_wnd2->sc.section="Settings";
pref_wnd2->sc.key="pref_audio_explosion_gain";
pref_wnd2->sc.keypostfix=-1;					//ini file Key post fix
pref_wnd2->sc.def="100";							//default to use if ini value not avail
pref_wnd2->sc.iretval=&pref_audio_explosion_gain; //address of int to be modified, -1 means none
pref_wnd2->sc.dretval=(double*)-1;				//address of double to be modified, -1 means none
pref_wnd2->sc.sretval=(string*)-1;				//address of string to be modified, -1 means none
pref_wnd2->sc.cb = cb_user2;					//address of a callback if any, 0 means none
pref_wnd2->sc.dynamic = 1;						//allow immediate dynamic change of user var
pref_wnd2->sc.uniq_id = 3;                      //allows identification of an actual control, rather that using its row and column values, don't use 0xffffffff
pref_wnd2->AddControl();

pref_wnd2->CreateRow(25);						//specify optional row height





pref_wnd2->ClearToDefCtrl();					//this will clear sc struct to safe default values

pref_wnd2->sc.type=cnInputIntPref;
pref_wnd2->sc.x= 200;
pref_wnd2->sc.y=0;
pref_wnd2->sc.w=30;
pref_wnd2->sc.h=18;
pref_wnd2->sc.label="Thrust Plosive Gain";
pref_wnd2->sc.label_type = FL_NORMAL_LABEL;      //label effects such a FL_EMBOSSED_LABEL, FL_ENGRAVED_LABEL, FL_SHADOW_LABEL
pref_wnd2->sc.label_align = FL_ALIGN_LEFT;
pref_wnd2->sc.tooltip="thrust frying pan plosive gain, 0-->150";						//tool tip
pref_wnd2->sc.options="";						//menu button drop down options
pref_wnd2->sc.labelfont=4;						//-1 means use fltk default
pref_wnd2->sc.labelsize=10;						//-1 means use fltk default
pref_wnd2->sc.textfont=4;
pref_wnd2->sc.textsize=10;
pref_wnd2->sc.section="Settings";
pref_wnd2->sc.key="pref_audio_fryingpan_plosive_gain";
pref_wnd2->sc.keypostfix=-1;					//ini file Key post fix
pref_wnd2->sc.def="100";							//default to use if ini value not avail
pref_wnd2->sc.iretval=&pref_audio_fryingpan_plosive_gain; //address of int to be modified, -1 means none
pref_wnd2->sc.dretval=(double*)-1;				//address of double to be modified, -1 means none
pref_wnd2->sc.sretval=(string*)-1;				//address of string to be modified, -1 means none
pref_wnd2->sc.cb = cb_user2;					//address of a callback if any, 0 means none
pref_wnd2->sc.dynamic = 1;						//allow immediate dynamic change of user var
pref_wnd2->sc.uniq_id = 9;                      //allows identification of an actual control, rather that using its row and column values, don't use 0xffffffff
pref_wnd2->AddControl();

pref_wnd2->CreateRow(25);						//specify optional row height




pref_wnd2->ClearToDefCtrl();					//this will clear sc struct to safe default values

pref_wnd2->sc.type=cnInputIntPref;
pref_wnd2->sc.x= 200;
pref_wnd2->sc.y=0;
pref_wnd2->sc.w=30;
pref_wnd2->sc.h=18;
pref_wnd2->sc.label="Thrust Plosive Distortion";
pref_wnd2->sc.label_type = FL_NORMAL_LABEL;      //label effects such a FL_EMBOSSED_LABEL, FL_ENGRAVED_LABEL, FL_SHADOW_LABEL
pref_wnd2->sc.label_align = FL_ALIGN_LEFT;
pref_wnd2->sc.tooltip="thrust frying pan plosive distortion level, 0-->150";						//tool tip
pref_wnd2->sc.options="";						//menu button drop down options
pref_wnd2->sc.labelfont=4;						//-1 means use fltk default
pref_wnd2->sc.labelsize=10;						//-1 means use fltk default
pref_wnd2->sc.textfont=4;
pref_wnd2->sc.textsize=10;
pref_wnd2->sc.section="Settings";
pref_wnd2->sc.key="pref_distortion_gain";
pref_wnd2->sc.keypostfix=-1;					//ini file Key post fix
pref_wnd2->sc.def="90";							//default to use if ini value not avail
pref_wnd2->sc.iretval=&pref_distortion_gain; //address of int to be modified, -1 means none
pref_wnd2->sc.dretval=(double*)-1;				//address of double to be modified, -1 means none
pref_wnd2->sc.sretval=(string*)-1;				//address of string to be modified, -1 means none
pref_wnd2->sc.cb = cb_user2;					//address of a callback if any, 0 means none
pref_wnd2->sc.dynamic = 1;						//allow immediate dynamic change of user var
pref_wnd2->sc.uniq_id = 9;                      //allows identification of an actual control, rather that using its row and column values, don't use 0xffffffff
pref_wnd2->AddControl();

pref_wnd2->CreateRow(25);						//specify optional row height




pref_wnd2->ClearToDefCtrl();			//this will clear sc struct to safe default values

pref_wnd2->sc.type=cnCheckPref;
pref_wnd2->sc.x=200;
pref_wnd2->sc.y=0;
pref_wnd2->sc.w=18;
pref_wnd2->sc.h=18;
pref_wnd2->sc.label="Thrust Bass Boost";
pref_wnd2->sc.label_type = FL_NORMAL_LABEL;      //label effects such a FL_EMBOSSED_LABEL, FL_ENGRAVED_LABEL, FL_SHADOW_LABEL
pref_wnd2->sc.label_align = FL_ALIGN_LEFT;
pref_wnd2->sc.tooltip="boost audio bass rumble at higher thrust levels";						//tool tip
pref_wnd2->sc.options="";						//menu button drop down options
pref_wnd2->sc.labelfont=4;						//-1 means use fltk default
pref_wnd2->sc.labelsize=10;						//-1 means use fltk default
pref_wnd2->sc.textfont=4;
pref_wnd2->sc.textsize=10;
pref_wnd2->sc.section="Settings";
pref_wnd2->sc.key="pref_audio_bass_boost";
pref_wnd2->sc.keypostfix=-1;					//ini file Key post fix
pref_wnd2->sc.def="1";							//default to use if ini value not avail
pref_wnd2->sc.iretval=&pref_audio_bass_boost;						//address of int to be modified, -1 means none
pref_wnd2->sc.dretval=(double*)-1;				//address of double to be modified, -1 means none
pref_wnd2->sc.sretval=(string*)-1;				//address of string to be modified, -1 means none
pref_wnd2->sc.cb = cb_user2;					//address of a callback if any, 0 means none
pref_wnd2->sc.dynamic = 1;						//allow immediate dynamic change of user var
pref_wnd2->sc.uniq_id = 4;                      //allows identification of an actual control, rather that using its row and column values, don't use 0xffffffff
pref_wnd2->AddControl();

pref_wnd2->CreateRow(20);					//specify optional row height






pref_wnd2->ClearToDefCtrl();			//this will clear sc struct to safe default values

pref_wnd2->sc.type=cnCheckPref;
pref_wnd2->sc.x=200;
pref_wnd2->sc.y=0;
pref_wnd2->sc.w=18;
pref_wnd2->sc.h=18;
pref_wnd2->sc.label="Reverb Enable";
pref_wnd2->sc.label_type = FL_NORMAL_LABEL;     //label effects such a FL_EMBOSSED_LABEL, FL_ENGRAVED_LABEL, FL_SHADOW_LABEL
pref_wnd2->sc.label_align = FL_ALIGN_LEFT;
pref_wnd2->sc.tooltip="enable reverb effect";	//tool tip
pref_wnd2->sc.options="";						//menu button drop down options
pref_wnd2->sc.labelfont=4;						//-1 means use fltk default
pref_wnd2->sc.labelsize=10;						//-1 means use fltk default
pref_wnd2->sc.textfont=4;
pref_wnd2->sc.textsize=10;
pref_wnd2->sc.section="Settings";
pref_wnd2->sc.key="pref_audio_reverb_on";
pref_wnd2->sc.keypostfix=-1;					//ini file Key post fix
pref_wnd2->sc.def="1";							//default to use if ini value not avail
pref_wnd2->sc.iretval=&pref_audio_reverb_on;						//address of int to be modified, -1 means none
pref_wnd2->sc.dretval=(double*)-1;				//address of double to be modified, -1 means none
pref_wnd2->sc.sretval=(string*)-1;				//address of string to be modified, -1 means none
pref_wnd2->sc.cb = cb_user2;					//address of a callback if any, 0 means none
pref_wnd2->sc.dynamic = 1;						//allow immediate dynamic change of user var
pref_wnd2->sc.uniq_id = 5;                      //allows identification of an actual control, rather that using its row and column values, don't use 0xffffffff
pref_wnd2->AddControl();

pref_wnd2->CreateRow(20);					//specify optional row height




pref_wnd2->ClearToDefCtrl();					//this will clear sc struct to safe default values

pref_wnd2->sc.type=cnInputIntPref;
pref_wnd2->sc.x= 200;
pref_wnd2->sc.y=0;
pref_wnd2->sc.w=30;
pref_wnd2->sc.h=18;
pref_wnd2->sc.label="Reverb Room Size";
pref_wnd2->sc.label_type = FL_NORMAL_LABEL;      //label effects such a FL_EMBOSSED_LABEL, FL_ENGRAVED_LABEL, FL_SHADOW_LABEL
pref_wnd2->sc.label_align = FL_ALIGN_LEFT;
pref_wnd2->sc.tooltip="0 for small room, 100 for large room, 0-->100";						//tool tip
pref_wnd2->sc.options="";						//menu button drop down options
pref_wnd2->sc.labelfont=4;						//-1 means use fltk default
pref_wnd2->sc.labelsize=10;						//-1 means use fltk default
pref_wnd2->sc.textfont=4;
pref_wnd2->sc.textsize=10;
pref_wnd2->sc.section="Settings";
pref_wnd2->sc.key="pref_audio_reverb_room_size";
pref_wnd2->sc.keypostfix=-1;					//ini file Key post fix
pref_wnd2->sc.def="30";							//default to use if ini value not avail
pref_wnd2->sc.iretval=&pref_audio_reverb_room_size; //address of int to be modified, -1 means none
pref_wnd2->sc.dretval=(double*)-1;				//address of double to be modified, -1 means none
pref_wnd2->sc.sretval=(string*)-1;				//address of string to be modified, -1 means none
pref_wnd2->sc.cb = cb_user2;					//address of a callback if any, 0 means none
pref_wnd2->sc.dynamic = 1;						//allow immediate dynamic change of user var
pref_wnd2->sc.uniq_id = 6;                      //allows identification of an actual control, rather that using its row and column values, don't use 0xffffffff
pref_wnd2->AddControl();

pref_wnd2->CreateRow(25);						//specify optional row height



pref_wnd2->ClearToDefCtrl();					//this will clear sc struct to safe default values

pref_wnd2->sc.type=cnInputIntPref;
pref_wnd2->sc.x= 200;
pref_wnd2->sc.y=0;
pref_wnd2->sc.w=30;
pref_wnd2->sc.h=18;
pref_wnd2->sc.label="Reverb Damping";
pref_wnd2->sc.label_type = FL_NORMAL_LABEL;      //label effects such a FL_EMBOSSED_LABEL, FL_ENGRAVED_LABEL, FL_SHADOW_LABEL
pref_wnd2->sc.label_align = FL_ALIGN_LEFT;
pref_wnd2->sc.tooltip="0 is minimal damping, 100 max damping, 0-->100";						//tool tip
pref_wnd2->sc.options="";						//menu button drop down options
pref_wnd2->sc.labelfont=4;						//-1 means use fltk default
pref_wnd2->sc.labelsize=10;						//-1 means use fltk default
pref_wnd2->sc.textfont=4;
pref_wnd2->sc.textsize=10;
pref_wnd2->sc.section="Settings";
pref_wnd2->sc.key="pref_audio_reverb_damping";
pref_wnd2->sc.keypostfix=-1;					//ini file Key post fix
pref_wnd2->sc.def="70";							//default to use if ini value not avail
pref_wnd2->sc.iretval=&pref_audio_reverb_damping; //address of int to be modified, -1 means none
pref_wnd2->sc.dretval=(double*)-1;				//address of double to be modified, -1 means none
pref_wnd2->sc.sretval=(string*)-1;				//address of string to be modified, -1 means none
pref_wnd2->sc.cb = cb_user2;					//address of a callback if any, 0 means none
pref_wnd2->sc.dynamic = 1;						//allow immediate dynamic change of user var
pref_wnd2->sc.uniq_id = 7;                      //allows identification of an actual control, rather that using its row and column values, don't use 0xffffffff
pref_wnd2->AddControl();

pref_wnd2->CreateRow(25);						//specify optional row height





pref_wnd2->ClearToDefCtrl();					//this will clear sc struct to safe default values

pref_wnd2->sc.type=cnInputIntPref;
pref_wnd2->sc.x= 200;
pref_wnd2->sc.y=0;
pref_wnd2->sc.w=30;
pref_wnd2->sc.h=18;
pref_wnd2->sc.label="Reverb Width";
pref_wnd2->sc.label_type = FL_NORMAL_LABEL;      //label effects such a FL_EMBOSSED_LABEL, FL_ENGRAVED_LABEL, FL_SHADOW_LABEL
pref_wnd2->sc.label_align = FL_ALIGN_LEFT;
pref_wnd2->sc.tooltip="0 is minimum, 100 max, 0-->100";						//tool tip
pref_wnd2->sc.options="";						//menu button drop down options
pref_wnd2->sc.labelfont=4;						//-1 means use fltk default
pref_wnd2->sc.labelsize=10;						//-1 means use fltk default
pref_wnd2->sc.textfont=4;
pref_wnd2->sc.textsize=10;
pref_wnd2->sc.section="Settings";
pref_wnd2->sc.key="pref_audio_reverb_width";
pref_wnd2->sc.keypostfix=-1;					//ini file Key post fix
pref_wnd2->sc.def="100";							//default to use if ini value not avail
pref_wnd2->sc.iretval=&pref_audio_reverb_width; //address of int to be modified, -1 means none
pref_wnd2->sc.dretval=(double*)-1;				//address of double to be modified, -1 means none
pref_wnd2->sc.sretval=(string*)-1;				//address of string to be modified, -1 means none
pref_wnd2->sc.cb = cb_user2;					//address of a callback if any, 0 means none
pref_wnd2->sc.dynamic = 1;						//allow immediate dynamic change of user var
pref_wnd2->sc.uniq_id = 8;                      //allows identification of an actual control, rather that using its row and column values, don't use 0xffffffff
pref_wnd2->AddControl();

pref_wnd2->CreateRow(25);						//specify optional row height





pref_wnd2->ClearToDefCtrl();					//this will clear sc struct to safe default values

pref_wnd2->sc.type=cnInputIntPref;
pref_wnd2->sc.x= 200;
pref_wnd2->sc.y=0;
pref_wnd2->sc.w=30;
pref_wnd2->sc.h=18;
pref_wnd2->sc.label="Reverb Effect Mix";
pref_wnd2->sc.label_type = FL_NORMAL_LABEL;      //label effects such a FL_EMBOSSED_LABEL, FL_ENGRAVED_LABEL, FL_SHADOW_LABEL
pref_wnd2->sc.label_align = FL_ALIGN_LEFT;
pref_wnd2->sc.tooltip="dry/wet mix ratio, 0 is dry, 100 is wet, 0-->100";						//tool tip
pref_wnd2->sc.options="";						//menu button drop down options
pref_wnd2->sc.labelfont=4;						//-1 means use fltk default
pref_wnd2->sc.labelsize=10;						//-1 means use fltk default
pref_wnd2->sc.textfont=4;
pref_wnd2->sc.textsize=10;
pref_wnd2->sc.section="Settings";
pref_wnd2->sc.key="pref_audio_reverb_dry_wet";
pref_wnd2->sc.keypostfix=-1;					//ini file Key post fix
pref_wnd2->sc.def="30";							//default to use if ini value not avail
pref_wnd2->sc.iretval=&pref_audio_reverb_dry_wet; //address of int to be modified, -1 means none
pref_wnd2->sc.dretval=(double*)-1;				//address of double to be modified, -1 means none
pref_wnd2->sc.sretval=(string*)-1;				//address of string to be modified, -1 means none
pref_wnd2->sc.cb = cb_user2;					//address of a callback if any, 0 means none
pref_wnd2->sc.dynamic = 1;						//allow immediate dynamic change of user var
pref_wnd2->sc.uniq_id = 9;                      //allows identification of an actual control, rather that using its row and column values, don't use 0xffffffff
pref_wnd2->AddControl();

pref_wnd2->CreateRow(25);						//specify optional row height




pref_wnd2->ClearToDefCtrl();					//this will clear sc struct to safe default values

pref_wnd2->sc.type=cnInputIntPref;
pref_wnd2->sc.x= 200;
pref_wnd2->sc.y=0;
pref_wnd2->sc.w=30;
pref_wnd2->sc.h=18;
pref_wnd2->sc.label="Audio Gain";
pref_wnd2->sc.label_type = FL_NORMAL_LABEL;      //label effects such a FL_EMBOSSED_LABEL, FL_ENGRAVED_LABEL, FL_SHADOW_LABEL
pref_wnd2->sc.label_align = FL_ALIGN_LEFT;
pref_wnd2->sc.tooltip="enter a percentage value between 0-->200";						//tool tip
pref_wnd2->sc.options="";						//menu button drop down options
pref_wnd2->sc.labelfont=4;						//-1 means use fltk default
pref_wnd2->sc.labelsize=10;						//-1 means use fltk default
pref_wnd2->sc.textfont=4;
pref_wnd2->sc.textsize=10;
pref_wnd2->sc.section="Settings";
pref_wnd2->sc.key="pref_audio_gain";
pref_wnd2->sc.keypostfix=-1;					//ini file Key post fix
pref_wnd2->sc.def="80";							//default to use if ini value not avail
pref_wnd2->sc.iretval=&pref_audio_gain;			//address of int to be modified, -1 means none
pref_wnd2->sc.dretval=(double*)-1;				//address of double to be modified, -1 means none
pref_wnd2->sc.sretval=(string*)-1;				//address of string to be modified, -1 means none
pref_wnd2->sc.cb = cb_user2;					//address of a callback if any, 0 means none
pref_wnd2->sc.dynamic = 1;						//allow immediate dynamic change of user var
pref_wnd2->sc.uniq_id = 10;                      //allows identification of an actual control, rather that using its row and column values, don't use 0xffffffff
pref_wnd2->AddControl();

pref_wnd2->CreateRow(25);						//specify optional row height



pref_wnd2->ClearToDefCtrl();			//this will clear sc struct to safe default values

pref_wnd2->sc.type=cnCheckPref;
pref_wnd2->sc.x=200;
pref_wnd2->sc.y=0;
pref_wnd2->sc.w=18;
pref_wnd2->sc.h=18;
pref_wnd2->sc.label="Development Mode";
pref_wnd2->sc.label_type = FL_NORMAL_LABEL;     //label effects such a FL_EMBOSSED_LABEL, FL_ENGRAVED_LABEL, FL_SHADOW_LABEL
pref_wnd2->sc.label_align = FL_ALIGN_LEFT;
pref_wnd2->sc.tooltip="enables llem ship or moonscape object editing";	//tool tip
pref_wnd2->sc.options="";						//menu button drop down options
pref_wnd2->sc.labelfont=4;						//-1 means use fltk default
pref_wnd2->sc.labelsize=10;						//-1 means use fltk default
pref_wnd2->sc.textfont=4;
pref_wnd2->sc.textsize=10;
pref_wnd2->sc.section="Settings";
pref_wnd2->sc.key="pref_development_mode";
pref_wnd2->sc.keypostfix=-1;					//ini file Key post fix
pref_wnd2->sc.def="0";							//default to use if ini value not avail
pref_wnd2->sc.iretval=&pref_development_mode;						//address of int to be modified, -1 means none
pref_wnd2->sc.dretval=(double*)-1;				//address of double to be modified, -1 means none
pref_wnd2->sc.sretval=(string*)-1;				//address of string to be modified, -1 means none
pref_wnd2->sc.cb = cb_user2;					//address of a callback if any, 0 means none
pref_wnd2->sc.dynamic = 1;						//allow immediate dynamic change of user var
pref_wnd2->sc.uniq_id = 11;                      //allows identification of an actual control, rather that using its row and column values, don't use 0xffffffff
pref_wnd2->AddControl();

pref_wnd2->CreateRow(20);					//specify optional row height



pref_wnd2->ClearToDefCtrl();			//this will clear sc struct to safe default values

pref_wnd2->sc.type=cnStaticTextPref;
pref_wnd2->sc.x=200-75;
pref_wnd2->sc.y=0;
pref_wnd2->sc.w=150;
pref_wnd2->sc.h=18;
pref_wnd2->sc.radio_group_name = "";
pref_wnd2->sc.label="-- Expertise Level --";
pref_wnd2->sc.label_type = FL_NORMAL_LABEL;     //label effects such a FL_EMBOSSED_LABEL, FL_ENGRAVED_LABEL, FL_SHADOW_LABEL
pref_wnd2->sc.label_align = FL_ALIGN_CENTER;
pref_wnd2->sc.tooltip="select expertise level, land with a downward velocity (dy) not passing the indicated limit,\nhigher expertise requires a lower touchdown velocity for a successful landing,\nyou earn more points for each landing when the expertise level is set higher";	//tool tip
pref_wnd2->sc.options="";						//menu button drop down options
pref_wnd2->sc.labelfont=4;						//-1 means use fltk default
pref_wnd2->sc.labelsize=10;						//-1 means use fltk default
pref_wnd2->sc.textfont=4;
pref_wnd2->sc.textsize=10;
pref_wnd2->sc.section="";
pref_wnd2->sc.key="";
pref_wnd2->sc.keypostfix=-1;					//ini file Key post fix
pref_wnd2->sc.def="";							//default to use if ini value not avail
pref_wnd2->sc.iretval=(int*)-1;					//address of int to be modified, -1 means none
pref_wnd2->sc.dretval=(double*)-1;				//address of double to be modified, -1 means none
pref_wnd2->sc.sretval=(string*)-1;				//address of string to be modified, -1 means none
pref_wnd2->sc.cb = cb_user2;					//address of a callback if any, 0 means none
pref_wnd2->sc.dynamic = 1;						//allow immediate dynamic change of user var
pref_wnd2->sc.uniq_id = 12;                      //allows identification of an actual control, rather that using its row and column values, don't use 0xffffffff
pref_wnd2->AddControl();

pref_wnd2->CreateRow(20);	









pref_wnd2->ClearToDefCtrl();			//this will clear sc struct to safe default values

pref_wnd2->sc.type=cnRadioButtonPref;
pref_wnd2->sc.x=200;
pref_wnd2->sc.y=0;
pref_wnd2->sc.w=18;
pref_wnd2->sc.h=18;
pref_wnd2->sc.radio_group_name = "group0";
pref_wnd2->sc.label="Novice (-5.0)";
pref_wnd2->sc.label_type = FL_NORMAL_LABEL;     //label effects such a FL_EMBOSSED_LABEL, FL_ENGRAVED_LABEL, FL_SHADOW_LABEL
pref_wnd2->sc.label_align = FL_ALIGN_LEFT;
pref_wnd2->sc.tooltip="select expertise level, land with a downward velocity (dy) not passing the indicated limit,\nhigher expertise requires a lower touchdown velocity for a successful landing,\nyou earn more points for each landing when the expertise level is set higher";	//tool tip
pref_wnd2->sc.options="";						//menu button drop down options
pref_wnd2->sc.labelfont=4;						//-1 means use fltk default
pref_wnd2->sc.labelsize=10;						//-1 means use fltk default
pref_wnd2->sc.textfont=4;
pref_wnd2->sc.textsize=10;
pref_wnd2->sc.section="Settings";
pref_wnd2->sc.key="pref_expertise0";
pref_wnd2->sc.keypostfix=-1;					//ini file Key post fix
pref_wnd2->sc.def="0";							//default to use if ini value not avail
pref_wnd2->sc.iretval=&pref_expertise0;			//address of int to be modified, -1 means none
pref_wnd2->sc.dretval=(double*)-1;				//address of double to be modified, -1 means none
pref_wnd2->sc.sretval=(string*)-1;				//address of string to be modified, -1 means none
pref_wnd2->sc.cb = cb_user2;					//address of a callback if any, 0 means none
pref_wnd2->sc.dynamic = 1;						//allow immediate dynamic change of user var
pref_wnd2->sc.uniq_id = 13;                      //allows identification of an actual control, rather that using its row and column values, don't use 0xffffffff
pref_wnd2->AddControl();

pref_wnd2->CreateRow(20);	






pref_wnd2->ClearToDefCtrl();			//this will clear sc struct to safe default values

pref_wnd2->sc.type=cnRadioButtonPref;
pref_wnd2->sc.x=200;
pref_wnd2->sc.y=0;
pref_wnd2->sc.w=18;
pref_wnd2->sc.h=18;
pref_wnd2->sc.radio_group_name = "group0";
pref_wnd2->sc.label="Advanced Beginner (-4.0)";
pref_wnd2->sc.label_type = FL_NORMAL_LABEL;     //label effects such a FL_EMBOSSED_LABEL, FL_ENGRAVED_LABEL, FL_SHADOW_LABEL
pref_wnd2->sc.label_align = FL_ALIGN_LEFT;
pref_wnd2->sc.tooltip="select expertise level, land with a downward velocity (dy) not passing the indicated limit,\nhigher expertise requires a lower touchdown velocity for a successful landing,\nyou earn more points for each landing when the expertise level is set higher";	//tool tip
pref_wnd2->sc.options="";						//menu button drop down options
pref_wnd2->sc.labelfont=4;						//-1 means use fltk default
pref_wnd2->sc.labelsize=10;						//-1 means use fltk default
pref_wnd2->sc.textfont=4;
pref_wnd2->sc.textsize=10;
pref_wnd2->sc.section="Settings";
pref_wnd2->sc.key="pref_expertise1";
pref_wnd2->sc.keypostfix=-1;					//ini file Key post fix
pref_wnd2->sc.def="1";							//default to use if ini value not avail
pref_wnd2->sc.iretval=&pref_expertise1;			//address of int to be modified, -1 means none
pref_wnd2->sc.dretval=(double*)-1;				//address of double to be modified, -1 means none
pref_wnd2->sc.sretval=(string*)-1;				//address of string to be modified, -1 means none
pref_wnd2->sc.cb = cb_user2;					//address of a callback if any, 0 means none
pref_wnd2->sc.dynamic = 1;						//allow immediate dynamic change of user var
pref_wnd2->sc.uniq_id = 14;                      //allows identification of an actual control, rather that using its row and column values, don't use 0xffffffff
pref_wnd2->AddControl();

pref_wnd2->CreateRow(20);	







pref_wnd2->ClearToDefCtrl();			//this will clear sc struct to safe default values

pref_wnd2->sc.type=cnRadioButtonPref;
pref_wnd2->sc.x=200;
pref_wnd2->sc.y=0;
pref_wnd2->sc.w=18;
pref_wnd2->sc.h=18;
pref_wnd2->sc.radio_group_name = "group0";
pref_wnd2->sc.label="Competent (-2.5)";
pref_wnd2->sc.label_type = FL_NORMAL_LABEL;     //label effects such a FL_EMBOSSED_LABEL, FL_ENGRAVED_LABEL, FL_SHADOW_LABEL
pref_wnd2->sc.label_align = FL_ALIGN_LEFT;
pref_wnd2->sc.tooltip="select expertise level, land with a downward velocity (dy) not passing the indicated limit,\nhigher expertise requires a lower touchdown velocity for a successful landing,\nyou earn more points for each landing when the expertise level is set higher";	//tool tip
pref_wnd2->sc.options="";						//menu button drop down options
pref_wnd2->sc.labelfont=4;						//-1 means use fltk default
pref_wnd2->sc.labelsize=10;						//-1 means use fltk default
pref_wnd2->sc.textfont=4;
pref_wnd2->sc.textsize=10;
pref_wnd2->sc.section="Settings";
pref_wnd2->sc.key="pref_expertise2";
pref_wnd2->sc.keypostfix=-1;					//ini file Key post fix
pref_wnd2->sc.def="0";							//default to use if ini value not avail
pref_wnd2->sc.iretval=&pref_expertise2;			//address of int to be modified, -1 means none
pref_wnd2->sc.dretval=(double*)-1;				//address of double to be modified, -1 means none
pref_wnd2->sc.sretval=(string*)-1;				//address of string to be modified, -1 means none
pref_wnd2->sc.cb = cb_user2;					//address of a callback if any, 0 means none
pref_wnd2->sc.dynamic = 1;						//allow immediate dynamic change of user var
pref_wnd2->sc.uniq_id = 15;                      //allows identification of an actual control, rather that using its row and column values, don't use 0xffffffff
pref_wnd2->AddControl();

pref_wnd2->CreateRow(20);	



pref_wnd2->ClearToDefCtrl();			//this will clear sc struct to safe default values

pref_wnd2->sc.type=cnRadioButtonPref;
pref_wnd2->sc.x=200;
pref_wnd2->sc.y=0;
pref_wnd2->sc.w=18;
pref_wnd2->sc.h=18;
pref_wnd2->sc.radio_group_name = "group0";
pref_wnd2->sc.label="Proficient (-1.5)";
pref_wnd2->sc.label_type = FL_NORMAL_LABEL;     //label effects such a FL_EMBOSSED_LABEL, FL_ENGRAVED_LABEL, FL_SHADOW_LABEL
pref_wnd2->sc.label_align = FL_ALIGN_LEFT;
pref_wnd2->sc.tooltip="select expertise level, land with a downward velocity (dy) not passing the indicated limit,\nhigher expertise requires a lower touchdown velocity for a successful landing,\nyou earn more points for each landing when the expertise level is set higher";	//tool tip
pref_wnd2->sc.options="";						//menu button drop down options
pref_wnd2->sc.labelfont=4;						//-1 means use fltk default
pref_wnd2->sc.labelsize=10;						//-1 means use fltk default
pref_wnd2->sc.textfont=4;
pref_wnd2->sc.textsize=10;
pref_wnd2->sc.section="Settings";
pref_wnd2->sc.key="pref_expertise3";
pref_wnd2->sc.keypostfix=-1;					//ini file Key post fix
pref_wnd2->sc.def="0";							//default to use if ini value not avail
pref_wnd2->sc.iretval=&pref_expertise3;			//address of int to be modified, -1 means none
pref_wnd2->sc.dretval=(double*)-1;				//address of double to be modified, -1 means none
pref_wnd2->sc.sretval=(string*)-1;				//address of string to be modified, -1 means none
pref_wnd2->sc.cb = cb_user2;					//address of a callback if any, 0 means none
pref_wnd2->sc.dynamic = 1;						//allow immediate dynamic change of user var
pref_wnd2->sc.uniq_id = 16;                      //allows identification of an actual control, rather that using its row and column values, don't use 0xffffffff
pref_wnd2->AddControl();

pref_wnd2->CreateRow(20);



pref_wnd2->ClearToDefCtrl();			//this will clear sc struct to safe default values

pref_wnd2->sc.type=cnRadioButtonPref;
pref_wnd2->sc.x=200;
pref_wnd2->sc.y=0;
pref_wnd2->sc.w=18;
pref_wnd2->sc.h=18;
pref_wnd2->sc.radio_group_name = "group0";
pref_wnd2->sc.label="Expert (-1.0)";
pref_wnd2->sc.label_type = FL_NORMAL_LABEL;     //label effects such a FL_EMBOSSED_LABEL, FL_ENGRAVED_LABEL, FL_SHADOW_LABEL
pref_wnd2->sc.label_align = FL_ALIGN_LEFT;
pref_wnd2->sc.tooltip="select expertise level, land with a downward velocity (dy) not passing the indicated limit,\nhigher expertise requires a lower touchdown velocity for a successful landing,\nyou earn more points for each landing when the expertise level is set higher";	//tool tip
pref_wnd2->sc.options="";						//menu button drop down options
pref_wnd2->sc.labelfont=4;						//-1 means use fltk default
pref_wnd2->sc.labelsize=10;						//-1 means use fltk default
pref_wnd2->sc.textfont=4;
pref_wnd2->sc.textsize=10;
pref_wnd2->sc.section="Settings";
pref_wnd2->sc.key="pref_expertise4";
pref_wnd2->sc.keypostfix=-1;					//ini file Key post fix
pref_wnd2->sc.def="0";							//default to use if ini value not avail
pref_wnd2->sc.iretval=&pref_expertise4;			//address of int to be modified, -1 means none
pref_wnd2->sc.dretval=(double*)-1;				//address of double to be modified, -1 means none
pref_wnd2->sc.sretval=(string*)-1;				//address of string to be modified, -1 means none
pref_wnd2->sc.cb = cb_user2;					//address of a callback if any, 0 means none
pref_wnd2->sc.dynamic = 1;						//allow immediate dynamic change of user var
pref_wnd2->sc.uniq_id = 17;                      //allows identification of an actual control, rather that using its row and column values, don't use 0xffffffff
pref_wnd2->AddControl();

pref_wnd2->CreateRow(20);	
/*
pref_wnd->ClearToDefCtrl();					//this will clear sc struct to safe default values

pref_wnd2->sc.type=cnInputDoublePref;
pref_wnd2->sc.x=200;
pref_wnd2->sc.y=0;
pref_wnd2->sc.w=150;
pref_wnd2->sc.h=20;
pref_wnd2->sc.label="Enter a Floating Point Num:";
pref_wnd2->sc.label_type = FL_NORMAL_LABEL;      //label effects such a FL_EMBOSSED_LABEL, FL_ENGRAVED_LABEL, FL_SHADOW_LABEL
pref_wnd2->sc.label_align = FL_ALIGN_LEFT;
pref_wnd2->sc.tooltip="";						//tool tip
pref_wnd2->sc.options="";						//menu button drop down options
pref_wnd2->sc.labelfont=-1;						//-1 means use fltk default
pref_wnd2->sc.labelsize=-1;						//-1 means use fltk default
pref_wnd2->sc.textfont=-1;
pref_wnd2->sc.textsize=-1;
pref_wnd2->sc.section="MyPref";
pref_wnd2->sc.key="Float1";
pref_wnd2->sc.keypostfix=-1;					//ini file Key post fix
pref_wnd2->sc.def="0";							//default to use if ini value not avail
pref_wnd2->sc.iretval=(int*)-1;					//address of int to be modified, -1 means none
pref_wnd2->sc.dretval=&gd0;						//address of double to be modified, -1 means none
pref_wnd2->sc.sretval=(string*)-1;				//address of string to be modified, -1 means none
pref_wnd2->sc.cb = cb_user2;					//address of a callback if any, 0 means none
pref_wnd2->sc.dynamic = 1;						//allow immediate dynamic change of user var
pref_wnd2->sc.uniq_id = 4;                      //allows identification of an actual control, rather that using its row and column values, don't use 0xffffffff
pref_wnd2->AddControl();

pref_wnd2->CreateRow(25);						//specify optional row height


pref_wnd->ClearToDefCtrl();					//this will clear sc struct to safe default values

pref_wnd2->sc.type=cnInputHexPref;
pref_wnd2->sc.x=190;
pref_wnd2->sc.y=0;
pref_wnd2->sc.w=150;
pref_wnd2->sc.h=20;
pref_wnd2->sc.label="Enter a Hex Num:";
pref_wnd2->sc.label_type = FL_NORMAL_LABEL;      //label effects such a FL_EMBOSSED_LABEL, FL_ENGRAVED_LABEL, FL_SHADOW_LABEL
pref_wnd2->sc.label_align = FL_ALIGN_LEFT;
pref_wnd2->sc.tooltip="";						//tool tip
pref_wnd2->sc.options="";						//menu button drop down options
pref_wnd2->sc.labelfont=-1;						//-1 means use fltk default
pref_wnd2->sc.labelsize=-1;						//-1 means use fltk default
pref_wnd2->sc.textfont=-1;
pref_wnd2->sc.textsize=-1;
pref_wnd2->sc.section="MyPref";
pref_wnd2->sc.key="Hex1";
pref_wnd2->sc.keypostfix=-1;					//ini file Key post fix
pref_wnd2->sc.def="0";							//default to use if ini value not avail
pref_wnd2->sc.iretval=&gi4;						//address of int to be modified, -1 means none
pref_wnd2->sc.dretval=(double*)-1;				//address of double to be modified, -1 means none
pref_wnd2->sc.sretval=(string*)-1;				//address of string to be modified, -1 means none
pref_wnd2->sc.cb = cb_user2;					//address of a callback if any, 0 means none
pref_wnd2->sc.dynamic = 1;						//allow immediate dynamic change of user var
pref_wnd2->sc.uniq_id = 5;                      //allows identification of an actual control, rather that using its row and column values, don't use 0xffffffff
pref_wnd2->AddControl();

pref_wnd2->CreateRow(25);						//specify optional row height



pref_wnd2->ClearToDefCtrl();					//this will clear sc struct to safe default values

pref_wnd2->sc.type=cnInputPref;
pref_wnd2->sc.x=100;
pref_wnd2->sc.y=0;
pref_wnd2->sc.w=570;
pref_wnd2->sc.h=25;
pref_wnd2->sc.label="FavEditor:";
pref_wnd2->sc.label_type = FL_NORMAL_LABEL;      //label effects such a FL_EMBOSSED_LABEL, FL_ENGRAVED_LABEL, FL_SHADOW_LABEL
pref_wnd2->sc.label_align = FL_ALIGN_LEFT;
pref_wnd2->sc.tooltip="Enter Appname of your favorite text editor"; //tool tip
pref_wnd2->sc.options="";						//menu button drop down options
pref_wnd2->sc.labelfont=-1;						//-1 means use fltk default
pref_wnd2->sc.labelsize=-1;						//-1 means use fltk default
pref_wnd2->sc.textfont=4;
pref_wnd2->sc.textsize=12;
pref_wnd2->sc.section="Pref";
pref_wnd2->sc.key="FavEditor";
pref_wnd2->sc.keypostfix=-1;						//ini file Key post fix
pref_wnd2->sc.def="";							//default to use if ini value not avail
pref_wnd2->sc.iretval=(int*)-1;			       	//address of int to be modified, -1 means none
pref_wnd2->sc.sretval=&fav_editor;				//address of string to be modified, -1 means none
pref_wnd2->sc.cb = cb_user2;					//address of a callback if any, 0 means none
pref_wnd2->sc.dynamic = 1;						//allow immediate dynamic change of user var
pref_wnd2->sc.uniq_id = 6;                      //allows identification of an actual control, rather that using its row and column values, don't use 0xffffffff
pref_wnd2->AddControl();

pref_wnd2->CreateRow( 25 );							//specify optional row height




pref_wnd2->sc.type=cnGCColColour;
pref_wnd2->sc.x=70;
pref_wnd2->sc.y=2;
pref_wnd2->sc.w=84;
pref_wnd2->sc.h=20;
pref_wnd2->sc.label="Colour1";
pref_wnd2->sc.label_type = FL_NORMAL_LABEL;      //label effects such a FL_EMBOSSED_LABEL, FL_ENGRAVED_LABEL, FL_SHADOW_LABEL
pref_wnd2->sc.label_align = FL_ALIGN_LEFT;
pref_wnd2->sc.tooltip="Set r,g,b colour value"; //tool tip
pref_wnd2->sc.options="";						//menu button drop down options
pref_wnd2->sc.labelfont=-1;						//-1 means use fltk default
pref_wnd2->sc.labelsize=-1;						//-1 means use fltk default
pref_wnd2->sc.textfont=4;
pref_wnd2->sc.textsize=12;
pref_wnd2->sc.section="Pref";
pref_wnd2->sc.key="Colour1";
pref_wnd2->sc.keypostfix=-1;					//ini file Key post fix
pref_wnd2->sc.def="255,255,0";						//default to use if ini value not avail
pref_wnd2->sc.iretval=(int*)-1;			       	//address of int to be modified, -1 means none
pref_wnd2->sc.sretval=&sg_col1;				//address of string to be modified, -1 means none
pref_wnd2->sc.cb = cb_user2;					//address of a callback if any, 0 means none
pref_wnd2->sc.dynamic = 1;						//allow immediate dynamic change of user var
pref_wnd2->sc.uniq_id = 7;                      //allows identification of an actual control, rather that using its row and column values, don't use 0xffffffff
pref_wnd2->AddControl();

pref_wnd2->CreateRow(25);					//specify optional row height

*/

pref_wnd2->End();								//do end for all windows

}













#define cnrows 30
#define cnctrls 10

int val[ cnrows ][ cnctrls ];
string sval[ cnrows ][ cnctrls ];















//this is the callback that is called by buttons that specified is in - 
//definition ....pref_wnd->sc.cb=(void*)cb_user;
//'*o' is the PrefWnd* ptr 
//'row' is the row the control lives on, 0 is first row
//'ctrl' is the number of the controlon that row, 0 is first control
void cb_user2(void *o, int row,int ctrl)
{
PrefWnd *w = (PrefWnd *) o;


unsigned int id = w->ctrl_list[ row ][ ctrl ].uniq_id;

printf("\ncb_user2() - Ping by Control on Row=%d at control count Ctrl=%d on this row, uniq_id: %d\n", row, ctrl, id );
}













void cb_btRunAnyway(Fl_Widget *w, void* v)
{
Fl_Widget* wnd=(Fl_Widget*)w;
Fl_Window *win;
win=(Fl_Window*)wnd->parent();

iAppExistDontRun = 0;
win->~Fl_Window();
}






void cb_btDontRunExit(Fl_Widget* w, void* v)
{
Fl_Widget* wnd=(Fl_Widget*)w;
Fl_Window *win;
win=(Fl_Window*)wnd->parent();

win->~Fl_Window();
}





//linux code
#ifndef compile_for_windows 



//v1.32 - handle a 'BadWindow' without causing a crash
int x11_error_handler(Display* display, XErrorEvent* error)
{

char msg[80];
XGetErrorText( display, error->error_code, msg, 80 );
fprintf(stderr, "x11_error_handler() - Error code %s", msg);
return 0;
}





//v1.32 - sets 'XSetErrorHandler()'
//gets its ID, -- fixed memory leak using XFetchName (used XFree) 01-10-10
int FindWindowID(string csName,Display *display, Window &wid)
{
Window root, parent, *children;
unsigned int numWindows = 0;
int ret = 0;
bool vb = 0;

XSetErrorHandler( x11_error_handler );


//*display = XOpenDisplay(NULL);
//gDisp = XOpenDisplay(NULL);



if( !XQueryTree( display, RootWindow( display, 0 ), &root, &parent, &children, &numWindows) )
	{
	return 0;
	}

if( children == 0 ) return 0;


int i = 0;
for(i=0; i < numWindows ; i++)
	{
	Window root2, parent2, *children2;

	unsigned int numWindows2 = 0;

	if( children[i] == 0 ) continue;

		char *name0;
		if( XFetchName(display, children[i], &name0) )
			{
			if(vb) printf( "FindWindowID() - %d/%d: looking for '%s', found '%s'\n", i, numWindows, csName.c_str(), name0 );	

			XFree(name0);
			}
		else{
			if(vb) printf( "FindWindowID() - %d/%d: looking for '%s', could not get name\n", i, numWindows, csName.c_str() );	
//			continue;
			}

	 if( !XQueryTree( display, children[i], &root2, &parent2, &children2, &numWindows2 ) )
		{
		continue;
		}


	if( children2 == 0 ) continue;


	for(int j = 0; j < numWindows2 ; j++)
		{	
		char *name1;
		if( XFetchName( display, children2[j], &name1 ) )
			{
			if(vb) printf( "FindWindowID() [child] - %d/%d: looking for '%s', found '%s'\n", j, numWindows2, csName.c_str(), name1 );

			if(strcmp( csName.c_str(), name1 ) == 0 )
				{
//				XMoveWindow(display, children2[j], -100, -100);
//				XMoveWindow(display, children2[j], -100, -100);
//				XMoveWindow(display, children2[j], -100, -100);
//				XResizeWindow(display, children2[j], 1088, 612+22);
//				XMoveWindow(*display, children2[j], 1100, 22);
				if(vb) printf( "FindWindowID() [child] - %d/%d: MATCH FOUND '%s'\n", j, numWindows2, csName.c_str(), name1  );
				wid = children2[j];
				ret = 1;
//				if(ret)
//					{
//					printf("\n\nTrying to Move %x  %x\n\n",gDisp, gw);
//					XMoveWindow(gDisp, gw, 700, 22);
//					return 1;
//					}
				}
			XFree( name1 );
			}
		else{
			if(vb) printf( "FindWindowID() [child] - %d/%d, could not get name\n", j, numWindows2 );
			}
		
		if (ret) break;
		}


	if(children2) XFree(children2);
	if (ret) break;
	}

if( children ) XFree(children);

return  ret;
}

#endif






//v1.31
void BringWindowToFront(string csTitle)
{

//linux code
#ifndef compile_for_windows

Display *gDisp0;			//v1.31


gDisp0 = XOpenDisplay(NULL);

Window wid;
if( FindWindowID( csTitle,gDisp0, wid ))
	{
	XUnmapWindow(gDisp0, wid);
	XMapWindow(gDisp0, wid);
	XFlush(gDisp0);
	}

XCloseDisplay(gDisp0);		//added this to see if valgrind showed improvement - it didn't
#endif


//windows code
#ifdef compile_for_windows
HWND hWnd;
//csAppName.LoadString(IDS_APPNAME);

hWnd = FindWindow( 0, cnsAppName );

if( hWnd )
	{
	::BringWindowToTop( hWnd );
//	::SetForegroundWindow( hWnd );
//	::PostMessage(hWnd,WM_MAXIMIZE,0,0);
	::ShowWindow( hWnd, SW_RESTORE );
	}
#endif

}











//linux code
#ifndef compile_for_windows 

//test if window with csAppName already exists, if so create inital main window with
//two options to either run app, or to exit.
//if no window with same name exists returns 0
//if 'exit' option chosen, exit(0) is called and no return happens
//if 'run anyway' option is chosen, returns 1
int CheckInstanceExists(string csAppName)
{
string csTmp;


Display *gDisp0;			//v1.31
gDisp0 = XOpenDisplay(NULL);

if( gDisp0 == 0 )			//ubuntu addition, incase 'pkexec' is used and access to display was not granted
	{
	printf( "CheckInstanceExists() - failed at call: 'XOpenDisplay()'\n" );
	return 1;				//returning 1 will close app
	}


Window wid;


if(FindWindowID(csAppName,gDisp0,wid))		//a window with same name exists?
	{
	BringWindowToFront( csAppName );

	XCloseDisplay(gDisp0);		//added this to see if valgrind showed improvement - it didn't

	Fl_Window *wndInitial = new Fl_Window(50,50,330,90);
	wndInitial->label("Possible Instance Already Running");
	
	Fl_Box *bxHeading = new Fl_Box(10,10,200, 15, "Another Window With Same Name Was Found.");
	bxHeading->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

	strpf(csTmp,"App Name: '%s'",csAppName.c_str()); 
	Fl_Box *bxAppName = new Fl_Box(10,30,200, 15,csTmp.c_str());
	bxAppName->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

	Fl_Button *btRunAnyway = new Fl_Button(25,55,130,25,"Run App Anyway");
	btRunAnyway->labelsize(12);
	btRunAnyway->callback(cb_btRunAnyway,0);

	Fl_Button *btDontRunExit = new Fl_Button(btRunAnyway->x()+btRunAnyway->w()+15,55,130,25,"Don't Run App, Exit");
	btDontRunExit->labelsize(12);
	btDontRunExit->callback(cb_btDontRunExit,0);

	wndInitial->end();
	wndInitial->show();

	Fl::run();

	return iAppExistDontRun;
	}
else{
	iAppExistDontRun=0;
	}


return iAppExistDontRun;

}

#endif














//windows code
#ifdef compile_for_windows 

//test if window with csAppName already exists, if so create inital main window with
//two options to either run app, or to exit.
//if no window with same name exists returns 0
//if 'exit' option chosen, exit(0) is called and no return happens
//if 'run anyway' option is chosen, returns 1
int CheckInstanceExists( string csAppName )
{
string csTmp;

HWND hWnd;
//csAppName.LoadString(IDS_APPNAME);

hWnd = FindWindow( 0, csAppName.c_str() );

if( hWnd )
	{
	BringWindowToFront( csAppName );
//	::BringWindowToTop( hWnd );
//::SetForegroundWindow( hWnd );
//::PostMessage(hWnd,WM_MAXIMIZE,0,0);
//	::ShowWindow( hWnd, SW_RESTORE );
Sleep(1000);

	Fl_Window *wndInitial = new Fl_Window(50,50,330,90);
	wndInitial->label("Possible Instance Already Running");
	
	Fl_Box *bxHeading = new Fl_Box(10,10,200, 15, "Another Window With Same Name Was Found.");
	bxHeading->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

	strpf(csTmp,"App Name: '%s'",csAppName.c_str()); 
	Fl_Box *bxAppName = new Fl_Box(10,30,200, 15,csTmp.c_str());
	bxAppName->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

	Fl_Button *btRunAnyway = new Fl_Button(25,55,130,25,"Run App Anyway");
	btRunAnyway->labelsize(12);
	btRunAnyway->callback(cb_btRunAnyway,0);

	Fl_Button *btDontRunExit = new Fl_Button(btRunAnyway->x()+btRunAnyway->w()+15,55,130,25,"Don't Run App, Exit");
	btDontRunExit->labelsize(12);
	btDontRunExit->callback(cb_btDontRunExit,0);

	wndInitial->end();
	wndInitial->show();
	wndInitial->hide();
	wndInitial->show();
	Fl::run();

	return iAppExistDontRun;
	}
else iAppExistDontRun = 0;

return iAppExistDontRun;
}

#endif

















//moded: 07-feb 2017 to ensure no vparams that are null strings are stored
//extract command line details from windows, is called by:  get_path_app_params()

//from GCCmdLine::GetAppName()
//eg: prog.com											//no path
//eg. "c:\dos\edit prog.com"							//with path\prog in quotes
//eg. "c:\dos\edit prog.com" c:\dos\junk.txt			//with path\prog in quotes and path\file
//eg. c\dos\edit.com /p /c		(as in screen-savers)	//path\prog and params no quotes

void extract_cmd_line( string cmdline, string &path, string &appname, vector<string> &vparams )
{
string stmp;
char ch;

path = "";
appname = "";
vparams.clear();
bool in_str = 0;
bool in_quote = 0;
bool beyond_app_name = 0;
//bool app_name_in_quotes = 0;

//cmdline = "c:/dos/edit prog.com";
//cmdline = "\"c:/dos/edit prog.com\" hello 123";
//cmdline = "c:/dos/edit.com hello 123";

int len =  cmdline.length();

if( len == 0 ) return;

for( int i = 0; i < len; i++ )
	{
	ch = cmdline[ i ];
	
	if( ch == '\"' )									//quote?
		{
		if( in_quote )
			{
			in_quote = 0;								//if here found closing quote
			goto got_param;
			}
		else{
			in_quote = 1;
			}
		}
	else{
		if( ch == ' ' )									//space?
			{
			if( !in_quote )				//if not in quote and space must be end of param
				{
				if( in_str ) goto got_param;
				}
			}
		else{
			in_str = 1;
			}

		if( in_str ) stmp += ch;
		}

	continue;

	got_param:

	in_str = 0;
	if ( beyond_app_name == 0 )					//store where approp
		{
		path = stmp;
		beyond_app_name = 1;
		}
	else{
		//store if not just a space
		if( stmp.compare( " " ) != 0 ) vparams.push_back( stmp );
		}

	stmp = "";
	}


//if here end of params reached, store where approp
if ( beyond_app_name == 0 )
	{
	path = stmp;
	}
else{
	if( stmp.length() != 0 ) vparams.push_back( stmp );
	}




appname = path;

len = path.length();
if( len == 0 ) return;

int pos = path.rfind( dir_seperator );

if( pos == string::npos )					//no directory path found?
	{
	path = "";	
	return;
	}

if( ( pos + 1 ) < len ) appname = path.substr( pos + 1,  pos + 1 - len );	//extract appname
path = path.substr( 0,  pos );								//extract path


//windows code
#ifdef compile_for_windows 
#endif

}



















//fixed windows version: v1.20 07-dec-2017, correctly handles paths with spaces that are not quoted (e.g: run from a batch file)
// !! will FAIL if params have a dir seperator in them as in:  D:\New Folder\skeleton.exe "d:\grrr" a "1 2" 3 4
//!!fixed windows version: 07-feb-2017
//find this app's path, app name and cmdline params,
//linux version uses 'argc, argc' for cmdline param extraction
//windows version uses
void get_path_app_params( string dir_sep, int argc, char **argv, string &path_out, string &app_name, vector<string> &vparams )
{
printf( "get_path_app_params()\n" );

string s1, s2, path;
mystr m1;

vparams.clear();

//linux code
#ifndef compile_for_windows

//get the actual path this app lives in
#define MAXPATHLEN 1025   // make this larger if you need to

int length;
char fullpath[ MAXPATHLEN ];

// /proc/self is a symbolic link to the process-ID subdir
// of /proc, e.g. /proc/4323 when the pid of the process
// of this program is 4323.
//
// Inside /proc/<pid> there is a symbolic link to the
// executable that is running as this <pid>.  This symbolic
// link is called "exe".
//
// So if we read the path where the symlink /proc/self/exe
// points to we have the full path of the executable.


length = readlink("/proc/self/exe", fullpath, sizeof(fullpath));


//printf("readlink: '%s'\n", fullpath );

// Catch some errors:
if (length < 0)
	{
	syslog(LOG_ALERT,"Error resolving symlink /proc/self/exe.\n");
	fprintf(stderr, "Error resolving symlink /proc/self/exe.\n");
	exit(0);
	}

if (length >= MAXPATHLEN)
	{
	syslog(LOG_ALERT, "Path too long. Truncated.\n");
	fprintf(stderr, "Path too long. Truncated.\n");
	exit(0);
	}

//I don't know why, but the string this readlink() function 
//returns is appended with a '@'


fullpath[length] = '\0';      // Strip '@' off the end


//printf("Full path is: %s\n", fullpath);
//syslog(LOG_ALERT,"Full path is: %s\n", fullpath);


m1 = fullpath;
m1.ExtractPath( dir_sep[ 0 ], path_out );

m1 = fullpath;
m1.ExtractFilename( dir_sep[ 0 ], app_name );

for( int i = 1; i < argc; i++ )
	{
	vparams.push_back( argv[ i ] );
	}
//syslog(LOG_ALERT,"Path only is: %s\n", csPathFilename.c_str());

#endif



//windows code
#ifdef compile_for_windows 
UINT i,uiLen;                    //eg. c\dos\edit.com /p /c		(as in screen-savers)
bool bQuotes;
string csCmdLineStr;


//----------------------------
//from GCCmdLine::GetAppName()
//eg: prog.com											//no path
//eg. "c:\dos\edit prog.com"							//with path\prog in quotes
//eg. "c:\dos\edit prog.com" c:\dos\junk.txt			//with path\prog in quotes and path\file
//eg. c\dos\edit.com /p /c		(as in screen-savers)	//path\prog and params no quotes
// !! will FAIL if params have a dir seperator in them as in:  D:\New Folder\skeleton.exe "d:\grrr" a "1 2" 3 4

csCmdLineStr = GetCommandLine();
printf("csCmdLineStr: '%s'\n", csCmdLineStr.c_str() );
//csCmdLineStr = "skeleton.exe abc";
//printf("csCmdLineStr= '%s'\n", csCmdLineStr.c_str() );

char path_appname[ MAX_PATH ];
GetModuleFileName( NULL, path_appname, MAX_PATH );

printf("path_appname= '%s'\n", path_appname );

//----
//added v1.20, now extracts app path that has spaces in it,
//uses the last directory seperator as indicator of end of path and start of filename and its parameters
m1 = path_appname;
string spath;
spath = path_appname;


if( m1.ExtractPath( dir_seperator[ 0 ], s1 ) )
	{
//	printf("wincmd2: '%s'\n", s1.c_str() );
	spath = s1;
	}
else{
	printf("get_path_app_params() - ExtractPath() - failed to extract path from: '%s'\n", spath.c_str() );
	}

m1 = csCmdLineStr;
if( m1.ExtractFilename( dir_seperator[ 0 ], s1 ) )
	{
	csCmdLineStr = s1;
	}
else{
	printf("get_path_app_params() - ExtractFilename() - failed to extract path from: '%s'\n", spath.c_str() );
	}

printf("csCmdLineStr2: '%s'\n", csCmdLineStr.c_str() );

//----



csCmdLineStr += " ";
m1 = csCmdLineStr;
m1.cut_just_past_first_find_and_keep_right( s1, " ", 0 ); 	//strip off app name to just leave params

//reform full command line
//s2 = path_appname;spath	removed v1.20
s2 = "c:";						//added v1.20, add a dummy path that has no spaces so 'extract_cmd_line()' will still extract app name and its params
s2 += dir_seperator[ 0 ];
s2 += csCmdLineStr;				//make combined pathname appname and cmdline params
//s2 += s1;						removed v1.20 //make combined pathname appname and cmdline params

//printf( "get_path_app_params() - s2: '%s'\n", s2.c_str() );

extract_cmd_line( s2, path_out, app_name, vparams );

path_out = spath;				//added v1.20, ignore original path_out as it might have had spaces
#endif




printf( "get_path_app_params() - path_out: '%s'\n", path_out.c_str() );
printf( "get_path_app_params() - app_name: '%s'\n", app_name.c_str() );


for( int i = 0; i < vparams.size(); i++ )
	{
	printf( "get_path_app_params() - vparams[%d]: '%s'\n", i, vparams[ i ].c_str() );
	}

}








//find this apps path
void get_app_path( string &path_out )
{
string s1, path;
mystr m1;


//linux code
#ifndef compile_for_windows

//get the actual path this app lives in
#define MAXPATHLEN 1025   // make this larger if you need to

int length;
char fullpath[MAXPATHLEN];

// /proc/self is a symbolic link to the process-ID subdir
// of /proc, e.g. /proc/4323 when the pid of the process
// of this program is 4323.
//
// Inside /proc/<pid> there is a symbolic link to the
// executable that is running as this <pid>.  This symbolic
// link is called "exe".
//
// So if we read the path where the symlink /proc/self/exe
// points to we have the full path of the executable.


length = readlink("/proc/self/exe", fullpath, sizeof(fullpath));
 
// Catch some errors:
if (length < 0)
	{
	syslog(LOG_ALERT,"Error resolving symlink /proc/self/exe.\n");
	fprintf(stderr, "Error resolving symlink /proc/self/exe.\n");
	exit(0);
	}

if (length >= MAXPATHLEN)
	{
	syslog(LOG_ALERT, "Path too long. Truncated.\n");
	fprintf(stderr, "Path too long. Truncated.\n");
	exit(0);
	}

//I don't know why, but the string this readlink() function 
//returns is appended with a '@'

fullpath[length] = '\0';      // Strip '@' off the end


//printf("Full path is: %s\n", fullpath);
//syslog(LOG_ALERT,"Full path is: %s\n", fullpath);

path = fullpath;
size_t found = path.rfind( "/" );
if ( found != string::npos ) path_out = path.substr( 0, found );
//syslog(LOG_ALERT,"Path only is: %s\n", csPathFilename.c_str());

#endif



//windows code
#ifdef compile_for_windows 
UINT i,uiLen;                    //eg. c\dos\edit.com /p /c		(as in screen-savers)
bool bQuotes;
string csCmdLineStr;


//----------------------------
//from GCCmdLine::GetAppName()
//eg: prog.com											//no path
//eg. "c:\dos\edit prog.com"							//with path\prog in quotes
//eg. "c:\dos\edit prog.com" c:\dos\junk.txt			//with path\prog in quotes and path\file
//eg. c\dos\edit.com /p /c		(as in screen-savers)	//path\prog and params no quotes
csCmdLineStr = GetCommandLine();

//csCmdLineStr = "skeleton.exe abc";
//printf("csCmdLineStr= '%s'\n", csCmdLineStr.c_str() );


string appname;
vector<string> vparams;
get_cmd_line( csCmdLineStr, path_out, appname, vparams  );

#endif


printf( "csPathFilename= %s\n", path_out.c_str() );

}














void LoadSettings(string csIniFilename)
{
string csTmp;
int x,y,cx,cy;

GCProfile p(csIniFilename);
//x=p.GetPrivateProfileLONG("Settings","WinX",100);
//y=p.GetPrivateProfileLONG("Settings","WinY",100);
//cx=p.GetPrivateProfileLONG("Settings","WinCX",750);
//cy=p.GetPrivateProfileLONG("Settings","WinCY",550);

//wndMain->position( x , y );	
//wndMain->size( cx , cy );	

x=p.GetPrivateProfileLONG("Settings","win_x",70);
y=p.GetPrivateProfileLONG("Settings","win_y",30);
cx=p.GetPrivateProfileLONG("Settings","win_cx",1700);
cy=p.GetPrivateProfileLONG("Settings","win_cy",900);

if( x < 0 ) x = 0;
if( x > 1800 ) x = 1800;

if( y < 0 ) y = 0;
if( y > 1000 ) y = 1000;

if( cx < 400 ) cx = 400;
if( cx > 1900 ) cx = 1900;

if( cy < 400 ) cy = 400;
if( cy > 1000 ) cy = 1000;

wnd_llem->position( x , y );	
wnd_llem->size( cx , cy );	


p.GetPrivateProfileStr( "Settings", "slast_ship_fname", "zzllem.txt", &slast_ship_fname );
p.GetPrivateProfileStr( "Settings", "slast_moonscape_fname", "zzground.txt", &slast_moonscape_fname );
p.GetPrivateProfileStr( "Settings", "sship_state_fname", "zzship_state0.txt", &sship_state_fname );
 

p.GetPrivateProfileStr( "Settings", "slast_highscore_name", "Jane Doe", &slast_highscore_name );

if(pref_wnd2!=0) pref_wnd2->Load(p);

}









void SaveSettings(string csIniFilename)
{
GCProfile p(csIniFilename);


//p.WritePrivateProfileLONG("Settings","WinX", wndMain->x()-iBorderWidth);
//p.WritePrivateProfileLONG("Settings","WinY", wndMain->y()-iBorderHeight);
//p.WritePrivateProfileLONG("Settings","WinCX", wndMain->w());
//p.WritePrivateProfileLONG("Settings","WinCY", wndMain->h());



//this uses previously saved window sizing value, hopefully grabbed before window was maximized by user see: dble_wnd::resize()
//remove window border offset when saving window pos settings
//p.WritePrivateProfileLONG("Settings","WinX", wndMain->restore_size_x - iBorderWidth );
//p.WritePrivateProfileLONG("Settings","WinY", wndMain->restore_size_y - iBorderHeight);
//p.WritePrivateProfileLONG("Settings","WinCX", wndMain->restore_size_w );
//p.WritePrivateProfileLONG("Settings","WinCY", wndMain->restore_size_h );


p.WritePrivateProfileLONG("Settings","win_x", wnd_llem->x() );
p.WritePrivateProfileLONG("Settings","win_y", wnd_llem->y() );
p.WritePrivateProfileLONG("Settings","win_cx", wnd_llem->w() );
p.WritePrivateProfileLONG("Settings","win_cy", wnd_llem->h() );


p.WritePrivateProfileStr( "Settings", "slast_ship_fname", slast_ship_fname );
p.WritePrivateProfileStr( "Settings", "slast_moonscape_fname", slast_moonscape_fname );
p.WritePrivateProfileStr( "Settings", "sship_state_fname", sship_state_fname );

p.WritePrivateProfileStr( "Settings", "slast_highscore_name", slast_highscore_name );

if(pref_wnd2!=0) pref_wnd2->Save(p);


}




void DoQuit()
{
stop_audio();

if( revb_bf0 ) delete[] revb_bf0;
if( revb_bf1 ) delete[] revb_bf1;

if( revb_bf10 ) delete[] revb_bf10;
if( revb_bf11 ) delete[] revb_bf11;




SaveSettings(csIniFilename);

if(pref_wnd2!=0) delete pref_wnd2;


//windows code
#ifdef compile_for_windows
CloseHandle( h_mutex1 );							//close mutex obj
#endif

exit(0);
}







/*

void cb_skeleton_help_geany( Fl_Widget *, void * )
{
printf( "cb_skeleton_help_geany()\n" );

string pathname;
pathname = '\"';					//incase path has white spaces
pathname += app_path;
pathname += dir_seperator;
pathname += cns_open_editor;
pathname += '\"';					//incase path has white spaces
pathname += " ";
pathname += cns_help_filename;

RunShell( pathname );					//do both shell cmds

printf( "path: '%s'\n", pathname.c_str() );

}

*/



void cb_btAbout(Fl_Widget *, void *)
{
string s1, st;

Fl_Window *wnd = new Fl_Window( wnd_llem->x()+20, wnd_llem->y()+20,500,150);
wnd->label("About");
Fl_Input *teText = new Fl_Input(10,10,wnd->w()-20,wnd->h()-20,"");
teText->type(FL_MULTILINE_OUTPUT);
teText->textsize(12);

strpf( s1, "%s,  %s,  Built: %s\n", cnsAppWndName, "v1.01", cns_build_date );
st += s1;

strpf( s1, "\nTry to land at various landing zones.\nThe softer the landing the more fuel you earn..." );
st += s1;

strpf( s1, "\nSee 'help.txt' file for more details." );
st += s1;


teText->value(st.c_str());
wnd->end();

#ifndef compile_for_windows
wnd->set_modal();
#endif

wnd->show();

}







/*
void cb_btOpen(Fl_Widget *, void *)
{

//char *pPathName = fl_file_chooser("Open Record Schedule File?", "*",0);
//if (!pPathName) return;

//GCProfile p(csIniFilename);
//p.WritePrivateProfileStr("Settings","LastScheduleFile",pPathName);

}
*/



void cb_open_folder(Fl_Widget *, void *)
{
string s1, s2;

s1 = "./";

if( my_dir_chooser( s2, "Select Folder?", "*",  (char*)s1.c_str(), 0, font_num, font_size ) )
	{
	printf( "You select folder: '%s'\n", s2.c_str() );
	}
}






void cb_open_file(Fl_Widget *, void *)
{
string s1, s2;

s1 = "./";

if( my_file_chooser( s2, "Select File?", "*", s1.c_str(), 0, font_num, font_size ) )
	{
    mystr m1 = s2;
    m1.ExtractFilename( dir_seperator[ 0 ], s2 );
    
	printf( "You select file: '%s'\n", s2.c_str() );
	}
}




/*
void cb_open_2(Fl_Widget *, void *)
{
string s1;

s1 ="/mnt/home/PuppyLinux/MyPrj/MyPrj-DevC++/bkup/";

Fl_File_Chooser *fc = new Fl_File_Chooser( s1.c_str(), 0, FL_SINGLE, "Open a file" );
fc->textfont( font_num );
fc->textsize( font_size );
//fc->redraw();
fc->show();
//fc->hide();
//fc->show();
//fc->fileName->show();
}

*/






void cb_btSave(Fl_Widget *, void *)
{
//char *pPathName = fl_file_chooser("Save Record Schedule File?", "*",0);
//if (!pPathName) return;

//GCProfile p(csIniFilename);
//p.WritePrivateProfileStr("Settings","LastScheduleFile",pPathName);

}







void cb_btQuit(Fl_Widget *, void *)
{
DoQuit();
}





void cb_bt_help( Fl_Widget *, void *)
{
mystr m1;
string pathname, s1;
pathname += '"';
pathname += app_path;
pathname += dir_seperator;
pathname += cns_open_editor;
pathname += '"';
pathname += " ";
pathname += '"';
pathname += app_path;
pathname += dir_seperator;
pathname += cns_help_filename;
pathname += '"';

s1 = pathname;
//m1 = pathname;

//m1.FindReplace( s1, " ", "\\ ", 0 );

printf( "cb_bt_help() - '%s'\n", s1.c_str() );

stop_audio();

RunShell( s1 );					//do both shell cmds

start_audio();
}











int timer_ratio_10 = 0;
int timer_ratio_20 = 0;



void cb_timer1(void *)
{
string s1;
mystr m1;

Fl::repeat_timeout( timer_period, cb_timer1 );

timer_ratio_10++;
if(  timer_ratio_10 >= 10 ) timer_ratio_10 = 0;					//10 x cn_timer_period

timer_ratio_20++;
if(  timer_ratio_20 >= 20 ) timer_ratio_20 = 0;					//20 x cn_timer_period


if( timer_ratio_10 == 0 );
if( timer_ratio_20 == 0 );

//if( timer_ratio_20 < 10 ) 
//	{
//	printf( "cb_timer1() - low\n" );
//	wndMain->icon( theapp_icon0 );
//	}
//else{
//	printf( "cb_timer1() - high\n" );
//	wndMain->icon( theapp_icon1 );
//	}


//if( audio_started ) process_graphs();


//float measured_tframe = cn_timer_period;

if( wnd_llem ) wnd_llem->tick( timer_period );




}













//linux code
#ifndef compile_for_windows

//execute shell cmd
int RunShell(string sin)
{

if ( sin.length() == 0 ) return 0;

//make command to cd working dir to app's dir and execute app (params in "", incase of spaces)
//strpf(csCmd,"cd \"%s\";\"%s\" \"%s\"",csPath.c_str(),sEntry[iEntryNum].csStartFunct.c_str(),csFile.c_str());

pid_t child_pid;

child_pid=fork();		//create a child process	

if(child_pid==-1)		//failed to fork?
	{
	printf("\nRunShell() failed to fork\n");
	return 0;
	}

if(child_pid!=0)		//parent fork? i.e. child pid is avail
	{
	int status;
	printf("\nwaitpid: %d, RunShell start\n",child_pid);	

	while(1)
		{
		waitpid(child_pid,&status,0);		//wait for return val from child so a zombie process is not left in system
		printf("\nwaitpid %d RunShell stop\n",child_pid);
		if(WIFEXITED(status)) break;		//confirm status returned shows the child terminated
		}	
	}
else{					//child fork (0) ?
//	printf("\nRunning Shell: %s\n",csCmd.c_str());
	printf("\nRunShell system cmd started: %s\n",sin.c_str());	
	system(sin.c_str());
	printf("\nRunShell system cmd finished \n");	
	exit(1);
	}
return 1;
}

#endif











//windows code
#ifdef compile_for_windows

//execute shell cmd as a process that can be monitored
int RunShell( string sin )
{
BOOL result;
wstring ws1;

if ( sin.length() == 0 ) return 0;


mystr m1 = sin;

m1.mbcstr_wcstr( ws1 );	//convert utf8 string to windows wchar string array


memset(&processInformation, 0, sizeof(processInformation));


STARTUPINFOW StartInfoW; 							// name structure
memset(&StartInfoW, 0, sizeof(StartInfoW));
StartInfoW.cb = sizeof(StartInfoW);

StartInfoW.wShowWindow = SW_HIDE;

result = CreateProcessW( NULL, (WCHAR*)ws1.c_str(), NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &StartInfoW, &processInformation);

if ( result == 0)
	{
	
	return 0;
	}

return 1;



//bkup_filelist_SEI[ 0 ].cbSize = sizeof( bkup_filelist_SEI[ 0 ] ); 
//bkup_filelist_SEI[ 0 ].lpVerb = "open"; 
//bkup_filelist_SEI[ 0 ].lpFile = sin.c_str(); 
//bkup_filelist_SEI[ 0 ].lpParameters= 0; 
//bkup_filelist_SEI[ 0 ].nShow = SW_HIDE; 
//bkup_filelist_SEI[ 0 ].fMask = SEE_MASK_NOCLOSEPROCESS; 

//ShellExecuteEx( &bkup_filelist_SEI[ 0 ] );     //execute batch file



//WCHAR cmd[] = L"cmd.exe /c pause";
//LPCWSTR dir = L"c:\\";
//STARTUPINFOW si = { 0 };
//si.cb = sizeof(si);
//PROCESS_INFORMATION pi;

//STARTUPINFO StartInfo; 							// name structure
//PROCESS_INFORMATION ProcInfo; 						// name structure
//memset(&ProcInfo, 0, sizeof(ProcInfo));				// Set up memory block
//memset(&StartInfo, 0 , sizeof(StartInfo)); 			// Set up memory block
//StartInfo.cb = sizeof(StartInfo); 					// Set structure size

//int res = CreateProcess( NULL, (char*)sin.c_str(), 0, 0, TRUE, 0, NULL, NULL, &StartInfo, &ProcInfo );

}

#endif













//open preference specified editor with supplied fname as parameter 
void open_editor( string fname )
{
string s1;

s1 = "\"";
s1 += fav_editor;
s1 += "\"";
s1 += " ";
s1 += "\"";
s1 += fname;
s1 += "\"";


//linux code
#ifndef compile_for_windows
RunShell( s1 );
#endif



//windows code
#ifdef compile_for_windows
wstring ws1;
mystr m1 = s1;

m1.mbcstr_wcstr( ws1 );	//convert utf8 string to windows wchar string array

//WCHAR cmd[] = L"cmd.exe /c pause";
//LPCWSTR dir = L"c:\\";
//STARTUPINFOW si = { 0 };
//si.cb = sizeof(si);
//PROCESS_INFORMATION pi;

STARTUPINFOW StartInfoW; 							// name structure
PROCESS_INFORMATION ProcInfo; 						// name structure
memset(&ProcInfo, 0, sizeof(ProcInfo));				// Set up memory block
memset(&StartInfoW, 0 , sizeof(StartInfoW)); 		// Set up memory block
StartInfoW.cb = sizeof(StartInfoW); 				// Set structure size

int res = CreateProcessW(NULL, (WCHAR*)ws1.c_str(), 0, 0, TRUE, CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &StartInfoW, &ProcInfo );

#endif
}




























































































 
 
bool open_wave_file( string &sfname )
{
string s1, s2;


if( my_file_chooser( s2, "Select Wave File?", "*", slast_wave_fname.c_str(), 0, font_num, font_size ) )
	{
//    mystr m1 = s2;
//    m1.ExtractFilename( dir_seperator[ 0 ], s2 );
    
    slast_wave_fname = s2;
    
	printf( "open_wave_file() '%s'\n", slast_wave_fname.c_str() );
	return 1;
	}
return 0;
}















void create_filter_iir( en_filter_pass_type_tag filt_type, float filt_freq_in, float filt_q_in, st_iir_2nd_order_tag &iir )
{
vector<double> vfilt_coeff;
float db_gain = 0;

if( !calc_filter_iir_2nd_order( filt_type, filt_freq_in, filt_q_in, db_gain, srate, vfilt_coeff ) )
	{
	printf( "create_filter_iir() - failed to calc filter coeffs, freq %f, q %f\n", filt_freq0, filt_q0 );
	return;
	}

//printf( "create_filter0() - iir freq %f, q %f   %f %f %f %f %f\n", filt_freq0, filt_q0, iir0.coeff[0], iir0.coeff[1], iir0.coeff[2], iir0.coeff[3], iir0.coeff[4] );

iir.bypass = 0;
iir.coeff[0] = vfilt_coeff[0];		//a1
iir.coeff[1] = vfilt_coeff[1];		//a2
iir.coeff[2] = vfilt_coeff[2];		//b0
iir.coeff[3] = vfilt_coeff[3];		//b1
iir.coeff[4] = vfilt_coeff[4];		//b2

iir.delay0[0] = 0;
iir.delay0[1] = 0;

iir.delay1[0] = 0;
iir.delay1[1] = 0;
iir.bypass = 1;

}





/*
//thrust filter
void create_filter0()
{
en_filter_pass_type_tag filt_type = fpt_lowpass;
vector<double> vfilt_coeff;
float db_gain = 0;

if( !calc_filter_iir_2nd_order( filt_type, filt_freq0, filt_q0, db_gain, srate, vfilt_coeff ) )
	{
	printf( "create_filter0() - failed to calc filter coeffs, freq %f, q %f\n", filt_freq0, filt_q0 );
	return;
	}

//printf( "create_filter0() - iir0 freq %f, q %f   %f %f %f %f %f\n", filt_freq0, filt_q0, iir0.coeff[0], iir0.coeff[1], iir0.coeff[2], iir0.coeff[3], iir0.coeff[4] );

iir0.bypass = 0;
iir0.coeff[0] = vfilt_coeff[0];		//a1
iir0.coeff[1] = vfilt_coeff[1];		//a2
iir0.coeff[2] = vfilt_coeff[2];		//b0
iir0.coeff[3] = vfilt_coeff[3];		//b1
iir0.coeff[4] = vfilt_coeff[4];		//b2

iir0.delay0[0] = 0;
iir0.delay0[1] = 0;

iir0.delay1[0] = 0;
iir0.delay1[1] = 0;
iir0.bypass = 1;

}
*/






/*

//thrust subsonic filter
void create_filter1()
{
en_filter_pass_type_tag filt_type = fpt_lowpass;
vector<double> vfilt_coeff;
float db_gain = 0;

float filt_freq1 = 25;
float filt_q1 = 3;

if( !calc_filter_iir_2nd_order( filt_type, filt_freq1, filt_q1, db_gain, srate, vfilt_coeff ) )
	{
	printf( "create_filter1() - failed to calc filter coeffs, freq %f, q %f\n", filt_freq0, filt_q0 );
	return;
	}

//printf( "create_filter1() - iir1 freq %f, q %f   %f %f %f %f %f\n", filt_freq0, filt_q0, iir1.coeff[0], iir1.coeff[1], iir1.coeff[2], iir1.coeff[3], iir1.coeff[4] );


iir1.bypass = 0;
iir1.coeff[0] = vfilt_coeff[0];		//a1
iir1.coeff[1] = vfilt_coeff[1];		//a2
iir1.coeff[2] = vfilt_coeff[2];		//b0
iir1.coeff[3] = vfilt_coeff[3];		//b1
iir1.coeff[4] = vfilt_coeff[4];		//b2

iir1.delay0[0] = 0;
iir1.delay0[1] = 0;

iir1.delay1[0] = 0;
iir1.delay1[1] = 0;
iir1.bypass = 1;


//iir0
}
*/




/*

//explosion high freq filter
void create_filter2()
{
en_filter_pass_type_tag filt_type = fpt_lowpass;
vector<double> vfilt_coeff;
float db_gain = 0;

float filt_freq1 = 200;
float filt_q1 = 0.5;

if( !calc_filter_iir_2nd_order( filt_type, filt_freq1, filt_q1, db_gain, srate, vfilt_coeff ) )
	{
	printf( "create_filter2() - failed to calc filter coeffs, freq %f, q %f\n", filt_freq0, filt_q0 );
	return;
	}

//printf( "create_filter2() - iir1 freq %f, q %f   %f %f %f %f %f\n", filt_freq0, filt_q0, iir1.coeff[0], iir1.coeff[1], iir1.coeff[2], iir1.coeff[3], iir1.coeff[4] );


iir20.bypass = 0;
iir20.coeff[0] = vfilt_coeff[0];		//a1
iir20.coeff[1] = vfilt_coeff[1];		//a2
iir20.coeff[2] = vfilt_coeff[2];		//b0
iir20.coeff[3] = vfilt_coeff[3];		//b1
iir20.coeff[4] = vfilt_coeff[4];		//b2

iir20.delay0[0] = 0;
iir20.delay0[1] = 0;

iir20.delay1[0] = 0;
iir20.delay1[1] = 0;
iir20.bypass = 1;

}









//explosion low freq filter
void create_filter3()
{
en_filter_pass_type_tag filt_type = fpt_lowpass;
vector<double> vfilt_coeff;
float db_gain = 0;

float filt_freq1 = 70;
float filt_q1 = 2;

if( !calc_filter_iir_2nd_order( filt_type, filt_freq1, filt_q1, db_gain, srate, vfilt_coeff ) )
	{
	printf( "create_filter3() - failed to calc filter coeffs, freq %f, q %f\n", filt_freq0, filt_q0 );
	return;
	}

//printf( "create_filter2() - iir1 freq %f, q %f   %f %f %f %f %f\n", filt_freq0, filt_q0, iir1.coeff[0], iir1.coeff[1], iir1.coeff[2], iir1.coeff[3], iir1.coeff[4] );


iir30.bypass = 0;
iir30.coeff[0] = vfilt_coeff[0];		//a1
iir30.coeff[1] = vfilt_coeff[1];		//a2
iir30.coeff[2] = vfilt_coeff[2];		//b0
iir30.coeff[3] = vfilt_coeff[3];		//b1
iir30.coeff[4] = vfilt_coeff[4];		//b2

iir30.delay0[0] = 0;
iir30.delay0[1] = 0;

iir30.delay1[0] = 0;
iir30.delay1[1] = 0;
iir30.bypass = 1;

}

*/




/*
void create_reverb()
{
revb_size0 = 48000*0.14f;
revb_size1 = 48000*0.22f;
revb_bf0 = new float[revb_size0];
revb_bf1 = new float[revb_size1];

revb_size10 = 48000*0.22f;
revb_size11 = 48000*0.11f;
revb_bf10 = new float[revb_size10];
revb_bf11 = new float[revb_size11];

memset( revb_bf0, 0, revb_size0*sizeof(float) );
memset( revb_bf1, 0, revb_size1*sizeof(float) );

memset( revb_bf10, 0, revb_size10*sizeof(float) );
memset( revb_bf11, 0, revb_size11*sizeof(float) );

}
*/




/*
void create_reverb_hold()
{
revb_size0 = 48000*0.07f;
revb_size1 = 48000*0.32f;
revb_bf0 = new float[revb_size0];
revb_bf1 = new float[revb_size1];

revb_size10 = 48000*0.33f;
revb_size11 = 48000*0.06f;
revb_bf10 = new float[revb_size10];
revb_bf11 = new float[revb_size11];

memset( revb_bf0, 0, revb_size0*sizeof(float) );
memset( revb_bf1, 0, revb_size1*sizeof(float) );

memset( revb_bf10, 0, revb_size10*sizeof(float) );
memset( revb_bf11, 0, revb_size11*sizeof(float) );

}

*/

















void cb_wndmain(Fl_Widget*, void* v)
{
Fl_Window* wnd = (Fl_Window*)v;

//wndMain->iconize();
//wndMain->hide();
//wndMain->show();

DoQuit();
}





//fast_mgraph fgph0, fgph1, fgph2;

vector<float> fgph_vx;
vector<float> fgph_vy0;
vector<float> fgph_vy1;
vector<float> fgph_vy2;

/*
void plot_mgraph2()
{

fgph_vx.clear();
fgph_vy0.clear();
fgph_vy1.clear();

string s1 = fgph2.label();
s1 += " - plot_mgraph2()";
fgph2.copy_label( s1.c_str() );
fgph2.position( 60, 30 );
fgph2.scale_y( 1.0, 1.0, 1.0, 0.1 );
fgph2.shift_y( 0.0, -0.1, -0.10, -0.2 );
fgph2.font_size( 9 );
fgph2.set_sig_dig( 2 );
fgph2.sample_rect_hints_distancex = 0;
fgph2.sample_rect_hints_distancey = 0;

#define twopi 2*(float)M_PI
int cnt = 1000;
for( int i = 0; i < cnt; i++ )
	{
	float f0 = sinf( (float)i/cnt * twopi * 5 );
	float f1 = 0.5*sinf( (float)i/cnt * twopi * 10 );
	float f2 = 0.8*sinf( (float)i/cnt * twopi * 15 );
	fgph_vx.push_back( i );
	fgph_vy0.push_back( f0 );
	fgph_vy1.push_back( f1 );
	fgph_vy2.push_back( f2 );
	}

//fgph.plotxy( gph0_vx, gph0_vamp0, gph0_vamp1, gph0_vamp2, gph0_vamp4,"brwn", "drkg", "drkcy", "drkr", "ofw", "drkb", "drkgry", "trace 1", "trace 2", "trace 3", "trace 4"  );
//fgph.plotxy( gph0_vx, gph0_vamp0, gph0_vamp1 );
//fgph.plotxy( fgph_vx, fgph_vy0, fgph_vy1 );

//fgph0.plotxy( fgph0_vx, fgph0_vy0, "drky", "ofw", "drkb", "blk", "pc srate" );	//3 traces cols, bkgd col, axis col, text col, and trace colour coded labels, see defined colours: 'user_col'
fgph2.plotxy( fgph_vx, fgph_vy0, fgph_vy1, fgph_vy2, "drkr", "drkg", "r", "ofw", "drkb", "blk", "pc srate", "label2", "label3" );	//3 traces cols, bkgd col, axis col, text col, and trace colour coded labels, see defined colours: 'user_col'

}



void plot_mgraph1()
{
fgph_vx.clear();
fgph_vy0.clear();
fgph_vy1.clear();

string s1 = fgph1.label();
s1 += " - plot_mgraph1()";
fgph1.copy_label( s1.c_str() );
fgph1.position( 30, 80 );
fgph1.scale_y( 1.0, 1.0, 1.0, 0.1 );
fgph1.shift_y( 0.0, -0.1, -0.10, -0.2 );
fgph1.font_size( 9 );
fgph1.set_sig_dig( 2 );
fgph1.sample_rect_hints_distancex = 0;
fgph1.sample_rect_hints_distancey = 0;

#define twopi 2*(float)M_PI
int cnt = 1000;
for( int i = 0; i < cnt; i++ )
	{
	float f0 = sinf( (float)i/cnt * twopi * 5 );
	float f1 = 0.8*sinf( (float)i/cnt * twopi * 10 );
	fgph_vx.push_back( i );
	fgph_vy0.push_back( f0 );
	fgph_vy1.push_back( f1 );
	}

//fgph.plotxy( gph0_vx, gph0_vamp0, gph0_vamp1, gph0_vamp2, gph0_vamp4,"brwn", "drkg", "drkcy", "drkr", "ofw", "drkb", "drkgry", "trace 1", "trace 2", "trace 3", "trace 4"  );
//fgph.plotxy( gph0_vx, gph0_vamp0, gph0_vamp1 );
//fgph.plotxy( fgph_vx, fgph_vy0, fgph_vy1 );

//fgph0.plotxy( fgph0_vx, fgph0_vy0, "drky", "ofw", "drkb", "blk", "pc srate" );	//3 traces cols, bkgd col, axis col, text col, and trace colour coded labels, see defined colours: 'user_col'
fgph1.plotxy( fgph_vx, fgph_vy0, fgph_vy1, "drkr", "r", "ofw", "drkb", "blk", "pc srate", "label2" );	//3 traces cols, bkgd col, axis col, text col, and trace colour coded labels, see defined colours: 'user_col'

}













void plot_mgraph0()
{
fgph_vx.clear();
fgph_vy0.clear();
fgph_vy1.clear();

string s1 = fgph0.label();
s1 += " - plot_mgraph0()";
fgph0.copy_label( s1.c_str() );
fgph0.position( 10, 130 );
fgph0.scale_y( 1.0, 1.0, 1.0, 0.1 );
fgph0.shift_y( 0.0, -0.1, -0.10, -0.2 );
fgph0.font_size( 9 );
fgph0.set_sig_dig( 2 );
fgph0.sample_rect_hints_distancex = 0;
fgph0.sample_rect_hints_distancey = 0;

#define twopi 2*(float)M_PI
int cnt = 1000;
for( int i = 0; i < cnt; i++ )
	{
	float f0 = sinf( (float)i/cnt * twopi * 5 );
	float f1 = 0.8*sinf( (float)i/cnt * twopi * 10 );
	fgph_vx.push_back( i );
	fgph_vy0.push_back( f0 + f1 );
	}

//fgph.plotxy( gph0_vx, gph0_vamp0, gph0_vamp1, gph0_vamp2, gph0_vamp4,"brwn", "drkg", "drkcy", "drkr", "ofw", "drkb", "drkgry", "trace 1", "trace 2", "trace 3", "trace 4"  );
//fgph.plotxy( gph0_vx, gph0_vamp0, gph0_vamp1 );
//fgph.plotxy( fgph_vx, fgph_vy0, fgph_vy1 );

//fgph0.plotxy( fgph0_vx, fgph0_vy0, "drky", "ofw", "drkb", "blk", "pc srate" );	//3 traces cols, bkgd col, axis col, text col, and trace colour coded labels, see defined colours: 'user_col'
fgph0.plotxy( fgph_vx, fgph_vy0, "drkr", "ofw", "drkb", "blk", "pc srate" );	//3 traces cols, bkgd col, axis col, text col, and trace colour coded labels, see defined colours: 'user_col'

}
*/



//called by audio callback realtime code
void env_inc( st_envlp_tag *st )
{
if( st->idx < st->cnt )
	{
	int step_cur = st->tbl[st->idx].step_cur;
	
	st->cur_amp += st->tbl[st->idx].delta_amp;

//printf("env envlp0_idx %d %d  %d\n", envlp0_idx, envlp0_cnt, st[envlp0_idx].step_cur );

	step_cur++;
	st->tbl[st->idx].step_cur = step_cur;
	
	if( step_cur == st->tbl[st->idx].step_cnt )
		{
		st->idx++;
		
		st->cur_amp = st->tbl[st->idx].amp;							//this fixes up any minor accumulation errors by setting val exactly

		if( st->idx < st->cnt )
			{
			st->tbl[st->idx].step_cur = 0;
			st->cur_amp = st->tbl[st->idx].amp;
			}
			
		if( st->idx >= st->cnt ) st->running = 0;					//flag done
		}
	else{
		}
//printf("env step_cur %d %f\n", step_cur, envlp0_cur_amp );
	}
}







//env for higher freq part of explosion
void env_build_explosion_env0( int smpl_srate, bool init_as_finished )
{
bool vb = 0;

st_envlp_tag *st = st_envlp0;

st->tbl[0].amp = 0.0f;
st->tbl[0].tim = 0.02f;

st->tbl[1].amp = 1.0f;
st->tbl[1].tim = 2.0f;

st->tbl[2].amp = 0.0f;
st->tbl[2].tim = 1.0f;

int number_of_entries = 3;													//number of envelope entries

st->cnt = 0;
st->idx = 0;
st->cur_amp = st->tbl[0].amp;

if( init_as_finished ) 
	{
	st->idx = number_of_entries;
	st->running = 0;
	}
else{
	st->running = 1;
	}

for( int i = 0; i < ( number_of_entries - 1 ); i++ )
	{
	float diff = st->tbl[i + 1].amp - st->tbl[i].amp;
	float steps = st->tbl[i].tim * smpl_srate;
	
	st->tbl[i].step_cnt = steps;
	st->tbl[i].step_cur = 0;
	st->tbl[i].delta_amp = diff / steps;
	
	if(vb) printf( "build_explosion_env0() - %d: diff %f  steps %f, delta_amp %f\n", i, diff, steps, st->tbl[i].delta_amp );

	}
	
st->cnt = number_of_entries - 1;										//setting this starts off env transitions in audio code

}






//env for low freq part of explosion
void env_build_explosion_env1( int smpl_srate, bool init_as_finished )
{
bool vb = 0;

st_envlp_tag *st = st_envlp1;

st->tbl[0].amp = 0.0f;
st->tbl[0].tim = 0.05f;

st->tbl[1].amp = 3.0f;
st->tbl[1].tim = 0.5f;

st->tbl[2].amp = 0.0f;
st->tbl[2].tim = 0.2f;

st->tbl[3].amp = 1.5f;
st->tbl[3].tim = 1.4f;

st->tbl[4].amp = 0.0f;
st->tbl[4].tim = 5.0f;

int number_of_entries = 5;													//number of envelope entries

st->cnt = 0;
st->idx = 0;
st->cur_amp = st->tbl[0].amp;

if( init_as_finished ) st->idx = number_of_entries;

for( int i = 0; i < ( number_of_entries - 1 ); i++ )
	{
		
	float diff = st->tbl[i + 1].amp - st->tbl[i].amp;
	float steps = st->tbl[i].tim * smpl_srate;
	
	st->tbl[i].step_cnt = steps;
	st->tbl[i].step_cur = 0;
	st->tbl[i].delta_amp = diff / steps;
	
	if(vb) printf( "build_explosion_env1() - %d: diff %f  steps %f, delta_amp %f\n", i, diff, steps, st->tbl[i].delta_amp );

	}
	
st->cnt = number_of_entries - 1;										//setting this starts off env transitions in audio code

}











void env_build_explosion_env2( int smpl_srate, bool init_as_finished )
{
bool vb = 0;

st_envlp_tag *st = st_envlp2;

st->tbl[0].amp = 0.0f;
st->tbl[0].tim = 0.01f;

st->tbl[1].amp = 1.0f;
st->tbl[1].tim = 0.01f;

st->tbl[2].amp = 0.0f;
st->tbl[2].tim = 0.1f;

int number_of_entries = 3;													//number of envelope entries

st->cnt = 0;
st->idx = 0;
st->cur_amp = st->tbl[0].amp;

if( init_as_finished ) st->idx = number_of_entries;

for( int i = 0; i < ( number_of_entries - 1 ); i++ )
	{
		
	float diff = st->tbl[i + 1].amp - st->tbl[i].amp;
	float steps = st->tbl[i].tim * smpl_srate;
	
	st->tbl[i].step_cnt = steps;
	st->tbl[i].step_cur = 0;
	st->tbl[i].delta_amp = diff / steps;
	
	if(vb) printf( "build_explosion_env1() - %d: diff %f  steps %f, delta_amp %f\n", i, diff, steps, st->tbl[i].delta_amp );

	}
	
st->cnt = number_of_entries - 1;										//setting this starts off env transitions in audio code

}














bool start_audio()
{
printf("start_audio()\n" );


rta.verbose = 1;

//rtaudio_probe();									//dump a list of devices to console


RtAudio::StreamOptions options;

options.streamName = cnsAppName;
options.numberOfBuffers = 2;						//for jack (at least) this is hard set by jack's settings and can't be changed via this parameter

options.flags = 0; 	//0 means interleaved, use oring options, refer 'RtAudioStreamFlags': RTAUDIO_NONINTERLEAVED, RTAUDIO_MINIMIZE_LATENCY, RTAUDIO_HOG_DEVICE, RTAUDIO_SCHEDULE_REALTIME, RTAUDIO_ALSA_USE_DEFAULT, RTAUDIO_JACK_DONT_CONNECT
					// !!! when using RTAUDIO_JACK_DONT_CONNECT, you can create any number of channels, you can't do this if auto connecting as 'openStream()' will fail if there is not enough channel mating ports  

options.priority = 0;								//only used with flag 'RTAUDIO_SCHEDULE_REALTIME'



uint16_t device_num_out = 0;						//use 0 for default device to be used
int channels_out = 2;								//if auto connecting (not RTAUDIO_JACK_DONT_CONNECT), there must be enough mathing ports or 'openStream()' will fail
int first_chan_out = 0;

uint16_t frames = 2048;
unsigned int audio_format = RTAUDIO_FLOAT32;		//see rtaudio docs 'RtAudioFormat' for supported format types, adj audio proc code to suit

//st_osc_params.freq0 = 200;							//set up some audio proc callback user params
//st_osc_params.gain0 = 0.1;

//st_osc_params.freq1 = 600;
//st_osc_params.gain1 = 0.1;
//st_rta_arg.usr_ptr = (void*)&st_osc_params;


fvb->create( 48000 );

if( !rta.start_stream_out( device_num_out, channels_out, first_chan_out, srate, frames, audio_format, &options, &st_rta_arg, (void*)cb_audio_proc_rtaudio ) )		//output only
//if( !rta.start_stream_out( device_num_out, channels_out, first_chan_out, srate, frames, audio_format, &options, &st_rta_arg, (void*)cb_audio_proc_rtaudio_migrate ) )		//output only
	{
	printf("start_audio() - failed to open audio device!!!!\n" );
	return 0;	
	}
else{
	printf("start_audio() - audio out device %d opened, options.flags 0x%02x, srate is: %d, framecnt: %d\n", device_num_out, options.flags, rta.get_srate(), rta.get_framecnt()  );							 //output only	
	}

srate = rta.get_srate();
framecnt = rta.get_framecnt();

printf("start_audio() - srate %d, framecnt: %d\n", srate, framecnt );


time_per_sample = 1.0/srate;


//fftw_init( framecnt );

audio_started = 1;
return 1;
}










void stop_audio()
{
printf( "stop_audio()\n" );
audio_started = 0;

mystr m1;
m1.delay_ms( (float)framecnt / srate * 1e3 );												//wait one audio proc period


rta.stop_stream();

fvb->destroy();

//fftw_destroy();

//if ( fftw_p_c0 ) fftw_destroy_plan( fftw_p_c0 );
//fftw_p_c0 = 0;

//if ( fftw_in_c0 ) fftw_free( fftw_in_c0 );
//if ( fftw_out_c0 ) fftw_free( fftw_out_c0 );
//fftw_in_c0 = 0;
//fftw_out_c0 = 0;
}







bool open_audio_file( string sfname )
{
aud_loaded = 0;
audptr = 0;
dfract_audptr = 0;

af0.verb = 1;                                	//store progress in: string 'log'
af0.log_to_printf = 1;                       	//show to console as well


if( !af0.find_file_format( "", sfname, saf0.format ) )
	{
	printf("open_audio_file() - failed to detct audio file format: '%s'\n", sfname.c_str());
	return 0;
	}

if( !af0.load_malloc( "", sfname, 0, saf0 ) )
	{
	printf("open_audio_file() - failed af.load() '%s'\n", sfname.c_str());
	return 0;
	}

audptr = 0;
dfract_audptr = 0;
aud_loaded = 1;

//saf0.encoding = 3;
//saf0.offset = 0;
//saf0.format = en_af_sun;
//saf0.channels = 2;
//saf0.srate = saf1.srate;
//af0.save_malloc( "", "zzdump.au", 32767, saf0 );




//for( int i = 0; i < af0.sizech0; i++ )
	{
//	float f0 = af0.pch0[ i ];											//src
	
//	vf.push_back( f0 );
	
//	float f1 = af0.pch1[ i++ ];
	
//		if( i > 48000 * 22 ) { f1=f2=0;}								//sudden mute to hear reverb decay
//		bf[0][j]=f1*0.8;
//		bf[1][j]=f2*0.8;
	}

return 1;
}









int main(int argc, char **argv)
{
//-------> !!!!!!! Fl::visual( FL_RGB );								//window might need this for offscreen drawing


//Fl::scheme("plastic");							//NOTE!!!!  for some reason the: 'Fl::scheme("plastic")' scheme causes scroll child objs not to be cleared/redrawn correctly,
													//this leave pixel debris when a scroll obj is scrolled, its children leave pixels behind, the: 'Fl::scheme("gtk+")' scheme does not suffer this
Fl::scheme("plastic");
Fl::scheme("gtk+");

string s, s1, fname, dir_sep;
bool add_ini = 1;									//assume need to add ini extension	

//Fl::set_font( FL_HELVETICA, "Helvetica bold");
//Fl::set_font( FL_COURIER, "");
//Fl::set_font( FL_COURIER, "Courier bold italic");

mains_font = fl_font();
mains_fontsize = fl_size();


fname = cnsAppName;							//assume ini file will have same name as app
dir_sep = "";								//assume no path specified, so no / or \ (dos\windows)



//test if window with same name found and ask if still to run this -
// will return 1 if user presses 'Don't Run, Exit' button
if( CheckInstanceExists( cnsAppWndName ) ) return 0;

//getchar();

//linux code
#ifndef compile_for_windows
dir_seperator = "/";									//use unix folder directory seperator
#endif


dir_sep = dir_seperator;



//windows code
//attach a command line console, so printf works
#ifdef compile_for_windows
int hCrt;
FILE *hf;

AllocConsole();
hCrt = _open_osfhandle( (long) GetStdHandle(STD_OUTPUT_HANDLE), _O_TEXT);
hf = _fdopen( hCrt, "w" );
*stdout = *hf;
setvbuf( stdout, NULL, _IONBF, 0 );
#endif


//    test_mystr();
//    test_time();
//    test_dir();


/*
string app_name;
vector<string> vparams;

get_path_app_params( dir_seperator, argc, argv, app_path, app_name, vparams );

app_path = "";
app_name = "zzzzzzapp_name";

*/

get_app_path( app_path );				//get app's path



//path = app_path;


//windows code
#ifdef compile_for_windows
h_mutex1 = CreateMutex( NULL, FALSE, NULL );  // make a win mutex obj, not signalled ( not locked)
#endif




//handle command line params
printf("\n\nLLEM App\n\n");
    printf("~~~~~~~~");


printf("\nSpecify no switches to use config with app's filename in app's dir\n");
printf("\nSpecify '--cf ffff' to use config with filename 'ffff'\n");


if( argc == 1 )
	{
	printf( "-- Using app's dir for config storage.\n" );
	}


if( argc == 3 )
	{
	if( strcmp( argv[ 1 ], "--cf" ) == 0 )
		{
		printf( "-- Using specified filename for config storage.\n" );
		fname = argv[ 2 ];
		app_path = "";
		dir_sep = "";								//no directory seperator
		add_ini = 0;								//user spec fname don't add ini ext
		}
	}





if( app_path.length() == 0 )
	{
	csIniFilename = app_path + fname;					//make config file pathfilename
	}
else{
	csIniFilename = app_path + dir_sep + fname;			//make config file pathfilename
	}
if( add_ini ) csIniFilename += ".ini";                  //need ini if no user spcified fname





printf("\n\n-> Config pathfilename is:'%s', this will be used for config recall and saving.\n",csIniFilename.c_str());



//wndMain = new dble_wnd( 50, 50, 780, 550 );
//wndMain->label( cnsAppWndName );

//add icon to app window, this will also appear in taskbar
	{

	Fl_Pixmap *theapp_16x16_icon0 = new Fl_Pixmap( iconapp_xpm );		//see 'iconapp.xpm'

	theapp_icon0 = new Fl_RGB_Image( theapp_16x16_icon0, Fl_Color(0) );
//	wndMain->icon( theapp_icon );
//	wndMain->xclass("MusicAppClass");

	}



//calc offset window border, for removal when saving settings
//iBorderWidth=wndMain->x();
//iBorderHeight=wndMain->y();
//wndMain->border(1);
//iBorderWidth=wndMain->x()-iBorderWidth;
//iBorderHeight=wndMain->y()-iBorderHeight;





//wndMain->end();
//wndMain->callback((Fl_Callback *)cb_wndmain, wndMain);



wnd_llem = new llem_wnd( 70, 30, 1700, 900, cnsAppWndName );
wnd_llem->end();
Fl_Box *bx = new Fl_Box( 0, 30, 180, 180 );

wnd_llem->size_range( 700, 450 );				//<------------- this also helps with resizing

wnd_llem->resizable( bx );	
wnd_llem->callback((Fl_Callback *)cb_wndmain, wnd_llem );


//wndMain->size_range( 700, 450 );				//<------------- this also helps with resizing




make_pref2_wnd();




LoadSettings(csIniFilename); 


fvb = new freeverb_reverb( 0 );
fvb->verbose = 1;


wnd_llem->icon( theapp_icon0 );

if( wnd_llem ) wnd_llem->init( 1 );							//do this after 'LoadSettings()' so any last filename(s) are loaded

//thrust filter,lf
gain_iir0 = 1.0f;
float filt_freq = 80;
float filt_q = 3;
create_filter_iir( fpt_lowpass, filt_freq, filt_q, iir0 );

//thrust filter, hf
gain_iir1 = 1.0f;
filt_freq = 100;
filt_q = 1;
create_filter_iir( fpt_lowpass, filt_freq, filt_q, iir1 );

//thrust filter - subsonic boost
gain_iir2 = 1.0f;
filt_freq = 35.0f;
filt_q = 3.0f;
create_filter_iir( fpt_lowpass, filt_freq, filt_q, iir2 );


//explosion filter, hf
gain_iir20 = 1.0f;
filt_freq = 600.0f;
filt_q = 0.2f;
create_filter_iir( fpt_lowpass, filt_freq, filt_q, iir20 );


//explosion filter, lf
gain_iir30 = 1.5f;
filt_freq = 70.0f;
filt_q = 3.0f;
create_filter_iir( fpt_lowpass, filt_freq, filt_q, iir30 );



//thrust frying pan noise filter
gain_iir40 = 1.0f;
filt_freq = 300.0f;
filt_q = 0.5f;
create_filter_iir( fpt_lowpass, filt_freq, filt_q, iir40 );



//create_reverb();

slast_wave_fname = "/home/gc/Desktop/mp3/9. I Have No Home - Ed Wood Soundtrack.wav";
open_audio_file( slast_wave_fname );


env_build_explosion_env0( srate, 1 );
env_build_explosion_env1( srate, 1 );
env_build_explosion_env2( srate, 1 );

start_audio();


//update_fonts(); //needed this after LoadSettings() so the ini font value is loaded via font_pref_wnd->Load(p);

//fi_unicode_greek->textfont( font_num );	//needed this after LoadSettings() so the ini font value is loaded via font_pref_wnd->Load(p);
//fi_unicode->textfont( font_num );		//needed this after LoadSettings() so the ini font value is loaded via font_pref_wnd->Load(p);






/*
int cnt = cn_grph_buf_size;
for( int i = 0; i < cnt; i++ )
	{
	vgph0.push_back( grph_bf0[i] );
	vgph1.push_back( grph_bf1[i] );
	vgph1.push_back( grph_bf2[i] );
	}
graph_update( vgph0, vgph1, vgph2 );
*/


//wndMain->show(argc, argv);





if( wnd_llem ) wnd_llem->show();

//plot_mgraph0();
//plot_mgraph1();
//plot_mgraph2();

//fgph0.hide();
//fgph1.hide();
//fgph2.hide();

//fl_font( (Fl_Font) font_num, font_size );





Fl::add_timeout( 0.5, cb_timer1 );		//update controls, post queued messages

fl_message_font( (Fl_Font) font_num, font_size );

 
int iAppRet=Fl::run();

return iAppRet;

}












































