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

//llem_code.cpp
//v1.01	  2021-aug-25	//


//antialiasing is performed by drawing the scene (src image) at 4x the required size, then downsampling by 4x using a filter to create a smaller scene which is what is shown in the gui window (dest image). This produces high quality lines with negligible jaggies, however it is rather cpu intensive, so to keep the frame rate usable some techniques are used to only filter the scene where details exist and have changed. Most of the empty background is not filtered.
//The downsampling filter utilises a large number of src image pixels to create each dest image pixel, typically a 15x15 square of src pixels are convolved with the filter kernel to produce one dest pixel. The resultant dest image has a somewhat blurrier look but this is the trade off to avoid jaggies. Various filter kernels are selectable, and their widths are adjustable which allows sharper dest images, but then aliasing artifacts start to become prominent.
//The code is purely c++,  no sse/simd cpu instructions are used (apart from those implemented by the compiler when it optimises), gpu hardware has not been coded for use. Therefore, there is quite a lot of room to accelerate rendering further.



#include "llem_code.h"


#define cn_deg2rad ( M_PI / 180 )
#define pi ( M_PI )
#define twopi ( M_PI * 2.0 )
#define pi2 ( M_PI / 2.0 )
#define cn_bytes_per_pixel 3

extern llem_wnd *wnd_llem;

extern int mains_font;
extern int mains_fontsize;

//fast_mgraph fgph4;

extern vector<float> fgph_vx;
extern vector<float> fgph_vy0;
extern vector<float> fgph_vy1;
extern vector<float> fgph_vy2;

extern int srate;

extern float timer_period;

//extern float audio_gain;
extern float audio_thrust_gain;
extern float audio_thrust_gain_slewed;
//extern float audio_thrust_gain_slewed_zeroed;
extern float filt_freq0;
extern float filt_q0;
extern void create_filter0();

float bounce_amp;

extern st_envlp_tag st_envlp2[];
extern void env_build_explosion_env0( int smpl_srate, bool init_as_finished );
extern void env_build_explosion_env1( int smpl_srate, bool init_as_finished );
extern void env_build_explosion_env2( int smpl_srate, bool init_as_finished );

extern int pref_allow_bounce_collision_det;
extern int pref_invert_mousewheel;

extern int pref_expertise0;						//refer: 'landing_rating_speed_limit[]'
extern int pref_expertise1;
extern int pref_expertise2;
extern int pref_expertise3;
extern int pref_expertise4;
extern int pref_development_mode;


extern void cb_pref2(Fl_Widget*w, void* v);
extern void DoQuit();

extern bool mute;

float init_posy = 0;													//only used for testing newtonian acceleration and displacement calcs (due to gravity)


mystr mtick_time;
mystr mdrw_time;
mystr mhandy_timer;

uint64_t draw_cnt = 0;
uint64_t draw_crosshair_cnt = 0;
int filter_calc_cnt = 0;
int filter_nonwhite_cnt = 0;


int obj_create_active = 0;
int obj_create_idx = 0;							//0 is ship,    1 is ground (moonscape)
int obj_create_line_idx = 0;
bool obj_create_line_endpoint = 0;
float obj_create_line_dx, obj_create_line_dy;

extern string slast_ship_fname;
extern string slast_moonscape_fname;
extern string sship_state_fname;
string obj_create_fname = slast_ship_fname;


int dbg_0 = 0;

line_clip lineclp;


//function prototypes
void cb_menu_file( Fl_Widget *v, void *w );
void cb_menu_edit( Fl_Widget *v, void *w );
void cb_menu_highscore( Fl_Widget *v, void *w );
void cb_develope( Fl_Widget *v, void *w );
void cb_help( Fl_Widget *w, void *v);


extern void cb_bt_help( Fl_Widget *w, void *v );
extern void cb_btAbout(Fl_Widget *, void *);



extern st_envlp_tag st_envlp0[];
extern st_envlp_tag st_envlp1[];
extern st_envlp_tag st_envlp2[];

extern string slast_highscore_name;


//these MUST match 'en_line_type'
st_line_type_names_tag st_line_type_names[] =
{
"en_lt_ship :0",
"en_lt_ship_collision_det_pads_left :1",								
"en_lt_ship_collision_det_pads_right :2",
"en_lt_ship_collision_det_sides :3",

"en_lt_spare4 :4",
"en_lt_spare5 :5",
"en_lt_spare6 :6",
"en_lt_spare7 :7",
"en_lt_spare8 :8",
"en_lt_spare9 :9",
"en_lt_spare10 :10",
"en_lt_spare11 :11",
"en_lt_spare12 :12",

"en_lt_ground_non_landing_pad :13",
"en_lt_ground_landing_zone_with_fuel :14",
"en_lt_ground_landing_zone_no_fuel :15",
"en_lt_ground_landing_zone_fuel_digit :16",
"en_lt_end_marker",
};


							//used to detect end of enum defs 


//expertise
#define cn_difficulty_options_max 5
float landing_rating_speed_limit[ cn_difficulty_options_max ] = 		//anything more negative (speed downwards) is a crash
{
-5.0f,										//Novice Level
-4.0f,										//Advanced Beginner
-2.5f,										//Competent
-1.5f,										//Proficient
-1.0f,										//Expert
};




// !!! adj 'cn_landing_rating_mesg_max' to number of entries in 's_landing_rating[]'
#define cn_landing_rating_mesg_max 4
string s_landing_rating[ cn_landing_rating_mesg_max ]=
{
"Heavy Landing",			//see 'cn_landing_rating_mesg_max'
"Average Landing",	
"Good Landing",	
"Great Landing",	
};




//--------------------- Main Menu --------------------------


Fl_Menu_Item llem_menuitems_development[] =
{
	{ "&File", 0, 0, 0, FL_SUBMENU },
		{ "&New Game..", 0	, (Fl_Callback *)cb_menu_file, 0 },
//		{ "&Open Settings..", FL_CTRL + 'o'	, (Fl_Callback *)cb_menu_file, 0 },
//		{ "&Save Settings..", FL_CTRL + 's'	, (Fl_Callback *)cb_menu_file, 1, FL_MENU_DIVIDER },

		{ "&Load Moonscape..", 0, (Fl_Callback *)cb_menu_file, 2 },
		{ "E&xit", 0	, (Fl_Callback *)cb_menu_file, 3 },
		{ 0 },

	{ "&Edit", 0, 0, 0, FL_SUBMENU },
		{ "&Preferences", 0, (Fl_Callback *)cb_menu_edit, 0 },
		{ 0 },

	{ "&HighScore", 0, 0, 0, FL_SUBMENU },
		{ "&Show", 0, (Fl_Callback *)cb_menu_highscore, 0 },
		{ 0 },

	{ "&Develope", 0, 0, 0, FL_SUBMENU },
		{ "&Toggle Obj Editor ('e')", 0, (Fl_Callback *)cb_develope, 0, FL_MENU_DIVIDER },
		{ "&Load Ship State...('l' to repeat)", 0, (Fl_Callback *)cb_develope, 1 },
		{ "&Save Ship State...", 0, (Fl_Callback *)cb_develope, 2 },
//		{ "&Restore Window Size...", 0, (Fl_Callback *)cb_debug, 4 },
		{ 0 },

	{ "&Help", 0, 0, 0, FL_SUBMENU },
		{ "'help.txt'", 0				, (Fl_Callback *)cb_help, 0, FL_MENU_DIVIDER  },
		{ "&About", 0				, (Fl_Callback *)cb_help, 1 },
		{ 0 },

	{ 0 }
};




Fl_Menu_Item llem_menuitems[] =
{
	{ "&File", 0, 0, 0, FL_SUBMENU },
		{ "&New Game", 0	, (Fl_Callback *)cb_menu_file, 0 },
//		{ "&Open Settings..", FL_CTRL + 'o'	, (Fl_Callback *)cb_menu_file, 0 },
//		{ "&Save Settings..", FL_CTRL + 's'	, (Fl_Callback *)cb_menu_file, 1, FL_MENU_DIVIDER },

		{ "&Load Moonscape..", 0, (Fl_Callback *)cb_menu_file, 2 },
		{ "E&xit", 0	, (Fl_Callback *)cb_menu_file, 0 },
		{ 0 },

	{ "&Edit", 0, 0, 0, FL_SUBMENU },
		{ "&Preferences", 0, (Fl_Callback *)cb_menu_edit, 0 },
		{ 0 },

	{ "&HighScore", 0, 0, 0, FL_SUBMENU },
		{ "&Show", 0, (Fl_Callback *)cb_menu_highscore, 0 },
		{ 0 },

	{ "&Help", 0, 0, 0, FL_SUBMENU },
		{ "'help.txt'", 0				, (Fl_Callback *)cb_help, 0, FL_MENU_DIVIDER  },
		{ "&About", 0				, (Fl_Callback *)cb_help, 1 },
		{ 0 },

	{ 0 }
};

//----------------------------------------------------------





void cb_help( Fl_Widget *w, void *v)
{
int which = (intptr_t)v;

if( which == 0 )									//help
	{
	cb_bt_help( w, v );
	}

if( which == 1 )									//about
	{
	cb_btAbout(w, v );
	}
}





void cb_menu_file( Fl_Widget *w, void *v )
{
string s1;
mystr m1;

int which = (intptr_t)v;

llem_wnd* ow = (llem_wnd*) w->parent();

if( which == 0 )									//new game
	{
	ow->init( 0 );
	}

if( which == 2 )									//load moonscape
	{
	char *pPathName = fl_file_chooser( s1.c_str(), "*", (const char*)slast_moonscape_fname.c_str(), 0 );
	if (!pPathName) return;
	
	s1 = pPathName;

	
	string sdesc;
	if( ow->file_open_obj( ow->idx_ground, 0, 0, s1, 1, sdesc ) )
		{
		
		
		slast_moonscape_fname = s1;

		m1 = sdesc;
		m1.EscToStr();
		ow->sdesc_ground = m1.szptr();
		
		ow->ground_landing_zone_colour_and_fuel_digits();
		
		ow->zoom_set_detent( 0 );

		}
				
	}


if( which == 3 )									//load moonscape
	{
	DoQuit();
	}
//strpf( s1, "Open Settings is under development: '%s'", last_filename.c_str() );
//fl_alert( s1.c_str(), 0 );
}












void cb_menu_edit( Fl_Widget *w, void *v )
{
string s1;

int which = (intptr_t)v;

if( which == 0 )									//preferences
	{
	cb_pref2( w, v );	
	}

//strpf( s1, "Open Settings is under development: '%s'", last_filename.c_str() );
//fl_alert( s1.c_str(), 0 );
}








//highest(first) to lowest(last)
static int sort_highscore(const st_highscore_tag &s1, const st_highscore_tag &s2 )
{
if( s2.score > s1.score ) return 0;

return 1;
}








bool llem_wnd::file_load_highscore( string fname )
{
bool vb = 1;
mystr m1;	
string s1, s2;

st_highscore_tag oh;

if(vb)printf("file_load_highscore() - reading '%s'\n", fname.c_str() );

vhighscore.clear();
oh.expertise = 0;
oh.score = 0;
oh.best_dy = -100;
oh.crashes = 0;
oh.sdate = "";
oh.sname_player = "";
oh.sname_llem = "";
oh.sdesc_llem = "";
oh.sfname_llem = "";
oh.sname_moonscape = "";
oh.sdesc_moonscape = "";
oh.sfname_moonscape = "";

vhighscore.resize( cn_highscore_entries_max, oh );


if( m1.readfile( fname, 2000 ) )
	{
	vector<string> vstr0, vstr1;

	m1.LoadVectorStrings( vstr0, '\n' );
	if( vstr0.size() == 0 ) return 0;

	for( int i = 0; i < vstr0.size(); i++ )
		{
		m1 = vstr0[i];
		if(vb)printf("file_load_highscore() -vstr0[%d]  '%s'\n", i, vstr0[i].c_str() );

		if( m1.LoadVectorStrings( vstr1, ',' ) )
			{
			if( vstr1.size() < 14 ) continue;
			if(vb)printf("file_load_highscore() - vstr1.size() %d\n", vstr1.size() );

			sscanf( vstr1[0].c_str(), "%d", &oh.expertise ); 

			sscanf( vstr1[1].c_str(), "%d", &oh.score );

			oh.slandings_tot = vstr1[2];

			sscanf( vstr1[3].c_str(), "%f", &oh.best_dy );

			sscanf( vstr1[4].c_str(), "%d", &oh.crashes );


			m1 = vstr1[5];
			m1.EscToStr();
			oh.sdate = m1.szptr();

			m1 = vstr1[6];
			m1.EscToStr();
			oh.stime = m1.szptr();

			m1 = vstr1[7];
			m1.EscToStr();
			oh.sname_player = m1.szptr();

			m1 = vstr1[8];
			m1.EscToStr();
			oh.sname_llem = m1.szptr();

			m1 = vstr1[9];
			m1.EscToStr();
			oh.sdesc_llem = m1.szptr();

			m1 = vstr1[10];
			m1.EscToStr();
			oh.sfname_llem = m1.szptr();

			m1 = vstr1[11];
			m1.EscToStr();
			oh.sname_moonscape = m1.szptr();

			m1 = vstr1[12];
			m1.EscToStr();
			oh.sdesc_moonscape = m1.szptr();

			m1 = vstr1[13];
			m1.EscToStr();
			oh.sfname_moonscape = m1.szptr();

			vhighscore[i] = oh;
			}
		}
	}
else{
	printf("file_load_highscore() - error reading '%s'\n", fname.c_str() );
	return 0;
	}


if( vhighscore.size() > cn_highscore_entries_max ) vhighscore.resize( cn_highscore_entries_max );

//std::stable_sort( vhighscore.begin(), vhighscore.end(), sort_highscore );		//sort vector by highscore


for( int i = 0; i < vhighscore.size(); i++ )
	{
	oh = vhighscore[i];
	if(vb)printf("----- highscore [%03d] -----\n", i );
	if(vb)printf("expertise:        %02d\n", oh.expertise );
	if(vb)printf("score:            %05d\n", oh.score );
	if(vb)printf("best_dy:          %.2f\n", oh.best_dy );
	if(vb)printf("crashes:          %02d\n", oh.crashes );
	if(vb)printf("sdate:            '%s'\n", oh.sdate.c_str() );
	if(vb)printf("stime:            '%s'\n", oh.stime.c_str() );
	if(vb)printf("sname_player:     '%s'\n", oh.sname_player.c_str() );
	if(vb)printf("sname_llem:       '%s'\n", oh.sname_llem.c_str() );
	if(vb)printf("sdesc_llem:       '%s'\n", oh.sdesc_llem.c_str() );
	if(vb)printf("sfname_llem:      '%s'\n", oh.sfname_llem.c_str() );
	if(vb)printf("sname_moonscape:  '%s'\n", oh.sname_moonscape.c_str() );
	if(vb)printf("sdesc_moonscape:  '%s'\n", oh.sdesc_moonscape.c_str() );
	if(vb)printf("sfname_moonscape: '%s'\n", oh.sfname_moonscape.c_str() );
	if(vb)printf("--------------------------\n" );
	}

return 1;
}










//returns index to 'vhighscore[]',  if 'score' can be stored in list at or above the lowest score entry
//else returns -1
int llem_wnd::highscore_has_been_achieved( int score )
{

if( vhighscore.size() == 0 ) return 0;

for( int i = 0; i < vhighscore.size(); i++ )
	{
	st_highscore_tag oh = vhighscore[i];
	if( score >= oh.score ) return i;
	}

//if( vhighscore.size() < (cn_highscore_entries_max - 1) ) return ( vhighscore.size() ); 	//still unused entries in list?
return -1;
}






bool llem_wnd::file_save_highscore( string fname )
{
bool vb = 1;
mystr m1;	
string s1, st;

st_highscore_tag oh;

if(vb)printf("file_save_highscore() - writing '%s'\n", fname.c_str() );


for( int i = 0; i < vhighscore.size(); i++ )
	{
	oh = vhighscore[i];
	if( oh.score == 0 ) continue;

	strpf( s1, "%d,", oh.expertise );
	st += s1;

	strpf( s1, "%d,", oh.score );
	st += s1;

	strpf( s1, "%s,", oh.slandings_tot.c_str() );
	st += s1;

	strpf( s1, "%.2f,", oh.best_dy );
	st += s1;

	strpf( s1, "%d,", oh.crashes );
	st += s1;

	strpf( s1, "%s,", oh.sdate.c_str() );
	st += s1;

	strpf( s1, "%s,", oh.stime.c_str() );
	st += s1;

	m1 = oh.sname_player;
	m1.StrToEscMostCommon3();
	strpf( s1, "%s,", m1.szptr() );
	st += s1;

	m1 = oh.sname_llem;
	m1.StrToEscMostCommon3();
	strpf( s1, "%s,", m1.szptr() );
	st += s1;

	m1 = oh.sdesc_llem;
	m1.StrToEscMostCommon3();
	strpf( s1, "%s,", m1.szptr() );
	st += s1;

	m1 = oh.sfname_llem;
	m1.StrToEscMostCommon3();
	strpf( s1, "%s,", m1.szptr() );
	st += s1;

	m1 = oh.sname_moonscape;
	m1.StrToEscMostCommon3();
	strpf( s1, "%s,", m1.szptr() );
	st += s1;

	m1 = oh.sdesc_moonscape;
	m1.StrToEscMostCommon3();
	strpf( s1, "%s,", m1.szptr() );
	st += s1;

	m1 = oh.sfname_moonscape;
	m1.StrToEscMostCommon3();
	strpf( s1, "%s", m1.szptr() );					//no ending comma
	st += s1;
	
	st += "\n";
	}

m1 = st;

if( !m1.writefile( fname ) )
	{
	if(vb)printf("file_save_highscore() - failed writing '%s'\n", fname.c_str() );
	return 0;
	}

return 1;
}







void llem_wnd::file_load_ship_state( string fname )
{

mystr m1;	

if( m1.readfile( fname, 2000 ) )
	{
	vector<string> vstr0;
			
	m1.LoadVectorStrings( vstr0, '\n' );
	if( vstr0.size() > 0 )
		{
		int idx = idx_ship;
		vobj[idx] = st_obj_ship_copy;
		
		sscanf( vstr0[0].c_str(), "%f %f %f %f %f %f", &vobj[idx].x, &vobj[idx].y, &vobj[idx].dx, &vobj[idx].dy, &vobj[idx].theta_detentzone, &thrust_wheel );

		printf( "cb_debug() - '%s' %f %f %f %f %f %f\n", vstr0[0].c_str(),  vobj[idx].x, vobj[idx].y, vobj[idx].dx, vobj[idx].dy, vobj[idx].theta_detentzone, thrust_wheel );

		animate_bounce = 0;

		game_state = en_gs_landing;

//			set_thrust( 0.0 );

//			vobj[idx].x = 267.7;
//			vobj[idx].y = -457.0;

//			vobj[idx].dx = 1.4;
//			vobj[idx].dy = 0;

		zoom_extents_goes_off = 1;
		panx = vobj[idx].x;
		pany = vobj[idx].y;

		zoomx = zoomy = 12;
		zoom_last = 0;
		zoom_detent_last = 3;						//set some value to cause a zoom change
		zoom_holdoff_time_cnt = 0.25;				//set some value to cause a zoom change

		ground_landing_restore_fuel_levels();
		ground_landing_zone_fuel_remove_digits();
		ground_landing_zone_colour_and_fuel_digits();
		
//			pause = 0;
		scrn_regen = 1;
		}
	}
}













void llem_wnd::highscore_enter()
{
string s1, sname;
mystr m1;

int score0 = score; 

if( !b_highscore_ask ) return;


if( score0 > 0 )
	{
	int idx = highscore_has_been_achieved( score0 );
	printf("highscore_enter() - idx: %d\n", idx );
//getchar();

	if( idx >= 0 )
		{
		strpf( s1, "Your score places you at position %d in Highscore list.\nEnter your name: ", idx+1 );
		char *sz = fl_input( s1.c_str(), slast_highscore_name.c_str() );

		if( sz != 0 )
			{
			slast_highscore_name = sz;
			sname = sz;
//			b_highscore_ask = 0;
			}
		else{
			sname = "Anonymous";
			}
		st_highscore_tag oh;

		struct tm tt;
		m1.get_time_now( tt );

		string dow, dom, mon_num, mon_name, year, year_short;

		string sdate;
		m1.make_date_str( tt, dow, dom, mon_num, mon_name, year, year_short );
		strpf( sdate, "%s-%s-%s", year.c_str(), mon_num.c_str(), dom.c_str() );

		string hrs, mins, secs, time;
		m1.make_time_str( tt, hrs, mins, secs, time );


		oh.expertise = expertise_level;
		oh.score = score0;

		strpf( s1, "%02d/%02d", landings_tot, landing_zones_avail );
		oh.slandings_tot = s1;

		oh.best_dy = best_dy;
		oh.crashes = crashings_tot;
		
		oh.sdate = sdate;

		oh.stime = time;
		oh.sname_player = sname;
		oh.sname_llem = "name_llem";
		oh.sdesc_llem = "desc_llem";
		oh.sfname_llem = slast_ship_fname;
		oh.sname_moonscape = "name_moonscape";
		oh.sdesc_moonscape = "sdesc_moonscape";
		oh.sfname_moonscape = slast_moonscape_fname;

		vhighscore.insert( vhighscore.begin() + idx, oh );
		vhighscore.resize( cn_highscore_entries_max );
		}
	}

//std::stable_sort( vhighscore.begin(), vhighscore.end(), sort_highscore );		//sort vector by highscore

file_save_highscore( cns_highscore_fname );
}









void cb_menu_highscore( Fl_Widget *w, void *v )
{
string s1, s2;
string sexpt;
mystr m1;

llem_wnd* ow = (llem_wnd*) w->parent();

int which = (intptr_t)v;



if( which == 0 )									//toggle obj editor
	{
	ow->set_pause( 1 );
	
	string s1, st;

	Fl_Window *wnd = new Fl_Window( ow->x()+20, ow->y()+20, 800, 500 );
	wnd->label("High Score List");
	Fl_Input *teText = new Fl_Input( 10, 10, wnd->w()-20, wnd->h()-20, "");
	teText->type(FL_MULTILINE_OUTPUT);
	teText->textfont(4);
	teText->textsize(9);


		//01 ....  AdnceBeginr  001400   -1.30      1          2022-09-04   12:46:53    Will!

	st = "Place    Expertise     Score    Landings   BestDy     Crashes    Date         Time        Name";
	
	st += '\n';
	st += "--------------------------------------------------------------------------------------------------";
	st += '\n';
	for( int i = 0; i < ow->vhighscore.size(); i++ )
		{
		st_highscore_tag oh = ow->vhighscore[i];

		if( oh.score <= 0 ) continue;
		
		string sline;
		strpf( s1, "%02d ....  ", i+1 );		
		sline += s1;

		
		if( oh.expertise == 0 ) sexpt = "Novice        ";
		if( oh.expertise == 1 ) sexpt = "AdnceBeginr   ";
		if( oh.expertise == 2 ) sexpt = "Competent     ";
		if( oh.expertise == 3 ) sexpt = "Proficient    ";
		if( oh.expertise == 4 ) sexpt = "Expert        ";
		sline += sexpt;



		strpf( s1, "%06d   ", oh.score );				
		sline += s1;

		strpf( s1, "%s      ", oh.slandings_tot.c_str() );				
		sline += s1;

		strpf( s1, "%.2f",  oh.best_dy );				
		m1 = s1;
		m1.padstr( s1, 11, 11 );		
		sline += s1;

		strpf( s1, "%d          ", oh.crashes );				
		sline += s1;

		strpf( s1, "%s   ", oh.sdate.c_str() );		
		sline += s1;

		strpf( s1, "%s    ", oh.stime.c_str() );		
		sline += s1;

		strpf( s1, "%s", oh.sname_player.c_str() );		
		sline += s1;
		
		st += sline;
		st += '\n';
		
		
/*
		oh.expertise = expertise_level;
		oh.score = score0;
		oh.sdate = s1;
 strpf( s1, "%s,  %s,  Built: %s\n", cnsAppWndName, "v1.01", cns_build_date );		
		oh.stime = time;
		oh.sname_player = slast_highscore_name;
		oh.sname_llem = "name_llem";
		oh.sdesc_llem = "desc_llem";
		oh.sfname_llem = "fname_llem";
		oh.sname_moonscape = "name_moonscape";
		oh.sdesc_moonscape = "sdesc_moonscape";
		oh.sfname_moonscape = "fname_moonscape";

		vhighscore.insert( vhighscore.begin() + idx, oh );
		vhighscore.resize( cn_highscore_entries_max );
*/
		}



	//strpf(s,"%s, v1.05, Mar 2011\n\nBasic app skeleton foundation to build onto...",cnsAppWndName);
	teText->value(st.c_str());
	wnd->end();

	#ifndef compile_for_windows
	wnd->set_modal();
	#endif

	wnd->show();


	}
}





void cb_develope( Fl_Widget *w, void *v )
{
string s1;

llem_wnd* ow = (llem_wnd*) w->parent();

int which = (intptr_t)v;


if( which == 0 )									//toggle obj editor
	{
	ow->do_key( 'e', 1 );
	}



if( which == 1 )									//load ship state
	{
	char *pPathName = fl_file_chooser( "Load Ship State ?", "*", (const char*)sship_state_fname.c_str(), 0 );
	if (!pPathName) return;
	
	sship_state_fname = pPathName;

	ow->file_load_ship_state( sship_state_fname );
	}

if( which == 2 )									//save ship state
	{
	char *pPathName = fl_file_chooser( "Save Ship State ?", "*", (const char*)sship_state_fname.c_str(), 0 );
	if (!pPathName) return;
	
	mystr m1;	
	bool do_write = 0;
	unsigned long long int filesz;
	if ( m1.filesize( pPathName, filesz ) )
		{
		strpf( s1, "File already exists, Overwrite it? : '%s'", pPathName );
		int ret = fl_choice( s1.c_str(),"Cancel","Overwrite", 0 );
		if( ret == 1 )
			{
			do_write = 1;
			}
		}
	else{
		do_write = 1;
		}

	if( do_write )
		{
		sship_state_fname = pPathName;

		int idx = ow->idx_ship;
		strpf( s1, "%f %f %f %f %f %f\n", ow->vobj[idx].x, ow->vobj[idx].y, ow->vobj[idx].dx, ow->vobj[idx].dy, ow->vobj[idx].theta_detentzone, ow->thrust_wheel );

		m1 = s1;
		m1.writefile( sship_state_fname );
		}
	}



/*
if( which == 4 )									//restore window size
	{
	wnd_llem->iconize();
	wnd_llem->hide();
	wnd_llem->show();
	}
*/
//strpf( s1, "Open Settings is under development: '%s'", last_filename.c_str() );
//fl_alert( s1.c_str(), 0 );
}





/*
void plot_mgraph0_llem( float *kern0, int kern_size )
{
fgph_vx.clear();
fgph_vy0.clear();
fgph_vy1.clear();

string s1 = fgph4.label();
s1 += " - plot_mgraph0_llem() - filter kernel";
fgph4.copy_label( s1.c_str() );
fgph4.position( 10, 130 );
fgph4.scale_y( 1.0, 1.0, 1.0, 0.1 );
fgph4.shift_y( 0.0, -0.1, -0.10, -0.2 );
fgph4.font_size( 9 );
fgph4.set_sig_dig( 2 );
fgph4.sample_rect_hints_distancex = 0;
fgph4.sample_rect_hints_distancey = 0;

int cnt = kern_size;
for( int i = 0; i < cnt; i++ )
	{
	float f0 = sinf( (float)i/cnt * twopi * 5 );
	float f1 = 0.8*sinf( (float)i/cnt * twopi * 10 );
	
	f0 = kern0[i];
	fgph_vx.push_back( i );
	fgph_vy0.push_back( f0 );
	}

//fgph.plotxy( gph0_vx, gph0_vamp0, gph0_vamp1, gph0_vamp2, gph0_vamp4,"brwn", "drkg", "drkcy", "drkr", "ofw", "drkb", "drkgry", "trace 1", "trace 2", "trace 3", "trace 4"  );
//fgph.plotxy( gph0_vx, gph0_vamp0, gph0_vamp1 );
//fgph.plotxy( fgph_vx, fgph_vy0, fgph_vy1 );

//fgph0.plotxy( fgph0_vx, fgph0_vy0, "drky", "ofw", "drkb", "blk", "pc srate" );	//3 traces cols, bkgd col, axis col, text col, and trace colour coded labels, see defined colours: 'user_col'
fgph4.plotxy( fgph_vx, fgph_vy0, "drkr", "ofw", "drkb", "blk", "pc srate" );	//3 traces cols, bkgd col, axis col, text col, and trace colour coded labels, see defined colours: 'user_col'

}
*/




int sort_cmp_line_by_x(const st_line_tag &s1, const st_line_tag &s2 )
{
if( s1.x1 < s2.x1 ) return 1;											//return 1 if '<'		!! important to get this right for 'stable_sort()'

return 0;																//return 0 if '>='
}





double llem_sinc(double x)
{
if (x != 0)
	{
	x *= M_PI;
	return ( sin(x)/x);
	}
return 1;
}



static bool filter_kernel( en_filter_window_type_tag wnd_type, float *w, uint16_t *w16_t, int N )
{
int ilow;
float x;


for( double n = 0; n < N; n++ )
	{
	int i = (int)n;
    
	switch( wnd_type )
		{
		case fwt_rect:
			w[ i ] = 1.0;                              										//rect window impulse resp
		break;

		case fwt_bartlett:
            ilow = ( N - 1.0 ) / 2.0 ;                        //work out which formula to use (what side of the triangle peak we are in)
            
            if( i <= ilow ) w[ i ] = 2.0 * n / ( N - 1.0 ) ;                                //bartlett/triangle window impulse resp
            else w[ i ] =  2.0 - 2.0 * n / ( N - 1.0 ) ;
		break;

		case fwt_hann:
			w[ i ] = 0.5 * ( 1.0 - cos( ( twopi * n / ( N - 1.0 ) ) ) );                     //hann window impulse resp
		break;

		case fwt_bartlett_hanning:
            w[ i ] = 0.62 - 0.48 * fabs( n / ( N - 1.0 ) - 0.5 ) + 0.38 * cos( ( twopi * ( n / ( N - 1.0 ) - 0.5 ) ) );        //bartlett-hanning window impulse resp
		break;

		case fwt_hamming:
            w[ i ] = 0.54 - 0.46 * cos( twopi * n / ( N - 1.0 ) );            //hamming window impulse resp, note the text in  http://www.mikroe.com.. listed above is slightly incorrect, used wikipedia version
		break;

		case fwt_blackman:
            w[ i ] = 0.42 - 0.5 * cos( twopi * n / ( N - 1.0 ) ) + 0.08 * cos( 2.0 * twopi * n / ( N - 1.0 ) );        //blackman window impulse resp
		break;

		case fwt_blackman_harris:
            w[ i ] = 0.35875 - 0.48829 * cos( twopi * n / ( N - 1.0 ) ) + 0.14128 * cos( 2.0 * twopi * n / ( N - 1.0 ) ) - 0.01168 * cos( 3.0 * twopi * n / ( N - 1.0 ) );        //blackman-harris window impulse resp
		break;

		case fwt_lanczos1:
			x = 2.0f * (float)n/(N-1);
			x -= 1;
			if (x < 0) x = - x;
			if (x < 1) w[ i ] = ( llem_sinc( x ) * llem_sinc( x / 1.0 ) );
			else w[ i ] = 0;
		break;

		case fwt_lanczos1_5:
			x = 3.0f * (float)n/(N-1);
			x -= 1.5f;
			if (x < 0) x = - x;
			if (x < 1.5) w[ i ] = ( llem_sinc( x ) * llem_sinc( x / 1.5 ) );
			else w[ i ] = 0;
		break;


		case fwt_lanczos2:
			x = 4.0f * (float)n/(N-1);
			x -= 2;
			if (x < 0) x = - x;
			if (x < 2) w[ i ] = ( llem_sinc( x ) * llem_sinc( x / 2.0 ) );
			else w[ i ] = 0;
		break;

		case fwt_lanczos3:
			x = 6.0f * (float)n/(N-1);
			x -= 3;
			if (x < 0) x = - x;
			if (x < 3) w[ i ] = ( llem_sinc( x ) * llem_sinc( x / 3.0 ) );
			else w[ i ] = 0;
		break;

		default:
			printf( "filter_fir_windowed() - unknown filter window type(wnd): %u\n", wnd_type );
			return 0;
		break;
        }

    w16_t[i] = 65535 * w[ i ]; 
	}
	
return 1;
}






//----------------------------------------------------------

llem_wnd::llem_wnd( int xx, int yy, int wid, int hei, const char *label ) : Fl_Double_Window( xx, yy, wid ,hei, label )
{
menu = new Fl_Menu_Bar( 0, 0, w(), 25 );
//menu = 0;

menu->textsize(12);
menu->copy( llem_menuitems, (void*) 0 );
menu->show();

menu_hei = 0;



ctrl_key = shift_key = 0;
left_button = 0;
middle_button = 0;
right_button = 0;

key_a = 0;
key_d = 0;
key_w = 0;
key_r = 0;
key_f = 0;
key_k = 0;
key_m = 0;
key_x = 0;
key_y = 0;
key_s = 0;
key_z = 0;
key_up = 0;
key_down = 0;
key_left = 0;
key_right = 0;
key_space = 0;

toggle_space = 0;

en_filter_window_type_tag wnd_type = fwt_blackman_harris;

rebuild_filter_kernel = 1;
kern_size = 15;														//odd gives a 1.0 peak
float kern0[ 256 ];

//offscr = 0;
pixbf0 = 0;
pixbf1 = 0;



bounding_scale_down = 1;
tframe = 0;

midx = w() / 2;
midy = h() / 2;

adj_show_filtered = 1;
adj_scale = 0.125;
adj_angle = 0;
adj_offsx = 4470;
adj_offsy = -920;
adj_line_width = 3;
adj_filter = fwt_blackman_harris;

first_draw = 1;

set_pause( 0 );

mtick_time.time_start( mtick_time.ns_tim_start );
mhandy_timer.time_start( mhandy_timer.ns_tim_start );

measured_tframe = timer_period;

scrn_regen = 0;
//scrn_bf_bkgd = 0;

backgrnd_r = 255;
backgrnd_g = 255;
backgrnd_b = 255;

line_r = 0;
line_g = 0;
line_b = 0;

plot_col_r = 0;
plot_col_g = 0;
plot_col_b = 0;


ship_rect_x1 = 0;
ship_rect_y1 = 0;
ship_rect_x2 = 0;
ship_rect_y2 = 0;
ship_rect_ww = 0;
ship_rect_hh = 0;


//slast_moonscape_fname = "zzground.txt";


flash_time0 = 0.1f;
flash_cnt0 = flash_time0;
flash0 = 0;

flash_time1 = 0.3f;
flash_cnt1 = flash_time1;
flash1 = 0;

draws_missed = 0;

file_load_highscore( cns_highscore_fname );

//init( 1 );
}






llem_wnd::~llem_wnd()
{
//if ( offscr != 0 ) fl_delete_offscreen( offscr );
if ( pixbf0 != 0 ) delete[] pixbf0;
if ( pixbf0 != 0 ) delete[] pixbf1;

//if ( pixbf_r0 != 0 ) delete[] pixbf_r0;
//if ( pixbf_g0 != 0 ) delete[] pixbf_g0;
//if ( pixbf_b0 != 0 ) delete[] pixbf_b0;

}











void llem_wnd::init( bool full_init )
{
string s1;
mystr m1;

srand( time(0) );

show_help = 0;

show_debug_info = 0;


develope_mode = pref_development_mode;

//develope_mode = 1;

if( develope_mode >= 1 ) menu->copy( llem_menuitems_development, (void*) 0 );
else menu->copy( llem_menuitems, (void*) 0 );


set_pause( 0 );
exploding_time_key_inhibit = 2;
key_inhibit = 0;

game_state = en_gs_landing;
animate_bounce = 0;
animate_bounce_time_cnt = 0;

fuel_new_game = 1200;
fuel = fuel_new_game;
fuel_bonus = 0;
fuel_earnt_tot = 0;
landings_tot = 0;
crashings_tot = 0;
crash_limit = 3;
score = 0;

vobj.clear();

//NOTE make SURE clipper is set to allow for the fact that 'set_rect()' will plot a rect larger than spec bounds (by filter 'kern_size/2'), else you will get buffer corruption/crashing
//NOTE make SURE clipper is set to allow for the fact that 'set_pixel3()' will plot a pixel before its 'xx,yy' coords, else you will get buffer corruption/crashing

//lineclp.clip_vportleft = kern_size/2;					//NOTE: downward is positive for 4x times line plotter, for: 'line_clip()', 'clip_poly()' algorithm
//lineclp.clip_vportright = w()*4 - kern_size/2;

//lineclp.clip_vporttop = h()*4 - kern_size/2;				//bottom limit of plot area, NOTE: downward is positive for 4x times line plotter, for: 'line_clip()', 'clip_poly()' algorithm
//lineclp.clip_vportbot = kern_size/2;						//top limit of plot area (top of screen), SET THIS to at least 'kern_size/2' else 'set_rect()' will cause buffer corruption/crashing

//if( full_init )
//	{
//	set_clipper_details( 0 );
//	}

first_draw = 1;

accel_grav = -10;

time_sim = 0;

panx = 0.0f;//1056.0f;
pany = 0.0f;
//pan_auto = 1;


adj_scale = 1.0;

zoom_extents = 0;
zoom_extents_goes_off = 0;
zoom_extents_turn_on_time_cnt = -0.1;
zoom_extents_turn_on_time = 8;


zoom_auto = 1;
zoomx = zoomy = 1.0;
//zoom_lastx = zoomx;
//zoom_lasty = zoomy;

zoom_detent_last = -1;					//any number that will tigger a zoom change
zoom_last = 0;
zoom_holdoff_time_cnt = 0.25;			//some low time value to tigger a zoom change

view_x0 = 10;
view_y0 = 0;
view_x1 = 1800*4;
view_y1 = 900*4;

adj_offsx = 679;//1056.0f;
adj_offsy = -987;//520;

adj_offsx = 0;
adj_offsy = 800;//520;

//adj_offsx = 267.7;						//use for fast landing as its near a landing zone
//adj_offsy = -457.0;//520;

//adj_offsx = -1255;
//adj_offsy = 359.8;//520;


panx = adj_offsx;
pany = adj_offsy;

ship_vel_landing_crash = -4.0f;
landing_rating_quantized = 0;


int shape = 0;
bool affected_by_grav = 1;
bool apply_dxdy = 1;
float scale = 1;
float xx = 0;
float yy = 0;
float dx = 0;
float dy = 0;
float time_to_live = 5 / tframe;

thrust_wheel = 0.0f;
flame_on = 0;
flame_flicker = 0;


add_user_ship( shape, affected_by_grav, apply_dxdy, scale, xx, yy, dx, dy, time_to_live );


xx = 0;
yy = 0;

vobj[0].x = adj_offsx;
vobj[0].y = adj_offsy;
//vobj[0].offsx = 0;
//vobj[0].offsy = 0;

string sdesc;
bool clear_vsub = 0;
file_open_obj( 0, 0, 0, slast_ship_fname, clear_vsub, sdesc );

m1 = sdesc;
m1.EscToStr();
sdesc_llem = m1.szptr();

printf("slast_ship_fname '%s'\n", slast_ship_fname.c_str() );

printf("sdesc_llem '%s'\n", sdesc_llem.c_str() );
//getchar();

obj_vertex_offset( 0, 0, 0, cn_ship_scale, cn_ship_scale );

calc_objs_line_midpoints( idx_ship );

set_line_update_flag( 0, 1 );

explode_ship_init_params();

st_obj_ship_copy = vobj[0];

init_posy = vobj[idx_ship].y;


m1 = sdesc_llem;
m1.StrToEscMostCommon3();

if( full_init ) file_save_obj( 0, "zzzllem_bak.txt", m1.szptr() );

//obj_vertex_offset( idx_ship, 0, 100, 1, 1 );


if( 1 )
	{
	add_ground( shape, affected_by_grav, apply_dxdy, scale, xx, yy, dx, dy, time_to_live );

	s1 = slast_moonscape_fname;
//	string s2 = "zzground_back.txt";

//printf( "init() - slast_moonscape_fname '%s'\n", slast_moonscape_fname.c_str() );
//getchar();

	float sclex = 11.2;
	float scley = 5.9;

//	file_open_obj2( 1, s1, sclex, scley );

	string sdesc;
	if( !file_open_obj( 1, 0, 0, s1, clear_vsub, sdesc ) )
		{
		}
		
	m1 = sdesc;
	m1.EscToStr();
	
	sdesc_ground = m1.szptr();
	if( full_init ) file_save_obj( 1, "zzzground_bak.txt", sdesc );
	
		
//	file_save_obj( 1, s2 );
	
	vobj[1].x = 0;
	vobj[1].y = 0;
	
//	obj_vertex_offset( 1, -2500, -1000, 1, 1 );
	
	ground_landing_zone_colour_and_fuel_digits();
	}


//-------- debug -------
if( 0 )
	{
	st_line_tag oo;
	oo.flags = en_ft_update;

	oo.drawn_x1 = 0;														//put anything in here thats initally valid for plotting
	oo.drawn_y1 = 0;
	oo.drawn_x2 = 1;
	oo.drawn_y2 = 1;

	oo.x1 = 10;
	oo.y1 = 10;
	oo.x2 = 3000;
	oo.y2 = 1500;

	oo.r = 0;
	oo.g = 0;
	oo.b = 0;

	vobj[1].vline.push_back( oo );
	}
//----------------------

calc_objs_line_midpoints( idx_ground );

set_line_update_flag( 1, 1 );








add_crosshair( shape, affected_by_grav, apply_dxdy, scale, xx, yy, dx, dy, time_to_live );
set_line_update_flag( 2, 1 );





//vobj[0].dx = 1.0;

//add_rect( 0, 0, 50, 50, 1 );

//add_rect( 200, 200, 300, 200, 1 );



taking_off_wait_for_a_draw_to_occur = 0;

expertise_level = 1;

ship_restore( 1, 1 );



b_highscore_ask = 1;
best_dy = -100;

//if( develope_mode == 0 ) ship_restore( 1, 1 );
}
















//NOTE make SURE clipper is set to allow for the fact that 'set_rect()' will plot a rect larger than spec bounds (by filter 'kern_size/2'), else you will get buffer corruption/crashing
//NOTE make SURE clipper is set to allow for the fact that 'set_pixel3()' will plot a pixel before its 'xx,yy' coords, else you will get buffer corruption/crashing

void llem_wnd::set_clipper_details( int menu_offy )
{
//NOTE make SURE clipper is set to allow for the fact that 'set_rect()' will plot a rect larger than spec bounds (by filter 'kern_size/2'), else you will get buffer corruption/crashing
//NOTE make SURE clipper is set to allow for the fact that 'set_pixel3()' will plot a pixel before its 'xx,yy' coords, else you will get buffer corruption/crashing

//lineclp.clip_vportleft = -w()/2;						//upward is positive, for: 'line_clip()', 'clip_poly()' algorithm
//lineclp.clip_vportright = + w()/2 ;
//lineclp.clip_vporttop = h()/2;						//NOTE: 'clip_vporttop' here is in an increasing direction up the screen
//lineclp.clip_vportbot = -h()/2;

int safety_gap = 5;	//this help avoid buffer corruption/crashing (min seems to be 1, any lower and a crash might occur when moving llem by mouse with high zoom and panning changes)

lineclp.clip_vportleft = kern_size/2 + safety_gap;				//NOTE: downward is positive for 4x times line plotter, for: 'line_clip()', 'clip_poly()' algorithm
lineclp.clip_vportright = w()*4 - kern_size/2 - safety_gap;

lineclp.clip_vporttop = h()*4 - kern_size/2 - safety_gap;			//bottom limit of plot area, NOTE: downward is positive for 4x times line plotter, for: 'line_clip()', 'clip_poly()' algorithm
lineclp.clip_vportbot = kern_size/2 + menu_offy*4 + safety_gap;		//top limit of plot area (top of screen), SET THIS to at least 'kern_size/2' else 'set_rect()' will cause buffer corruption/crashing


//printf( "set_clipper_details() left %d %d  top %d %d\n", lineclp.clip_vportleft, lineclp.clip_vportright, lineclp.clip_vporttop, lineclp.clip_vportbot );

}










void llem_wnd::set_line_update_flag( int idx, bool state )
{

int i = idx;

for( int j = 0; j < vobj[ i ].vline.size(); j++ )
	{
	if( state ) vobj[ i ].vline[j].flags |= en_ft_update;
	else vobj[ i ].vline[j].flags &= ~en_ft_update;
	}

int sub_cnt = vobj[ i ].vsub.size();
if( sub_cnt != 0 )
	{
	for( int k = 0; k < sub_cnt; k++ )
		{
		int line_cnt = vobj[ i ].vsub[k].vline.size();
		for( int j = 0; j < line_cnt; j++ )
			{
			if( state ) vobj[ i ].vsub[k].vline[j].flags |= en_ft_update;
			else  vobj[ i ].vsub[k].vline[j].flags &= ~en_ft_update;
			}
		}
	}
	
}








void llem_wnd::set_line_drawn_flag( int idx, bool state )
{

int i = idx;

for( int j = 0; j < vobj[ i ].vline.size(); j++ )
	{
	vobj[ i ].vline[j].drawn = state;
	}

int sub_cnt = vobj[ i ].vsub.size();
if( sub_cnt != 0 )
	{
	for( int k = 0; k < sub_cnt; k++ )
		{
		int line_cnt = vobj[ i ].vsub[k].vline.size();
		for( int j = 0; j < line_cnt; j++ )
			{
			vobj[ i ].vsub[k].vline[j].drawn = state;
			}
		}
	}
	
}











//----- line intersecting a rectangle (originally a line clip algo, BUT NOT USED like this here ------
//refer https://en.wikipedia.org/wiki/Liang%E2%80%93Barsky_algorithm

// this function gives the maximum
float llem_wnd::LiangBarsky_maxi(float arr[],int n) {
  float m = 0;
  for (int i = 0; i < n; ++i)
    if (m < arr[i])
      m = arr[i];
  return m;
}



// this function gives the minimum
float llem_wnd::LiangBarsky_mini(float arr[], int n) {
  float m = 1;
  for (int i = 0; i < n; ++i)
    if (m > arr[i])
      m = arr[i];
  return m;
}







//NOT USED HERE as a line clipper, rather its used as a line intersecting rectangle detector
//rect defined by xmin,ymin etc params,
//line defined by x1,y1 etc params
//refer https://en.wikipedia.org/wiki/Liang%E2%80%93Barsky_algorithm

bool llem_wnd::LiangBarsky_line_rect_intersect(float xmin, float ymin, float xmax, float ymax, float x1, float y1, float x2, float y2, int dbg_line_idx ) 
{
  // defining variables
  float p1 = -(x2 - x1);
  float p2 = -p1;
  float p3 = -(y2 - y1);
  float p4 = -p3;

  float q1 = x1 - xmin;
  float q2 = xmax - x1;
  float q3 = y1 - ymin;
  float q4 = ymax - y1;

  float posarr[5], negarr[5];
  int posind = 1, negind = 1;
  posarr[0] = 1;
  negarr[0] = 0;

//  rectangle(xmin, ymin, xmax, ymax); // drawing the clipping window
//if( dbg_line_idx == 111 ) printf( "LiangBarsky_line_rect_intersect() - checking line %d\n", dbg_line_idx );

  if ((p1 == 0 && q1 < 0) || (p2 == 0 && q2 < 0) || (p3 == 0 && q3 < 0) || (p4 == 0 && q4 < 0)) {

//if( dbg_line_idx == 111 ) printf( "LiangBarsky_line_rect_intersect() - line is parallel to the rect\n");
      return 0;
  }
  if (p1 != 0) {
    float r1 = q1 / p1;
    float r2 = q2 / p2;
    if (p1 < 0) {
      negarr[negind++] = r1; // for negative p1, add it to negative array
      posarr[posind++] = r2; // and add p2 to positive array
    } else {
      negarr[negind++] = r2;
      posarr[posind++] = r1;
    }
  }
  if (p3 != 0) {
    float r3 = q3 / p3;
    float r4 = q4 / p4;
    if (p3 < 0) {
      negarr[negind++] = r3;
      posarr[posind++] = r4;
    } else {
      negarr[negind++] = r4;
      posarr[posind++] = r3;
    }
  }

  float xn1, yn1, xn2, yn2;
  float rn1, rn2;
  rn1 = LiangBarsky_maxi(negarr, negind); // maximum of negative array
  rn2 = LiangBarsky_mini(posarr, posind); // minimum of positive array

  if (rn1 > rn2)  { // reject
//if( dbg_line_idx == 111 ) printf( "LiangBarsky_line_rect_intersect() - line is outside the rect\n" );
    return 0;
  }

//printf( "LiangBarsky_line_rect_intersect() - line %d intersects the rect\n", dbg_line_idx );
return 1;

//  xn1 = x1 + p2 * rn1;
//  yn1 = y1 + p4 * rn1; // computing new points

//  xn2 = x1 + p2 * rn2;
//  yn2 = y1 + p4 * rn2;

//  setcolor(CYAN);

//  line(xn1, yn1, xn2, yn2); // the drawing the new line

//  setlinestyle(1, 1, 0);

//  line(x1, y1, xn1, yn1);
//  line(x2, y2, xn2, yn2);
}
//----------------------------------------------------




//----- line intersecting line ------

//refer https://www.geeksforgeeks.org/check-if-two-given-line-segments-intersect/


 
// Given three collinear points p, q, r, the function checks if
// point q lies on line segment 'pr'
bool llem_wnd::LineIntersect_onSegment( st_Point p, st_Point q, st_Point r)
{
    if (q.x <= max(p.x, r.x) && q.x >= min(p.x, r.x) &&
        q.y <= max(p.y, r.y) && q.y >= min(p.y, r.y))
       return true;
 
    return false;
}
 
 
 
 
 
// To find orientation of ordered triplet (p, q, r).
// The function returns following values
// 0 --> p, q and r are collinear
// 1 --> Clockwise
// 2 --> Counterclockwise
int llem_wnd::LineIntersect_orientation( st_Point p, st_Point q, st_Point r )
{
    // See https://www.geeksforgeeks.org/orientation-3-ordered-points/
    // for details of below formula.
    int val = (q.y - p.y) * (r.x - q.x) -
              (q.x - p.x) * (r.y - q.y);
 
    if (val == 0) return 0;  // collinear
 
    return (val > 0)? 1: 2; // clock or counterclock wise
}






// The main function that returns true if line segment 'p1q1'
// and 'p2q2' intersect.
bool llem_wnd::LineIntersect_doIntersect( st_Point p1, st_Point q1, st_Point p2, st_Point q2 )
{
    // Find the four orientations needed for general and
    // special cases
    int o1 = LineIntersect_orientation(p1, q1, p2);
    int o2 = LineIntersect_orientation(p1, q1, q2);
    int o3 = LineIntersect_orientation(p2, q2, p1);
    int o4 = LineIntersect_orientation(p2, q2, q1);
 
    // General case
    if (o1 != o2 && o3 != o4)
        return true;
 
    // Special Cases
    // p1, q1 and p2 are collinear and p2 lies on segment p1q1
    if (o1 == 0 && LineIntersect_onSegment(p1, p2, q1)) return true;
 
    // p1, q1 and q2 are collinear and q2 lies on segment p1q1
    if (o2 == 0 && LineIntersect_onSegment(p1, q2, q1)) return true;
 
    // p2, q2 and p1 are collinear and p1 lies on segment p2q2
    if (o3 == 0 && LineIntersect_onSegment(p2, p1, q2)) return true;
 
     // p2, q2 and q1 are collinear and q1 lies on segment p2q2
    if (o4 == 0 && LineIntersect_onSegment(p2, q1, q2)) return true;
 
    return false; // Doesn't fall in any of the above cases
}


//----------------------------------------------------









//in spec 'vobj[idx]' - if any of its lines appear (partial of wholly) in spec rect then set that line's flag to spec 'state'
void llem_wnd::set_line_update_flag_if_in_rect( int idx, int rx1, int ry1, int ww, int hh, bool state )
{

int rx2 = rx1 + ww;
int ry2 = ry1 + hh;

int i = idx;

int obj_cnt = vobj.size();

   
float xx = vobj[ i ].x;
float yy = vobj[ i ].y;
float theta = vobj[ i ].theta;

if( vobj[ i ].col_temp_frame_cnt > 0 ) set_col( vobj[ i ].col_temp );	//show temp col if req
else set_col( vobj[ i ].col );

vector<st_line_tag> vo;
vo = vobj[ i ].vline;
float scl = vobj[ i ].scale;

idx_obj_rect_overlap = -1;
idx_line_rect_overlap = -1;

for( int j = 0; j < vo.size(); j++ )
	{
	int x1 = vo[ j ].drawn_x1;
	int y1 = vo[ j ].drawn_y1;
	int x2 = vo[ j ].drawn_x2;
	int y2 = vo[ j ].drawn_y2;




int dbg_line_idx = j;

float rect_xmin = rx1;
float rect_ymin = ry1;
float rect_xmax = rx1 + ww;
float rect_ymax = ry1 + hh;
float line_x1 = x1;
float line_y1 = y1;
float line_x2 = x2;
float line_y2 = y2;

bool point_inside = LiangBarsky_line_rect_intersect( rect_xmin, rect_ymin, rect_xmax, rect_ymax, line_x1, line_y1, line_x2, line_y2, dbg_line_idx );


/*
	if( x1 > x2 )
		{
		int tmp = x1;
		x1 = x2;
		x2 = tmp;	
		}

	if( y1 > y2 )
		{
		int tmp = y1;
		y1 = y2;
		y2 = tmp;	
		}

	bool point_inside = 0;
	bool inx = 0;
	bool iny = 0;

	if( ( x1 >= rx1 ) && ( x1 <= rx2 ) ) inx = 1;
	if( ( x2 >= rx1 ) && ( x2 <= rx2 ) ) inx = 1;
	if( ( y1 >= ry1 ) && ( y1 <= ry2 ) ) iny = 1;
	if( ( y2 >= ry1 ) && ( y2 <= ry2 ) ) iny = 1;

	if( inx && iny ) point_inside = 1;

	if( !point_inside )
		{
		bool inx = 0;
		bool iny = 0;

		if( ( rx1 >= x1 ) && ( rx1 <= x2 ) ) inx = 1;
		if( ( rx2 >= x1 ) && ( rx2 <= x2 ) ) inx = 1;
		if( ( ry1 >= y1 ) && ( ry1 <= y2 ) ) iny = 1;
		if( ( ry2 >= y1 ) && ( ry2 <= y2 ) ) iny = 1;
		if( inx && iny ) point_inside = 1;
		}






if( ( j == 111 ) || ( j == 112 ) || ( j == 113 ) )
	{
//	printf( "set_line_update_flag_if_in_rect() - line %d: x1 %d %d %d %d   rx1 %d %d %d %d inside %d\n", j, x1, y1, x2, y2, rx1, ry1, rx2, ry2, point_inside  );
	
	}
*/

/*
	if( ( x1 >= rx1 ) && ( x1 <= rx2 ) ) inx = 1;

	if( ( y1 >= ry1 ) && ( y1 <= ry2 ) ) iny = 1;	
		{
		if( ( y1 >= ry1 ) && ( y1 <= ry2 ) ) 
			{
			point_inside = 1;
			}
		}
	
	if( !point_inside )
		{
		if( ( x2 >= rx1 ) && ( x2 <= rx2 ) )
			{
			if( ( y2 >= ry1 ) && ( y2 <= ry2 ) ) 
				{
				point_inside = 1;
				}
			}
		}
*/
		
	if( point_inside )
		{
		idx_obj_rect_overlap = i;
		idx_line_rect_overlap = j;
//		printf("set_line_update_flag_if_in_rect() - line idx %d\n", j );
		if( state ) vobj[ i ].vline[j].flags |= en_ft_update;
		else vobj[ i ].vline[j].flags &= ~en_ft_update;
		}

	}


//sub objs
int sub_cnt = vobj[ i ].vsub.size();
if( sub_cnt != 0 )
	{
	for( int k = 0; k < sub_cnt; k++ )
		{
		int line_cnt = vobj[ i ].vsub[k].vline.size();
		for( int j = 0; j < line_cnt; j++ )
			{
			vo = vobj[ i ].vsub[k].vline;
			for( int j = 0; j < vo.size(); j++ )
				{
				int x1 = vo[ j ].drawn_x1;
				int y1 = vo[ j ].drawn_y1;
				int x2 = vo[ j ].drawn_x2;
				int y2 = vo[ j ].drawn_y2;

int dbg_line_idx = j;

float rect_xmin = rx1;
float rect_ymin = ry1;
float rect_xmax = rx1 + ww;
float rect_ymax = ry1 + hh;
float line_x1 = x1;
float line_y1 = y1;
float line_x2 = x2;
float line_y2 = y2;

bool point_inside = LiangBarsky_line_rect_intersect( rect_xmin, rect_ymin, rect_xmax, rect_ymax, line_x1, line_y1, line_x2, line_y2, dbg_line_idx );

/*
				if( x1 > x2 )
					{
					int tmp = x1;
					x1 = x2;
					x2 = tmp;	
					}

				if( y1 > y2 )
					{
					int tmp = y1;
					y1 = y2;
					y2 = tmp;	
					}

				bool point_inside = 0;
				bool inx = 0;
				bool iny = 0;

				if( ( x1 >= rx1 ) && ( x1 <= rx2 ) ) inx = 1;
				if( ( x2 >= rx1 ) && ( x2 <= rx2 ) ) inx = 1;
				if( ( y1 >= ry1 ) && ( y1 <= ry2 ) ) iny = 1;
				if( ( y2 >= ry1 ) && ( y2 <= ry2 ) ) iny = 1;

				if( inx && iny ) point_inside = 1;
*/
				if( point_inside ) 
					{
					printf( "set_line_update_flag_if_in_rect() - intersect was in a vsub[] line\n" );

					if( state ) vobj[ i ].vsub[k].vline[j].flags |= en_ft_update;
					else  vobj[ i ].vsub[k].vline[k].flags &= ~en_ft_update;
					}

				}
			}
        }

    }
	
}








//in spec 'vobj[idx]' - if any of its lines appear (partial of wholly) in spec rect then set that line's flag to spec 'state'
void llem_wnd::set_line_update_flag_if_in_rect_old( int idx, int rx1, int ry1, int ww, int hh, bool state )
{

int rx2 = rx1 + ww;
int ry2 = ry1 + hh;

int i = idx;

int obj_cnt = vobj.size();

   
float xx = vobj[ i ].x;
float yy = vobj[ i ].y;
float theta = vobj[ i ].theta;

if( vobj[ i ].col_temp_frame_cnt > 0 ) set_col( vobj[ i ].col_temp );	//show temp col if req
else set_col( vobj[ i ].col );

vector<st_line_tag> vo;
vo = vobj[ i ].vline;
float scl = vobj[ i ].scale;

idx_obj_rect_overlap = -1;
idx_line_rect_overlap = -1;

for( int j = 0; j < vo.size(); j++ )
	{
	int x1 = vo[ j ].drawn_x1;
	int y1 = vo[ j ].drawn_y1;
	int x2 = vo[ j ].drawn_x2;
	int y2 = vo[ j ].drawn_y2;




int dbg_line_idx = j;

float rect_xmin = rx1;
float rect_ymin = ry1;
float rect_xmax = rx1 + ww;
float rect_ymax = ry1 + hh;
float line_x1 = x1;
float line_y1 = y1;
float line_x2 = x2;
float line_y2 = y2;


	if( x1 > x2 )
		{
		int tmp = x1;
		x1 = x2;
		x2 = tmp;	
		}

	if( y1 > y2 )
		{
		int tmp = y1;
		y1 = y2;
		y2 = tmp;	
		}

	bool point_inside = 0;
	bool inx = 0;
	bool iny = 0;

	if( ( x1 >= rx1 ) && ( x1 <= rx2 ) ) inx = 1;
	if( ( x2 >= rx1 ) && ( x2 <= rx2 ) ) inx = 1;
	if( ( y1 >= ry1 ) && ( y1 <= ry2 ) ) iny = 1;
	if( ( y2 >= ry1 ) && ( y2 <= ry2 ) ) iny = 1;

	if( inx && iny ) point_inside = 1;

	if( !point_inside )
		{
		bool inx = 0;
		bool iny = 0;

		if( ( rx1 >= x1 ) && ( rx1 <= x2 ) ) inx = 1;
		if( ( rx2 >= x1 ) && ( rx2 <= x2 ) ) inx = 1;
		if( ( ry1 >= y1 ) && ( ry1 <= y2 ) ) iny = 1;
		if( ( ry2 >= y1 ) && ( ry2 <= y2 ) ) iny = 1;
		if( inx && iny ) point_inside = 1;
		}






if( ( j == 111 ) || ( j == 112 ) || ( j == 113 ) )
	{
//	printf( "set_line_update_flag_if_in_rect() - line %d: x1 %d %d %d %d   rx1 %d %d %d %d inside %d\n", j, x1, y1, x2, y2, rx1, ry1, rx2, ry2, point_inside  );
	
	}


	if( ( x1 >= rx1 ) && ( x1 <= rx2 ) ) inx = 1;

	if( ( y1 >= ry1 ) && ( y1 <= ry2 ) ) iny = 1;	
		{
		if( ( y1 >= ry1 ) && ( y1 <= ry2 ) ) 
			{
			point_inside = 1;
			}
		}
	
	if( !point_inside )
		{
		if( ( x2 >= rx1 ) && ( x2 <= rx2 ) )
			{
			if( ( y2 >= ry1 ) && ( y2 <= ry2 ) ) 
				{
				point_inside = 1;
				}
			}
		}


		
	if( point_inside )
		{
		idx_obj_rect_overlap = i;
		idx_line_rect_overlap = j;
//		printf("set_line_update_flag_if_in_rect() - line idx %d\n", j );
		if( state ) vobj[ i ].vline[j].flags |= en_ft_update;
		else vobj[ i ].vline[j].flags &= ~en_ft_update;
		}

	}


//sub objs
int sub_cnt = vobj[ i ].vsub.size();
if( sub_cnt != 0 )
	{
	for( int k = 0; k < sub_cnt; k++ )
		{
		int line_cnt = vobj[ i ].vsub[k].vline.size();
		for( int j = 0; j < line_cnt; j++ )
			{
			vo = vobj[ i ].vsub[k].vline;
			for( int j = 0; j < vo.size(); j++ )
				{
				int x1 = vo[ j ].drawn_x1;
				int y1 = vo[ j ].drawn_y1;
				int x2 = vo[ j ].drawn_x2;
				int y2 = vo[ j ].drawn_y2;

				if( x1 > x2 )
					{
					int tmp = x1;
					x1 = x2;
					x2 = tmp;	
					}

				if( y1 > y2 )
					{
					int tmp = y1;
					y1 = y2;
					y2 = tmp;	
					}

				bool point_inside = 0;
				bool inx = 0;
				bool iny = 0;

				if( ( x1 >= rx1 ) && ( x1 <= rx2 ) ) inx = 1;
				if( ( x2 >= rx1 ) && ( x2 <= rx2 ) ) inx = 1;
				if( ( y1 >= ry1 ) && ( y1 <= ry2 ) ) iny = 1;
				if( ( y2 >= ry1 ) && ( y2 <= ry2 ) ) iny = 1;

				if( inx && iny ) point_inside = 1;

				if( point_inside ) 
					{
					if( state ) vobj[ i ].vsub[k].vline[j].flags |= en_ft_update;
					else  vobj[ i ].vsub[k].vline[k].flags &= ~en_ft_update;
					}

/*
				if( ( x1 >= xx ) && ( x2 <= rx2 ) ) 
					{
					if( ( y1 >= yy ) && ( y2 <= ry2 ) ) 
						{
						if( state ) vobj[ i ].vsub[k].vline[j].flags |= en_ft_update;
						else  vobj[ i ].vsub[k].vline[k].flags &= ~en_ft_update;
						}
					}
*/
				}
			}
        }

    }
	
}





















void llem_wnd::obj_vertex_offset( int idx, int offx, int offy, float scalex, float scaley )
{
for( int j = 0; j < vobj[ idx ].vline.size(); j++ )
	{
	vobj[ idx ].vline[ j ].x1 *= scalex;
	vobj[ idx ].vline[ j ].y1 *= scaley;
	vobj[ idx ].vline[ j ].x2 *= scalex;
	vobj[ idx ].vline[ j ].y2 *= scaley;

	vobj[ idx ].vline[ j ].x1 += offx;
	vobj[ idx ].vline[ j ].y1 += offy;
	vobj[ idx ].vline[ j ].x2 += offx;
	vobj[ idx ].vline[ j ].y2 += offy;
	}


for( int i = 0; i < vobj[ idx ].vsub.size(); i++ )
	{
	for( int j = 0; j < vobj[ idx ].vsub[i].vline.size(); j++ )
		{
		vobj[ idx ].vsub[i].vline[ j ].x1 *= scalex;
		vobj[ idx ].vsub[i].vline[ j ].y1 *= scaley;
		vobj[ idx ].vsub[i].vline[ j ].x2 *= scalex;
		vobj[ idx ].vsub[i].vline[ j ].y2 *= scaley;

		vobj[ idx ].vsub[i].vline[ j ].x1 += offx;
		vobj[ idx ].vsub[i].vline[ j ].y1 += offy;
		vobj[ idx ].vsub[i].vline[ j ].x2 += offx;
		vobj[ idx ].vsub[i].vline[ j ].y2 += offy;
		}
	}
	
set_bounding_rect( idx, bounding_scale_down, bounding_scale_down, 1 );
}









void llem_wnd::draw_llem( int px, int py, int line_width )
{


int iF = fl_font();
int iS = fl_size();



fl_color( 0, 0, 0 );
//fl_arc( midx - rad, midy - rad, rad*2, rad*2, 0, 360 );


fl_line_style ( FL_SOLID, line_width );        	 			//for windows you must set line setting after the colour, see the manual

/*
float rad = 75;
float rad2 =300;

float deg = 0;
for( int i = 0; i < 64; i++ )
	{
	float fx1 = cos( cn_deg2rad * deg + spoke_angle*cn_deg2rad );
	float fy1 = sin( cn_deg2rad * deg + spoke_angle*cn_deg2rad );
	
	float fx0 = fx1 * rad2;
	float fy0 = fy1 * rad2;

	fx1 *= rad;
	fy1*= rad;
	
	fl_line(  px + nearbyint(fx0), py + nearbyint(fy0), px + nearbyint(fx1), py + nearbyint(fy1) ); 
	deg += 5.625;
	}

fl_line_style ( FL_SOLID, 3 );        	 					//for windows you must set line setting after the colour, see the manual

rad = line_width;
fl_arc( px - rad, py - rad, rad*2, rad*2, 0, 360 );

fl_line_style ( FL_SOLID, 0 );        	 					//for windows you must set line setting after the colour, see the manual

*/


fl_font( 4, 40 );

string s1;
strpf( s1, "line_width: %d", line_width );

fl_draw( s1.c_str(), px, h() - 100, 1, 1, FL_ALIGN_CENTER );

fl_font( iF, iS );

}






void llem_wnd::draw_obj_create_params(  int px, int py, int col )
{
string s1, st, sf;

int iF = fl_font();
int iS = fl_size();

fl_font( 4, 9 );



fl_color( 255, 255, 255 );
fl_rectf( px, py, 200, 30 );
fl_color( 0, 0, 0 );
strpf( s1, "----- EDITING (press 'e') [line idx %d] -----\n", obj_create_line_idx );
st += s1;
strpf( s1, "---- 'o' toggles between ship/ground objs [obj idx %d] -----\n", obj_create_idx );
st += s1;
strpf( s1, "---- 'x' toggles crosshairs  x1 %f y1 %f, x2 %f y2 %f -----\n",vobj[obj_create_idx].vline[ obj_create_line_idx ].x1, vobj[obj_create_idx].vline[ obj_create_line_idx ].y1 ,vobj[obj_create_idx].vline[ obj_create_line_idx ].x2, vobj[obj_create_idx].vline[ obj_create_line_idx ].y2 );
st += s1;
strpf( s1, "---- 'l' to make horiz: dx %f dy %f ----- \n", obj_create_line_dx, obj_create_line_dy );
st += s1;
strpf( s1, "---- 'f' fuel landzone inc ('ctrl-f' dec) -----\n" );
st += s1;
strpf( s1, "---- '[ ]' changes zoom -----\n" ); 
st += s1;
strpf( s1, "---- '/' sorts lines by x -----\n" );
st += s1;

en_line_type lt = vobj[obj_create_idx].vline[ obj_create_line_idx ].line_type; 
strpf( s1, "---- 'ctrl-c' to cycle 'line_type' -----\n" );
st += s1;
strpf( s1, "---- line_type: '%s'\n", st_line_type_names[lt].name );
st += s1;
	
	

fl_draw( st.c_str(), px, py, 1, 1, FL_ALIGN_LEFT | FL_ALIGN_TOP );

fl_font( iF, iS );

}





void llem_wnd::draw_status(  int px, int py, int col )
{
string s1, st, sf;

int iF = fl_font();
int iS = fl_size();

fl_font( 4, 9 );

fl_color( 255, 255, 255 );
fl_rectf( px, py, 200, 40 );
fl_color( 0, 0, 0 );

strpf( s1, "dx %.2f dy %.2f\n", vobj[0].dx, vobj[0].dy );
st += s1;
strpf( s1, "dv %.2f fuel %d\n", vobj[0].dv, (int)fuel );
st += s1;
if( toggle_space ) 
	{
	strpf( s1, "move on hold, press 'space'...\n" );
	st += s1;
	}
fl_draw( st.c_str(), px, py, 1, 1, FL_ALIGN_LEFT | FL_ALIGN_TOP );

fl_font( iF, iS );

}







void llem_wnd::draw_help( int px, int py, int col )
{
string s1, st, sf;

int iF = fl_font();
int iS = fl_size();

set_col( col );
fl_font( 4, 10 );

fl_color( 255, 255, 255 );
fl_rectf( px - 5, py - 70, 500 , h() - py + 70 );

set_col( col );
fl_font( 4, 10 );


strpf( s1, "'w'+'wheel' line_width: %d,\t'e' to edit obj\n", adj_line_width );
st += s1;

strpf( s1, "'x'+'wheel' posx: %d,\t'z' to reset position\n", adj_offsx );
st += s1;

strpf( s1, "'y'+'wheel' posy: %d,   'm' to move position by mouse\n", adj_offsy );
st += s1;

strpf( s1, "'r'+'wheel' angle: %f\n", adj_angle );
st += s1;

strpf( s1, "'s'+'wheel' scale: %f\n", adj_scale );
st += s1;

sf = "??";
if( adj_filter == fwt_rect ) sf = "fwt_rect";
//if( spoke_filter == fwt_kaiser ) sf = "fwt_kaiser";
if( adj_filter == fwt_bartlett ) sf = "fwt_bartlett";
if( adj_filter == fwt_hann ) sf = "fwt_hann";
if( adj_filter == fwt_bartlett_hanning ) sf = "fwt_bartlett_hanning";
if( adj_filter == fwt_hamming ) sf = "fwt_hamming";
if( adj_filter == fwt_blackman ) sf = "fwt_blackman";
if( adj_filter == fwt_blackman_harris ) sf = "fwt_blackman_harris";
if( adj_filter == fwt_lanczos1 ) sf = "fwt_lanczos1";
if( adj_filter == fwt_lanczos1_5 ) sf = "fwt_lanczos1_5";
if( adj_filter == fwt_lanczos2 ) sf = "fwt_lanczos2";
if( adj_filter == fwt_lanczos3 ) sf = "fwt_lanczos3";


strpf( s1, "'f'+'wheel' filter type: '%s'\n", sf.c_str() ) ;
st += s1;

strpf( s1, "'k'+'wheel' filter kernel size: %d\n", kern_size );
st += s1;

strpf( s1, "'t'+'wheel' toggle filtered: %d (allows capture for manual 'mt paint' processing)\n", adj_show_filtered );
st += s1;

strpf( s1, "\n" );
st += s1;

strpf( s1, "mousex %d %d  shipx %.2f %.2f, panx %f %f,   fps %f   (%f mS)\n", mousex, mousey, vobj[0].x, vobj[0].y, panx, pany, fps, 1/fps );
st += s1;

fl_draw( st.c_str(), px, py, 1, 1, FL_ALIGN_LEFT );

fl_font( iF, iS );


}




/*

void llem_wnd::draw_line( int ix0, int iy0, int ix1, int iy1, unsigned char* bf, int sizex, int sizey )
{
ix0 = w()/2;
iy0 = h()/2;

ix1 = w();//ix0 + 400;
iy1 = iy0 + 40;

int dx = ix1 - ix0;
int dy = iy1 - iy0;
float m = (float)dy/dx;
float b = -(m*ix0 - iy0);

for( int i = 0; i < dx; i++ )
	{
	float fy = m*(ix0 + i) + b;

	int ptr = (nearbyint(fy) * sizex*cn_bytes_per_pixel)  +  (ix0 + i)*cn_bytes_per_pixel;
	
	bf[ ptr + 0 ] = 0x0;
	bf[ ptr + 1 ] = 0x0;
	bf[ ptr + 2 ] = 0x0;
	
//	fl_point( ix0 + i, nearbyint(fy) );
	}

}
*/



void llem_wnd::setcolref( colref col )
{
fl_color( col.r , col.g , col.b );
}







//0xaabbggrr
void llem_wnd::set_col( unsigned int col )
{
colref cr; 

cr.r = col & 0xff; 
cr.g = ( col >> 8 ) & 0xff; 
cr.b = ( col >> 16 ) & 0xff; 

setcolref( cr );
}







//translate rotate translate
void llem_wnd::rotate( float xx, float yy, float &x1, float &y1, float &x2, float &y2, float theta )
{

//xx = ( x1 - x2 );
//yy = ( y1 - y2 );


float tx1 = x1 - xx;
float ty1 = y1 - yy;

float tx2 = x2 - xx;
float ty2 = y2 - yy;


float rx1 = tx1 * cosf( theta ) - ty1 * sinf( theta );
float ry1 = tx1 * sinf( theta ) + ty1 * cosf( theta );

float rx2 = tx2 * cosf( theta ) - ty2 * sinf( theta );
float ry2 = tx2 * sinf( theta ) + ty2 * cosf( theta );

x1 = rx1 + xx;
y1 = ry1 + yy;

x2 = rx2 + xx;
y2 = ry2 + yy;

}








void llem_wnd::set_pixel( int xx, int yy )
{
//fl_point( xx, yy );

int ptr;

// ptr = yy*bf0szx + xx;


//pixbf_r0[ ptr ] = 0x00;
//pixbf_g0[ ptr ] = 0x00;
//pixbf_b0[ ptr ] = 0x00;


ptr = yy * bf0szx * cn_bytes_per_pixel   +   xx * cn_bytes_per_pixel;
pixbf0[ ptr + 0 ] = plot_col_r;
pixbf0[ ptr + 1 ] = plot_col_g;
pixbf0[ ptr + 2 ] = plot_col_b;


if( 0 )
	{
	st_pixel_tag o;			
	o.x = xx;
	o.y = yy;		
	vpxl.push_back( o );
	}


//printf( "set_pixel() - xx %d  yy %d \n", xx, yy );		

}







//this plots at negative locations to 'xx,yy', so beware of buffer location corruptions/crashes
//make sure line clipper is adj to avoid buffer corruptions/crashes
void llem_wnd::set_pixel3( int xx, int yy )
{

set_pixel( xx, yy );
set_pixel( xx - 1, yy );
set_pixel( xx + 1, yy );

set_pixel( xx, yy - 1 );
set_pixel( xx - 1, yy - 1 );
set_pixel( xx + 1, yy - 1 );

set_pixel( xx, yy + 1 );
set_pixel( xx - 1, yy + 1 );
set_pixel( xx + 1, yy + 1 );

if( 0 )
	{
	st_pixel_tag o;			
	o.x = xx;
	o.y = yy;		
	vpxl_cntr.push_back( o );
	}

}





void llem_wnd::set_pixel2( int xx, int yy )
{
set_pixel( xx, yy );

set_pixel( xx + 1, yy );

set_pixel( xx, yy + 1 );
set_pixel( xx + 1, yy + 1 );

}




/*
void llem_wnd::line_zero_based( double x1, double y1, double x2, double y2 )
{

double xx1 = x1;
double yy1 = y1;

double xx2 = x2;
double yy2 = y2;


//fl_line( nearbyint( xx1 ), nearbyint( yy1 ), nearbyint( xx2 ), nearbyint( yy2 ) );



st_line2_tag o;

o.x1 = xx1;
o.y1 = yy1;
o.x2 = xx2;
o.y2 = yy2;

vlin.push_back( o );



if( 1 )
	{
	float dx = xx2 - xx1;
	float dy = yy2 - yy1;
	
	
	
	if( fabsf( dx ) > fabsf( dy ) )
		{
		if( xx1 > xx2 )
			{
			float swap = xx1;
			xx1 = xx2;
			xx2 = swap;
			
			swap = yy1;	
			yy1 = yy2;
			yy2 = swap;
			
			dx = -dx;
			dy = -dy;
			}

if( dbg_0 )
	{
//	printf( "lineidx: %d,  dx big   x1 %f y1 %f x2 %f y2 %f   dx %f  dy %f\n", obj_create_line_idx, xx1, yy1, xx2, yy2, dx, dy );		
	}

	
		float m = ( yy2 - yy1 ) / ( xx2 - xx1 );

		float b = -(m*xx1 - yy1);
			
		for( int i = xx1; i < xx2; i++ )
			{
			int xx = i;
			int yy = m*i + b;
			
			
			set_pixel3( xx, yy );
			
			}	


		}



	if( fabsf( dy ) > fabsf( dx ) )
		{
		if( yy1 > yy2 )
			{
			float swap = xx1;
			xx1 = xx2;
			xx2 = swap;
			
			swap = yy1;	
			yy1 = yy2;
			yy2 = swap;
			
			dx = -dx;
			dy = -dy;
			}

if( dbg_0 )
	{
//	printf( "lineidx: %d,  dy big   x1 %f y1 %f x2 %f y2 %f   dx %f  dy %f\n", obj_create_line_idx, xx1, yy1, xx2, yy2, dx, dy );		
	}
	
		float m = ( xx2 - xx1 ) / ( yy2 - yy1 );

		float b = -(m*yy1 - xx1);
			
		for( int i = yy1; i < yy2; i++ )
			{
			int yy = i;
			int xx = m*i + b;
			set_pixel3( xx, yy );
			}	

		}
	}

}

*/





//this writes to locations before and after rect bounds, dependant on filter kernel size
//make sure line clipper is adj to avoid buffer corruptions/crashes
void llem_wnd::set_rect( int x1, int y1, int x2, int y2, int col )
{
int dx;
int dy;
int xx1, yy1;
int xx2, yy2;



if( x1 <= x2 ) { dx = x2 - x1; xx1 = x1; xx2 = x2; }
else { dx = x1 - x2; xx1 = x2; xx2 = x1; }

if( y1 <= y2 ) { dy = y2 - y1; yy1 = y1; yy2 = y2; }
else { dy = y1 - y2;  yy1 = y2; yy2 = y1; }

int ks = kern_size/2;

xx1 -= ks;
yy1 -= ks;

xx2 += ks;
yy2 += ks;


if( xx1 < ks ) xx1 = ks;
if( yy1 < ks ) yy1 = ks;

if( xx2 < ks ) xx2 = ks;
if( yy2 < ks ) yy2 = ks;

//printf( "set_rect() xx1: %d %d %d %d\n", xx1, yy1, xx2, yy2 );

int r = col & 0xff;
int g = ( col >> 8 ) & 0xff;
int b = ( col >> 16 ) & 0xff;
for( int y = yy1; y <= yy2; y++ )
	{
	for( int x = xx1; x <= xx2; x++ )
		{
		int ptr = y*bf0szx*cn_bytes_per_pixel + x*cn_bytes_per_pixel;
		
		bool change = 1;
//		if( pixbf0[ ptr + 0 ] != 255 ) change = 1;
//		if( pixbf0[ ptr + 1 ] != 255 ) change = 1;
//		if( pixbf0[ ptr + 2 ] != 255 ) change = 1;
		
		if( change )
			{
			pixbf0[ ptr + 0 ] = r;
			pixbf0[ ptr + 1 ] = g;
			pixbf0[ ptr + 2 ] = b;
			}
		}
	}

}




//this writes to locations before and after rect bounds, dependant on filter kernel size
//make sure line clipper is adj to avoid buffer corruptions/crashes
void llem_wnd::set_rect( int x1, int y1, int x2, int y2, int r, int g, int b )
{
int dx;
int dy;
int xx1, yy1;
int xx2, yy2;

if( x1 <= x2 ) { dx = x2 - x1; xx1 = x1; xx2 = x2; }
else { dx = x1 - x2; xx1 = x2; xx2 = x1; }

if( y1 <= y2 ) { dy = y2 - y1; yy1 = y1; yy2 = y2; }
else { dy = y1 - y2;  yy1 = y2; yy2 = y1; }

int ks = kern_size/2;

xx1 -= ks;
yy1 -= ks;

xx2 += ks;
yy2 += ks;


if( xx1 < ks ) xx1 = ks;
if( yy1 < ks ) yy1 = ks;

if( xx2 < ks ) xx2 = ks;
if( yy2 < ks ) yy2 = ks;


for( int y = yy1; y <= yy2; y++ )
	{
	for( int x = xx1; x <= xx2; x++ )
		{
		int ptr = y*bf0szx*cn_bytes_per_pixel + x*cn_bytes_per_pixel;
		
		bool change = 1;
//		if( pixbf0[ ptr + 0 ] != 255 ) change = 1;
//		if( pixbf0[ ptr + 1 ] != 255 ) change = 1;
//		if( pixbf0[ ptr + 2 ] != 255 ) change = 1;
		
		if( change )
			{
			pixbf0[ ptr + 0 ] = r;
			pixbf0[ ptr + 1 ] = g;
			pixbf0[ ptr + 2 ] = b;
			}
		}
	}

}








void llem_wnd::line( int x1, int y1, int x2, int y2, int r, int g, int b, bool bkgd_only, bool dont_set_pixels, int dbg_line_idx )
{
float xx1 = x1;
float yy1 = y1;

float xx2 = x2;
float yy2 = y2;


//bool outsidex = 0;
//if( ( xx1 < view_x0 ) && ( xx2 < view_x1 ) ) outsidex = 1;

//if( ( xx1 > view_x1 ) && ( xx2 > view_x1 ) ) outsidex = 1;

//if( outsidex ) return;

//fl_line( nearbyint( xx1 ), nearbyint( yy1 ), nearbyint( xx2 ), nearbyint( yy2 ) );

//if( ( y1 < 0 ) || ( y2 < 0) )
	{
//printf("line() - y1 %d  y2 %d\n", y1, y2 );	
//	getchar();
	
	}

st_line2_tag o;

o.x1 = xx1;
o.y1 = yy1;
o.x2 = xx2;
o.y2 = yy2;

o.r = r;
o.g = g;
o.b = b;

vlin.push_back( o );


if( bkgd_only ) 
	{
//	set_rect( xx1, yy1, xx2, yy2, 0xffffff );
	set_rect( xx1, yy1, xx2, yy2, backgrnd_r, backgrnd_g, backgrnd_b );
	return;
	}


//if( dbg_line_idx != 2 ) return;

//if( bkgd_only ) set_plot_col( backgrnd_r, backgrnd_g, backgrnd_b );
//else set_plot_col( line_r, line_g, line_b );

set_plot_col( o.r, o.g, o.b );

if( 1 )
	{
	float dx = xx2 - xx1;
	float dy = yy2 - yy1;

	if( fabsf( dx ) >= fabsf( dy ) )
		{
		if( xx1 > xx2 )
			{
			float swap = xx1;
			xx1 = xx2;
			xx2 = swap;
			
			swap = yy1;	
			yy1 = yy2;
			yy2 = swap;
			
			dx = -dx;
			dy = -dy;
			}

		float m = ( yy2 - yy1 ) / ( xx2 - xx1 );

		float b = -(m*xx1 - yy1);

//printf("%03d dx %f %f b %f m %f\n", dbg_line_idx, dx, dy, b, m );	
		
		int sub = 0;
		for( int i = xx1; i < xx2; i+=1 )
			{
			int xx = i;
			int yy = m*i + b;
			
			if( !dont_set_pixels )
				{
				if( adj_line_width == 3 ) set_pixel3( xx, yy );
				else if( adj_line_width == 2 ) set_pixel2( xx, yy );
				else set_pixel( xx, yy );
				}
/*
			if( 0 )
				{

				sub++;
				if( sub >= 3 )
					{
					st_pixel_tag o;			
					o.x = xx;
					o.y = yy;		
					vpxl_cntr.push_back( o );
					sub = 0;
					}
				}
*/
			}	
		}
	else{
		//dy > dx
		if( yy1 > yy2 )
			{
			float swap = xx1;
			xx1 = xx2;
			xx2 = swap;
			
			swap = yy1;	
			yy1 = yy2;
			yy2 = swap;
			
			dx = -dx;
			dy = -dy;
			}

if( dbg_0 )
	{
//	printf( "lineidx: %d,  dy big   x1 %f y1 %f x2 %f y2 %f   dx %f  dy %f\n", obj_create_line_idx, xx1, yy1, xx2, yy2, dx, dy );		
	}
	
		float m = ( xx2 - xx1 ) / ( yy2 - yy1 );

		float b = -(m*yy1 - xx1);
			
//printf("%03d dx %f %f b %f m %f\n", dbg_line_idx, dx, dy, b, m );	
		int sub = 0;
		for( int i = yy1; i < yy2; i+=1 )
			{
			int yy = i;
			int xx = m*i + b;
			
			if( !dont_set_pixels )
				{
				if( adj_line_width == 3 ) set_pixel3( xx, yy );
				else if( adj_line_width == 2 ) set_pixel2( xx, yy );
				else set_pixel( xx, yy );
				}
/*
			if( 0 )
				{

				sub++;
				if( sub >= 3 )
					{
					st_pixel_tag o;			
					o.x = xx;
					o.y = yy;		
					vpxl_cntr.push_back( o );
					sub = 0;
					}
				}
*/
			}	
		}


//	if( 1 )
//		{
//		printf( "lineidx: %d,  dx big   x1 %f y1 %f x2 %f y2 %f   dx %f  dy %f\n", obj_create_line_idx, xx1, yy1, xx2, yy2, dx, dy );		
//		}
	}



}






/*
void llem_wnd::draw_landing_zone_text( unsigned int idx )
{
string s1;




return;




if( idx >= vobj.size() ) return; 

vector<st_line_tag> vo;
vo = vobj[ idx ].vline;

for( int j = 0; j < vo.size(); j++ )
	{
	if( ( vo[j].line_type != en_lt_ground_landing_zone_with_fuel ) || ( vo[j].line_type == en_lt_ground_landing_zone_no_fuel ) ) continue;
	
	int x0 = vo[j].drawn_x1;
	int y1 = vo[j].drawn_y1;
	
	int fuel = 100;
	strpf( s1, "%d", fuel );
	
	fl_draw( s1.c_str(), 800, 400, 1, 1, FL_ALIGN_CENTER ); 
	}
}
*/













int skip_draw_obj_type = -1;//en_ot_ground;



void llem_wnd::draw_objs( int only_this_idx, bool bkgd_only )
{

int obj_cnt = vobj.size();

int i;
if( only_this_idx >= 0 ) 
	{
	i = only_this_idx;
	obj_cnt = i + 1;
	}

//if( only_this_idx == 0 ) return;

//printf("draw_objs() - only_this_idx %d\n", only_this_idx );

int minx;
int miny;
int maxx;
int maxy;
int abs_x1;
int abs_y1;
int abs_x2;
int abs_y2;

for( i; i < obj_cnt; i++ )
    {
	if( vobj[ i ].type == skip_draw_obj_type ) continue;
	if( !vobj[ i ].visible ) continue;



//printf( "draw_objs() i %d\n", i );
	bool create_mode_flash2_local = 0;
	
	if( i == idx_crosshair ) 
		{
		create_mode_flash2_local = flash1;
		}


	minx = 10000;									//set these to some extreme
	miny = 10000;
	maxx = -10000;
	maxy = -10000;

    float xx = nearbyint( vobj[ i ].x*zoomx - panx*zoomx );
    float yy = nearbyint( vobj[ i ].y*zoomx - pany*zoomy );
    
//    float ofx = vobj[ i ].offsx;
//    float ofy = vobj[ i ].offsy;
   
    float theta_detentzone = vobj[ i ].theta_detentzone;

	if( vobj[ i ].col_temp_frame_cnt > 0 ) set_col( vobj[ i ].col_temp );	//show temp col if req
    else set_col( vobj[ i ].col );

 	bool create_mode_hide = 0;
 	
	vector<st_line_tag> vo;
    vo = vobj[ i ].vline;
    float scl = vobj[ i ].scale;
    for( int j = 0; j < vo.size(); j++ )
        {
		bool dont_set_pixels = 0;

		//don't draw lines used for collision detection
		if( ( vo[ j ].line_type == en_lt_ship_collision_det_pads_left ) || ( vo[ j ].line_type == en_lt_ship_collision_det_pads_right ) || ( vo[ j ].line_type == en_lt_ship_collision_det_sides ) ) dont_set_pixels = 1;
		if( obj_create_active ) dont_set_pixels = 0;

		if( bkgd_only == 0 )
			{
			if( ( vo[ j ].flags & en_ft_update ) == 0 ) continue;

			float x1 = vo[ j ].x1 * scl * zoomx;
			float y1 = vo[ j ].y1 * scl * zoomy;
			float x2 = vo[ j ].x2 * scl * zoomx;
			float y2 = vo[ j ].y2 * scl * zoomy;

//			if( ( theta <= 0.005f ) || (  theta >= 0.005f ) ) rotate( 0, 0, x1, y1, x2, y2, theta );


			rotate( 0, 0, x1, y1, x2, y2, theta_detentzone );

			dbg_0 = 0;
			if( obj_create_active )
				{
				create_mode_hide = 0;
				if( i == obj_create_idx ) 
					{
					if( j == obj_create_line_idx )
						{
						dbg_0 = 1;
//						if( !(draw_cnt%5) ) create_mode_hide = 1;
						
						if( flash0 )  create_mode_hide = 1;
						}
					}
				}

	//		if( j == 10 ) continue;
			x1 += xx;
			y1 += yy;
			x2 += xx;
			y2 += yy;

			
			abs_x1 = midx + x1;
			abs_y1 = midy - y1;
			abs_x2 = midx + x2;
			abs_y2 = midy - y2;

	
			if( abs_x1 < minx ) minx = abs_x1;								//make a 'drawn' bounding rect of whole obj
			if( abs_y1 < miny ) miny = abs_y1;
			
			if( abs_x1 > maxx ) maxx = abs_x1;
			if( abs_y1 > maxy ) maxy = abs_y1;
			
			if( abs_x2 < minx ) minx = abs_x2;
			if( abs_y2 < miny ) miny = abs_y2;
			
			if( abs_x2 > maxx ) maxx = abs_x2;
			if( abs_y2 > maxy ) maxy = abs_y2;

			int rr = vo[ j ].r;
			int gg = vo[ j ].g;
			int bb = vo[ j ].b;

			if( ( vobj[ i ].type == en_ot_user_ship ) && ( vobj[ i ].is_exploding ) )
				{
//				rr = 250;				//don't use 255 for red as this will confuse 'check_empty()'
//				gg = 0;
//				bb = 255;
				}
			else{
				}


			if( ( !create_mode_hide ) && ( !create_mode_flash2_local ) )
				{
//				abs_x1 = 20;
//				abs_y1 = 1000;
//				abs_x2 = 6500;
//				abs_y2 = 3500;

				if( lineclp.line_clip_int( abs_x1, abs_y1, abs_x2, abs_y2 ) )
					{
					lines_drawn++;
					vobj[ i ].vline[j].drawn = 1;
					
//printf("00 abs_x1 %d %d %d %d\n",abs_x1,abs_y1,abs_x2,abs_y2);
					if( !dont_set_pixels ) line( abs_x1, abs_y1, abs_x2, abs_y2, rr, gg, bb, bkgd_only, 0, j );

					vobj[ i ].vline[j].drawn_x1 = abs_x1;							//keep 'drawn' line coords
					vobj[ i ].vline[j].drawn_y1 = abs_y1;
					vobj[ i ].vline[j].drawn_x2 = abs_x2;
					vobj[ i ].vline[j].drawn_y2 = abs_y2;
					}
				else{
					vobj[ i ].vline[j].drawn = 0;
					}

				}
			}
		else{
			//clear backgound
			if( ( !create_mode_hide ) && ( !create_mode_flash2_local ) )
				{
				
				if( vobj[ i ].vline[j].drawn )
					{
					vobj[ i ].vline[j].drawn = 0;
					lines_erased++;
//printf("01 drawn_x1 %d %d %d %d\n",vobj[ i ].vline[j].drawn_x1,vobj[ i ].vline[j].drawn_y1,vobj[ i ].vline[j].drawn_x2,vobj[ i ].vline[j].drawn_y2);
					line( vobj[ i ].vline[j].drawn_x1, vobj[ i ].vline[j].drawn_y1, vobj[ i ].vline[j].drawn_x2, vobj[ i ].vline[j].drawn_y2, vo[ j ].r, vo[ j ].g, vo[ j ].b, bkgd_only, 0, j );
					}
				}
			}
//draw_rect_src( abs_x1 - 0, abs_y1 - 0, abs_x2 - abs_x1 + 0, abs_y2 - abs_y1 + 0 );	
        }


	//draw sub objs
	int sub_cnt = vobj[ i ].vsub.size();
	if( sub_cnt != 0 )
		{
		int is = vobj[ i ].sub_obj_idx;
		if( ( is < sub_cnt ) && ( is >= 0 ) )
			{
			vo = vobj[ i ].vsub[is].vline;
			for( int j = 0; j < vo.size(); j++ )
				{

				if( bkgd_only == 0 )
					{
					float x1 = vo[ j ].x1 * scl * zoomx;
					float y1 = vo[ j ].y1 * scl * zoomy;
					float x2 = vo[ j ].x2 * scl * zoomx;
					float y2 = vo[ j ].y2 * scl * zoomy;

//					if( ( theta <= 0.005f ) || (  theta >= 0.005f ) ) rotate( 0, 0, x1, y1, x2, y2, theta );
					
					rotate( 0, 0, x1, y1, x2, y2, theta_detentzone );

					bool create_mode_hide = 0;
					dbg_0 = 0;
					if( obj_create_active )
						{
						if( i == obj_create_idx ) 
							{
							if( j == obj_create_line_idx )
								{
								dbg_0 = 1;
//								if( !(draw_cnt%20) ) create_mode_hide = 1;
								if( flash0 )  create_mode_hide = 1;
								}
							}
						}

			//		if( j == 10 ) continue;
			
					x1 += xx;
					y1 += yy;
					x2 += xx;
					y2 += yy;

					abs_x1 = midx + x1;
					abs_y1 = midy - y1;
					abs_x2 = midx + x2;
					abs_y2 = midy - y2;

					if( abs_x1 < minx ) minx = abs_x1;						//make a 'drawn' bounding rect of whole obj
					if( abs_y1 < miny ) miny = abs_y1;
					
					if( abs_x1 > maxx ) maxx = abs_x1;
					if( abs_y1 > maxy ) maxy = abs_y1;
					
					if( abs_x2 < minx ) minx = abs_x2;
					if( abs_y2 < miny ) miny = abs_y2;
					
					if( abs_x2 > maxx ) maxx = abs_x2;
					if( abs_y2 > maxy ) maxy = abs_y2;

					if( ( !create_mode_hide ) && ( !create_mode_flash2_local ) ) 
						{
						if( lineclp.line_clip_int( abs_x1, abs_y1, abs_x2, abs_y2 ) )
							{
							lines_drawn++;
							vobj[ i ].vsub[is].vline[j].drawn = 1;

//printf("02 abs_x1 %d %d %d %d\n",abs_x1,abs_y1,abs_x2,abs_y2);

							int rr = vo[ j ].r;
							int gg = vo[ j ].g;
							int bb = vo[ j ].b;
							
							if( ( fuel < 200 ) && ( fabs(rnd()) > 0.5 ) && ( thrust_wheel > 0.0 ) )				
								{
								rr = 246;								//orange flame flicker
								gg = 104;
								bb = 14;
								}
							line( abs_x1, abs_y1, abs_x2, abs_y2, rr, gg, bb, bkgd_only, 0, 1000+j );

							vobj[ i ].vsub[is].vline[j].drawn_x1 = abs_x1;			//keep 'drawn' line coords
							vobj[ i ].vsub[is].vline[j].drawn_y1 = abs_y1;
							vobj[ i ].vsub[is].vline[j].drawn_x2 = abs_x2;
							vobj[ i ].vsub[is].vline[j].drawn_y2 = abs_y2;
							}
						else{
							vobj[ i ].vsub[is].vline[j].drawn = 0;
							}
						}
					}
				else{
					if( ( !create_mode_hide ) && ( !create_mode_flash2_local ) )
						{
						
						if( vobj[ i ].vsub[is].vline[j].drawn )
							{
							lines_erased++;
							vobj[ i ].vsub[is].vline[j].drawn = 0;
//printf("03 drawn_x1 %d %d %d %d\n",vobj[ i ].vsub[is].vline[j].drawn_x1,vobj[ i ].vsub[is].vline[j].drawn_y1,vobj[ i ].vsub[is].vline[j].drawn_x2,vobj[ i ].vsub[is].vline[j].drawn_y2);
							line( vo[j].drawn_x1, vo[j].drawn_y1, vo[j].drawn_x2, vo[j].drawn_y2, vo[j].r, vo[j].g, vo[j].b, bkgd_only, 0, 1000+j );
							}
						}
					}
				}
			}
        }

	vobj[ i ].drawn_bound_x1 = minx;									//this is a rect around the whole obj
	vobj[ i ].drawn_bound_y1 = miny;
	vobj[ i ].drawn_bound_x2 = maxx;
	vobj[ i ].drawn_bound_y2 = maxy;
	
	if( i == 0 )
		{
//		draw_rect_src( vobj[ i ].drawn_bound_x1, vobj[ i ].drawn_bound_y1, vobj[ i ].drawn_bound_x2 - vobj[ i ].drawn_bound_x1, vobj[ i ].drawn_bound_y2 - vobj[ i ].drawn_bound_y1 );	
			
		}
		
    }
}









float llem_wnd::get_obj_vel( unsigned int idx )
{

int obj_cnt = vobj.size();

if( idx >= obj_cnt ) 
	{
	return 0;
	}

float vel = sqrt( pow( vobj[ idx ].dx, 2 ) + pow( vobj[ idx ].dy, 2 ) );

return vel;
}










void llem_wnd::draw_obj_fl_line( int idx, bool bkgd_only )
{

int obj_cnt = vobj.size();

if( idx >= obj_cnt ) 
	{
	return;
	}


int minx;
int miny;
int maxx;
int maxy;
int abs_x1;
int abs_y1;
int abs_x2;
int abs_y2;

int i = idx;

//for( i = 0; i < obj_cnt; i++ )
    {
//	if( vobj[ i ].type == skip_draw_obj_type ) continue;

	minx = 10000;									//set these to some exteme
	miny = 10000;
	maxx = -10000;
	maxy = -10000;
	
    float xx = nearbyint( vobj[ i ].x );
    float yy = nearbyint( vobj[ i ].y );
    float theta = vobj[ i ].theta;

	if( vobj[ i ].col_temp_frame_cnt > 0 ) set_col( vobj[ i ].col_temp );	//show temp col if req
    else set_col( vobj[ i ].col );

    vector<st_line_tag> vo;
    vo = vobj[ i ].vline;
    float scl = vobj[ i ].scale;
    for( int j = 0; j < vo.size(); j++ )
        {
		if( ( vo[ j ].flags & en_ft_update ) == 0 ) continue;
		
        float x1 = vo[ j ].x1 * scl;
        float y1 = vo[ j ].y1 * scl;
        float x2 = vo[ j ].x2 * scl;
        float y2 = vo[ j ].y2 * scl;

        if( ( theta <= 0.005f ) || (  theta >= 0.005f ) ) rotate( 0, 0, x1, y1, x2, y2, theta );

		bool hide = 0;
		dbg_0 = 0;
		if( obj_create_active )
			{
			if( i == obj_create_idx ) 
				{
				if( j == obj_create_line_idx )
					{
					dbg_0 = 1;
					if( !(draw_cnt%20) ) hide = 1;
					}
				}
			}

//		if( j == 10 ) continue;
		x1 += xx;
		y1 += yy;
		x2 += xx;
		y2 += yy;

		abs_x1 = midx + x1;
		abs_y1 = midy - y1;
		abs_x2 = midx + x2;
		abs_y2 = midy - y2;
		
		if( abs_x1 < minx ) minx = abs_x1;								//make a 'drawn' bounding rect of whole obj
		if( abs_y1 < miny ) miny = abs_y1;
		
		if( abs_x1 > maxx ) maxx = abs_x1;
		if( abs_y1 > maxy ) maxy = abs_y1;
		
		if( abs_x2 < minx ) minx = abs_x2;
		if( abs_y2 < miny ) miny = abs_y2;
		
		if( abs_x2 > maxx ) maxx = abs_x2;
		if( abs_y2 > maxy ) maxy = abs_y2;

		fl_color( vo[ j ].r, vo[ j ].g, vo[ j ].b );
		if( !hide ) fl_line( abs_x1, abs_y1, abs_x2, abs_y2 );
		
		vobj[ i ].vline[j].drawn_x1 = abs_x1;							//keep 'drawn' line coords
		vobj[ i ].vline[j].drawn_y1 = abs_y1;
		vobj[ i ].vline[j].drawn_x2 = abs_x2;
		vobj[ i ].vline[j].drawn_y2 = abs_y2;
		
        }


	//draw sub objs
	int sub_cnt = vobj[ i ].vsub.size();
	if( sub_cnt != 0 )
		{
		int is = vobj[ i ].sub_obj_idx;
		if( ( is < sub_cnt ) && ( is >= 0 ) )
			{
			vo = vobj[ i ].vsub[is].vline;
			for( int j = 0; j < vo.size(); j++ )
				{
				float x1 = vo[ j ].x1 * scl;
				float y1 = vo[ j ].y1 * scl;
				float x2 = vo[ j ].x2 * scl;
				float y2 = vo[ j ].y2 * scl;

				if( ( theta <= 0.005f ) || (  theta >= 0.005f ) ) rotate( 0, 0, x1, y1, x2, y2, theta );

				bool hide = 0;
				dbg_0 = 0;
				if( obj_create_active )
					{
					if( i == obj_create_idx ) 
						{
						if( j == obj_create_line_idx )
							{
							dbg_0 = 1;
							if( !(draw_cnt%20) ) hide = 1;
							}
						}
					}

		//		if( j == 10 ) continue;
		
				x1 += xx;
				y1 += yy;
				x2 += xx;
				y2 += yy;

				 
				abs_x1 = midx + x1;
				abs_y1 = midy - y1;
				abs_x2 = midx + x2;
				abs_y2 = midy - y2;

				if( abs_x1 < minx ) minx = abs_x1;						//make a 'drawn' bounding rect of whole obj
				if( abs_y1 < miny ) miny = abs_y1;
				
				if( abs_x1 > maxx ) maxx = abs_x1;
				if( abs_y1 > maxy ) maxy = abs_y1;
				
				if( abs_x2 < minx ) minx = abs_x2;
				if( abs_y2 < miny ) miny = abs_y2;
				
				if( abs_x2 > maxx ) maxx = abs_x2;
				if( abs_y2 > maxy ) maxy = abs_y2;

				fl_color( vo[ j ].r, vo[ j ].g, vo[ j ].b );
				if( !hide ) fl_line( abs_x1, abs_y1, abs_x2, abs_y2 );

				vobj[ i ].vsub[is].vline[j].drawn_x1 = abs_x1;			//keep 'drawn' line coords
				vobj[ i ].vsub[is].vline[j].drawn_y1 = abs_y1;
				vobj[ i ].vsub[is].vline[j].drawn_x2 = abs_x2;
				vobj[ i ].vsub[is].vline[j].drawn_y2 = abs_y2;
				}
			}
        }

	vobj[ i ].drawn_bound_x1 = minx;									//this is a rect around the whole obj
	vobj[ i ].drawn_bound_y1 = miny;
	vobj[ i ].drawn_bound_x2 = maxx;
	vobj[ i ].drawn_bound_y2 = maxy;
    }
}





















void llem_wnd::set_plot_col( int r, int g, int b )
{
plot_col_r = r;
plot_col_g = g;
plot_col_b = b;
}







void llem_wnd::get_pixel( int xx, int yy, int &r, int &g, int &b )
{
unsigned char *bf = pixbf0;

if( ( yy < 0 ) || ( xx < 0 ) )
	{
	r = backgrnd_r;
	g = backgrnd_g;
	b = backgrnd_b;
	return;
	}

int psrc = (yy * sizex*cn_bytes_per_pixel)  +  xx*cn_bytes_per_pixel;

r = bf[ psrc + 0 ];
g = bf[ psrc + 1 ];
b = bf[ psrc + 2 ];

//bf[ psrc + 0 ] = 0;
//bf[ psrc + 1 ] = 0;
//bf[ psrc + 2 ] = 0;
}




bool llem_wnd::has_subpixels( int xx, int yy )
{
unsigned char *bf = pixbf0;

//return 1;
 
//if( yy == 0 ) return 0;
//if( yy == 1 ) return 0;

//if( xx == 0 ) return 0;
//if( xx == 1 ) return 0;


int r, g, b;

get_pixel( xx, yy, r, g, b );
if( r != backgrnd_r ) return 1;

get_pixel( xx - 1, yy, r, g, b );
if( r != backgrnd_r ) return 1;

get_pixel( xx + 1, yy, r, g, b );
if( r != backgrnd_r ) return 1;


get_pixel( xx, yy - 1, r, g, b );
if( r != backgrnd_r ) return 1;

get_pixel( xx - 1, yy - 1, r, g, b );
if( r != backgrnd_r ) return 1;

get_pixel( xx - 1, yy - 1, r, g, b );
if( r != backgrnd_r ) return 1;


get_pixel( xx, yy + 1, r, g, b );
if( r != backgrnd_r ) return 1;

get_pixel( xx + 1, yy + 1, r, g, b );
if( r != backgrnd_r ) return 1;

get_pixel( xx + 1, yy + 1, r, g, b );
if( r != backgrnd_r ) return 1;
return 0;

}




bool llem_wnd::has_neighbours( int xx, int yy, int kern_size )
{
unsigned char *bf = pixbf0;

//return 1;
 
//if( yy == 0 ) return 0;
//if( yy == 1 ) return 0;

//if( xx == 0 ) return 0;
//if( xx == 1 ) return 0;

int r, g, b;


if( has_subpixels( xx, yy ) ) return 1;


if( has_subpixels( xx - 1*3, yy ) ) return 1;
if( has_subpixels( xx - 2*3, yy ) ) return 1;

if( has_subpixels( xx + 1*3, yy  ) ) return 1;
if( has_subpixels( xx + 2*3, yy  ) ) return 1;


if( has_subpixels( xx, yy - 1*3 ) ) return 1;

if( has_subpixels( xx - 1*3, yy - 1*3 ) ) return 1;
if( has_subpixels( xx - 2*3, yy - 1*3 ) ) return 1;

if( has_subpixels( xx + 1*3, yy - 1*3  ) ) return 1;
if( has_subpixels( xx + 2*3, yy - 1*3  ) ) return 1;



if( has_subpixels( xx, yy - 2*3 ) ) return 1;

if( has_subpixels( xx - 1*3, yy - 2*3 ) ) return 1;
if( has_subpixels( xx - 2*3, yy - 2*3 ) ) return 1;

if( has_subpixels( xx + 1*3, yy - 2*3  ) ) return 1;
if( has_subpixels( xx + 2*3, yy - 2*3  ) ) return 1;



if( has_subpixels( xx, yy + 1*3 ) ) return 1;

if( has_subpixels( xx - 1*3, yy + 1*3 ) ) return 1;
if( has_subpixels( xx - 2*3, yy + 1*3 ) ) return 1;

if( has_subpixels( xx + 1*3, yy + 1*3  ) ) return 1;
if( has_subpixels( xx + 2*3, yy + 1*3  ) ) return 1;



if( has_subpixels( xx, yy + 2*3 ) ) return 1;

if( has_subpixels( xx - 1*3, yy + 2*3 ) ) return 1;
if( has_subpixels( xx - 2*3, yy + 2*3 ) ) return 1;

if( has_subpixels( xx + 1*3, yy + 2*3  ) ) return 1;
if( has_subpixels( xx + 2*3, yy + 2*3  ) ) return 1;


return 0;
}



void llem_wnd::draw_rect( int xx, int yy, int ww, int hh )
{
fl_rect( xx, yy, ww, hh );
}








void llem_wnd::draw_rect_src( int xx, int yy, int ww, int hh )
{
int x1 = xx;
int y1 = yy;
int x2 = xx + ww;
int y2 = yy + hh;

bool dont_set_pixels = 0;

if( lineclp.line_clip_int( x1, y1, x1, y2 ) )
	{

	line( x1, y1, x1, y2, 0, 0, 0, 0, dont_set_pixels, -1 );
	}

if( lineclp.line_clip_int( x1, y2, x2, y2 ) )
	{
	line( x1, y2, x2, y2, 0, 0, 0, 0, dont_set_pixels, -1 );
	}

if( lineclp.line_clip_int( x2, y2, x2, y1 ) )
	{
	line( x2, y2, x2, y1, 0, 0, 0, 0, dont_set_pixels, -1 );
	}

if( lineclp.line_clip_int( x2, y1, x1, y1 ) )
	{
	line( x2, y1, x1, y1, 0, 0, 0, 0, dont_set_pixels, -1 );
	}

//fl_rect( xx, yy, ww, hh );
}










int skip_cnt = 0;

bool llem_wnd::check_empty( int xx, int yy )
{

unsigned char *bf0 = pixbf0;

bool empty = 1;
for( int ky = 0; ky < kern_size; ky++ )
	{
	int srcy = yy + ky;
	
	for( int kx = 0; kx < kern_size; kx++ )
		{				
		int srcx = xx + kx;
		
		int psrc = (srcy * bf0szx * cn_bytes_per_pixel)  +  srcx*cn_bytes_per_pixel;
		if ( bf0[ psrc + 0 ] != backgrnd_r ) { empty = 0; skip_cnt++; goto done_check; }
		}
	}
done_check:

return empty;	
}









//'x1, y1, width, height' point to a rectangle of src buf pixels to be filtered
//'dest_wid, dest_hei' are set to the dest filtered rect dimensions 
void llem_wnd::aa_filter_block( int x1, int y1, int width, int height, float scle, int &dest_wid, int &dest_hei )
{
unsigned char *bf0 = pixbf0;
unsigned char *bf1 = pixbf1;


//printf( "aa_filter_block() x1 %d %d  width %d %d\n", x1, y1, width, height );

int pdest = 0;
//	bool at_edge;

bool done_whgt_sum = 0;
float wght_sum = 0;

int x2 = x1 + width;
int y2 = y1 + height;

float size_srcx = x2 - kern_size;						//don't scan rightmost and bottom most pixels so as to avoid convolution going past end of src image dimensions
float size_srcy = y2 - kern_size;
int size_destx = 0;
int size_desty = 0;


float scale_inv = 1.0f/scle;




filter_calc_cnt = 0;
filter_nonwhite_cnt = 0;
//image scaling - convolve filter and image  (note: the kernel has not been centred over src pixels, so dest would be shifted slightly to the right by 'kern_size')
size_desty = 0;
for( float fy = y1; fy < size_srcy; fy += scale_inv )
	{
	size_destx = 0;
	for( float fx = x1; fx < size_srcx; fx += scale_inv )
		{
		float sumr = 0;
		float sumg = 0;
		float sumb = 0;
		bool need_filter = 1;

//		need_filter = 1;//has_neighbours( nearbyint(fx),  nearbyint(fy), kern_size );
//			if( !need_filter ) { filter_skip_cnt++; goto skip_filter; }


//			int psrc = (fy * sizex*cn_bytes_per_pixel)  +  fx*cn_bytes_per_pixel;
		
		
//			int hsp = has_subpixels( fx, fy );
		
//			if( !hsp ) { need_filter = 0;  goto skip_filter; }
//			else { need_filter = 1; filter_nonwhite_cnt++; }
//					need_filter = 0;

		if( check_empty( nearbyint(fx), nearbyint(fy) ) )
			{
			need_filter = 0;
			goto skip_filter;	
			}

		for( int ky = 0; ky < kern_size; ky++ )
			{
			int srcy = nearbyint(fy) + ky;
			
//				if (srcy >= sizey) at_edge = 1;
//				else at_edge = 0;

			float fky = kern0[ ky ];

			for( int kx = 0; kx < kern_size; kx++ )
				{
				int srcx = nearbyint(fx) + kx;
				
//					if (srcx >= sizex) at_edge = 1;
//					else at_edge = 0;


				int psrc = (srcy * bf0szx * cn_bytes_per_pixel)  +  srcx*cn_bytes_per_pixel;
				
				float fkx = kern0[ kx ];
				uint16_t uik = kern0_16_t[ kx ];

				if( !done_whgt_sum ) wght_sum += fky*fkx;
				
				float fk = fky*fkx;
				
				unsigned char uc;

//					if( need_filter )
					{
					uc = bf0[ psrc + 0 ];								
//						if( at_edge ) uc = 0;				
					sumr += uc*fk;											//mac

					uc = bf0[ psrc + 1 ];				
//						if( at_edge ) uc = 0;				
					sumg += uc*fk;
					
					uc = bf0[ psrc + 2 ];				
//						if( at_edge ) uc = 0;				
					sumb += uc*fk;
					
					filter_calc_cnt++;
					}
//					else{
//						goto skip_filter;
//						}
				}
			
			}
		
		skip_filter:
		
		if( need_filter )
			{
			float ff = sumr / wght_sum;			
			if( ff < 0 ) ff = 0;
			if( ff > 255 ) ff = 255;
			bf1[ pdest + 0 ] = ff;								//place new pixel	

			ff = sumg / wght_sum;			
			if( ff < 0 ) ff = 0;
			if( ff > 255 ) ff = 255;
			bf1[ pdest + 1 ] = ff;	

			ff = sumb / wght_sum;			
			if( ff < 0 ) ff = 0;
			if( ff > 255 ) ff = 255;
			bf1[ pdest + 2 ] = ff;
			done_whgt_sum = 1;
			}
		else{
			bf1[ pdest + 0 ] = backgrnd_r;							//place new pixel	
			bf1[ pdest + 1 ] = backgrnd_g;	
			bf1[ pdest + 2 ] = backgrnd_b;
			}

		pdest += cn_bytes_per_pixel;

		size_destx++;
		}

	size_desty++;
	}

dest_wid = size_destx;
dest_hei = size_desty;

}









void llem_wnd::draw_buf_to_wnd( unsigned char * bf, int posx, int posy, int sizex, int sizey )
{
fl_draw_image( ( const uchar* ) bf, posx, posy, sizex, sizey, cn_bytes_per_pixel, sizex * cn_bytes_per_pixel );

}







//filters individual src pixel rather than rectangle - very slow
void llem_wnd::filter_lines_per_pixel()
{
int dest_dx, dest_dy;

//return;

//	if( idx >= vobj.size() ) return;
	
for( int i = 0; i < vpxl_cntr.size(); i++ )
	{

	int x1;
	int y1;
	int x2;
	int y2;
	
	int ww;
	int hh;
	
	int ks = kern_size;
	
	x1 = vpxl_cntr[i].x - ks;
	y1 = vpxl_cntr[i].y - ks;
	x2 = vpxl_cntr[i].x + ks;
	y2 = vpxl_cntr[i].y + ks;
	
	float scle = 0.25;

	int modulo = 1/scle;
	while( x1 % modulo ) x1--;				//keep filtered (shrunk) block on a pixel
	while( y1 % modulo ) y1--;
	
	ww = x2 - x1;
	hh = y2 - y1;
	
	
	aa_filter_block( x1, y1, ww, hh, scle, dest_dx, dest_dy );

	int dest_x;
	int dest_y;
	
//	dest_x = nearbyint( x1 * scle );
//	dest_y = nearbyint( y1 * scle );
	dest_x = x1 * scle;
	dest_y = y1 * scle;
	draw_buf_to_wnd( pixbf1, dest_x, dest_y + 600, dest_dx, dest_dy );

//	vlin[i].bf_x = dest_x;
//	vlin[i].bf_y = dest_y + 600;
//	vlin[i].bf_w = dest_dx;
//	vlin[i].bf_h = dest_dy;

//break;

//	int last_dx = dest_dx;
//	int last_dy = dest_dy;

//	x1 = 40 + ww - kern_size;
//	y1 = 0;
//	ww = 70;
//	hh = 220;
//	aa_filter_block( x1, y1, ww, hh, 0.25, dest_dx, dest_dy );


//	draw_buf_to_wnd( pixbf1, 10 + last_dx, 400, dest_dx, dest_dy );
	}

}












//filters a rectangle of pixels encompassing a line into a smaller rectangle image, then draws image into window,
//dest is 4x smaller than src
void llem_wnd::filter_lines()
{
int dest_dx, dest_dy;

//return;

//	if( idx >= vobj.size() ) return;
	
for( int i = 0; i < vlin.size(); i++ )
	{
	int x1;
	int y1;
	int x2;
	int y2;
	
	int ww;
	int hh;
	
	int ks = kern_size;
	if ( vlin[i].x1 <= vlin[i].x2 ) { x1 = vlin[i].x1 - ks; x2 = vlin[i].x2 + ks; }
	else { x1 = vlin[i].x2 - ks; x2 = vlin[i].x1 + ks; }
	
	
	if ( vlin[i].y1 <= vlin[i].y2 ) { y1 = vlin[i].y1 - ks; y2 = vlin[i].y2 + ks; }
	else { y1 = vlin[i].y2 - ks; y2 = vlin[i].y1 + ks; }


	float scle = 0.25f;
	int modulo = 1/scle;
	while( x1 % modulo ) x1--;				//keep filtered (shrunk) block on a pixel
	while( y1 % modulo ) y1--;
	
	ww = x2 - x1;
	hh = y2 - y1;
	
	if( x1 < 0 ) x1 = 0;
	if( y1 < 0 ) y1 = 0;

//printf( "filter_lines() x1 %d %d  ww %d %d dest_dx %d %d\n", x1, y1, ww, hh, dest_dx, dest_dy );
	aa_filter_block( x1, y1, ww, hh, scle, dest_dx, dest_dy );

	int dest_x;
	int dest_y;
	
//	dest_x = nearbyint( x1 * scle );
//	dest_y = nearbyint( y1 * scle );
	dest_x = x1 * scle;
	dest_y = y1 * scle;
	draw_buf_to_wnd( pixbf1, dest_x, dest_y + 00, dest_dx, dest_dy );

//	vlin[i].bf_x = dest_x;
//	vlin[i].bf_y = dest_y + 600;
//	vlin[i].bf_w = dest_dx;
//	vlin[i].bf_h = dest_dy;

//break;

//	int last_dx = dest_dx;
//	int last_dy = dest_dy;

//	x1 = 40 + ww - kern_size;
//	y1 = 0;
//	ww = 70;
//	hh = 220;
//	aa_filter_block( x1, y1, ww, hh, 0.25, dest_dx, dest_dy );


//	draw_buf_to_wnd( pixbf1, 10 + last_dx, 400, dest_dx, dest_dy );
	}

}





















void llem_wnd::draw()
{
string s1, st;

if( mousey < 25 ) 
	{
	Fl_Double_Window::draw();
	scrn_regen = 1;
	}

set_clipper_details( menu_hei + 2 );

draw_ship_float_x = vobj[idx_ship].x;
draw_ship_float_y = vobj[idx_ship].y;
draw_ship_float_dx = vobj[idx_ship].dx;
draw_ship_float_dy = vobj[idx_ship].dy;
draw_panx = panx;
draw_pany = pany;

if( animate_bounce_collision_check_state == 2 ) animate_bounce_collision_check_state = 3;	//go to next state

//printf( "llem_wnd::draw() -animate_bounce_collision_check_state %d\n", animate_bounce_collision_check_state );


mdrw_time.time_start( mdrw_time.ns_tim_start );

int iF = mains_font;
int iS = mains_fontsize;



bool alloc = 0;
if( sizex != w() ) alloc = 1;
if( sizey != h() ) alloc = 1;


sizex = w();
sizey = h();


midx = sizex / 2 * 4;
midy = sizey / 2 * 4;

int size_destx;
int size_desty;

vpxl.clear();
vpxl_cntr.clear();
vlin.clear();
skip_cnt = 0;

lines_drawn = 0;
lines_erased = 0;

fl_color( 255, 255, 255 );
//fl_rectf( 0, 0, sizex , sizey );
if( first_draw )
	{
	fl_rectf( 0, menu_hei, sizex , sizey );
	}


bf0szx = sizex*4;
bf0szy = sizey*4;

//en_filter_window_type_tag wnd_type = fwt_blackman_harris;

en_filter_window_type_tag wnd_type = adj_filter;
if( rebuild_filter_kernel ) 
	{
	filter_kernel( wnd_type, kern0, kern0_16_t, kern_size );
//	plot_mgraph0_llem( kern0, kern_size );
//fgph4.hide();
	}
rebuild_filter_kernel = 0;


if( alloc )
	{	
	if( pixbf0 ) delete[] pixbf0;
	if( pixbf1 ) delete[] pixbf1;

	pixbf0 = new unsigned char [ bf0szx * bf0szy * cn_bytes_per_pixel ];	//4x plotting buffer
	pixbf1 = new unsigned char [ sizex * sizey * cn_bytes_per_pixel ];		//1x filtered plotting buffer

	scrn_regen = 1;
	}









//fl_color( 255, 255, 255 );
//fl_rectf( 0, 0, sizex , sizey );

//fl_begin_offscreen( offscr );											//to offscreen buffer

fl_color( 80, 80, 80 );

//fl_color( 255, 255, 255 );

float rad = 75;


fl_line_style ( FL_SOLID, adj_line_width );        	 					//for windows you must set line setting after the colour, see the manual

fl_color( 0, 0, 0 );
//fl_arc( midx - rad, midy - rad, rad*2, rad*2, 0, 360 );


float scale = 0.25;



//vobj[0].theta = adj_angle;
vobj[0].scale = adj_scale;


if( develope_mode >= 1 )
	{
	if ( key_m )
		{
		if( game_state != en_gs_landing ) vobj[idx_ship] = st_obj_ship_copy;

		animate_bounce = 0;
		game_state = en_gs_landing;
		vobj[0].x = mousex * 1/scale - midx;
		vobj[0].y = 2400 - (mousey * 1/scale);
		
		vobj[0].dx = 0;
		vobj[0].dy = 0;
		}
	}




if( first_draw )
	{
	first_draw = 0;
	scrn_regen = 1;

	set_line_update_flag( idx_ground, 1 );								//set flags to draw all of ground
	draw_objs( idx_ground, 1 );									//drawobj's lines, DO THIS HERE to fill some variables initially, such as: '.vobj[].vline[].drawn_y2'
	}

if( obj_create_active ) scrn_regen = 1;

if( scrn_regen ) 
	{
	printf( "draw() - -------------------scrn_regen ---------------- \n" );
	
	memset( pixbf0, 255, bf0szx * bf0szy * cn_bytes_per_pixel );		//THIS is rather SLOW
	memset( pixbf1, 255, sizex * sizey * cn_bytes_per_pixel );			//THIS is rather SLOW

	fl_draw_image( pixbf1, 0, menu_hei, sizex, sizey, cn_bytes_per_pixel, sizex * cn_bytes_per_pixel );


//	vobj[0].x = adj_offsx;
//	vobj[0].y = adj_offsy;
//fl_color( 0, 0, 0 );
//	fl_rectf( 0, 0, sizex , sizey );

	set_line_update_flag( idx_ground, 1 );								//set flags to draw all of ground
	draw_objs( idx_ground, 0 );											//drawobj's lines, inital ground draw


//	draw_objs( idx_ground, 1 );									//drawobj's lines (erase)
//	draw_objs( idx_ground, 0 );									//drawobj's lines, inital ground draw

	set_line_update_flag( idx_ground, 0 );								//set flags for update, ground now only drawn when ship's position flag's it
//	set_line_drawn_flag( idx_ground, 0 );								//clearing flag here, will helps identify which lines will be actually drawn (those not rejected by line clipper)
	scrn_regen = 0;
	}




//set_plot_col( 255, 255, 255 );
//set_pixel3( 200, 200 );
//set_pixel3( 201, 201 );
//set_pixel3( 201, 201 );

//set_plot_col( 0, 0, 0 );
//set_pixel3( 200, 200 );



set_obj_line_col( vobj[idx_ship].vsub[ 0 ].vline, backgrnd_r, backgrnd_g, backgrnd_b ); //prime to clear flame
set_obj_line_col( vobj[idx_ship].vsub[ 1 ].vline, backgrnd_r, backgrnd_g, backgrnd_b ); //prime to clear flame

draw_objs( idx_ship, 1 );												//just draw filled rect encompassing obj's lines (erase)



vobj[idx_ship].sub_obj_idx = flame_flicker;								//toggle flame shape

if( flame_on )
	{
	set_obj_line_col( vobj[idx_ship].vsub[ 0 ].vline, 0, 0, 255 );		//prime to show flame
	set_obj_line_col( vobj[idx_ship].vsub[ 1 ].vline, 0, 0, 255 );		//prime to show flame
	}

//set_rect( ship_rect_x1, ship_rect_y1, ship_rect_x2, ship_rect_y2, 0xffff80 );

draw_objs( idx_crosshair, 0 );											//draw obj's lines as background(erase) if flagged

draw_objs( idx_ship, 0 );												//draw obj's lines as background(erase) if flagged



set_line_update_flag( idx_ground, 0 );

set_line_update_flag_if_in_rect( idx_ground, ship_rect_x1, ship_rect_y1, ship_rect_ww, ship_rect_hh, 1 );		//ground has been affected by ship ?

draw_objs( idx_ground, 0 );												//draw obj's lines if flagged


set_line_update_flag( idx_crosshair, 1 );
draw_objs( idx_crosshair, 0 );											//draw obj's lines if flagged

//line( 20, -200, 1000*4, 900*4, 0,0,0, 0, 0 );



//draw_landing_zone_text( idx_ground );									//drawobj's lines, inital ground draw



//set_line_update_flag_if_in_rect( idx_ground, ship_rect_x1, ship_rect_y1, ship_rect_ww, ship_rect_hh, 1 );		//ground has been affected by ship ?

//set_obj_line_col_single( vobj[idx_ground].vline, idx_line_rect_overlap, 0, 0 ,0 );



//printf("llem_wnd::draw() - 'vlin' %d\n", vlin.size() );



//if( scrn_regen ) 
//	{
//	if( scrn_bf_bkgd ) fl_read_image( scrn_bf_bkgd, 0, 0, w(), h(), 0 );			//read offscreen obj pixels into buffer
//	scrn_regen = 0;
	
//	skip_draw_obj_type = en_ot_ground;
//	}

//skip_draw_obj_type = en_ot_ground;

//if( !scrn_regen ) fl_draw_image( scrn_bf_bkgd, 0, 0, w(), h() - 500, 3, 0);





int minx = 10000;									//any large num will do
int miny = 10000;
int maxx = -10000;
int maxy = -10000;


/*
for( int i = 0; i < vpxl.size(); i++ )
	{
	if( vpxl[i].x < minx ) minx = vpxl[i].x;
	if( vpxl[i].y < miny ) miny = vpxl[i].y;


	if( vpxl[i].x > maxx ) maxx = vpxl[i].x;
	if( vpxl[i].y > maxy ) maxy = vpxl[i].y;
	}
*/


//printf("vpxl.size() %d  vlin %d  size %d %d %d %d\n", vpxl.size(), vlin.size(), minx, miny, maxx, maxy );

//getchar();
//maxx -= 100;

fl_color( 255, 0, 0 );
fl_line_style ( FL_SOLID, 0 );        	 					//for windows you must set line setting after the colour, see the manual




int blk_x1, blk_y1, blk_w, blk_h;


blk_x1 = minx - kern_size;
blk_y1 = miny - kern_size;

blk_w = maxx + kern_size;
blk_h = maxy + kern_size;


blk_x1 = 0;
blk_y1 = 0;
blk_w = sizex;
blk_h = sizey;

//draw_rect( minx, miny, blk_w, blk_h );





int dest_dx, dest_dy;
dest_dx = 100;
dest_dy = 100;
//aa_filter_draw( vlin[0].x1, vlin[0].y1, vlin[0].x2, vlin[0].y2, 0.25, dest_dx, dest_dy );

fl_color( 0, 0, 0 );
fl_line_style ( FL_SOLID, 1 );        	 					//for windows you must set line setting after the colour, see the manual
//fl_line( 5, 50, 80, 80 );





//fl_read_image( pixbf0, 0, 0, sizex, sizey, 0 );						//read offscreen obj pixels into buffer

//fl_end_offscreen();

//if( 0 ) fl_copy_offscreen( 0, 0, blk_w,  blk_h, offscr, 0, 0 );		//draw offscreen obj to visible buffer


//this is very SLOW
//if( adj_show_filtered ) fl_draw_image( ( const uchar* ) pixbf0, 0, 0, sizex, sizey, cn_bytes_per_pixel, bf0szx * cn_bytes_per_pixel );







filter_lines();															//filter each line creating a rect pixel block, then copy rect to wnd
//filter_lines_per_pixel();


taking_off_wait_for_a_draw_to_occur = 0;								//now ok for 'check_collision()' to execute




//draw_rect( dbg_x1, dbg_y1, dbg_x2 - dbg_x1, dbg_y2 - dbg_y1 );


/*

int x1 = 40;
int y1 = 0;
int ww = 80;
int hh = 220;
aa_filter_block( x1, y1, ww, hh, 0.25, dest_dx, dest_dy );

draw_buf_to_wnd( pixbf1, 10, 400, dest_dx, dest_dy );

int last_dx = dest_dx;
int last_dy = dest_dy;

x1 = 40 + ww - kern_size;
y1 = 0;
ww = 70;
hh = 220;
aa_filter_block( x1, y1, ww, hh, 0.25, dest_dx, dest_dy );


draw_buf_to_wnd( pixbf1, 10 + last_dx, 400, dest_dx, dest_dy );
*/





/*
block fill
for( int y = 0; y < dest_dy; y++ )
	{
	for( int x = 0; x < dest_dx; x++ )
		{
		int ptr = (y * dest_dx*cn_bytes_per_pixel)  +  x*cn_bytes_per_pixel;
		
		pixbf1[ ptr + 0 ] = 0;											//remove red channel	
//		ptr += cn_bytes_per_pixel;
		}
	}
*/

size_destx = dest_dx;
size_desty = dest_dy;

//printf("dest_dx %d %d \n", dest_dx, dest_dy );

//getchar();

int ptr = 0;															//modify pixels

/*
for( int y = 0; y < sizey; y++ )
	{
	for( int x = 0; x < sizex; x++ )
		{
//		pixbf0[ ptr + 0 ] = 0;											//remove red channel	
		ptr += cn_bytes_per_pixel;
		}
	}
*/

ptr = 0;
float scale_inv = 1.0f/scale;
float fx = 0;
float fy = 0;
int size_srcx;
int size_srcy;

size_srcx = blk_w;
size_srcy = blk_h;



//float renorm = kern_size;

//int kern_cntr = kern_size/2 + 1;
//printf( "kern_cntr: %d\n", kern_cntr );



int sizex2 = size_destx;
int sizey2 = size_desty;

//draw_line( midx, midy, midx + 400, midy + 40, pixbf0, sizex, sizey );



//fl_draw_image( ( const uchar* ) pixbf1, 0, 0, sizex, sizey, cn_bytes_per_pixel, sizex * cn_bytes_per_pixel );	//draw back onto offscreen obj
//----


//fl_end_offscreen();


//if( 1 ) fl_copy_offscreen( 0, 0, blk_w,  blk_h, offscr, 0, 0 );					//draw offscreen obj to visible buffer


//draw_buf_to_wnd( pixbf1, 10, 400, sizex2, sizey2 );


//if( 1 ) fl_draw_image( ( const uchar* ) pixbf1, 10, 400, sizex2, sizey2, cn_bytes_per_pixel, sizex2 * cn_bytes_per_pixel );	//draw filtered into wnd
//else fl_draw_image( ( const uchar* ) pixbf0, 0, 0, sizex, sizey, cn_bytes_per_pixel, sizex * cn_bytes_per_pixel );



//show raw unfiltered plot buffer that is 4 times the size of final filtered image
if( !adj_show_filtered ) 
	{
	int unfilt_offx = mousex*4;
	int unfilt_offy = mousey*4;

	uchar* unfiltbf0 = pixbf0 + unfilt_offy * bf0szx*cn_bytes_per_pixel +   unfilt_offx * cn_bytes_per_pixel;
	fl_draw_image( ( const uchar* ) unfiltbf0, 0, menu_hei, sizex, sizey, cn_bytes_per_pixel, bf0szx * cn_bytes_per_pixel );
	}





fl_line_style ( FL_SOLID, 0 );        	 					//for windows you must set line setting after the colour, see the manual



double dt = mdrw_time.time_passed( mdrw_time.ns_tim_start );
fps = 1/dt;
if( !(draw_cnt%200) )
	{
	printf( "draw frame time %f  fps %f filter_calc_cnt %d  %d, measured_tframe %f\n", dt, fps, filter_calc_cnt, filter_nonwhite_cnt, measured_tframe );
//printf( "skip_cnt %d\n", skip_cnt );
	}

b_draws_occurring = 1;
draws_missed = 0;


//----
int enlarge = 15;
ship_rect_x1 = vobj[ idx_ship ].drawn_bound_x1 - enlarge;
ship_rect_y1 = vobj[ idx_ship ].drawn_bound_y1 - enlarge;

ship_rect_x2 = vobj[ idx_ship ].drawn_bound_x2 + enlarge;
ship_rect_y2 = vobj[ idx_ship ].drawn_bound_y2 + enlarge;

ship_rect_ww = ship_rect_x2 - ship_rect_x1;
ship_rect_hh = ship_rect_y2 - ship_rect_y1;
//draw_rect_src( ship_rect_x1, ship_rect_y1, ship_rect_ww, ship_rect_hh );

//fl_color( 0, 0, 0 );
//fl_rectf( ship_rect_x1, ship_rect_y1, ship_rect_ww, ship_rect_hh );	//fill rect surrounding ship

//printf( "drawn_bound_x1: %d %d %d %d\n", vobj[ idx_ship ].drawn_bound_x1, vobj[ idx_ship ].drawn_bound_y1, vobj[ idx_ship ].drawn_bound_x2, vobj[ idx_ship ].drawn_bound_y2 );
//----


/*
if( 0 )
	{
	fl_color( 255, 255, 255 );
	fl_rectf( 0, 0, w(), h() );

	draw_obj_fl_line( 2, 0 );
	draw_obj_fl_line( 3, 0 );

	bool ovlp = obj_overlap( 2, 3 );

	if( ovlp ) set_obj_line_col( vobj[2].vline, 255, 0, 0 );
	else set_obj_line_col( vobj[2].vline, 0, 0, 255 );
	}
*/


//fl_rect( 200, 200, 300, 200 );
draw_status( 5, menu_hei, 0x000000 );
if( obj_create_active ) draw_obj_create_params( 5, menu_hei + 30, 0x000000 );


	


if( show_help ) draw_help( 10, h() - 90, 0x000000 );

bool message_shown = 0;

if( ( game_state == en_gs_landed_ok_settled ) || ( game_state == en_gs_game_ended_all_landed0 ) )
	{
	int midxx = w()/2;
	int midyy = h()/2;

	fl_font( iF, 17 );
	
	fl_color( 255, 255, 255 );
	fl_rectf( midxx - 250, menu_hei, 500 , 200 );

	fl_color( 0, 0, 0 );
	strpf( s1, "Landed -- Fuel Earned: %d\n", (int)nearbyint(fuel_bonus) );
	st += s1;
	
	s_landing_rating[ landing_rating_quantized ];
		
	strpf( s1, "Rating %d %% -- %s\n", (int)nearbyint( landing_rating_ratio*100.0f ), s_landing_rating[ landing_rating_quantized ].c_str() );
	st += s1;

	strpf( s1, "Landings: %d/%d -- Fuel Earnings Tot: %d\n", landings_tot, landing_zones_avail, (int)nearbyint(fuel_earnt_tot) );
	st += s1;

	strpf( s1, "Score: %d\n", score );
	st += s1;

	strpf( s1, "Crashes: %d\n", crashings_tot );
	st += s1;

	strpf( s1, "=>Press a key...\n" );
	st += s1;

	fl_draw( st.c_str(), midxx - 250, menu_hei+5, 1, 1, FL_ALIGN_LEFT|FL_ALIGN_TOP );
	message_shown = 1;
	}

if( game_state == en_gs_landed_crash1 )
	{
	int midxx = w()/2;
	int midyy = h()/2;

	
	fl_font( iF, 17 );	

	fl_color( 255, 255, 255 );
	fl_rectf( midxx - 250, menu_hei, 500 , 200 );

	fl_color( 0, 0, 0 );
	strpf( s1, "Crashed!\n");
	st += s1;

	strpf( s1, "Landings: %d/%d -- Fuel Earnings Tot: %d\n", landings_tot, landing_zones_avail, (int)nearbyint(fuel_earnt_tot) );
	st += s1;

	strpf( s1, "Score: %d\n", score );
	st += s1;
	
	strpf( s1, "Crashes: %d\n", crashings_tot );
	st += s1;

	strpf( s1, "=>Press a key...\n" );
	st += s1;



//	int height;
//	int descent = fl_descent();

//	int width = 0;
//	fl_measure( st.c_str(), width, height );


	fl_draw( st.c_str(), midxx - 250, menu_hei+5, 1, 1, FL_ALIGN_LEFT|FL_ALIGN_TOP );
	
	message_shown = 1;
	}



if( game_state == en_gs_game_ended_no_fuel )
	{
	int midxx = w()/2;
	int midyy = h()/2;

	fl_font( iF, 17 );	

	fl_color( 255, 255, 255 );
	fl_rectf( midxx - 250, menu_hei, 500 , 200 );

	fl_color( 195, 90, 90 );

	strpf( s1, "Out of Fuel! -- Game Ended\n");
	st += s1;

	strpf( s1, "Landings: %d/%d -- Fuel Earnings Tot: %d\n", landings_tot, landing_zones_avail, (int)nearbyint(fuel_earnt_tot) );
	st += s1;

	strpf( s1, "Score: %d\n", score );
	st += s1;

	strpf( s1, "Crashes: %d\n", crashings_tot );
	st += s1;

	strpf( s1, "=>Press a key for a new game...\n" );
	st += s1;

	fl_draw( st.c_str(), midxx - 250, menu_hei+5, 1, 1, FL_ALIGN_LEFT|FL_ALIGN_TOP );
	}




if( game_state == en_gs_game_ended_too_many_crashes )
	{
	int midxx = w()/2;
	int midyy = h()/2;

	fl_font( iF, 17 );	

	fl_color( 255, 255, 255 );
	fl_rectf( midxx - 250, menu_hei, 500 , 200 );

	fl_color( 195, 90, 90 );

	strpf( s1, "Too many crashes! -- Game Ended\n");
	st += s1;

	strpf( s1, "Landings: %d/%d -- Fuel Earnings Tot: %d\n", landings_tot, landing_zones_avail, (int)nearbyint(fuel_earnt_tot) );
	st += s1;

	strpf( s1, "Score: %d\n", score );
	st += s1;

	strpf( s1, "Crashes: %d\n", crashings_tot );
	st += s1;

	strpf( s1, "=>Press a key for a new game...\n" );
	st += s1;

	fl_draw( st.c_str(), midxx - 250, menu_hei+5, 1, 1, FL_ALIGN_LEFT|FL_ALIGN_TOP );
	}




if( game_state == en_gs_game_ended_all_landed1 )
	{
	int midxx = w()/2;
	int midyy = h()/2;

	fl_font( iF, 17 );	

	fl_color( 255, 255, 255 );
	fl_rectf( midxx - 250, menu_hei, 500 , 200 );

	fl_color( 45, 183, 45 );

	strpf( s1, "All Done, Well Done! -- Game Ended\n");
	st += s1;

	strpf( s1, "Landings: %d/%d -- Fuel Earnings Tot: %d\n", landings_tot, landing_zones_avail, (int)nearbyint(fuel_earnt_tot) );
	st += s1;

	strpf( s1, "Score: %d\n", score );
	st += s1;

	strpf( s1, "Crashes: %d\n", crashings_tot );
	st += s1;

	strpf( s1, "=>Press a key for a new game...\n" );
	st += s1;

	fl_draw( st.c_str(), midxx - 250, menu_hei+5, 1, 1, FL_ALIGN_LEFT|FL_ALIGN_TOP );
	}






if( (pause) && ( !message_shown ) )
	{
	int midxx = w()/2;
	int midyy = h()/2;

	fl_font( iF, 17 );	

	fl_color( 255, 255, 255 );
	fl_rectf( midxx - 250, menu_hei, 500 , 200 );

	fl_color( 0, 0, 0 );
 
	strpf( s1, "--- PAUSED ---\n" );
	st += s1;

	strpf( s1, "Landings: %d/%d -- Fuel Earnings Tot: %d\n", landings_tot, landing_zones_avail, (int)nearbyint(fuel_earnt_tot) );
	st += s1;

	strpf( s1, "Score: %d\n", score );
	st += s1;

	strpf( s1, "Crashes: %d\n", crashings_tot );
	st += s1;

 	strpf( s1, "=>PAUSED, press 'p'...\n" );
	st += s1;

	fl_draw( st.c_str(), midxx - 250, menu_hei+5, 1, 1, FL_ALIGN_LEFT|FL_ALIGN_TOP );
	message_shown = 1;
	}

if( !(draw_cnt%200) ) printf( "lines_drawn %d, lines_erased %d\n", lines_drawn, lines_erased );

draw_cnt++;

fl_font( iF, iS );	

}








//used for obj loading
bool llem_wnd::file_open_obj( unsigned int idx, int offx, int offy, string fname, bool clear_vsub, string &sdesc )
{
bool vb = 1;
mystr m1, m2;
string s1, st;

sdesc = "";

if( idx >= vobj.size() ) return 0;

char szdesc[4096];
szdesc[0] = 0x0;

if( m1.readfile( fname, 2000 ) )
	{
	vector<string> vstr0, vstr1;
	
	vobj[idx].vline.clear();
	
	if( clear_vsub )
		{
		vobj[idx].vsub.clear();
		vobj[idx].sub_obj_idx = -1;
		}
	
	m1.LoadVectorStrings( vstr0, '\n' );	
	for( int i = 0; i < vstr0.size(); i++ )
		{
		int line_cnt = 0;
//		int ioffx, ioffy;
//		ioffx = 0;
//		ioffy = 0;

		sscanf(vstr0[i].c_str(), "%d", &line_cnt );
//		sscanf(vstr0[i].c_str(), "%d %d %d", &line_cnt, &ioffx, &ioffy );
		
		if( i == 0 )
			{
			m2 = vstr0[i];
			m2.LoadVectorStrings( vstr1, ',' );	
			if(vb)printf("file_open_obj() - line_cnt %02d: %d\n", i, line_cnt );			//num lines
			
			if( vstr1.size() > 1 )
				{
				m2 = vstr1[1];
				m2.EscToStr();
				sdesc = m2.szptr();
				
				if(vb)printf("file_open_obj() - desc: '%s'\n", sdesc.c_str() );				//desc
				}
			}
		else{
			int line_type;
			float x1, y1, x2, y2;
			int fuel_val;
			int spare;

			sscanf(vstr0[i].c_str(), "%d %d %d %f %f %f %f ", &line_type, &fuel_val, &spare, &x1, &y1, &x2, &y2 );
			if(vb)printf("%02d:   line_type %d  %d %d %f, %f %f %f\n", i, line_type, fuel_val, spare, x1, y1, x2, y2);

			st_line_tag oo;
			oo.line_type = (en_line_type)line_type;
			oo.fuel_val = fuel_val;

			if( oo.line_type != en_lt_ground_landing_zone_with_fuel )
				{
				oo.fuel_val = 0;
				}

			oo.drawn_x1 = 0;														//put anything in here thats initally valid for plotting
			oo.drawn_y1 = 0;
			oo.drawn_x2 = 1;
			oo.drawn_y2 = 1;

			if( x1 > x2 )					//make x1 the smaller val
				{
				float tmp = x1;
				x1 = x2;
				x2 = tmp;	

				tmp = y1;
				y1 = y2;
				y2 = tmp;	
				}

			oo.x1 = x1 + offx;
			oo.y1 = y1 + offy;
			oo.x2 = x2 + offx;
			oo.y2 = y2 + offy;
			
			oo.dx = 0;
			oo.dy = 0;
			oo.theta = 0;
			oo.theta_inc = 0;

			oo.r = 0;
			oo.g = 0;
			oo.b = 0;
			
			vobj[idx].vline.push_back( oo );

			set_line_type_col( idx, vobj[idx].vline.size() - 1 );
			}
		}
	set_bounding_rect( idx, bounding_scale_down, bounding_scale_down, 1 );
	return 1;
	}


return 0;
}









//used for older ground obj loading, which had a coord point per line
bool llem_wnd::file_open_obj_old( unsigned int idx, string fname, float sclex, float scley )
{
mystr m1;
string s1, st;

if( idx >= vobj.size() ) return 0;

/*			
st_line_tag oo;
oo.x1 = -800;
oo.y1 = 0;
oo.x2 = 800;
oo.y2 = 300;

	vobj[idx].vline.clear();
vobj[idx].vline.push_back( oo );
return 1;
*/

if( m1.readfile( fname, 10000 ) )
	{
	vector<string> vstr0;
	
	vobj[idx].vline.clear();
//	vobj[idx].vsub.clear();
	
	m1.LoadVectorStrings( vstr0, '\n' );
	
	float lastx, lasty;
	
	for( int i = 0; i < vstr0.size(); i++ )
		{
		float x1, y1, x2, y2;
		sscanf(vstr0[i].c_str(), "%f, %f", &x1, &y1 );
		printf("%02d:   %f, %f\n", i,  x1, y1);
		
//		float sclx = 11.2;
//		float scly = 5.9;
		if( i == 0 ) 
			{
			lastx = -820 + x1 * sclex;
			lasty = -500;//900 - y1 * scly;
			}
		else{
			st_line_tag oo;
			oo.flags = en_ft_update;
			oo.line_type = en_lt_ground_non_landing_pad;
			oo.fuel_val = 0;

			oo.drawn_x1 = 0;														//put anything in here thats initally valid for plotting
			oo.drawn_y1 = 0;
			oo.drawn_x2 = 1;
			oo.drawn_y2 = 1;

			oo.x1 = lastx;
			oo.y1 = lasty;
			oo.x2 = -820 + x1 * sclex;
			oo.y2 = 2600 - y1 * scley;

			oo.dx = 0;
			oo.dy = 0;
			oo.theta = 0;
			oo.theta_inc = 0;

			lastx = oo.x2;
			lasty = oo.y2;
			oo.r = 0;
			oo.g = 0;
			oo.b = 0;

			vobj[idx].vline.push_back( oo );
			}
		}
	
	if( 1 )
		{
		st_line_tag oo;
		oo.flags = en_ft_update;

		oo.drawn_x1 = 0;														//put anything in here thats initally valid for plotting
		oo.drawn_y1 = 0;
		oo.drawn_x2 = 1;
		oo.drawn_y2 = 1;

		oo.x1 = lastx;
		oo.y1 = lasty;
		oo.x2 = lastx;
		oo.y2 = 500;

		oo.dx = 0;
		oo.dy = 0;
		oo.theta = 0;
		oo.theta_inc = 0;

		oo.r = 0;
		oo.g = 0;
		oo.b = 0;

		vobj[idx].vline.push_back( oo );
		}


	vobj[ idx ].drawn_bound_x1 = 0;
	vobj[ idx ].drawn_bound_y1 = 0;
	vobj[ idx ].drawn_bound_x2 = 0;
	vobj[ idx ].drawn_bound_y2 = 0;

	return 1;
	}
return 0;
}







bool llem_wnd::file_save_obj( unsigned int idx, string fname, string sdesc )
{
mystr m1;
string s1, st;

if( idx >= vobj.size() ) return 0;

//count lines that will be saved
int cnt = 0;
for( int i = 0; i < vobj[idx].vline.size(); i++ )
	{
	int line_type = vobj[idx].vline[i].line_type;
	
	if( line_type == en_lt_ground_landing_zone_fuel_digit ) continue;	//dont save fuel 7 seg nums
	cnt++;
	}

//save lines
m1 = sdesc;
m1.StrToEscMostCommon3();

strpf( s1, "%d %s\n", cnt, m1.szptr() );
st += s1;

for( int i = 0; i < vobj[idx].vline.size(); i++ )
	{
	int line_type = vobj[idx].vline[i].line_type;
	
	if( line_type == en_lt_ground_landing_zone_fuel_digit ) continue;	//dont save fuel 7 seg nums
	
	int fuel_value = vobj[idx].vline[i].fuel_val;
	int spare = 0;
	strpf( s1, "%d %d %d %f %f %f %f\n", line_type, fuel_value, spare, vobj[idx].vline[i].x1, vobj[idx].vline[i].y1, vobj[idx].vline[i].x2, vobj[idx].vline[i].y2 );
	st += s1;
	}			
m1 = st;
m1.writefile( fname );

return 1;
}


void llem_wnd::set_pause( bool state )
{
pause = state;
mute = pause;
}


void llem_wnd::do_key( int key, bool state )
{
mystr m1;
string s1, st;




if( ( key == 'p' ) && ( state ) ) 
	{
	pause = !pause;
	set_pause( pause );

	scrn_regen = 1;
	}

/*

if( key_z )
	{
	zoom_extents = !zoom_extents;
	
	if( zoom_extents == 0 )
		{
		zoom_extents = 0;
		zoom_extents_goes_off = 1;	
		}
	scrn_regen = 1;
	}
*/


if( ( shift_key ) )
	{
	set_thrust( thrust_wheel -0.1f );
	}

if( ( ctrl_key ) )
	{
	set_thrust( thrust_wheel + 0.1f );
	}


if( ( key == 'z' ) && ( state ) )
	{
	set_thrust( 1.0 );
	}

if( ( key == 'z' ) && ( !state ) )
	{
	set_thrust( 0.0 );
	}

if( ( key == FL_Up ) && ( state ) )
	{
	set_thrust( 1.0 );
	}

if( ( key == FL_Up ) && ( !state ) )
	{
	set_thrust( 0.0 );
	}

if( develope_mode >= 1 )
	{
	if( ( key == 'e' ) && ( state ) )
		{
		ship_restore( 0, 0 );
		ground_landing_restore_fuel_levels();
		ground_landing_zone_fuel_remove_digits();
		ground_landing_zone_colour_and_fuel_digits();
		
		obj_create_active = !obj_create_active;
		if( obj_create_active )											//create mode?
			{
			
			set_pause( 1 );
			obj_create_line_idx = 0;
			obj_create_line_dx = 0;
			obj_create_line_dy = 0;

		
			vobj[obj_create_idx].visible = 1;
			vobj[ !obj_create_idx ].visible = 0;
			if( obj_create_idx == idx_ship ) 
				{
				zoom_detent_last = 5;
				zoom_detent_to_zoom( zoom_detent_last, zoomx );
				zoomy = zoomx;
				}

			if( obj_create_idx == idx_ground ) 
				{
				zoom_detent_last = 0;
				zoom_detent_to_zoom( zoom_detent_last, zoomx );
				zoomy = zoomx;
				}
			panx = vobj[obj_create_idx].x;
			pany = vobj[obj_create_idx].y;
			
			crosshair_build();
			vobj[idx_crosshair].visible = 1;
			scrn_regen = 1;
			}
		else{
			zoom_extents_goes_off = 1;
			zoom_detent = -2;
			zoom_detent_last = -1;
			vobj[obj_create_idx].visible = 1;
			vobj[ !obj_create_idx ].visible = 1;
//			toggle_space = 1;
			vobj[idx_crosshair].visible = 0;
			scrn_regen = 1;
			}
		}


	if( ctrl_key )
		{
		if( !obj_create_active )
			{
			if( pause )
				{
				float step = 0.1;
				if( shift_key ) step = 0.5;
				
				if( key == FL_Left )
					{
					vobj[obj_create_idx].x -= step;
					}

				if( key == FL_Right )
					{
					vobj[obj_create_idx].x += step;
					}

				if( key == FL_Up )
					{
					vobj[obj_create_idx].y += step;
					}

				if( key == FL_Down )
					{
					vobj[obj_create_idx].y -= step;
					}
				}
			}
		}
		
	
//	if( key == '[' ) 
//		{
//		panx = vobj[obj_create_idx].x;
//		pany = vobj[obj_create_idx].y;
//		scrn_regen = 1;
//		}

//	if( key == ']' )
//		{
//		panx = vobj[obj_create_idx].x + -20;
//		pany = vobj[obj_create_idx].y + 0;
//		scrn_regen = 1;
//		}
	
	if( !obj_create_active )
		{
		if( key == 'l' )
			{
			file_load_ship_state( sship_state_fname );
			}
		}
	
	if( key == 'b' )
		{
		bounce_amp = 1.0f;
		env_build_explosion_env2( srate, 0 );
		}

	if( ( key == 't' ) && ( !state ) ) 
		{
		adj_show_filtered = !adj_show_filtered;
		if( adj_show_filtered ) scrn_regen = 1;
		}

	}




if( develope_mode >= 1 )
	{

	if( ( key == ' ' ) && ( state ) )
		{
		if( game_state == en_gs_landing )
			{
//	printf( "game_state: %d, two: %f\n", game_state, 0 );
//			key_space = 1; 
			toggle_space = !toggle_space;
			}
		}

	if( ( key == 'x' ) && ( state ) )  
		{
		if( !obj_create_active )
			{
			vobj[idx_ship] = st_obj_ship_copy;
			ship_crash( "debug crash" );
			}
		}
	}


if( develope_mode >= 2 )
	{
	if( ( key == 'h' ) && ( state ) )
		{
		show_help = !show_help;
		scrn_regen = 1;
		}
	}
}







void llem_wnd::crosshair_build()
{
if( obj_create_line_endpoint == 0 )
	{
	vobj[idx_crosshair].scale = adj_scale;

	//horiz crosshair
	vobj[idx_crosshair].x = vobj[obj_create_idx].x;
	
	float x1 = vobj[obj_create_idx].vline[ obj_create_line_idx ].x1 - 200;
	float x2 = vobj[obj_create_idx].vline[ obj_create_line_idx ].x1 + 200;
	
	vobj[idx_crosshair].vline[0].x1 = x1; 
	vobj[idx_crosshair].vline[0].x2 = x2; 

	vobj[idx_crosshair].y = vobj[obj_create_idx].y; 
	vobj[idx_crosshair].vline[0].y1 = vobj[obj_create_idx].vline[ obj_create_line_idx ].y1; 
	vobj[idx_crosshair].vline[0].y2 = vobj[obj_create_idx].vline[ obj_create_line_idx ].y1; 

	//vert crosshair
	vobj[idx_crosshair].vline[1].x1 = vobj[obj_create_idx].vline[ obj_create_line_idx ].x1; 
	vobj[idx_crosshair].vline[1].x2 = vobj[obj_create_idx].vline[ obj_create_line_idx ].x1; 

	float y1 = vobj[obj_create_idx].vline[ obj_create_line_idx ].y1 - 200;
	float y2 = vobj[obj_create_idx].vline[ obj_create_line_idx ].y1 + 200;

	vobj[idx_crosshair].vline[1].y1 = y1; 
	vobj[idx_crosshair].vline[1].y2 = y2; 
	}
else{
	vobj[idx_crosshair].scale = adj_scale;

	//horiz crosshair
	vobj[idx_crosshair].x = vobj[obj_create_idx].x;

	float x1 = vobj[obj_create_idx].vline[ obj_create_line_idx ].x2 - 200;
	float x2 = vobj[obj_create_idx].vline[ obj_create_line_idx ].x2 + 200;

	vobj[idx_crosshair].vline[0].x1 = x1; 
	vobj[idx_crosshair].vline[0].x2 = x2; 

	vobj[idx_crosshair].y = vobj[obj_create_idx].y; 
	vobj[idx_crosshair].vline[0].y1 = vobj[obj_create_idx].vline[ obj_create_line_idx ].y2; 
	vobj[idx_crosshair].vline[0].y2 = vobj[obj_create_idx].vline[ obj_create_line_idx ].y2; 

	//vert crosshair
	vobj[idx_crosshair].vline[1].x1 = vobj[obj_create_idx].vline[ obj_create_line_idx ].x2; 
	vobj[idx_crosshair].vline[1].x2 = vobj[obj_create_idx].vline[ obj_create_line_idx ].x2; 

	float y1 = vobj[obj_create_idx].vline[ obj_create_line_idx ].y2 - 200;
	float y2 = vobj[obj_create_idx].vline[ obj_create_line_idx ].y2 + 200;

	vobj[idx_crosshair].vline[1].y1 = y1; 
	vobj[idx_crosshair].vline[1].y2 = y2; 
	}
}






void llem_wnd::do_key_obj_create( int key )
{
mystr m1, m2;
string s1, st;

if( !obj_create_active ) return;
		
		




if( obj_create_idx >= vobj.size() ) return;

scrn_regen = 1;


if( !ctrl_key )
	{
	if( key == 'o' ) 
		{
		obj_create_idx = !obj_create_idx;					//toggle objs
				
		if( obj_create_idx == 0 ) zoomx = zoomy = 24;						//ship
		if( obj_create_idx == 1 ) zoomx = zoomy = 1;						//ground

		obj_create_line_idx = 0;
		vobj[obj_create_idx].visible = 1;
		vobj[!obj_create_idx].visible = 0;
		panx = vobj[obj_create_idx].x + vobj[obj_create_idx].vline[obj_create_line_idx].x1;
		pany = vobj[obj_create_idx].y + vobj[obj_create_idx].vline[obj_create_line_idx].y1;
		}
	}

if( key == FL_Enter )
	{
	st_line_tag oo;
	
	oo.line_type = en_lt_ship;
	oo.fuel_val = 0;
	
	oo.drawn_x1 = 0;														//put anything in here thats initally valid for plotting
	oo.drawn_y1 = 0;
	oo.drawn_x2 = 1;
	oo.drawn_y2 = 1;

	oo.flags = en_ft_update;

	oo.dx = 0;
	oo.dy = 0;
	oo.theta = 0;
	oo.theta_inc = 0;
	
	if( obj_create_line_endpoint == 0 )
		{
		oo.x1 = vobj[obj_create_idx].vline[obj_create_line_idx].x1;
		oo.y1 = vobj[obj_create_idx].vline[obj_create_line_idx].y1;
		}
	else{
		oo.x1 = vobj[obj_create_idx].vline[obj_create_line_idx].x2;
		oo.y1 = vobj[obj_create_idx].vline[obj_create_line_idx].y2;
		}

	oo.x2 = oo.x1 + 20;
	oo.y2 = oo.y1 + 20;

	oo.r = 0;
	oo.g = 0;
	oo.b = 0;

	vobj[obj_create_idx].vline.push_back( oo );

	ground_landing_restore_fuel_levels();
	ground_landing_zone_fuel_remove_digits();

	std::stable_sort( vobj[obj_create_idx].vline.begin(), vobj[obj_create_idx].vline.end(), sort_cmp_line_by_x );

	ground_landing_zone_colour_and_fuel_digits();
	obj_create_line_idx++;
	}


if( vobj[obj_create_idx].vline.size() == 0 ) return;

if( obj_create_line_idx >= vobj[obj_create_idx].vline.size() )  return;

float delta = 0.125;
if( shift_key ) delta = 5;


//change sel endpoint
if( !ctrl_key )
	{
	if( key == FL_Up ) 
		{
		obj_create_line_endpoint = !obj_create_line_endpoint;
		}

	if( key == FL_Down )
		{
		obj_create_line_endpoint = !obj_create_line_endpoint;
		}
	}

if( !ctrl_key )
	{

	}
else{
	//adj endpoint position
	if( obj_create_line_endpoint == 0 )
		{
		if( key == FL_Left ) vobj[obj_create_idx].vline[ obj_create_line_idx ].x1 -= delta;
		if( key == FL_Right ) vobj[obj_create_idx].vline[ obj_create_line_idx ].x1 += delta;
		if( key == FL_Up ) vobj[obj_create_idx].vline[ obj_create_line_idx ].y1 += delta;
		if( key == FL_Down ) vobj[obj_create_idx].vline[ obj_create_line_idx ].y1 -= delta;

		ground_landing_restore_fuel_levels();
		ground_landing_zone_fuel_remove_digits();
		ground_landing_zone_colour_and_fuel_digits();
		}
	else{
		if( key == FL_Left ) vobj[obj_create_idx].vline[ obj_create_line_idx ].x2 -= delta;
		if( key == FL_Right ) vobj[obj_create_idx].vline[ obj_create_line_idx ].x2 += delta;
		if( key == FL_Up ) vobj[obj_create_idx].vline[ obj_create_line_idx ].y2 += delta;
		if( key == FL_Down ) vobj[obj_create_idx].vline[ obj_create_line_idx ].y2 -= delta;

		ground_landing_restore_fuel_levels();
		ground_landing_zone_fuel_remove_digits();
		ground_landing_zone_colour_and_fuel_digits();
		}
	}


if( key == FL_Delete )
	{
	vobj[obj_create_idx].vline.erase( vobj[obj_create_idx].vline.begin() + obj_create_line_idx );
	
	obj_create_line_idx --;
	if( obj_create_line_idx <=  0 ) obj_create_line_idx = 0;
	}


if( ctrl_key )
	{
	if( key == FL_Page_Up )
		{
		obj_vertex_offset( obj_create_idx, 0, 1, 1, 1 );
		}

	if( key == FL_Page_Down )
		{
		obj_vertex_offset( obj_create_idx, 0, -1, 1, 1 );
		}
	}
else{
	//line sel
	if( key == FL_Page_Up ) 
		{
		obj_create_line_idx--;
		if( obj_create_line_idx >= (int)vobj[obj_create_idx].vline.size() ) obj_create_line_idx = vobj[obj_create_idx].vline.size() - 1;
		if( obj_create_line_idx < 0 ) obj_create_line_idx = 0;
		if( zoom_detent != 0 )
			{
		 	panx = vobj[obj_create_idx].x + vobj[obj_create_idx].vline[obj_create_line_idx].x1;
			pany = vobj[obj_create_idx].y + vobj[obj_create_idx].vline[obj_create_line_idx].y1;
			}
		}

	if( key == FL_Page_Down ) 
		{
		obj_create_line_idx++;
		if( obj_create_line_idx >= (int)vobj[obj_create_idx].vline.size() ) obj_create_line_idx = vobj[obj_create_idx].vline.size() - 1;
		if( obj_create_line_idx < 0 ) obj_create_line_idx = 0;
		if( zoom_detent != 0 )
			{
		 	panx = vobj[obj_create_idx].x + vobj[obj_create_idx].vline[obj_create_line_idx].x1;
			pany = vobj[obj_create_idx].y + vobj[obj_create_idx].vline[obj_create_line_idx].y1;
			}
		}
	}




crosshair_build();



if( ctrl_key )
	{
	if( ( key == 's' ) && ( obj_create_active ) )
		{
		ctrl_key = 0;

		string sdesc;
		
		if( obj_create_idx == 0 ) 
			{
			obj_create_fname = slast_ship_fname;
			sdesc = sdesc_llem;
			}
		else{
			obj_create_fname = slast_moonscape_fname;
			sdesc = sdesc_ground;
			}
		
		bool do_write = 0;
		unsigned long long int filesz;
		char *pPathName = fl_file_chooser( "Obj Save ?", "*", (const char*)obj_create_fname.c_str(), 0 );
		if (!pPathName) return;


		if ( m1.filesize( pPathName, filesz ) )
			{
			strpf( s1, "Write Obj - File already exists, Overwrite it? : '%s'", pPathName );
			int ret = fl_choice( s1.c_str(), "Cancel", "Overwrite", 0 );
			if( ret == 1 )
				{
				do_write = 1;
				}
			}
		else{
			do_write = 1;
			}

		if( do_write )
			{
			obj_create_fname = pPathName;
			
			m2 = sdesc;
			m2.StrToEscMostCommon3();

			file_save_obj( obj_create_idx, obj_create_fname, m2.szptr() );

/*
			obj_create_fname = pPathName;
			strpf( s1, "%d\n", vobj[obj_create_idx].vline.size() );
			st += s1;
			
			
			for( int i = 0; i < vobj[obj_create_idx].vline.size(); i++ )
				{
				int line_type = vobj[obj_create_idx].vline[i].line_type;
				
				strpf( s1, "%d %f %f %f %f\n", line_type, vobj[obj_create_idx].vline[i].x1, vobj[obj_create_idx].vline[i].y1, vobj[obj_create_idx].vline[i].x2, vobj[obj_create_idx].vline[i].y2 );
				st += s1;
				}			
			m1 = st;
			m1.writefile( obj_create_fname );
*/
			}

		}
	}


if( ctrl_key )
	{
	if( ( key == 'o' ) && ( obj_create_active ) )
		{
		ctrl_key = 0;

		char *pPathName = fl_file_chooser( "Obj Open ?", "*", (const char*)obj_create_fname.c_str(), 0 );
		if (!pPathName) return;
		
		bool clear_vsub = 1;
		string sdesc;
		
		if( file_open_obj( obj_create_idx, 0, 0, pPathName, clear_vsub, sdesc ) )
			{
			obj_create_line_idx = 0;

			obj_vertex_offset( 0, 0, 100, 1, 1 );

			set_line_update_flag( 0, 1 );
			
//			set_bounding_rect( 0, bounding_scale_down, bounding_scale_down, 1 );

			obj_create_fname = pPathName;
			obj_create_line_idx = 0;
			}
		}
	}




//toggle line type from 'en_lt_normal' to 'en_lt_collision_det_pads' ...  etc
if( ( key == 'c' ) && ( ctrl_key ) )
	{
	int dir = 1;
//	if( ctrl_key ) dir = -1;

	int typ = vobj[obj_create_idx].vline[ obj_create_line_idx ].line_type;
	typ += dir;
	
	if( typ >= en_lt_ground_landing_zone_no_fuel ) typ = 0;
	vobj[obj_create_idx].vline[ obj_create_line_idx ].line_type = (en_line_type)typ;

	set_line_type_col( obj_create_idx, obj_create_line_idx );

	ground_landing_restore_fuel_levels();
	ground_landing_zone_fuel_remove_digits();

	ground_landing_zone_colour_and_fuel_digits();
	}
	
	
if( ( key == 'x' ) )
	{
	vobj[idx_crosshair].visible = !vobj[idx_crosshair].visible;
	}

if( ( key == 'l' ) )
	{
	if( obj_create_idx == idx_ship )									//make horiz ?
		{
		if( obj_create_line_endpoint == 0 )
			{
			vobj[obj_create_idx].vline[obj_create_line_idx].y2 = vobj[obj_create_idx].vline[obj_create_line_idx].y1;
			}
		else{
			vobj[obj_create_idx].vline[obj_create_line_idx].y1 = vobj[obj_create_idx].vline[obj_create_line_idx].y2;
			}
		}

	if( obj_create_idx == idx_ground )									//landingzone req? make line horiz
		{
		if( obj_create_line_endpoint == 0 )
			{
			vobj[obj_create_idx].vline[obj_create_line_idx].y2 = vobj[obj_create_idx].vline[obj_create_line_idx].y1;
			}
		else{
			vobj[obj_create_idx].vline[obj_create_line_idx].y1 = vobj[obj_create_idx].vline[obj_create_line_idx].y2;
			}
		vobj[obj_create_idx].vline[obj_create_line_idx].line_type = en_lt_ground_landing_zone_with_fuel;
		ground_landing_zone_fuel_remove_digits();
		ground_landing_zone_colour_and_fuel_digits();
		}
	}

if( ( key == '[' ) || ( key == ']' ) )													//landingzone req? make line horiz
	{
	if( key == '[' ) zoom_detent_last--;
	if( key == ']' ) zoom_detent_last++;
	if( zoom_detent_last < 0 ) zoom_detent_last = 8;
	if( zoom_detent_last > 8 ) zoom_detent_last = 0;
	zoom_detent_to_zoom( zoom_detent_last, zoomx );
	zoomy = zoomx;
	
	if( zoom_detent != 0 )
		{
		panx = vobj[obj_create_idx].x + vobj[obj_create_idx].vline[obj_create_line_idx].x1;
		pany = vobj[obj_create_idx].y + vobj[obj_create_idx].vline[obj_create_line_idx].y1;
		}
	}


if( ( key == 'f' ) )													//inc fuel
	{
	ground_landing_zone_fuel_remove_digits();
	
	int typ = vobj[obj_create_idx].vline[ obj_create_line_idx ].line_type;

	if( ( typ == en_lt_ground_landing_zone_with_fuel ) ) 
		{
		int fuel_val = vobj[obj_create_idx].vline[obj_create_line_idx].fuel_val;
		
		if( !ctrl_key) fuel_val++;
		else fuel_val--;
		
		if( fuel_val < 00 ) fuel_val = 99;
		if( fuel_val > 99 ) fuel_val = 0;
		vobj[obj_create_idx].vline[obj_create_line_idx].fuel_val = fuel_val;
		
		}
	ground_landing_zone_colour_and_fuel_digits();
	}

if( ( key == '/' ) )													//sort lines in x order
	{
	ground_landing_restore_fuel_levels();
	ground_landing_zone_fuel_remove_digits();
	std::stable_sort( vobj[obj_create_idx].vline.begin(), vobj[obj_create_idx].vline.end(), sort_cmp_line_by_x );
	ground_landing_zone_colour_and_fuel_digits();
	}


obj_create_line_dx = vobj[obj_create_idx].vline[obj_create_line_idx].x1 - vobj[obj_create_idx].vline[obj_create_line_idx].x2;
obj_create_line_dy = vobj[obj_create_idx].vline[obj_create_line_idx].y1 - vobj[obj_create_idx].vline[obj_create_line_idx].y2;

}





//only used for ship, and when creating/editing the obj's lines
void llem_wnd::set_line_type_col( unsigned int obj_idx, unsigned int line_idx )
{
if( obj_idx >= vobj.size() ) return;

vector<st_line_tag> vo;
vo = vobj[ obj_idx ].vline;


if( line_idx >= vo.size() ) return;

vobj[obj_idx].vline[ line_idx ].r = 0;
vobj[obj_idx].vline[ line_idx ].g = 0;
vobj[obj_idx].vline[ line_idx ].b = 0;			

if( vobj[obj_idx].vline[ line_idx ].line_type == en_lt_ship_collision_det_pads_left ) 
	{
	vobj[obj_idx].vline[ line_idx ].r = 0;				//don't use 255 for red as this will confuse 'check_empty()'
	vobj[obj_idx].vline[ line_idx ].g = 255;
	vobj[obj_idx].vline[ line_idx ].b = 0;			
	}

if( vobj[obj_idx].vline[ line_idx ].line_type == en_lt_ship_collision_det_pads_right ) 
	{
	vobj[obj_idx].vline[ line_idx ].r = 0;				//don't use 255 for red as this will confuse 'check_empty()'
	vobj[obj_idx].vline[ line_idx ].g = 180;
	vobj[obj_idx].vline[ line_idx ].b = 0;			
	}

if( vobj[obj_idx].vline[ line_idx ].line_type == en_lt_ship_collision_det_sides ) 
	{
	vobj[obj_idx].vline[ line_idx ].r = 250;				//don't use 255 for red as this will confuse 'check_empty()'
	vobj[obj_idx].vline[ line_idx ].g = 0;
	vobj[obj_idx].vline[ line_idx ].b = 0;			
	}
}




bool llem_wnd::pressed_key()
{
bool pressed = 0;

if( key_s ) pressed = 1;
if( key_w ) pressed = 1;
if( key_r ) pressed = 1;
if( key_f ) pressed = 1;
if( key_m ) pressed = 1;
if( key_y ) pressed = 1;
if( key_z ) pressed = 1;

return pressed;
}




		





float llem_wnd::get_thrust()
{
return thrust_wheel;
}





		
void llem_wnd::set_thrust( float thrust )
{
thrust_wheel = thrust;
if( thrust_wheel > 1.0f  ) thrust_wheel = 1.0;
if( thrust_wheel < 0.0f  ) thrust_wheel = 0.0;

audio_thrust_gain = 0.0 + 0.9*thrust_wheel;

if( thrust == 0.0f ) flame_on = 0;

}




float last_thrust_val = 0;


int llem_wnd::handle( int e )
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
	
if ( e == FL_MOVE )
	{
	mousex = Fl::event_x();
	mousey = Fl::event_y();
	

	if( mousey <= 25  )			//in menu area?
		{
		if(menu) menu->show();
		menu_hei = 25;
		need_redraw = 1;
		dont_pass_on = 0;
		}
	else{
		if( menu_hei != 0 )		//move out of menu?
			{
			if(menu) menu->hide();
			menu_hei = 0;
			scrn_regen = 1;
			need_redraw = 1;
			dont_pass_on = 0;
			}
		}




//	vobj[2].x = mousex - midx;
//	vobj[2].y = midy - mousey;
	
	need_redraw = 1;
	dont_pass_on = 0;
	}






if ( e == FL_PUSH )
	{
	last_thrust_val = get_thrust();
	
	if( Fl::event_button() == 1)
		{
		left_button = 1;
//		if( menu_hei == 0 ) set_thrust( 1 );
		}

	if( Fl::event_button() == 2)
		{
		middle_button = 1;
		
		if( ( menu_hei == 0 ) && ( game_state == en_gs_landing ) ) set_thrust( 1 );
		}

	if( Fl::event_button() == 3)
		{
		right_button = 1;
		if( ( menu_hei == 0 ) && ( game_state == en_gs_landing ) ) set_thrust( 1 );
		}
	need_redraw = 1;
	dont_pass_on = 0;
	}
	

if ( e == FL_RELEASE )
	{
	if( Fl::event_button() == 1 )
		{
 		left_button = 0;
		}

	if( Fl::event_button() == 2 )
		{
		middle_button = 0;
		if( ( menu_hei == 0 ) && ( game_state == en_gs_landing ) ) set_thrust( last_thrust_val );
		}

	if( Fl::event_button() == 3 )
		{
		right_button = 0;
		if( ( menu_hei == 0 ) && ( game_state == en_gs_landing ) ) set_thrust( last_thrust_val );
		}

	need_redraw = 1;
	dont_pass_on = 0;
	}




if ( e == FL_KEYDOWN )						//key press?
	{
	int key = Fl::event_key();

					
	if( develope_mode >= 1 ) 
		{
		if( key == ' ' ) goto skip_if_in_devl;	
		if( key == 't' ) goto skip_if_in_devl;	
		}

	if( key_inhibit ) goto skip_key;

	//take off ?
	if( game_state == en_gs_landed_ok_settled )
		{

		if( ( key != 'p') && ( key != FL_Left ) && (key != FL_Right) )
			{
			animate_bounce = 0;	

			zoom_extents_goes_off = 1;
			set_thrust( 0.4 );
			vobj[idx_ship].y += 2.0;
			vobj[idx_ship].dx = 0;
			vobj[idx_ship].dy = 15;
			game_state = en_gs_landing;

			taking_off_wait_for_a_draw_to_occur = 1;						//NOT ok for 'check_collision()' to execute
			scrn_regen = 1;
			}
		}







if( game_state == en_gs_game_ended_no_fuel )
	{
//	if( b_highscore_ask ) highscore_enter();
//	b_highscore_ask = 0;
	}

if( game_state == en_gs_game_ended_too_many_crashes )
	{
//	if( b_highscore_ask ) highscore_enter();
//	b_highscore_ask = 0;
	}

if( game_state == en_gs_game_ended_too_many_crashes )
	{
//	if( b_highscore_ask ) highscore_enter();
//	b_highscore_ask = 0;
	}

if( game_state == en_gs_game_ended_all_landed0 )						//completed all, and now just showing last landings stats ?
	{
//	if( b_highscore_ask ) highscore_enter();
//	b_highscore_ask = 0;
	}







	if( game_state == en_gs_landed_crash1 )
		{
		set_pause( 0 );
		ship_restore( 1, 1 );
		}

	if( game_state == en_gs_landed_crash0 )
		{
		game_state = en_gs_landed_crash1;
		}

	if( game_state == en_gs_game_ended_no_fuel )
		{
		if( b_highscore_ask ) highscore_enter();
		init( 0 );
		fuel = fuel_new_game;
		ship_restore( 1, 1 );
		}

	if( game_state == en_gs_game_ended_too_many_crashes )
		{
		if( b_highscore_ask ) highscore_enter();
		init( 0 );
		fuel = fuel_new_game;
		ship_restore( 1, 1 );
		}

	if( game_state == en_gs_game_ended_all_landed1 )				
		{
		if( b_highscore_ask ) highscore_enter();
		init( 0 );
		fuel = fuel_new_game;
		ship_restore( 1, 1 );
		}

	if( game_state == en_gs_game_ended_all_landed0 )					//completed all, and now just showing last landings stats ?
		{
		game_state = en_gs_game_ended_all_landed1;						//progress to actual end of game
		}


//	printf( "dble_wnd::handle() - key: 0x%02x\n", key );
		
	if( key == FL_Enter )		//is it CR ?
		{
		scrn_regen = 1;
		}


	if( ( key == FL_Control_L ) || ( key == FL_Control_R ) ) ctrl_key = 1;
	if( ( key == FL_Shift_L ) || (  key == FL_Shift_R ) ) shift_key = 1;

	if( key == 'a' ) key_a = 1;
	if( key == 'd' ) key_d = 1;
	if( key == 'w' ) key_w = 1;
	if( key == 'r' ) key_r = 1;
	if( key == 'f' ) key_f = 1;
	if( key == 'k' ) key_k = 1;
	if( key == 'm' ) key_m = 1;	
	if( key == 'x' ) key_x = 1;
	if( key == 'y' ) key_y = 1;
	if( key == 's' ) key_s = 1;
	if( key == 'z' ) key_z = 1;
	if( key == 'i' ) show_debug_info = !show_debug_info;



//	if( key == 't' ) spoke_show_filtered = !spoke_show_filtered;

	if( key == FL_Up ) key_up = 1;
	if( key == FL_Down ) key_down = 1;
	if( key == FL_Left ) key_left = 1;
	if( key == FL_Right ) key_right = 1;

		

skip_if_in_devl:
	do_key( key, 1 );

	do_key_obj_create( key );

skip_key:





	need_redraw = 1;
    dont_pass_on = 1;
	}


if ( e == FL_KEYUP )												//key release?
	{
	int key = Fl::event_key();
	
	if( ( key == FL_Control_L ) || ( key == FL_Control_R ) ) ctrl_key = 0;
	if( ( key == FL_Shift_L ) || (  key == FL_Shift_R ) ) shift_key = 0;

	if( key == 'a' ) key_a = 0;
	if( key == 'd' ) key_d = 0;
	if( key == 'w' ) key_w = 0;
	if( key == 'r' ) key_r = 0;
	if( key == 'f' ) key_f = 0;
	if( key == 'k' ) key_k = 0;
	if( key == 'm' ) key_m = 0;	
	if( key == 'x' ) key_x = 0;
	if( key == 'y' ) key_y = 0;
	if( key == 's' ) key_s = 0;
	if( key == 'z' ) key_z = 0;
	
//	if( key == ' ' ) key_space = 0;


	if( pause ) 
		{
		double dt = mhandy_timer.time_passed( mhandy_timer.ns_tim_start );
		mhandy_timer.time_start( mhandy_timer.ns_tim_start );

		printf( "paused - press a key, dt was: %f\n", dt );
//		getchar();
		}


	if( key == FL_Up ) key_up = 0;
	if( key == FL_Down ) key_down = 0;
	if( key == FL_Left ) key_left = 0;
	if( key == FL_Right ) key_right = 0;

	do_key( key, 0 );


	need_redraw = 1;
    dont_pass_on = 1;
	}



if ( e == FL_MOUSEWHEEL )
	{
	mousewheel = -Fl::event_dy();
	if( pref_invert_mousewheel ) mousewheel *= -1;

//	printf( "dble_wnd::handle() - mousewheel: %d\n", mousewheel );


	if( !pressed_key() )
		{
		if( mousewheel < 0 ) thrust_wheel += 0.025;
		else thrust_wheel -= 0.025;

		if( thrust_wheel < 0.001f ) thrust_wheel = 0.0f;
		if( thrust_wheel > 1.0f ) thrust_wheel = 1.0f;

		if( game_state == en_gs_landing ) set_thrust( thrust_wheel );

		if( thrust_wheel > 0.0f ) if( zoom_extents ) zoom_extents_goes_off = 1;
		
//		filt_freq0 = 20 + 120*thrust_wheel;
//		create_filter0();
		}

	if( develope_mode >= 2 )
		{
		if( key_w ) adj_line_width += mousewheel;
		if( adj_line_width < 0 ) adj_line_width = 0;
		if( adj_line_width > 10 ) adj_line_width = 10;

		if( key_x ) adj_offsx += mousewheel;
		if( key_y ) adj_offsy += mousewheel;

		if( key_r ) adj_angle += mousewheel/20.0f;
		if( key_s ) 
			{
			if( mousewheel > 0 ) adj_scale *= 0.9375;
			else adj_scale /= 0.9375;
			}
		
		if( key_k ) 
			{
			kern_size += mousewheel;
			if( kern_size < 1 ) kern_size = 1;
			if( kern_size > 20 ) kern_size = 20;
			rebuild_filter_kernel = 1;
			}

		if( key_f ) 
			{
			int ii = adj_filter;
			ii += mousewheel;
			
			if( ii < 1 ) ii = 1;
			if( ii > (int)fwt_lanczos3 ) ii = fwt_lanczos3;

			adj_filter = (en_filter_window_type_tag)ii;
			rebuild_filter_kernel = 1;
	//		if( spoke_filter == fwt_kaiser ) spoke_filter++;
			}
		}

	need_redraw = 1;
    dont_pass_on = 1;
	}





if ( need_redraw ) redraw();

if( dont_pass_on ) return 1;

return Fl_Double_Window::handle(e);

}
//----------------------------------------------------------







//make an arc of using lines
bool llem_wnd::make_arcseg( vector<st_line_tag> &vo, int r, int g, int b, int segments, float rad, float start_ang, float stop_ang )
{
float xx, yy;

if( segments < 1 ) return 0;


float theta_span = ( start_ang - stop_ang ) * cn_deg2rad;

float theta_inc = theta_span / segments;

float theta = start_ang * cn_deg2rad;


float fx2 = rad * cos( theta );				//initial point
float fy2 = rad * sin( theta );

st_line_tag o;
o.x1 = fx2;
o.y1 = fy2;
o.dx = 0;
o.dy = 0;
o.r = 0;
o.g = 0;
o.b = 0;


for( int i = 0; i < segments + 1; i++ )
	{
	float fx1 = rad * cos( theta );
	float fy1 = rad * sin( theta );

	o.x2 = fx1;
	o.y2 = fy1;
	vo.push_back( o );

	o.x1 = fx1;
	o.y1 = fy1;

	theta += theta_inc;
	}

return 1;
}








void llem_wnd::add_rect( int xx, int yy, int ww, int hh, float scale )
{
//return;

//build obj
st_obj_tag o;
st_line_tag oo;

o.type = en_ot_rect_test;
o.visible = 1;
o.col = 0x00000000;

oo.r = 0;
oo.g = 0;
oo.b = 0;

o.time_to_live = 5;

if( o.time_to_live != 0.0 ) 
	{
	o.finite_life = 1;
	}
else{
	o.finite_life = 0;
	}

o.col_temp = 0x008080ff;
o.col_temp_frames_total = 10;
o.col_temp_frame_cnt = 0;

o.scale = scale;
o.x = xx;
o.y = yy;
o.dx = 0;
o.dy = 0;
o.dv = sqrt( pow( o.dx, 2 ) + pow( o.dy, 2 ) );
o.max_dx = 2 * 1;
o.max_dy = 2 * 1;

o.theta = 0;//pi2 / 6;
o.theta_detentzone = o.theta;
o.theta_req = pi;
o.delta_theta_per_frame = 0.1;
o.delta_spin = 0.05*rnd();


o.thrust_max = 1800;
o.thrust = 0.0;
o.thrust_inc_per_frame = 80 * 1;							//make it relative to tframe: 80 / 0.02, was: 80
o.drag = 0.001;															//make it relative to tframe: 10 / 0.02, was: 4
o.mass = 9000;
o.accel = 0;
o.accelx = 0;
o.accely = 0;
//o.finite_life = 1;
o.mark_to_delete = 0;
o.has_thrust = 1;
o.can_wrap = 1;
o.is_exploding = 0;
o.explode_frames_total = 2.0 / tframe;
o.explode_frame_cnt = o.explode_frames_total;
o.explode_shape = 0;
o.missle_steer_delay = 2.0 / tframe;
o.grav_affected = 1;
o.magmine_in_flight_frame_cnt = 0;

o.arming_delay_frame_cnt = 3.0 / tframe;


oo.flags = en_ft_update;

oo.x1 = 0;
oo.y1 = 0;
oo.x2 = ww;
oo.y2 = 0;

o.vline.push_back( oo );

oo.x1 = ww;
oo.y1 = 0;
oo.x2 = ww;
oo.y2 = hh;

o.vline.push_back( oo );

oo.x1 = ww;
oo.y1 = hh;
oo.x2 = 0;
oo.y2 = hh;

o.vline.push_back( oo );

oo.x1 = 0;
oo.y1 = hh;
oo.x2 = 0;
oo.y2 = 0;

o.vline.push_back( oo );

vobj.push_back( o );
set_bounding_rect( vobj.size() - 1, bounding_scale_down, bounding_scale_down, 1 );
}





void llem_wnd::add_user_ship( int shape, bool affected_by_grav, bool apply_dxdy, double scale, double xx, double yy, double dx, double dy, float time_to_live )
{
//return;

//build obj
st_obj_tag o;
st_line_tag oo;

oo.drawn_x1 = 0;														//put anything in here thats initally valid for plotting
oo.drawn_y1 = 0;
oo.drawn_x2 = 1;
oo.drawn_y2 = 1;

o.type = en_ot_user_ship;
o.visible = 1;
o.col = 0x00000000;

oo.r = 0;
oo.g = 0;
oo.b = 0;

o.time_to_live = time_to_live;

if( time_to_live != 0.0 ) 
	{
	o.finite_life = 1;
	}
else{
	o.finite_life = 0;
	}

o.col_temp = 0x008080ff;
o.col_temp_frames_total = 10;
o.col_temp_frame_cnt = 0;

o.scale = scale;
o.x = xx;
o.y = yy;
//o.offsx = 0;
//o.offsy = 0;
//o.last_posx = xx;
//o.last_posy = yy;
o.dx = dx;
o.dy = dy;
o.dv = sqrt( pow( o.dx, 2 ) + pow( o.dy, 2 ) );
o.max_dx = 2 * 1;
o.max_dy = 2 * 1;

o.theta = 0;//pi2 / 6;
o.theta_detentzone = o.theta;
o.theta_req = pi;
o.delta_theta_per_frame = 0.1;
o.delta_spin = 0.05*rnd();


o.thrust_max = 1800;
o.thrust = 0.0;
o.thrust_inc_per_frame = 80 * 1;							//make it relative to tframe: 80 / 0.02, was: 80
o.drag = 0.001;															//make it relative to tframe: 10 / 0.02, was: 4
o.mass = 9000;
o.accel = 0;
o.accelx = 0;
o.accely = 0;
//o.finite_life = 1;
o.mark_to_delete = 0;
o.has_thrust = 1;
o.can_wrap = 1;
o.is_exploding = 0;
o.explode_frames_total = 2.0 / tframe;
o.explode_frame_cnt = o.explode_frames_total;
o.explode_shape = 0;
o.missle_steer_delay = 2.0 / tframe;
o.grav_affected = 1;
o.magmine_in_flight_frame_cnt = 0;

o.arming_delay_frame_cnt = 3.0 / tframe;


int segments = 10;
float rad = 90;
float start_ang = 0;
float stop_ang = 360;

make_arcseg( o.vline, 0, 0, 0, segments, rad, start_ang, stop_ang );

oo.flags = en_ft_update;

oo.drawn_x1 = 0;
oo.drawn_y1 = 0;
oo.drawn_x2 = 1;
oo.drawn_y2 = 1;

oo.x1 = -65;
oo.y1 = -45;
oo.x2 = -130;
oo.y2 = -90;

oo.dx = 0;
oo.dy = 0;
oo.theta = 0;
oo.theta_inc = 0;

o.vline.push_back( oo );


oo.x1 = -90;
oo.y1 = -100;
oo.x2 = -110;
oo.y2 = -150;

o.vline.push_back( oo );


oo.x1 = -110;
oo.y1 = -150;
oo.x2 = -110;
oo.y2 = -180;

o.vline.push_back( oo );


oo.x1 = -140;
oo.y1 = -180;
oo.x2 = -80;
oo.y2 = -180;

o.vline.push_back( oo );

/*
oo.x1 = 20;
oo.y1 = -20;
oo.x2 = -20;
oo.y2 = -20;

//o.vline.push_back( oo );


oo.x1 = -20;
oo.y1 = -20;
oo.x2 = -40;
oo.y2 = -55;

//o.vline.push_back( oo );


oo.x1 = 20;
oo.y1 = -20;
oo.x2 = 40;
oo.y2 = -55;

//o.vline.push_back( oo );


oo.x1 = 2;
oo.y1 = -2;
oo.x2 = 2;
oo.y2 = -2;

//o.vline.push_back( oo );


oo.x1 = -2;
oo.y1 = -2;
oo.x2 = -2;
oo.y2 = -2;

//o.vline.push_back( oo );
*/


//---- flame0 ----
st_obj_tag os;

oo.line_type = en_lt_ship;

oo.x1 = -5.5;
oo.y1 = -12;
oo.x2 = 0.25;
oo.y2 = -18;

os.vline.push_back( oo );



oo.x1 = 6.5;
oo.y1 = -12;
oo.x2 = 0.25;
oo.y2 = -18;

os.vline.push_back( oo );

o.vsub.push_back( os );

o.sub_obj_idx = -1;
//----




//---- flame1 ----
os.vline.clear();

oo.x1 = -5.5;
oo.y1 = -12;
oo.x2 = 0.25;
oo.y2 = -18;

os.vline.push_back( oo );



oo.x1 = 6.5;
oo.y1 = -12;
oo.x2 = 0.25;
oo.y2 = -18;

os.vline.push_back( oo );

o.vsub.push_back( os );

o.sub_obj_idx = -1;
//----


idx_ship = vobj.size();

vobj.push_back( o );
set_bounding_rect( idx_ship, bounding_scale_down, bounding_scale_down, 1 );

vobj[ idx_ship ].drawn_bound_x1 = 0;
vobj[ idx_ship ].drawn_bound_y1 = 0;
vobj[ idx_ship ].drawn_bound_x2 = 0;
vobj[ idx_ship ].drawn_bound_y2 = 0;
}








void llem_wnd::add_ground( int shape, bool affected_by_grav, bool apply_dxdy, double scale, double xx, double yy, double dx, double dy, float time_to_live )
{
//return;

//build obj
st_obj_tag o;
st_line_tag oo;

oo.drawn_x1 = 0;														//put anything in here thats initally valid for plotting
oo.drawn_y1 = 0;
oo.drawn_x2 = 1;
oo.drawn_y2 = 1;

o.type = en_ot_ground;
o.visible = 1;
o.col = 0x00000000;

oo.r = 0;
oo.g = 0;
oo.b = 0;

o.time_to_live = time_to_live;

if( time_to_live != 0.0 ) 
	{
	o.finite_life = 1;
	}
else{
	o.finite_life = 0;
	}

o.col_temp = 0x008080ff;
o.col_temp_frames_total = 10;
o.col_temp_frame_cnt = 0;

o.scale = scale;
o.x = xx;
o.y = yy;
//o.offsx = 0;
//o.offsy = 0;
//o.last_posx = xx;
//o.last_posy = yy;

o.dx = dx;
o.dy = dy;
o.dv = sqrt( pow( o.dx, 2 ) + pow( o.dy, 2 ) );
o.max_dx = 2 * 1;
o.max_dy = 2 * 1;

o.theta = 0;
o.theta_detentzone = o.theta;
o.theta_req = pi;
o.delta_theta_per_frame = 0.1;
o.delta_spin = 0.05*rnd();


o.thrust_max = 1800;
o.thrust = 0.0;
o.thrust_inc_per_frame = 80 * 1;							//make it relative to tframe: 80 / 0.02, was: 80
o.drag = 0.001;															//make it relative to tframe: 10 / 0.02, was: 4
o.mass = 9000;
o.accel = 0;
o.accelx = 0;
o.accely = 0;
//o.finite_life = 1;
o.mark_to_delete = 0;
o.has_thrust = 1;
o.can_wrap = 1;
o.is_exploding = 0;
o.explode_frames_total = 2.0 / tframe;
o.explode_frame_cnt = o.explode_frames_total;
o.explode_shape = 0;
o.missle_steer_delay = 2.0 / tframe;
o.grav_affected = 1;
o.magmine_in_flight_frame_cnt = 0;

o.arming_delay_frame_cnt = 3.0 / tframe;

oo.flags = en_ft_update;

oo.x1 = -50;
oo.y1 = 0;
oo.x2 = 50;
oo.y2 = 0;

oo.dx = 0;
oo.dy = 0;
oo.theta = 0;
oo.theta_inc = 0;

o.vline.push_back( oo );


oo.x1 = -20;
oo.y1 = 20;
oo.x2 = 20;
oo.y2 = 20;

//o.vline.push_back( oo );

oo.x1 = 20;
oo.y1 = 20;
oo.x2 = 20;
oo.y2 = -20;

//o.vline.push_back( oo );

oo.x1 = 20;
oo.y1 = -20;
oo.x2 = -20;
oo.y2 = -20;

//o.vline.push_back( oo );


oo.x1 = -20;
oo.y1 = -20;
oo.x2 = -40;
oo.y2 = -55;

//o.vline.push_back( oo );


oo.x1 = 20;
oo.y1 = -20;
oo.x2 = 40;
oo.y2 = -55;

//o.vline.push_back( oo );


oo.x1 = 2;
oo.y1 = -2;
oo.x2 = 2;
oo.y2 = -2;

//o.vline.push_back( oo );


oo.x1 = -2;
oo.y1 = -2;
oo.x2 = -2;
oo.y2 = -2;

//o.vline.push_back( oo );

o.sub_obj_idx = -1;

idx_ground = vobj.size();

vobj.push_back( o );
set_bounding_rect( idx_ground, bounding_scale_down, bounding_scale_down, 1 );


vobj[ idx_ground ].drawn_bound_x1 = 0;
vobj[ idx_ground ].drawn_bound_y1 = 0;
vobj[ idx_ground ].drawn_bound_x2 = 0;
vobj[ idx_ground ].drawn_bound_y2 = 0;
}







void llem_wnd::add_crosshair( int shape, bool affected_by_grav, bool apply_dxdy, double scale, double xx, double yy, double dx, double dy, float time_to_live )
{
//return;

//build obj
st_obj_tag o;
st_line_tag oo;

oo.drawn_x1 = 0;														//put anything in here thats initally valid for plotting
oo.drawn_y1 = 0;
oo.drawn_x2 = 1;
oo.drawn_y2 = 1;

oo.line_type = en_lt_ship;

o.type = en_ot_crosshair;
o.visible = 0;
o.col = 0x00000000;

oo.r = 180;
oo.g = 180;
oo.b = 180;

o.time_to_live = time_to_live;

if( time_to_live != 0.0 ) 
	{
	o.finite_life = 1;
	}
else{
	o.finite_life = 0;
	}

o.col_temp = 0x008080ff;
o.col_temp_frames_total = 10;
o.col_temp_frame_cnt = 0;

o.scale = scale;
o.x = xx;
o.y = yy;
//o.offsx = 0;
//o.offsy = 0;

//o.last_posx = xx;
//o.last_posy = yy;

o.dx = dx;
o.dy = dy;
o.dv = sqrt( pow( o.dx, 2 ) + pow( o.dy, 2 ) );
o.max_dx = 2 * 1;
o.max_dy = 2 * 1;

o.theta = 0;
o.theta_detentzone = o.theta;
o.theta_req = pi;
o.delta_theta_per_frame = 0.1;
o.delta_spin = 0.05*rnd();


o.thrust_max = 1800;
o.thrust = 0.0;
o.thrust_inc_per_frame = 80 * 1;							//make it relative to tframe: 80 / 0.02, was: 80
o.drag = 0.001;															//make it relative to tframe: 10 / 0.02, was: 4
o.mass = 9000;
o.accel = 0;
o.accelx = 0;
o.accely = 0;
//o.finite_life = 1;
o.mark_to_delete = 0;
o.has_thrust = 1;
o.can_wrap = 1;
o.is_exploding = 0;
o.explode_frames_total = 2.0 / tframe;
o.explode_frame_cnt = o.explode_frames_total;
o.explode_shape = 0;
o.missle_steer_delay = 2.0 / tframe;
o.grav_affected = 1;
o.magmine_in_flight_frame_cnt = 0;

o.arming_delay_frame_cnt = 3.0 / tframe;

oo.flags = en_ft_update;

oo.x1 = -100;
oo.y1 = 0;
oo.x2 = 100;
oo.y2 = 0;

o.vline.push_back( oo );


oo.x1 = 0;
oo.y1 = -100;
oo.x2 = 0;
oo.y2 = 100;


o.vline.push_back( oo );



o.sub_obj_idx = -1;

idx_crosshair = vobj.size();

vobj.push_back( o );
set_bounding_rect( idx_ground, bounding_scale_down, bounding_scale_down, 1 );

vobj[ idx_crosshair ].drawn_bound_x1 = 0;
vobj[ idx_crosshair ].drawn_bound_y1 = 0;
vobj[ idx_crosshair ].drawn_bound_x2 = 0;
vobj[ idx_crosshair ].drawn_bound_y2 = 0;
}










//forms a bounding rect around obj extremes, applies a scale(fudge) to bounding rect to cater for poss of obj being rotate
void llem_wnd::set_bounding_rect( int idx, double scalex, double scaley, bool include_sub_objs )
{

if( idx >= vobj.size() ) return;

float minx = 0;
float miny = 0;
float maxx = 0;
float maxy = 0;


vector<st_line_tag> vo;
vo = vobj[ idx ].vline;

for( int j = 0; j < vo.size(); j++ )
	{
	float x1 = vo[ j ].x1;
	float y1 = vo[ j ].y1;
	float x2 = vo[ j ].x2;
	float y2 = vo[ j ].y2;
	
	if( x1 < minx ) minx = x1;
	if( x1 > maxx ) maxx = x1;
	
	if( y1 < miny ) miny = y1;
	if( y1 > maxy ) maxy = y1;

	if( x2 < minx ) minx = x2;
	if( x2 > maxx ) maxx = x2;
	
	if( y2 < miny ) miny = y2;
	if( y2 > maxy ) maxy = y2;
	}

vobj[ idx ].bound_x1 = minx * scalex;
vobj[ idx ].bound_y2 = miny * scaley;

vobj[ idx ].bound_x2 = maxx * scalex;
vobj[ idx ].bound_y1 = maxy * scaley;
}










//generates a rnd num between -1.0 -> 1.0
double llem_wnd::rnd()
{
double drnd =  (double)( RAND_MAX / 2 - rand() ) / (double)( RAND_MAX / 2 );

return drnd;
}










void llem_wnd::set_obj_line_col( vector<st_line_tag> &vo, int r, int g, int b )
{
for( int i = 0; i < vo.size(); i++ )
	{
	vo[i].r = r;
	vo[i].g = g;
	vo[i].b = b;
	}
}







void llem_wnd::set_obj_line_col_single( vector<st_line_tag> &vo, int idx_line, int r, int g, int b )
{

if( idx_line < 0 ) return;
if( idx_line >= vo.size() ) return;
vo[idx_line].r = r;
vo[idx_line].g = g;
vo[idx_line].b = b;

}








void llem_wnd::explode_ship_init_line_params( int line_idx, float dx, float dy, float theta_inc )
{
if( line_idx >= vobj[ idx_ship ].vline.size() ) return;

vobj[ idx_ship ].vline[ line_idx ].dx = dx;
vobj[ idx_ship ].vline[ line_idx ].dy = dy;

vobj[ idx_ship ].vline[ line_idx ].theta = 0;
vobj[ idx_ship ].vline[ line_idx ].theta_inc = theta_inc;

}







//prep for ship explosion
void llem_wnd::explode_ship_init_params()
{
unsigned int idx = idx_ship;


float dispersey = 30;
float dispersex = 30;

vobj[ idx ].theta = 0.0f;												//remove ship's rotation for animation debris to move correctly, else debris will positions will be rotated
vobj[ idx ].theta_detentzone = 0.0f;	

for( int j = 0; j < vobj[ idx ].vline.size(); j++ )
//for( int j = 0; j < 1; j++ )
	{
	float ofx = vobj[ idx ].vline[j].middlex/2;							//line's position relative to center of ship
	float ofy = vobj[ idx ].vline[j].middley/2;

	float dx = ofx + dispersex*rnd();									//set displacement dir relative to center of ship
	float dy = ofy + dispersey*rnd();

	float mag = sqrt( dx*dx + dy*dy );									//total explosion fragment displacement 

	dx += vobj[ idx ].dx/1.0f;											//add ship's vel
	dy += vobj[ idx ].dy/1.0f;

	float angular_vel = (mag/100.0f)*(rnd()/2);

	explode_ship_init_line_params( j, dx, dy, angular_vel );
//explode_ship_init_line_params( 1, 10+dispersex*rnd(), -11+dispersey*rnd(), 0.25*rnd() );
//explode_ship_init_line_params( 2, -10+dispersex*rnd(), -31+dispersey*rnd(), 0.25*rnd() );
//explode_ship_init_line_params( 3, -10+dispersex*rnd(), -31+dispersey*rnd(), 0.25*rnd() );
	}

//for( int j = 0; j < vobj[ idx ].vline.size(); j++ )
	{
//	if( i == 0 ) vobj[ idx ].vline[ j ].dx = 10;
	}
}





//animate explosion
void llem_wnd::explode_ship_animate( float local_tframe )
{

unsigned int idx = idx_ship;

if( !vobj[ idx ].is_exploding ) return;

//return;

float timesqr = local_tframe * local_tframe;


for( int j = 0; j < vobj[ idx ].vline.size(); j++ )
	{
	float x0 = vobj[ idx ].vline[ j ].x1 - vobj[ idx ].vline[ j ].middlex;	//center line to 0,0 for rotation
	float y0 = vobj[ idx ].vline[ j ].y1 - vobj[ idx ].vline[ j ].middley;
	float x1 = vobj[ idx ].vline[ j ].x2 - vobj[ idx ].vline[ j ].middlex;
	float y1 = vobj[ idx ].vline[ j ].y2 - vobj[ idx ].vline[ j ].middley;
	
	float dx = vobj[ idx ].vline[ j ].dx;
	float dy = vobj[ idx ].vline[ j ].dy;
	
	float theta = vobj[ idx ].vline[ j ].theta;
	float theta_inc = vobj[ idx ].vline[ j ].theta_inc;


	theta += theta_inc;
	
	vobj[ idx ].vline[ j ].theta = theta;

//----rotate----
	float s0 = sinf( theta_inc );
	float c0 = cosf( theta_inc );
	
	vobj[ idx ].vline[ j ].x1 = x0 * c0  +  y0 * s0;
	vobj[ idx ].vline[ j ].x2 = x1 * c0  +  y1 * s0;
	vobj[ idx ].vline[ j ].y1 = y0 * c0  -  x0 * s0;
	vobj[ idx ].vline[ j ].y2 = y1 * c0  -  x1 * s0;


//	vobj[ idx ].vline[ j ].x1 = x0;
//	vobj[ idx ].vline[ j ].x2 = x1;
//	vobj[ idx ].vline[ j ].y1 = y0;
//	vobj[ idx ].vline[ j ].y2 = y1;
	
	vobj[ idx ].vline[ j ].x1 += vobj[ idx ].vline[ j ].middlex;
	vobj[ idx ].vline[ j ].y1 += vobj[ idx ].vline[ j ].middley;
	vobj[ idx ].vline[ j ].x2 += vobj[ idx ].vline[ j ].middlex;
	vobj[ idx ].vline[ j ].y2 += vobj[ idx ].vline[ j ].middley;
//------------


	dy = dy + accel_grav*local_tframe;											//change of velocity: v = u + at,     change of position: s = ut + 1/2.a.t^2


	float displace_y = (dy * local_tframe) + (0.5 * accel_grav * timesqr);		//s = ut + (0.5 * a*t^2)	displacement due to accel
																				//u is initial velocity

	vobj[ idx ].vline[ j ].dy = dy;
																		
	vobj[ idx ].vline[ j ].x1 += dx * local_tframe;
	vobj[ idx ].vline[ j ].y1 += displace_y;
	vobj[ idx ].vline[ j ].x2 += dx * local_tframe;
	vobj[ idx ].vline[ j ].y2 += displace_y;
//if( j == 0 ) printf( "explode_ship() - time_sim %f, posy %f\n", time_sim, vobj[ idx_ship ].vline[ j ].y1 );
	}



calc_objs_line_midpoints( idx_ship );
}












void llem_wnd::zoom_detent_to_zoom( int zoom_detent_in, float &zoom_out ) 
{
zoom_out = 1;

if( zoom_detent_in == 0 )
	{
	zoom_out = 1;
	}

if( zoom_detent_in == 1 )
	{
	zoom_out = 2;
	}

if( zoom_detent_in == 2 )
	{
	zoom_out = 8;
	}

if( zoom_detent_in == 3 )
	{
	zoom_out = 12;
	}

if( zoom_detent_in == 4 )
	{
	zoom_out = 24;
	}
	
if( zoom_detent_in == 5 )
	{
	zoom_out = 32;
	}
	
if( zoom_detent_in == 6 )
	{
	zoom_out = 48;
	}
	
if( zoom_detent_in == 7 )
	{
	zoom_out = 64;
	}
	
if( zoom_detent_in == 8 )
	{
	zoom_out = 80;
	}
}








void llem_wnd::zoom_set_detent( int zoom_detent_in )
{

float zz;

zoom_detent_to_zoom( zoom_detent_in, zz );
zoomx = zoomy = zz;

scrn_regen = 1;

}





//this is calc'd using the unrotated 'vobj[idx].bound_x1' rect type vals
bool llem_wnd::check_obj_at_screen_edge( unsigned int idx, float obj_bounds_enlarge_factor, bool &left, bool &right, bool &top, bool &bott )
{
if( idx >= vobj.size() ) return 0;

left = 0;
right = 0;
top = 0;
bott = 0;

bool ret = 0;


int mid_x = w()/2;
int mid_y = h()/2;

//conv coords to screen based vals
float posx = vobj[idx].x;
float posy = vobj[idx].y;

float supersample_factor = 4.0f;			//antialias supersample factor, line's are drawn at 4x size and downsampled by 4 via convolution filter, see 'filter_lines()'

posx = posx*zoomx - panx*zoomx;
posy = posy*zoomy - pany*zoomy;

posx /= supersample_factor;
posy /= supersample_factor;



//posx *= zoomx;
//posy *= zoomy;

//posx = 0;
//posy = 0;


posx += mid_x;
posy = -( posy - mid_y );



//float x1 = posx + -10;//posx + (vobj[idx].bound_x1 * zoomx * obj_bounds_enlarge_factor) / supersample_factor;
//float y1 = posy + -10;//posy - (vobj[idx].bound_y1 * zoomy * obj_bounds_enlarge_factor) / supersample_factor;

//float x2 = posx + 10;//posx + (vobj[idx].bound_x2 * zoomx * obj_bounds_enlarge_factor) / supersample_factor;
//float y2 = posy + 10;//posy - (vobj[idx].bound_y2 * zoomy * obj_bounds_enlarge_factor) / supersample_factor;


float x1 = posx + (vobj[idx].bound_x1 * obj_bounds_enlarge_factor * zoomx) / supersample_factor;
float y1 = posy - (vobj[idx].bound_y1 * obj_bounds_enlarge_factor * zoomy) / supersample_factor;

float x2 = posx + (vobj[idx].bound_x2 * obj_bounds_enlarge_factor * zoomx) / supersample_factor;
float y2 = posy - (vobj[idx].bound_y2 * obj_bounds_enlarge_factor * zoomy) / supersample_factor;

dbg_x1 = x1;
dbg_y1 = y1;
dbg_x2 = x2;
dbg_y2 = y2;


//printf( "check_obj_at_screen_edge() w() %d %d\n", w(), h() );
//printf( "check_obj_at_screen_edge()0 posx %f %f\n", posx, posy );
//printf( "check_obj_at_screen_edge()1 x1 %f %f %f %f\n", vobj[idx].bound_x1, vobj[idx].bound_y1, vobj[idx].bound_x2, vobj[idx].bound_y2 );
//printf( "check_obj_at_screen_edge()2 x1 %f %f %f %f\n", x1, y1, x2, y2 );

if( x1 <= 0 ) {left = 1; ret = 1;}
if( x2 >= w() ) {right = 1; ret = 1;}

if( y1 <= 0 ) {top = 1; ret = 1;}
if( y2 >= h() ) {bott = 1; ret = 1;}

return ret;
}








vector<float>vf0, vf1, vf2, vf3, vf4;



float timer_cycle0 = 0.05;
float timer0 = 0;


float move_timer = 0;

void llem_wnd::move_obj( float local_tframe )
{
string s1, st;
mystr m1;

time_sim += local_tframe;

timer0 -= local_tframe;

if( timer0 <=0 )
	{
	timer0 = timer_cycle0;
	}



//--------------------------------------------------------------
if( ( !pause ) && ( game_state == en_gs_landing ) )
	{
	if( ( key_up ) )
		{
	//	set_thrust( thrust_wheel - 0.025f );
		
	//	flame_on = 0;
	//	flame_flicker = 0;
	//	vobj[idx_ship].sub_obj_idx = -1;

	//	vobj[idx_ship].sub_obj_idx = 0;//!vobj[idx_ship].sub_obj_idx;								//flame flutter

	//	set_obj_line_col( vobj[idx_ship].vsub[ 0 ].vline, backgrnd_r, backgrnd_g, backgrnd_b );
	//	set_obj_line_col( vobj[idx_ship].vsub[ 1 ].vline, backgrnd_r, backgrnd_g, backgrnd_b );
		}
	else{
	//	set_thrust( thrust_wheel - 0.1 );
	//	if( !obj_create_active ) thrust_wheel = 1.0;
	//	else thrust_wheel = 0.0;
	//	flame_on = 1;
	//	flame_flicker = !flame_flicker;

	//	vobj[idx_ship].sub_obj_idx = !vobj[idx_ship].sub_obj_idx;			//toggle flame shape
	//	set_obj_line_col( vobj[idx_ship].vsub[ 0 ].vline, 0, 0, 255 );
	//	set_obj_line_col( vobj[idx_ship].vsub[ 1 ].vline, 0, 0, 255 );
		}
	//local_tframe = 0.05;



	if( vobj[idx_ship].is_exploding ) 
		{
		set_thrust( 0 );
		}

	if( fuel <= 0.0f ) set_thrust( 0 );

	if( ( thrust_wheel <= 0.001f ) || ( fuel <= 0.0f ) )
		{
		flame_on = 0;
		flame_flicker = 0;
		}
	else{
		//consume fuel
		if( !toggle_space ) fuel -= 15.0f * thrust_wheel  * local_tframe;
		if( fuel < 0.0f ) fuel = 0.0f;
		flame_on = 1;
		if( timer0 == timer_cycle0 ) flame_flicker = !flame_flicker;	//this select which sub obj to draw
		}

	if( vobj[idx_ship].vsub.size() >= 2 )
		{
		vobj[idx_ship].vsub[ 0 ].vline[0].x2 = 0.25 + 1.9*thrust_wheel*rnd();	//add some x flutter
		vobj[idx_ship].vsub[ 1 ].vline[0].x2 = 0.25 + 1.9*thrust_wheel*rnd();
		
		vobj[idx_ship].vsub[ 0 ].vline[1].x2 = vobj[idx_ship].vsub[ 0 ].vline[0].x2;
		vobj[idx_ship].vsub[ 1 ].vline[1].x2 = vobj[idx_ship].vsub[ 1 ].vline[0].x2;

		
		vobj[idx_ship].vsub[ 0 ].vline[0].y2 = -20 + 55 * -thrust_wheel;	//adjust flame size
		vobj[idx_ship].vsub[ 0 ].vline[1].y2 = -20 + 55 * -thrust_wheel;

		vobj[idx_ship].vsub[ 1 ].vline[0].y2 = -20 + 65 * -thrust_wheel;	//adjust flame size
		vobj[idx_ship].vsub[ 1 ].vline[1].y2 = -20 + 65 * -thrust_wheel;
		}

	move_timer += local_tframe;




	int i = 0;


	//----- rotation control
	if( ( key_left ) || ( key_a ) )
		{
		vobj[ i ].theta += 1.5f * local_tframe;	
		}
		
	if( ( key_right ) || ( key_s ) || ( key_d ) )
		{
		vobj[ i ].theta -= 1.5f * local_tframe;	
		}

	if( vobj[ i ].theta > twopi ) vobj[ i ].theta -= twopi;
	if( vobj[ i ].theta < -twopi ) vobj[ i ].theta += twopi;

	vobj[ i ].theta_detentzone = vobj[ i ].theta;


	if( ( vobj[ i ].theta_detentzone > -0.05 ) && ( vobj[ i ].theta_detentzone < 0.05 ) ) vobj[ i ].theta_detentzone = 0.0f;		//place a dead zone

	//if( ( key_left ) || ( key_right ) ) 
	//	{
	//	printf( "key_left/right: theta %f detentzone %f\n", vobj[ i ].theta, vobj[ i ].theta_detentzone );
	//	}

	//-----




	//----
	float accel_thrust = 0;



	vobj[ i ].thrust = 0.0;
	vobj[ i ].mass = 500;

	//if( key_up )
	if( thrust_wheel > 0.001 )
		{
		vobj[ i ].thrust = 10000 * thrust_wheel;
		accel_thrust = vobj[ i ].thrust / vobj[ i ].mass * 1.0;             //a = f/m       i.e: accel = force / mass
		}





	vobj[ i ].accely = accel_grav;											//gravity

	vobj[ i ].accelx = -accel_thrust * sin( vobj[ i ].theta_detentzone );	//thrust
	vobj[ i ].accely += accel_thrust * cos( vobj[ i ].theta_detentzone );


	float timesqr = local_tframe * local_tframe;

	vobj[ i ].dx = vobj[ i ].dx + vobj[ i ].accelx * local_tframe;								//v = u + at    i.e: vel = init_vel + accel * dt
	if( vobj[ i ].dx < -400 ) vobj[ i ].dx = -400; 
	if( vobj[ i ].dx > 400 ) vobj[ i ].dx = 400; 
	float displace_x =  (vobj[ i ].dx * local_tframe) + (0.5 * vobj[ i ].accelx * timesqr);		//s = ut + (0.5 * a*t^2)	displacement due to accel


	vobj[ i ].dy = vobj[ i ].dy + vobj[ i ].accely * local_tframe;								//v = u + at    i.e: vel = init_vel + accel * dt
	if( vobj[ i ].dy < -400 ) vobj[ i ].dy = -400; 
	if( vobj[ i ].dy > 400 ) vobj[ i ].dy = 400; 
	float displace_y =  (vobj[ i ].dy * local_tframe) + (0.5 * vobj[ i ].accely * timesqr);		//s = ut + (0.5 * a*t^2)	displacement due to accel

	vobj[ i ].dv = sqrt( pow( vobj[ i ].dx, 2 ) + pow( vobj[ i ].dy, 2 ) );


	if( (toggle_space) || ( vobj[ i ].is_exploding ) )
		{
		vobj[ i ].dx = 0;													//stop moving
		vobj[ i ].dy = 0;
		displace_x = 0;
		displace_y = 0;
		}

	//vobj[ i ].last_posx = vobj[ i ].x;
	//vobj[ i ].last_posy = vobj[ i ].y;

	if( !obj_create_active ) 
		{
		vobj[ i ].x += displace_x;											//move obj
		vobj[ i ].y += displace_y;
		}
	else{
		vobj[ i ].theta = 0;												//if in create mode force no rotation
		}


	//bf0szy


	//if( vobj[ i ].x < -3000 ) vobj[ i ].x = -3000;
	//if( vobj[ i ].x > bf0szx - 200 ) vobj[ i ].x = bf0szx - 200;

	if( 0 )
		{
		if( vobj[ i ].y < -1200 ) vobj[ i ].y = -1200;
		if( vobj[ i ].y > 1200 ) vobj[ i ].y = 1200;
		}

	//printf("vobj[ i ].x  %f %f\n", vobj[ i ].x, vobj[ i ].y);

	//------
	if( 0 )
		{
		vf0.push_back( local_tframe );
		vf1.push_back( move_timer );
		vf2.push_back(  vobj[ i ].accely * local_tframe );
		vf3.push_back(  vobj[ i ].dy );
		vf4.push_back(  vobj[ i ].y );

		if( vobj[ i ].y < -300 )
			{
			for( int i = 0; i < vf0.size(); i++ )
				{
				strpf( s1, "t %f  tt %f ac %f dy %f y %f\n", vf0[i], vf1[i], vf2[i], vf3[i], vf4[i] );
				st += s1;
				}
			m1 = st;
			
			m1.writefile( "zzdump10.txt" );
			getchar();
			}
		}
	//------

}

//--------------------------------------------------------------





if( obj_create_active ) zoom_auto = 0;
else zoom_auto = 1;



//wrap around - horiz
if( vobj[idx_ship].x > 3300 ) vobj[idx_ship].x = -3200;
if( vobj[idx_ship].x < -3250 ) vobj[idx_ship].x = 3200;



//---- zoom/panning ----

float req_zoomx = zoom_last;
float req_zoomy = zoom_last;

zoom_detent = zoom_detent_last;									//this carries the zoom factor as a simple integer, e.g. 1,2,3,4, for zooms of 1x, 2x, 8x, 12x

bool nearest_a_landing_zone = 0;


bool pan_center = 0;

if( zoom_auto )
	{
	vector<float>vdist_considered_near;
	
	vdist_considered_near.push_back( 50 );
	vdist_considered_near.push_back( 250 );
	vdist_considered_near.push_back( 500 );
	vdist_considered_near.push_back( 800 );
	
	int idx_line_closest;
	float nearest_dist = check_near_ground( vdist_considered_near, idx_line_closest );

//printf( "nearest_dist: %f\n", nearest_dist );

	if( nearest_dist >= 0 )
		{
		if( ( vobj[ idx_ground ].vline[ idx_line_closest ].line_type == en_lt_ground_landing_zone_with_fuel ) || ( vobj[ idx_ground ].vline[ idx_line_closest ].line_type == en_lt_ground_landing_zone_no_fuel ) ) 
			{
			nearest_a_landing_zone = 1;
//			printf( "nearest_dist: %f and its a 'en_lt_ground_landing_zone' line\n", nearest_dist );
			}
		bool moving_fast = 0; 
//		bool moving_fast_up = 0; 
		bool moving_slow = 0; 
//		if( (vobj[idx_ship].dy < -70 ) || ( vobj[idx_ship].dy > 50 ) ) moving_fast = 1;

		float vel = get_obj_vel( idx_ship );

//vel = 10;
//nearest_dist = 250;

		if( fabs( vel > 45 ) ) moving_fast = 1;

		
		if( fabs(vel) < 20 )
			{
			moving_slow = 1;	
			}

		if( moving_fast ) 
			{
			if( nearest_dist < 800 ) 
				{
				nearest_dist = 500;										//force a particular zoom
				if( zoom_detent != 1 )
					{
					zoom_immediately = 1;
					}
				}
			}


		if( nearest_dist == 800 )
			{
			zoom_detent = 0;
			}

		if( nearest_dist == 500 )
			{
			zoom_detent = 1;
			}


		if( ( nearest_dist == 250 ) )
			{
			zoom_detent = 2;
			}

		if( ( nearest_dist == 50 ) && ( nearest_a_landing_zone ) && ( moving_slow ) )
			{
			zoom_detent = 3;
			if( zoom_detent != zoom_detent_last ) pan_center = 1;
			}
		}
	}


//center on ship
if( !obj_create_active )
	{
	if( zoom_extents_goes_off )
		{
		zoom_extents = 0;
		zoom_extents_goes_off = 0;
		zoom_extents_turn_on_time_cnt = -0.1f;
	//	zoom_detent = 0;
	//	zoom_detent_last = -1;
		zoom_immediately = 1;
		panx = vobj[idx_ship].x;
		pany = vobj[idx_ship].y;
		scrn_regen = 1;
		}
	}

//printf( "zoom_detent: %d\n", zoom_detent );

//zoom override
if( zoom_extents )
	{
//	zoom_extents = 0;
	if( zoom_detent_last != 0 ) zoom_immediately = 1;
//	zoom_detent_last = -1;
	zoom_detent = 0;
	zoom_detent_last = 0;
//	scrn_regen = 1;
	}

//convert zoom detent to an actual zoom factor
zoom_detent_to_zoom( zoom_detent, req_zoomx );
req_zoomy = req_zoomx;


if(!obj_create_active )
	{
	if( zoom_detent != zoom_detent_last )		//zoom changed req?
		{
		if( zoom_detent == 0 ) zoom_holdoff_time_cnt = 5;
		if( zoom_detent == 1 ) zoom_holdoff_time_cnt = 5;
		if( zoom_detent == 2 ) zoom_holdoff_time_cnt = 5;
		if( zoom_detent == 3 ) zoom_immediately = 1;						//force zoom change immediately

		if( fabs( zoom_detent - zoom_detent_last ) > 2 ) zoom_immediately = 1;	//big change in zoom req, do immediately	

		zoom_last = req_zoomx;


	//printf("move_obj() - zoom_detent %d  zoomx %f req_zoomx %f\n", zoom_detent, req_zoomx, zoomx );	
		}
	}

if ( zoom_immediately != 0 ) 
	{
	zoom_holdoff_time_cnt = 0.2f;
	zoom_immediately = 0;
	}


//printf("move_obj() - zoom_detent %d  zoom_detent_last %d, zoom_holdoff_time_cnt %f\n", zoom_detent, zoom_detent_last, zoom_holdoff_time_cnt );	

zoom_detent_last = zoom_detent;

zoom_holdoff_time_cnt -= local_tframe;

if( ( zoom_holdoff_time_cnt >= 0.0 ) && ( zoom_holdoff_time_cnt <= 0.25f ) )	//timeout for a zoom change to happen, helps stop frequent zoom changes
	{
	zoom_holdoff_time_cnt = -0.001;

	if( !obj_create_active )
		{
		zoomx = req_zoomx; 
		zoomy = req_zoomy;
		}

	scrn_regen = 1;
	}



if( !obj_create_active )
	{
	if( pan_center )
		{
		panx = vobj[idx_ship].x;
		pany = vobj[idx_ship].y;
		zoom_holdoff_time_cnt = 5;
		scrn_regen = 1;
		}
	}


float obj_bounds_enlarge_factor = 10;

if( zoom_detent == 0 ) obj_bounds_enlarge_factor = 10.0; //this affects when a pan is required, if zoomed out further a pan happens a bit further away from a gui window edge
if( zoom_detent == 1 ) obj_bounds_enlarge_factor = 8.0;
if( zoom_detent == 2 ) obj_bounds_enlarge_factor = 4.0;
if( zoom_detent == 3 ) obj_bounds_enlarge_factor = 2.0;


bool edge_left, edge_right, edge_top, edge_bott;

bool at_edge = check_obj_at_screen_edge( idx_ship, obj_bounds_enlarge_factor, edge_left, edge_right, edge_top, edge_bott );

if( at_edge )
	{
//	printf( "check_obj_at_screen_edge() - reported at edge %d %d %d %d\n", edge_left, edge_right, edge_top, edge_bott );
	}



//printf("move_obj() - pan_spanx %f %f  pan_edgex %f %f  pany+pan_edgey %f \n", pan_spanx, pan_spany, pan_edgex, pan_edgey, pany + pan_edgey );	

//adj pan if req
if( !obj_create_active )
	{
	if( edge_right )
		{
		panx = vobj[idx_ship].x + 0;//pan_spanx * 0.35;
		pany = vobj[idx_ship].y;

		zoom_holdoff_time_cnt = 5;
		scrn_regen = 1;
		}

	if( edge_left )
		{
		panx = vobj[idx_ship].x - 0;//pan_edgex * 0.8;						//pan center and slightly to right edge
		pany = vobj[idx_ship].y;

		zoom_holdoff_time_cnt = 5;
		scrn_regen = 1;
		}

	if( edge_bott )														//near bottom of screen ?
		{
		panx = vobj[idx_ship].x;
		pany = vobj[idx_ship].y + 0;//pan_edgey * 0.8;

		zoom_holdoff_time_cnt = 5;
		scrn_regen = 1;
		}

	if( edge_top )														//near top of screen ?
		{
		panx = vobj[idx_ship].x;
		pany = vobj[idx_ship].y - 0;//pan_edgey;

		zoom_holdoff_time_cnt = 5;
		scrn_regen = 1;
		}
	}


if( !obj_create_active )
	{
	if( zoom_extents )
		{
		panx = 0;
		pany = 0;
		}
	}




//zoom_lastx = zoomx;
//zoom_lasty = zoomy;
//----


//panx = 0;
//pany = 0;


//printf( "move_obj() - panx %f %f\n", panx, pany );


//printf( "move_obj() - time_sim %f, init_posy %f  deltay %f\n", time_sim, init_posy, init_posy - vobj[idx_ship].y  );

}













void llem_wnd::print_line_coord( unsigned int obj_idx, unsigned int line_idx )
{
if( obj_idx >= vobj.size() ) return;


float xx = vobj[obj_idx].x;
float yy = vobj[obj_idx].y;

vector<st_line_tag> vo;
vo = vobj[ obj_idx ].vline;


if( line_idx >= vo.size() ) return;

printf("print_line_coord() - obj %d line_idx %d,   %f %f %f %f\n", obj_idx, line_idx, vo[ line_idx ].x1 + xx, vo[ line_idx ].y1 + yy, vo[ line_idx ].x2 + xx, vo[ line_idx ].y2 + yy );	
}













//this calcs the midpoint of each line in spec obj, useful for 'check_near_ground()' calcs
void llem_wnd::calc_objs_line_midpoints( unsigned int obj_idx )
{

if( obj_idx >= vobj.size() ) return;

unsigned int idx = obj_idx;

for( int j = 0; j < vobj[ idx ].vline.size(); j++ )
	{
	float x0 = vobj[ idx ].vline[ j ].x1;
	float y0 = vobj[ idx ].vline[ j ].y1;
	float x1 = vobj[ idx ].vline[ j ].x2;
	float y1 = vobj[ idx ].vline[ j ].y2;

//x0 = 0;
//y0 = 0;
//x1 = -100;
//y1 = -1;

	//calc line equation
	float m = ( y1-y0 )/( x1-x0 );			//y = m.x + b,    y - b = m.x,   -b = m.x - y,  b = -m.x + y,   b = y - m.x
		
	
	float b = y1 - m * x1;			 		//b = y - m.x
	
	float mid_x = ( x1 - x0 ) / 2 + x0;		//line's middle x val
	
	float mid_y = m * mid_x + b;			//find line's middle y val
	
	
	if( ( fabs( m ) > 1000 ) )											//line vertical?
		{
		mid_y = vobj[ idx ].vline[ j ].y1;
		}

	vobj[ idx ].vline[ j ].middlex = mid_x;
	vobj[ idx ].vline[ j ].middley = mid_y;
//printf("calc_objs_line_midpoints()   mid_x %f %f\n", mid_x, mid_y );
//getchar();
	}
}











//searches ground obj's lines for a line that has a distance from ship's pos that is less than values held in 'vdist_considered_near[]'
//returns value from 'vdist_considered_near[]' if a ground line with less distance is found, else -1 
//'idx_line_closest' holds the line index where above was satisfied, else -1

//MAKE SURE that 'vdist_considered_near[]' values are in increasing order, it won't work otherwise
float llem_wnd::check_near_ground( vector<float>vdist_considered_near, int &idx_line_closest )
{
idx_line_closest = -1;

if( vdist_considered_near.size() == 0 ) return -1;


int idx = idx_ground;

float len_min = 100000;					//some large number
	
float posx = vobj[ idx_ship ].x;
float posy = vobj[ idx_ship ].y;

int nearest_dist_idx = -1;
int checked_cnt = 0;

for( int j = 0; j < vobj[ idx ].vline.size(); j++ )
	{
//	if( vobj[ idx ].vline[ j ].drawn == 0 ) continue;
	
	float x0 = vobj[ idx ].vline[ j ].x1;
	float y0 = vobj[ idx ].vline[ j ].y1;
	float x1 = vobj[ idx ].vline[ j ].x2;
	float y1 = vobj[ idx ].vline[ j ].y2;

	float midx0 = vobj[ idx ].vline[ j ].middlex;
	float midy0 = vobj[ idx ].vline[ j ].middley;


	float dx =  posx - x0;												//one line endpoint
	float dy =  posy - y0;
	
	float len0 = sqrt( dx*dx + dy*dy );



	dx =  posx - x1;													//other line endpoint
	dy =  posy - y1;
	
	float len1 = sqrt( dx*dx + dy*dy );



	dx =  posx - midx0;													//middle of line
	dy =  posy - midy0;
	
	float len2 = sqrt( dx*dx + dy*dy );



	for( int k = 0; k < vdist_considered_near.size(); k++ )
		{
		float dist_considered_near = vdist_considered_near[k];
		
		if( len0 <= dist_considered_near )
			{
			if( len0 < len_min )
				{
				len_min = len0;
				idx_line_closest = j;
				
				nearest_dist_idx = k;
				}
			}

		if( len1 <= dist_considered_near )
			{
			if( len1 < len_min )
				{
				len_min = len1;
				idx_line_closest = j;

				nearest_dist_idx = k;
				}
			}


		if( len2 <= dist_considered_near )
			{
			if( len2 < len_min )
				{
				len_min = len2;
				idx_line_closest = j;

				nearest_dist_idx = k;
				}
			}
		}
	checked_cnt++;
	}

//printf( "check_near_ground() - ground lines checked %d\n", checked_cnt );



if( nearest_dist_idx == -1 ) return -1;

return vdist_considered_near[nearest_dist_idx];
}








struct st_7_seg_element_tag
{
float x1, y1, x2, y2;	
};


//          a
//         ----
//   f   /    /  b
//   g   ----
//   e /    /  c
//      ----
//       d


st_7_seg_element_tag st_7_seg_f = 
{
-1, 1, -1, 0
};

st_7_seg_element_tag st_7_seg_a = 
{
-1, 1, 1, 1	
};

st_7_seg_element_tag st_7_seg_b = 
{
1, 1, 1, 0
};

st_7_seg_element_tag st_7_seg_c = 
{
1, 0, 1, -1	
};


st_7_seg_element_tag st_7_seg_e = 
{
-1, 0, -1, -1	
};

st_7_seg_element_tag st_7_seg_d = 
{
-1, -1, 1, -1	
};


st_7_seg_element_tag st_7_seg_g = 
{
-1, 0, 1, 0	
};


void llem_wnd::add_7_seg_num( int num, float posx, float posy, int r, int g, int b, vector<st_line_tag> &vo )
{
st_line_tag o;

float italic_skew = 4;						//skew xpos dependent on ypos

float sclx = 7;
float scly = 10;


o.flags = en_ft_update;
o.line_type = en_lt_ground_landing_zone_fuel_digit;
o.fuel_val = 0;

o.drawn = 0;
o.drawn_x1 = 0;														//put anything in here thats initally valid for plotting
o.drawn_y1 = 0;
o.drawn_x2 = 1;
o.drawn_y2 = 1;

o.r = r;
o.g = g;
o.b = b;

//build number from segments
if( num == 0 )
	{
	o.x1 = posx + st_7_seg_f.x1 * sclx + nearbyint(st_7_seg_f.y1)*italic_skew;
	o.y1 = posy + st_7_seg_f.y1 * scly;

	o.x2 = posx + st_7_seg_f.x2 * sclx + nearbyint(st_7_seg_f.y2)*italic_skew;
	o.y2 = posy + st_7_seg_f.y2 * scly;
	
	vo.push_back( o );


	o.x1 = posx + st_7_seg_a.x1 * sclx + nearbyint(st_7_seg_a.y1)*italic_skew;
	o.y1 = posy + st_7_seg_a.y1 * scly;

	o.x2 = posx + st_7_seg_a.x2 * sclx + nearbyint(st_7_seg_a.y2)*italic_skew;
	o.y2 = posy + st_7_seg_a.y2 * scly;
	
	vo.push_back( o );


	o.x1 = posx + st_7_seg_b.x1 * sclx + nearbyint(st_7_seg_b.y1)*italic_skew;
	o.y1 = posy + st_7_seg_b.y1 * scly;

	o.x2 = posx + st_7_seg_b.x2 * sclx + nearbyint(st_7_seg_b.y2)*italic_skew;
	o.y2 = posy + st_7_seg_b.y2 * scly;
	
	vo.push_back( o );


	o.x1 = posx + st_7_seg_c.x1 * sclx + nearbyint(st_7_seg_c.y1)*italic_skew;
	o.y1 = posy + st_7_seg_c.y1 * scly;

	o.x2 = posx + st_7_seg_c.x2 * sclx + nearbyint(st_7_seg_c.y2)*italic_skew;
	o.y2 = posy + st_7_seg_c.y2 * scly;
	
	vo.push_back( o );


	o.x1 = posx + st_7_seg_e.x1 * sclx + st_7_seg_e.y1*italic_skew;
	o.y1 = posy + st_7_seg_e.y1 * scly;

	o.x2 = posx + st_7_seg_e.x2 * sclx + st_7_seg_e.y2*italic_skew;
	o.y2 = posy + st_7_seg_e.y2 * scly;
	
	vo.push_back( o );


	o.x1 = posx + st_7_seg_d.x1 * sclx + st_7_seg_d.y1*italic_skew;
	o.y1 = posy + st_7_seg_d.y1 * scly;

	o.x2 = posx + st_7_seg_d.x2 * sclx + st_7_seg_d.y2*italic_skew;
	o.y2 = posy + st_7_seg_d.y2 * scly;
	
	vo.push_back( o );
	}




if( num == 1 )
	{
	o.x1 = posx + st_7_seg_b.x1 * sclx + nearbyint(st_7_seg_b.y1)*italic_skew;
	o.y1 = posy + st_7_seg_b.y1 * scly;

	o.x2 = posx + st_7_seg_b.x2 * sclx + nearbyint(st_7_seg_b.y2)*italic_skew;
	o.y2 = posy + st_7_seg_b.y2 * scly;
	
	vo.push_back( o );


	o.x1 = posx + st_7_seg_c.x1 * sclx + nearbyint(st_7_seg_c.y1)*italic_skew;
	o.y1 = posy + st_7_seg_c.y1 * scly;

	o.x2 = posx + st_7_seg_c.x2 * sclx + nearbyint(st_7_seg_c.y2)*italic_skew;
	o.y2 = posy + st_7_seg_c.y2 * scly;
	
	vo.push_back( o );
	}


if( num == 2 )
	{
	o.x1 = posx + st_7_seg_a.x1 * sclx + nearbyint(st_7_seg_a.y1)*italic_skew;
	o.y1 = posy + st_7_seg_a.y1 * scly;

	o.x2 = posx + st_7_seg_a.x2 * sclx + nearbyint(st_7_seg_a.y2)*italic_skew;
	o.y2 = posy + st_7_seg_a.y2 * scly;
	
	vo.push_back( o );


	o.x1 = posx + st_7_seg_b.x1 * sclx + nearbyint(st_7_seg_b.y1)*italic_skew;
	o.y1 = posy + st_7_seg_b.y1 * scly;

	o.x2 = posx + st_7_seg_b.x2 * sclx + nearbyint(st_7_seg_b.y2)*italic_skew;
	o.y2 = posy + st_7_seg_b.y2 * scly;
	
	vo.push_back( o );


	o.x1 = posx + st_7_seg_e.x1 * sclx + st_7_seg_e.y1*italic_skew;
	o.y1 = posy + st_7_seg_e.y1 * scly;

	o.x2 = posx + st_7_seg_e.x2 * sclx + st_7_seg_e.y2*italic_skew;
	o.y2 = posy + st_7_seg_e.y2 * scly;
	
	vo.push_back( o );


	o.x1 = posx + st_7_seg_d.x1 * sclx + st_7_seg_d.y1*italic_skew;
	o.y1 = posy + st_7_seg_d.y1 * scly;

	o.x2 = posx + st_7_seg_d.x2 * sclx + st_7_seg_d.y2*italic_skew;
	o.y2 = posy + st_7_seg_d.y2 * scly;
	
	vo.push_back( o );


	o.x1 = posx + st_7_seg_g.x1 * sclx + st_7_seg_g.y1*italic_skew;
	o.y1 = posy + st_7_seg_g.y1 * scly;

	o.x2 = posx + st_7_seg_g.x2 * sclx + st_7_seg_g.y2*italic_skew;
	o.y2 = posy + st_7_seg_g.y2 * scly;
	
	vo.push_back( o );
	}


if( num == 3 )
	{
	o.x1 = posx + st_7_seg_a.x1 * sclx + nearbyint(st_7_seg_a.y1)*italic_skew;
	o.y1 = posy + st_7_seg_a.y1 * scly;

	o.x2 = posx + st_7_seg_a.x2 * sclx + nearbyint(st_7_seg_a.y2)*italic_skew;
	o.y2 = posy + st_7_seg_a.y2 * scly;
	
	vo.push_back( o );


	o.x1 = posx + st_7_seg_b.x1 * sclx + nearbyint(st_7_seg_b.y1)*italic_skew;
	o.y1 = posy + st_7_seg_b.y1 * scly;

	o.x2 = posx + st_7_seg_b.x2 * sclx + nearbyint(st_7_seg_b.y2)*italic_skew;
	o.y2 = posy + st_7_seg_b.y2 * scly;
	
	vo.push_back( o );


	o.x1 = posx + st_7_seg_c.x1 * sclx + nearbyint(st_7_seg_c.y1)*italic_skew;
	o.y1 = posy + st_7_seg_c.y1 * scly;

	o.x2 = posx + st_7_seg_c.x2 * sclx + nearbyint(st_7_seg_c.y2)*italic_skew;
	o.y2 = posy + st_7_seg_c.y2 * scly;
	
	vo.push_back( o );


	o.x1 = posx + st_7_seg_d.x1 * sclx + st_7_seg_d.y1*italic_skew;
	o.y1 = posy + st_7_seg_d.y1 * scly;

	o.x2 = posx + st_7_seg_d.x2 * sclx + st_7_seg_d.y2*italic_skew;
	o.y2 = posy + st_7_seg_d.y2 * scly;
	
	vo.push_back( o );


	o.x1 = posx + st_7_seg_g.x1 * sclx + st_7_seg_g.y1*italic_skew;
	o.y1 = posy + st_7_seg_g.y1 * scly;

	o.x2 = posx + st_7_seg_g.x2 * sclx + st_7_seg_g.y2*italic_skew;
	o.y2 = posy + st_7_seg_g.y2 * scly;
	
	vo.push_back( o );
	}






if( num == 4 )
	{
	o.x1 = posx + st_7_seg_f.x1 * sclx + nearbyint(st_7_seg_f.y1)*italic_skew;
	o.y1 = posy + st_7_seg_f.y1 * scly;

	o.x2 = posx + st_7_seg_f.x2 * sclx + nearbyint(st_7_seg_f.y2)*italic_skew;
	o.y2 = posy + st_7_seg_f.y2 * scly;
	
	vo.push_back( o );


	o.x1 = posx + st_7_seg_b.x1 * sclx + nearbyint(st_7_seg_b.y1)*italic_skew;
	o.y1 = posy + st_7_seg_b.y1 * scly;

	o.x2 = posx + st_7_seg_b.x2 * sclx + nearbyint(st_7_seg_b.y2)*italic_skew;
	o.y2 = posy + st_7_seg_b.y2 * scly;
	
	vo.push_back( o );


	o.x1 = posx + st_7_seg_c.x1 * sclx + nearbyint(st_7_seg_c.y1)*italic_skew;
	o.y1 = posy + st_7_seg_c.y1 * scly;

	o.x2 = posx + st_7_seg_c.x2 * sclx + nearbyint(st_7_seg_c.y2)*italic_skew;
	o.y2 = posy + st_7_seg_c.y2 * scly;
	
	vo.push_back( o );

	o.x1 = posx + st_7_seg_g.x1 * sclx + st_7_seg_g.y1*italic_skew;
	o.y1 = posy + st_7_seg_g.y1 * scly;

	o.x2 = posx + st_7_seg_g.x2 * sclx + st_7_seg_g.y2*italic_skew;
	o.y2 = posy + st_7_seg_g.y2 * scly;
	
	vo.push_back( o );
	}


if( num == 5 )
	{
	o.x1 = posx + st_7_seg_f.x1 * sclx + st_7_seg_f.y1*italic_skew;
	o.y1 = posy + st_7_seg_f.y1 * scly;

	o.x2 = posx + st_7_seg_f.x2 * sclx + st_7_seg_f.y2*italic_skew;
	o.y2 = posy + st_7_seg_f.y2 * scly;
	
	vo.push_back( o );


	o.x1 = posx + st_7_seg_a.x1 * sclx + st_7_seg_a.y1*italic_skew;
	o.y1 = posy + st_7_seg_a.y1 * scly;

	o.x2 = posx + st_7_seg_a.x2 * sclx + st_7_seg_a.y2*italic_skew;
	o.y2 = posy + st_7_seg_a.y2 * scly;
	
	vo.push_back( o );


	o.x1 = posx + st_7_seg_c.x1 * sclx + st_7_seg_c.y1*italic_skew;
	o.y1 = posy + st_7_seg_c.y1 * scly;

	o.x2 = posx + st_7_seg_c.x2 * sclx + st_7_seg_c.y2*italic_skew;
	o.y2 = posy + st_7_seg_c.y2 * scly;
	
	vo.push_back( o );


	o.x1 = posx + st_7_seg_d.x1 * sclx + st_7_seg_d.y1*italic_skew;
	o.y1 = posy + st_7_seg_d.y1 * scly;

	o.x2 = posx + st_7_seg_d.x2 * sclx + st_7_seg_d.y2*italic_skew;
	o.y2 = posy + st_7_seg_d.y2 * scly;
	
	vo.push_back( o );


	o.x1 = posx + st_7_seg_g.x1 * sclx + st_7_seg_g.y1*italic_skew;
	o.y1 = posy + st_7_seg_g.y1 * scly;

	o.x2 = posx + st_7_seg_g.x2 * sclx + st_7_seg_g.y2*italic_skew;
	o.y2 = posy + st_7_seg_g.y2 * scly;
	
	vo.push_back( o );
	}



if( num == 6 )
	{
	o.x1 = posx + st_7_seg_f.x1 * sclx + st_7_seg_f.y1*italic_skew;
	o.y1 = posy + st_7_seg_f.y1 * scly;

	o.x2 = posx + st_7_seg_f.x2 * sclx + st_7_seg_f.y2*italic_skew;
	o.y2 = posy + st_7_seg_f.y2 * scly;
	
	vo.push_back( o );


	o.x1 = posx + st_7_seg_a.x1 * sclx + st_7_seg_a.y1*italic_skew;
	o.y1 = posy + st_7_seg_a.y1 * scly;

	o.x2 = posx + st_7_seg_a.x2 * sclx + st_7_seg_a.y2*italic_skew;
	o.y2 = posy + st_7_seg_a.y2 * scly;
	
	vo.push_back( o );


	o.x1 = posx + st_7_seg_c.x1 * sclx + st_7_seg_c.y1*italic_skew;
	o.y1 = posy + st_7_seg_c.y1 * scly;

	o.x2 = posx + st_7_seg_c.x2 * sclx + st_7_seg_c.y2*italic_skew;
	o.y2 = posy + st_7_seg_c.y2 * scly;
	
	vo.push_back( o );


	o.x1 = posx + st_7_seg_e.x1 * sclx + st_7_seg_e.y1*italic_skew;
	o.y1 = posy + st_7_seg_e.y1 * scly;

	o.x2 = posx + st_7_seg_e.x2 * sclx + st_7_seg_e.y2*italic_skew;
	o.y2 = posy + st_7_seg_e.y2 * scly;
	
	vo.push_back( o );


	o.x1 = posx + st_7_seg_d.x1 * sclx + st_7_seg_d.y1*italic_skew;
	o.y1 = posy + st_7_seg_d.y1 * scly;

	o.x2 = posx + st_7_seg_d.x2 * sclx + st_7_seg_d.y2*italic_skew;
	o.y2 = posy + st_7_seg_d.y2 * scly;
	
	vo.push_back( o );


	o.x1 = posx + st_7_seg_g.x1 * sclx + st_7_seg_g.y1*italic_skew;
	o.y1 = posy + st_7_seg_g.y1 * scly;

	o.x2 = posx + st_7_seg_g.x2 * sclx + st_7_seg_g.y2*italic_skew;
	o.y2 = posy + st_7_seg_g.y2 * scly;
	
	vo.push_back( o );
	}




if( num == 7 )
	{
	o.x1 = posx + st_7_seg_a.x1 * sclx + st_7_seg_a.y1*italic_skew;
	o.y1 = posy + st_7_seg_a.y1 * scly;

	o.x2 = posx + st_7_seg_a.x2 * sclx + st_7_seg_a.y2*italic_skew;
	o.y2 = posy + st_7_seg_a.y2 * scly;
	
	vo.push_back( o );


	o.x1 = posx + st_7_seg_b.x1 * sclx + st_7_seg_b.y1*italic_skew;
	o.y1 = posy + st_7_seg_b.y1 * scly;

	o.x2 = posx + st_7_seg_b.x2 * sclx + st_7_seg_b.y2*italic_skew;
	o.y2 = posy + st_7_seg_b.y2 * scly;
	
	vo.push_back( o );


	o.x1 = posx + st_7_seg_c.x1 * sclx + st_7_seg_c.y1*italic_skew;
	o.y1 = posy + st_7_seg_c.y1 * scly;

	o.x2 = posx + st_7_seg_c.x2 * sclx + st_7_seg_c.y2*italic_skew;
	o.y2 = posy + st_7_seg_c.y2 * scly;
	
	vo.push_back( o );
	}




if( num == 8 )
	{
	o.x1 = posx + st_7_seg_f.x1 * sclx + st_7_seg_f.y1*italic_skew;
	o.y1 = posy + st_7_seg_f.y1 * scly;

	o.x2 = posx + st_7_seg_f.x2 * sclx + st_7_seg_f.y2*italic_skew;
	o.y2 = posy + st_7_seg_f.y2 * scly;
	
	vo.push_back( o );


	o.x1 = posx + st_7_seg_a.x1 * sclx + st_7_seg_a.y1*italic_skew;
	o.y1 = posy + st_7_seg_a.y1 * scly;

	o.x2 = posx + st_7_seg_a.x2 * sclx + st_7_seg_a.y2*italic_skew;
	o.y2 = posy + st_7_seg_a.y2 * scly;
	
	vo.push_back( o );


	o.x1 = posx + st_7_seg_b.x1 * sclx + st_7_seg_b.y1*italic_skew;
	o.y1 = posy + st_7_seg_b.y1 * scly;

	o.x2 = posx + st_7_seg_b.x2 * sclx + st_7_seg_b.y2*italic_skew;
	o.y2 = posy + st_7_seg_b.y2 * scly;
	
	vo.push_back( o );


	o.x1 = posx + st_7_seg_c.x1 * sclx + st_7_seg_c.y1*italic_skew;
	o.y1 = posy + st_7_seg_c.y1 * scly;

	o.x2 = posx + st_7_seg_c.x2 * sclx + st_7_seg_c.y2*italic_skew;
	o.y2 = posy + st_7_seg_c.y2 * scly;
	
	vo.push_back( o );


	o.x1 = posx + st_7_seg_e.x1 * sclx + st_7_seg_e.y1*italic_skew;
	o.y1 = posy + st_7_seg_e.y1 * scly;

	o.x2 = posx + st_7_seg_e.x2 * sclx + st_7_seg_e.y2*italic_skew;
	o.y2 = posy + st_7_seg_e.y2 * scly;
	
	vo.push_back( o );


	o.x1 = posx + st_7_seg_d.x1 * sclx + st_7_seg_d.y1*italic_skew;
	o.y1 = posy + st_7_seg_d.y1 * scly;

	o.x2 = posx + st_7_seg_d.x2 * sclx + st_7_seg_d.y2*italic_skew;
	o.y2 = posy + st_7_seg_d.y2 * scly;
	
	vo.push_back( o );


	o.x1 = posx + st_7_seg_g.x1 * sclx + st_7_seg_g.y1*italic_skew;
	o.y1 = posy + st_7_seg_g.y1 * scly;

	o.x2 = posx + st_7_seg_g.x2 * sclx + st_7_seg_g.y2*italic_skew;
	o.y2 = posy + st_7_seg_g.y2 * scly;
	
	vo.push_back( o );
	}






if( num == 9 )
	{
	o.x1 = posx + st_7_seg_f.x1 * sclx + st_7_seg_f.y1*italic_skew;
	o.y1 = posy + st_7_seg_f.y1 * scly;

	o.x2 = posx + st_7_seg_f.x2 * sclx + st_7_seg_f.y2*italic_skew;
	o.y2 = posy + st_7_seg_f.y2 * scly;
	
	vo.push_back( o );


	o.x1 = posx + st_7_seg_a.x1 * sclx + st_7_seg_a.y1*italic_skew;
	o.y1 = posy + st_7_seg_a.y1 * scly;

	o.x2 = posx + st_7_seg_a.x2 * sclx + st_7_seg_a.y2*italic_skew;
	o.y2 = posy + st_7_seg_a.y2 * scly;
	
	vo.push_back( o );


	o.x1 = posx + st_7_seg_b.x1 * sclx + st_7_seg_b.y1*italic_skew;
	o.y1 = posy + st_7_seg_b.y1 * scly;

	o.x2 = posx + st_7_seg_b.x2 * sclx + st_7_seg_b.y2*italic_skew;
	o.y2 = posy + st_7_seg_b.y2 * scly;
	
	vo.push_back( o );


	o.x1 = posx + st_7_seg_c.x1 * sclx + st_7_seg_c.y1*italic_skew;
	o.y1 = posy + st_7_seg_c.y1 * scly;

	o.x2 = posx + st_7_seg_c.x2 * sclx + st_7_seg_c.y2*italic_skew;
	o.y2 = posy + st_7_seg_c.y2 * scly;
	
	vo.push_back( o );


	o.x1 = posx + st_7_seg_d.x1 * sclx + st_7_seg_d.y1*italic_skew;
	o.y1 = posy + st_7_seg_d.y1 * scly;

	o.x2 = posx + st_7_seg_d.x2 * sclx + st_7_seg_d.y2*italic_skew;
	o.y2 = posy + st_7_seg_d.y2 * scly;
	
	vo.push_back( o );


	o.x1 = posx + st_7_seg_g.x1 * sclx + st_7_seg_g.y1*italic_skew;
	o.y1 = posy + st_7_seg_g.y1 * scly;

	o.x2 = posx + st_7_seg_g.x2 * sclx + st_7_seg_g.y2*italic_skew;
	o.y2 = posy + st_7_seg_g.y2 * scly;
	
	vo.push_back( o );
	}
}






void llem_wnd::ground_landing_restore_fuel_levels()
{
vector<st_line_tag> vo;
vo = vobj[ idx_ground ].vline;

for( int i = 0; i < vo.size(); i++ )
	{
	if( ( vo[i].line_type == en_lt_ground_landing_zone_no_fuel ) )
		{
		int dx = vo[i].x1 - vo[i].x2;
		int dy = vo[i].y1 - vo[i].y2;
		
		vo[i].line_type = en_lt_ground_landing_zone_with_fuel;
		}
		
	}

vobj[ idx_ground ].vline = vo;
}












//add landing zone colour and fuel seven seg digits
void llem_wnd::ground_landing_zone_colour_and_fuel_digits()
{
vector<st_line_tag> vo;
vo = vobj[ idx_ground ].vline;

landing_zones_avail = 0;
for( int i = 0; i < vo.size(); i++ )
	{
//	if( ( vo[i].line_type == en_lt_collision_det_pads ) || ( vo[i].line_type == en_lt_collision_det_sides ) )
		{
		float dx = vo[i].x1 - vo[i].x2;
		float dy = vo[i].y1 - vo[i].y2;
		
		if( dy == 0 )
			{
			if( ( vo[i].line_type == en_lt_ground_landing_zone_with_fuel ) || ( vo[i].line_type == en_lt_ground_landing_zone_no_fuel ) )
				{
				if( vo[i].line_type == en_lt_ground_landing_zone_with_fuel )
					{
					vo[i].r = 0;
					vo[i].g = 120;
					vo[i].b = 0;
					landing_zones_avail++;
					}

				if( vo[i].line_type == en_lt_ground_landing_zone_no_fuel )
					{
					vo[i].r = 180;
					vo[i].g = 40;
					vo[i].b = 0;
					landing_zones_avail++;
					}

				
				int fuel_val = vo[i].fuel_val;
				
				float leftx = vo[i].x1;
				if( leftx >  vo[i].x2 ) leftx = vo[i].x2;
				
				float fmidx = fabs(dx)/2;
				 
				//add msd if any
				if( fuel_val > 9 )
					{
					int msd = fuel_val / 10;
						
					add_7_seg_num( msd, leftx + fmidx - 30, vo[i].y1 - 25, 120, 120, 120, vo );		//add fuel amount number
					fuel_val -= msd*10;
					}
				//add lsd
				add_7_seg_num( fuel_val, leftx + fmidx, vo[i].y1 - 25, 120, 120, 120, vo );		//add fuel amount number
				}
			}
		
		}
		
	}
vobj[ idx_ground ].vline = vo;
}










//remove seven seg num lines
void llem_wnd::ground_landing_zone_fuel_remove_digits()
{
vector<st_line_tag> vo, vo2;
vo = vobj[ idx_ground ].vline;

for( int i = 0; i < vo.size(); i++ )
	{
	if( vo[i].line_type != en_lt_ground_landing_zone_fuel_digit )
		{
		vo2.push_back( vo[i] );
		}
	}
vobj[ idx_ground ].vline = vo2;
}









// !!!! collisions are assessed via the drawn line coords (at 4x size), using the last drawn line coords (integers)
// !!!!---- NOTE THIS REQUIRES a zoomed in (ie 'zoom_detent' == 3) render for it to work correctly when landing, i.e when checking the landing pad zones defined in the ship
// for non landing pad collisions the zoom does not really matter so much
//indexes are to lines where collisions were detected
//returns 0 if not able to check for collision
//returns 1 if good landing (good_cnt_left == 1, good_cnt_right == 1)
//returns 2 if crashed landing (one point of contact)
//returns 3 if crashed landing (too fast --- good_cnt_left == 1, good_cnt_right == 1 )
//returns 4 if crashed landing, unbalanced landing pad collisions, ( good_cnt_left != 1 or  good_cnt_right != 1)
//returns -1 skipped check
//returns -2 skipped check
//returns -3 skipped check

int llem_wnd::check_collision( vector<int>&vship_idx, vector<int>&vground_idx, int &fuel_val )
{
vship_idx.clear();
vground_idx.clear();
vector<st_line_tag> vo;
vo = vobj[ idx_ship ].vline;

int ret = 0;

if( taking_off_wait_for_a_draw_to_occur ) return -1;

if( first_draw ) return -2;

if( scrn_regen ) return -3;

fuel_val = 0;
int good_landing_zone_line_idx = -1;

for( int i = 0; i < vo.size(); i++ )
	{
	if( ( vo[i].line_type == en_lt_ship_collision_det_pads_left ) || ( vo[i].line_type == en_lt_ship_collision_det_pads_right ) || ( vo[i].line_type == en_lt_ship_collision_det_sides ) )
//	if( ( vo[i].line_type == en_lt_ship_collision_det_pads ) )
		{
		st_Point p0, p1;
		
		if( vo[i].drawn == 0 ) 
			{
			continue;
			}

//		p0.x = vo[i].x1 + vobj[ idx_ship ].x;
//		p0.y = vo[i].y1 + vobj[ idx_ship ].y;
//		p1.x = vo[i].x2 + vobj[ idx_ship ].x;
//		p1.y = vo[i].y2 + vobj[ idx_ship ].y;

		p0.x = vo[i].drawn_x1;
		p0.y = vo[i].drawn_y1;
		p1.x = vo[i].drawn_x2;
		p1.y = vo[i].drawn_y2;

//print_line_coord( idx_ship, 25 );



//		p0.x = 0;
//		p0.y = 0;
//		p1.x = 100;
//		p1.y = 0;
		
		vector<st_line_tag> vo2;
		vo2 = vobj[ idx_ground ].vline;
		for( int j = 0; j < vo2.size(); j++ )
			{
			if( vo2[j].drawn == 0 ) 
				{
//				printf( "check_collision() - not drawn ground line idx %d\n", j );
				continue;
				}
			
			if( vo2[j].line_type == en_lt_ground_landing_zone_fuel_digit ) continue;
			
			st_Point p2, p3;

//			p2.x = vo2[j].x1;
//			p2.y = vo2[j].y1;
//			p3.x = vo2[j].x2;
//			p3.y = vo2[j].y2;

			p2.x = vo2[j].drawn_x1;
			p2.y = vo2[j].drawn_y1;
			p3.x = vo2[j].drawn_x2;
			p3.y = vo2[j].drawn_y2;
//printf( "idx_ship vo[i].drawn_x1 %d %d\n", vo[i].drawn_x1, vo[i].drawn_y1 );
//printf( "idx_ship, was drawn %d vo[%d].drawn_x1 %d %d %d %d\n", vo[i].drawn, i, p0.x, p0.y, p1.x, p1.y );
//printf( "idx_ground, was drawn %d  vo[%d].drawn_x1 %d %d %d %d\n", vo2[i].drawn, j, p2.x, p2.y, p3.x, p3.y );


//			p2.x = 100.00;
//			p2.y = 0;
//			p3.x = 200;
//			p3.y = 0;
		
		
//printf( "check_collision()\n" );
	
			int dx = p0.x - p1.x;
			int dy = p0.y - p1.y;
			if( ( dx == 0 ) && ( dy == 0 ) ) continue;					//this can happen if obj is offscreen for some reason, so don't look for collision in that case

			dx = p2.x - p3.x;
			dy = p2.y - p3.y;
			if( ( dx == 0 ) && ( dy == 0 ) ) continue;
			

//if( i == 26 )
//	{
//	if( ( j == 93 ) || ( j == 93 ) ) 
//		{
//		printf( "check_collision() - line ground %d (ship line %d)  ship: p0 p1 %d %d %d %d\n", j, i, p0.x, p0.y, p1.x, p1.y );
//		printf( "check_collision() - line ground %d (ship line %d)  grnd: p2 p3 %d %d %d %d\n", j, i, p2.x, p2.y, p3.x, p3.y );
//		}
//	}
			if( LineIntersect_doIntersect( p0, p1, p2, p3 ) )
				{
//printf( "check_collision() - intersect\n" );
				 
//				if( ( vo[i].line_type == en_lt_collision_det_sides ) )
//					{
//					vobj[ idx_ship ].is_exploding = 1;
//					}
				vship_idx.push_back( i );
				vground_idx.push_back( j );
				}
			else{
				}
				
			}
			
		}

	}

//show to console for debugging
if( vship_idx.size() > 0 )
	{
	printf("check_collision() -" );
	
	for( int i = 0; i < vship_idx.size(); i++ )
		{
		printf(" ship line %d    ground line %d    ", vship_idx[i], vground_idx[i] );
		}
	printf("\n" );
	}


int good_cnt_left = 0;
int good_cnt_right = 0;
int bad_cnt = 0;

if( vship_idx.size() == 1 )
	{
	printf("check_collision() - crash detected %d points, dy %f\n", vship_idx.size(), vobj[idx_ship].dy );
//	ship_crash( "single point collision" );
	ret = 2;
	goto done_check;
	}

//tally up good and bad collisions
if( vship_idx.size() >= 2 )
	{
	printf("check_collision() - detected %d points, dy %f\n", vship_idx.size(), vobj[idx_ship].dy );

	for( int i = 0; i < vship_idx.size(); i++ )
		{
		int lship_idx = vship_idx[i];
		int lground_idx = vground_idx[i];

		if( ( vobj[ idx_ship ].vline[lship_idx].line_type == en_lt_ship_collision_det_pads_left ) || ( vobj[ idx_ship ].vline[lship_idx].line_type == en_lt_ship_collision_det_pads_right ) )
			{
			if( ( vobj[ idx_ground ].vline[lground_idx].line_type == en_lt_ground_landing_zone_with_fuel ) || ( vobj[ idx_ground ].vline[lground_idx].line_type == en_lt_ground_landing_zone_no_fuel ) )
				{
				if( vobj[ idx_ship ].vline[lship_idx].line_type == en_lt_ship_collision_det_pads_left ) good_cnt_left++;
				if( vobj[ idx_ship ].vline[lship_idx].line_type == en_lt_ship_collision_det_pads_right ) good_cnt_right++;
				fuel_val = vobj[ idx_ground ].vline[lground_idx].fuel_val;
				good_landing_zone_line_idx = lground_idx;
				}
			else{
				bad_cnt++;
				}
			}
		else{
			bad_cnt++;
			}
//		printf(" ship line %d    ground line %d   ", vship_idx[i], vground_idx[i] );
	//				return 1;
		}
//	printf("\n" );
	}


if( vship_idx.size() >= 2 )
	{
//	if( good_cnt != vship_idx.size() )
		{
		if( bad_cnt > 0 )
			{
//			ship_crash( "multi point collision with a bad_cnt" );
			printf("check_collision() - crash detected collision points %d, good_cnt_left %d, good_cnt_right %d, bad_cnt %d, dy %f\n", vship_idx.size(), good_cnt_left, good_cnt_right, bad_cnt, vobj[idx_ship].dy );
			ret = 2;
			goto done_check;
			}
		}

	if( ( good_cnt_left == 1 ) && ( good_cnt_right == 1 ) )
		{
		float max_landing_vel = landing_rating_speed_limit[ expertise_level ];

		if( vobj[ idx_ship ].dy >= max_landing_vel )							//not exceeding the negative limit (negative is downwards)
			{
			if( vobj[ idx_ground ].vline[ good_landing_zone_line_idx ].line_type == en_lt_ground_landing_zone_no_fuel ) fuel_val = 0;	//no more fuel at this landing zone ?

	//		ship_landed( "landed ok", fuel_val );
			vobj[ idx_ground ].vline[ good_landing_zone_line_idx ].line_type = en_lt_ground_landing_zone_no_fuel;	//flag no more fuel at this landing zone
			ground_landing_zone_fuel_remove_digits();
			ground_landing_zone_colour_and_fuel_digits();
			printf("check_collision() - landing detected collision points %d, good_cnt_left %d, good_cnt_right %d, bad_cnt %d, dy %f\n", vship_idx.size(), good_cnt_left, good_cnt_right, bad_cnt, vobj[idx_ship].dy );
			ret = 1;
			goto done_check;
			}
		else{
			ret = 3;
			printf("check_collision() - crash detected (too fast) collision points %d, good_cnt_left %d, good_cnt_right %d, bad_cnt %d, dy %f\n", vship_idx.size(), good_cnt_left, good_cnt_right, bad_cnt, vobj[idx_ship].dy );
			goto done_check;
			}
		}
	else{
		ret = 4;
		printf("check_collision() - crash detected collision points %d, good_cnt_left %d, good_cnt_right %d, bad_cnt %d, dy %f\n", vship_idx.size(), good_cnt_left, good_cnt_right, bad_cnt, vobj[idx_ship].dy );
		goto done_check;
		}
	}

done_check:
return ret;
}




























void llem_wnd::ship_crash( string sreason )
{
ship_crash_timeout = 10;

set_thrust( 0.0 );
game_state = en_gs_landed_crash0;

crashings_tot++;

explode_ship_init_params();
vobj[idx_ship].is_exploding = 1;

exploding_time_key_inhibit_cnt = exploding_time_key_inhibit;			//start a period of keybrd inhibit
key_inhibit = 1;

env_build_explosion_env0( srate, 0 );
env_build_explosion_env1( srate, 0 );

printf("ship_crash() - reason '%s'\n", sreason.c_str() );
printf("ship_crash() - xy %f %f dxy %f %f \n", vobj[idx_ship].x, vobj[idx_ship].y, vobj[idx_ship].dx, vobj[idx_ship].dy );
printf("ship_crash() - panxy %f %f \n", panx, pany );

if( crashings_tot >= crash_limit )
	{
	game_state = en_gs_game_ended_too_many_crashes;
	}


if( fuel <= 0.0f )
	{
	game_state = en_gs_game_ended_no_fuel;	
	}
scrn_regen = 1;
}







void llem_wnd::ship_landed( string sreason, int fuel_val )
{
landing_rating_quantized = 0;											//see 's_landing_rating[]'		

game_state = en_gs_landed_bouncing;
set_thrust( 0.0 );

landing_x = draw_ship_float_x;//vobj[ idx_ship ].x;						//save details
landing_y = draw_ship_float_y;//vobj[ idx_ship ].y;
landing_dx = draw_ship_float_dx;//vobj[ idx_ship ].dx;
landing_dy = draw_ship_float_dy;//vobj[ idx_ship ].dy;
landing_panx = draw_panx;
landing_pany = draw_pany;
landing_fuel_val = fuel_val;
landing_sreason = sreason;

//pause = 1;
animate_bounce = 1;														//start animation
animate_bounce_collision_check_state = 0;								//set 1st state
animate_bounce_time_cnt = 0.0f;
animate_bounce_time = 5.0f;
animate_bounce_dy = fabs( landing_dy )*2.0;								//make bounce proportional to downward vel

if( !pref_allow_bounce_collision_det ) vobj[idx_ship].dx = 0;			//don't allow bounce lateral movement?

printf("ship_landed() - reason '%s', xy %f %f  dxy %f %f\n", sreason.c_str(), landing_x, landing_y, landing_dx, landing_dy );

scrn_regen = 1;		//need this to redraw ground as fuel digits get overwritten by flame

}






void llem_wnd::ship_landed_calc_rating_award_fuel()
{
game_state = en_gs_landed_ok_settled;


float max_landing_vel = landing_rating_speed_limit[ expertise_level ];

vobj[idx_ship].dy = landing_dy;

landing_rating_ratio = 1.0 - landing_dy/max_landing_vel;		//make a landing rating ratio, 0.0001 is bad, 0.99 is good

landing_rating_quantized = nearbyint( (cn_landing_rating_mesg_max - 1) * landing_rating_ratio );	//make suitable val to access 's_landing_rating[]'

//limit to bounds of 's_landing_rating[]'
if( landing_rating_quantized > (cn_landing_rating_mesg_max - 1) ) landing_rating_quantized = (cn_landing_rating_mesg_max - 1);
if( landing_rating_quantized < 0 ) landing_rating_quantized = 0;

printf("ship_landed_calc_rating_award_fuel() - landing_rating_quantized %d\n", landing_rating_quantized );

float landing_rating_ratio_fudge = 0;

//bring up the low end of the ratio, so 'fuel_bonus' given is reasonable
if( landing_rating_ratio < 0.25 ) landing_rating_ratio_fudge = 0.5;				//see 's_landing_rating[]'

if( landing_rating_ratio >= 0.25 ) landing_rating_ratio_fudge = 0.7;

if( landing_rating_ratio >= 0.5 ) landing_rating_ratio_fudge = 1.0;

if( landing_rating_ratio >= 0.8 ) landing_rating_ratio_fudge = 1.25;

fuel_bonus = ( landing_fuel_val * 100.0f ) * landing_rating_ratio_fudge;				//scale landing zone's fuel value by 100, then scale it by how good landing was

score += fuel_bonus * (expertise_level + 1);

if( landing_dy > best_dy ) best_dy = landing_dy;


fuel += fuel_bonus;
fuel_earnt_tot += fuel_bonus;
if( fuel_bonus != 0.0f ) landings_tot++;

landing_sreason = "landed ok(post bounce)";

printf("ship_landed_calc_rating_award_fuel() - reason '%s', bonus fuel %f   landing_rating_ratio_fudge %f\n", landing_sreason.c_str(), landing_fuel_val * 100.0f, landing_rating_ratio_fudge );

scrn_regen = 1;		//need this to redraw ground as fuel digits get overwritten by flame

zoom_extents_turn_on_time_cnt = zoom_extents_turn_on_time;

if( landings_tot == landing_zones_avail )
	{
	game_state = en_gs_game_ended_all_landed0;
	}
	
}





void llem_wnd::ship_restore( bool rnd_thrust, bool rnd_theta )
{
animate_bounce = 0;

game_state = en_gs_landing;

vobj[idx_ship] = st_obj_ship_copy;
set_thrust( 0.0 );

vobj[idx_ship].x = 1000;
vobj[idx_ship].y = 600;

vobj[idx_ship].dx = 100*rnd();
vobj[idx_ship].dy = 0;

if( rnd_thrust ) set_thrust ( fabs( 0.3*rnd() ) );

if( rnd_theta ) vobj[idx_ship].theta = twopi*rnd();

panx = vobj[idx_ship].x;
pany = vobj[idx_ship].y;

//zoomx = zoomy = 8;
//zoom_last = 0;
zoom_detent_last = -1;						//set some value to cause a zoom change
zoom_holdoff_time_cnt = 0.25;				//set some value to cause a zoom change
scrn_regen = 1;
}








void llem_wnd::tick( float tframe_from_callback )
{
double dt = mtick_time.time_passed( mtick_time.ns_tim_start );
mtick_time.time_start( mtick_time.ns_tim_start );

if( dt <= 0 ) dt = 0.0001;


if( pref_expertise4 ) expertise_level = 4;
if( pref_expertise3 ) expertise_level = 3;
if( pref_expertise2 ) expertise_level = 2;
if( pref_expertise1 ) expertise_level = 1;
if( pref_expertise0 ) expertise_level = 0;



if( ( pref_development_mode != develope_mode ) ) 
	{
	develope_mode = pref_development_mode;
	init( 1 );
	}


//if( dt > 1 ) dt = 1;


flash_cnt0 -= dt;

if( flash_cnt0 < 0.0f )
	{
	flash_cnt0 = flash_time0;
	flash0 = !flash0;
	}

flash_cnt1 -= dt;

if( flash_cnt1 < 0.0f )
	{
	flash_cnt1 = flash_time1;
	flash1 = !flash1;
	}

ship_crash_timeout -= dt;

if( ship_crash_timeout < 0.0f )
	{
	ship_crash_timeout = 10000.0f;
	if( game_state == en_gs_landed_crash0 )
		{
		game_state = en_gs_landed_crash1;
		set_pause( 1 );
		}
	}


zoom_extents_turn_on_time_cnt -= dt;

if( ( zoom_extents_turn_on_time_cnt >= 0.0f ) && ( zoom_extents_turn_on_time_cnt <= 0.25f ) )
	{
	if( !pause) zoom_extents = 1;
//	scrn_reg = 1;
	}


exploding_time_key_inhibit_cnt -= dt;
if( ( exploding_time_key_inhibit_cnt >= 0.0f ) && ( exploding_time_key_inhibit_cnt <= 0.25f ) )
	{
	key_inhibit = 0;
	}



//dt = 0.03;

measured_tframe = dt;

//int idx = 0;
//if( rnd() > 0 ) idx = 1;

//vobj[0].sub_obj_idx = idx;

if( draw_cnt >= 10 ) move_obj( measured_tframe );


//----------- pre bounce collision test -----------
//see also 'animate_bounce' bounce code which does a final collision test after ship has settled down
vector<int> vship_idx;
vector<int> vground_idx;

if( ( !obj_create_active ) && ( !key_m ) )
	{
	int fuel_val = 0;
	if( game_state == en_gs_landing )
		{
		int ret = check_collision( vship_idx, vground_idx, fuel_val );
//		printf("tick() - checking collision ret %d  xy %f %f\n", ret, vobj[ idx_ship ].x, vobj[ idx_ship ].y );
		
		if( ret == 1 )				//good landing?	
			{
//		printf("tick() - checking collision ret %d  y %f\n", ret, vobj[ idx_ship ].y );
			ship_landed( "landed ok (pre bounce settling)", fuel_val );
			
			
			if( !st_envlp2->running ) 
				{
				float max_landing_vel = landing_rating_speed_limit[ expertise_level ];

				bounce_amp = fabs( landing_dy / max_landing_vel );		//set audio level of bounce
				env_build_explosion_env2( srate, 0 );
				}
			}

		if( ret == 2 )				//crash landing?	
			{
			ship_crash( "single point collision" );
			}

		if( ret == 3 )				//crash landing?	
			{
			ship_crash( "2 point collision, but too fast" );
			}

		if( ret == 4 )				//crash landing?	
			{
			ship_crash( "unbalanced landing pad collisions" );
			}
		}
	}
//-----------



//--------------------------------------------------------------
//animate landing bounce and check for collisions
animate_bounce_time = 5;
if( animate_bounce )
	{
	if( ( animate_bounce_collision_check_state == 0 ) || ( animate_bounce_collision_check_state == 1 ) ) 
		{
		//------ animate ship bounce
		animate_bounce_collision_check_state = 1;						//go to next state
		animate_bounce_time_cnt += measured_tframe;
		animate_bounce_dy += accel_grav*measured_tframe;
		vobj[ idx_ship ].dy = animate_bounce_dy;
		vobj[ idx_ship ].y += vobj[ idx_ship ].dy * measured_tframe;

		vobj[ idx_ship ].x += vobj[ idx_ship ].dx * measured_tframe;
		
		if( vobj[ idx_ship ].y < landing_y ) 							//ground bounce point ?
			{
			vobj[ idx_ship ].y = landing_y;								//position back to landing point

			animate_bounce_dy = -animate_bounce_dy * 0.8;				//invert vel and attenuate 

			vobj[ idx_ship ].dx *= 0.4;									//attenuate 
			
			if( !st_envlp2->running ) 
				{
				float max_landing_vel = landing_rating_speed_limit[ expertise_level ];

				bounce_amp = fabs( animate_bounce_dy / max_landing_vel );		//set audio level of bounce
				env_build_explosion_env2( srate, 0 );
				}

		//------ 
		
			//----------- settled?, prep to check if bounces cause a crash
			if( animate_bounce_dy < 0.8f ) 								//low enough dy to stop animation?						
				{
				vobj[ idx_ship ].dx = 0;
				vobj[ idx_ship ].dy = 0;

				vector<int> vship_idx;
				vector<int> vground_idx;

				if( ( !obj_create_active ) && ( !key_m ) )
					{
//					if( pref_allow_bounce_collision_det )							
						{
						if( animate_bounce_collision_check_state == 1 ) 
							{
							animate_bounce_collision_check_state = 2;	//goto to state 2, state 3 is reached in 'draw()'
						
							vobj[ idx_ship ].y = landing_y;				//position at original landing 'check_collision()' height,etc (this value was obtained from 'draw()' )
//	vobj[ idx_ship ].x = landing_x;
							panx = landing_panx;
							pany = landing_pany;
							scrn_regen = 1;								//force a full redraw so 4x drawn lines are refreshed nned this
							}											//for 'animate_bounce_collision_check_state' state 3 below
						}
					}
				}
			//----------- 
			}
		}
			

	//-----------
	//when bounce settled and after a 'draw()' check if ladning still ok or not
	if( animate_bounce_collision_check_state == 3 )						//this state occurs after a 'draw()'
		{
		animate_bounce = 0;
		animate_bounce_collision_check_state = 4;						//go to bounce 'ended' state
		
		int fuel_val = 0;
		bool crashed = 0;
printf("tick() - checking bounce collision\n" );
		int ret = check_collision( vship_idx, vground_idx, fuel_val );
printf("tick() - checking bounce collision ret %d  xy %f %f  dxy %f %f\n", ret, vobj[ idx_ship ].x, vobj[ idx_ship ].y, vobj[ idx_ship ].dx, vobj[ idx_ship ].dy );
		
//				if( ret == 1 )				//good landing?	
//					{
//					ship_landed( "landed ok", fuel_val );
//					}
			
		if( ( ret == 2 ) )													//bounce resulted in crash landing?	
			{
			ship_crash( "bounce single point collision" );
			crashed = 1;
			}

		if( ( ret == 4 ) )													//bounce resulted in crash landing?	
			{
			ship_crash( " unbalanced landing pad collisions" );
			crashed = 1;
			}

		if( !crashed )
			{
			ship_landed_calc_rating_award_fuel();						//bounce settled, still a good landing, reward user
			}
		}
				
	//-----------
		
	}
//--------------------------------------------------------------












if( !pause ) 
	{
	if( draw_cnt == 10 ) mhandy_timer.time_start( mhandy_timer.ns_tim_start );

	explode_ship_animate( measured_tframe );
	
//	if( draw_cnt >= 10 ) move_obj( measured_tframe );		//allow 'measured_tframe' to settle down after app start up
	}


redraw();



if( !b_draws_occurring ) draws_missed++;
else draws_missed = 0;

if( draws_missed > 5 )						//if wnd not being redrawn then set pause mode, this is req as collision code needs relies on line graphics being drawn every frame,
	{										//if ship is allowed to move without redraws occurring, ship will fall through ground undetected
	if( !(draws_missed%50) ) printf( "llem_wnd::tick() - b_draws_occurring %d, draws_missed %d\n", b_draws_occurring, draws_missed );
	set_pause( 1 );
	}

b_draws_occurring = 0;

}


