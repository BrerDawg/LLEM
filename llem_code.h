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

//llem_code.h
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

#include "globals.h"
#include "GCProfile.h"
//#include "my_input_wheel.h"
#include "filter_code.h"
#include "mgraph.h"
#include "line_clip_code.h"

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

#define cns_highscore_fname "highscore.txt"

#define cn_highscore_entries_max 50

using namespace std;


#define cn_ship_scale (1.0f)



struct st_Point
{
int x;
int y;
};



struct st_pixel_tag
{
int x;
int y;	
};



enum en_obj_type_tag
{
en_ot_user_ship,
en_ot_ground,
en_ot_exhaust,
en_ot_rect_test,
en_ot_crosshair,
};


enum en_line_flag_type_tag
{
en_ft_update = 1,														//use binary only to allow oring/anding of multiple flag types											
};




//DON'T CHANGE order of these are as they are saved to obj files
//adding to these, THEN ADD to 'st_line_type_names[]'
enum en_line_type
{
en_lt_ship = 0,															//a drawn line	
en_lt_ship_collision_det_pads_left = 1,									//non-drawn, landing pads, set left and right pads at the same y offset, they must be paired, position them around the 2 drawn landing pads								
en_lt_ship_collision_det_pads_right = 2,								//	"   "							
en_lt_ship_collision_det_sides = 3,										//non-drawn, various possible collision lines that are fatal

en_lt_spare4 = 4,
en_lt_spare5 = 5,
en_lt_spare6 = 6,
en_lt_spare7 = 7,
en_lt_spare8 = 8,
en_lt_spare9 = 9,
en_lt_spare10 = 10,
en_lt_spare11 = 11,
en_lt_spare12 = 12,

en_lt_ground_non_landing_pad = 13,										//a drawn line
en_lt_ground_landing_zone_with_fuel = 14,								//green
en_lt_ground_landing_zone_no_fuel = 15,									//red, all fuel used up
en_lt_ground_landing_zone_fuel_digit = 16,								//this indicates line belongs to a fuel seven 7 number
en_lt_end_marker = 17,													//used to detect end of enum defs 
};


//these MUST match 'en_line_type'
struct st_line_type_names_tag
{
char name[64];	
};



struct st_line_tag
{
en_line_type line_type;
int fuel_val;


float x1;
float y1;
float x2;
float y2;

float middlex, middley;			//useful for ground lines that are horizontal and long, such as at landing zones, this will help 'check_near_ground()' be more accurate if ship is in the middle and away from line endpoints

float dx;								//for explosion
float dy;
float theta;
float theta_inc;

bool drawn;
int drawn_x1;							//actual drawn pixel coords (at 4 times size to the filtered rendering)
int drawn_y1;
int drawn_x2;
int drawn_y2;

int r;
int g;
int b;

en_line_flag_type_tag flags;			//flag to show if line needs to be drawn
};



struct st_line2_tag
{
int x1;
int y1;
int x2;
int y2;

int r;
int g;
int b;


//int idx_obj;															//details of where line came from
//int idx_line;

//int bf_x;																//buffer rect that line was drawn onto
//int bf_y;
//int bf_w;
//int bf_h;

//int dx;								//for explosion
//int dy;

//unsigned int col;

};





struct st_obj_tag
{
bool visible;

float x;
float y;

float bound_x1;						//hold a rectangle bouning the obj's vertices
float bound_y1;
float bound_x2;
float bound_y2;


int drawn_bound_x1;
int drawn_bound_y1;
int drawn_bound_x2;
int drawn_bound_y2;

int idbg;
float dbgx;
float dbgy;
float dbgx2;
float dbgy2;


unsigned int col;
unsigned int col_temp;					//temp col
int col_temp_frames_total;   			//when reaches zero col_temp no longer used                          
int col_temp_frame_cnt;                 //when non zero col_temp is used instead of col                          

en_obj_type_tag type;
float dv;                              //directionless velocity
float dx;                              //vectored velocity
float dy;
float max_dx;                          //limit dx to this max and min
float max_dy;

int has_thrust;
float thrust;                          //cur thrust
float thrust_inc_per_frame;            //rate thrust can be inc'd per frame
float thrust_max;

float theta;                           //this is the current theta angle 'on its way' to 'theta_req'
float theta_req;                       //this is the required theta
float delta_theta_per_frame;           //this gives the rate of theta change
float delta_spin;						//this is used to give an obj spin
float theta_detentzone;					//allows a zero detent 

bool finite_life;
float time_to_live;                    //gives objs a finite life
bool mark_to_delete;                    //set this if obj needs to be deleted
float mass;
float accel;                           //directionless acceleration
float accelx;                          //vectored acceleration
float accely;
float drag;
int can_wrap;
int hits;                               //bullet hits
float time_per_bullet;                  //time between bullet firings
float time_bullet_remaining;			//time left before next bullet can be fired
float time_per_magmine_fire;            //time between magmine firings
float time_magmine_fire_remaining;		//time left before next magmine fire can be fired
int frames_per_bullet;                  //number of frame before next bullet can be fired
int bullet_frame_cnt;                   //when reaches zero next bullet can be fired                          
int is_exploding;                       //set if obj us exploding
int explode_frames_total;
int explode_frame_cnt;
int explode_shape;                      //set or reset to make 2 different looking explosions
int missle_steer_delay;                 //a delay in frames before missle steering in allowed, this helps missle get away from cpu ship
int grav_affected;                      //gravitational acceleration has an effect
int arming_delay_frame_cnt;				//used to stop a missle that's still coming up to speed being able to kill cpu's ship

double timed_burn;                    	//fixed burn
double timed_burn_thrust;
double scale;

vector<st_line_tag> vline;              //hold obj outline


int possible_owner;						//-1 = none, 0 = user,  1 = cpu
int magmine_in_flight_frame_cnt;		//


vector<st_obj_tag> vsub;				//holds a number of sub objs to allow animation effects
int sub_obj_idx;						//pointer to 'vsub'

};




enum en_game_state
{
en_gs_attract,
en_gs_paused,
en_gs_landing,
en_gs_landed_bouncing,													//initial touchdown but bounce animation is now running
en_gs_landed_ok_settled,
en_gs_landed_crash0,													//start timeout for animation to run
en_gs_landed_crash1,													//show message
en_gs_game_ended_no_fuel,												//show no fuel. game ended
en_gs_game_ended_all_landed0,											//just show stats of final landing as normal
en_gs_game_ended_all_landed1,											//show all done, game ended
en_gs_game_ended_too_many_crashes										//too many crashes, game ended

};



struct st_highscore_tag
{
int expertise;
int score;
string slandings_tot;													//e.g: 5/23
float best_dy;
int crashes;
string sdate;
string stime;
string sname_player;
string sname_llem;
string sdesc_llem;
string sfname_llem;
string sname_moonscape;
string sdesc_moonscape;
string sfname_moonscape;
};




class llem_wnd : public Fl_Double_Window
{
private:										//private var
int *buf;
int ctrl_key, shift_key;
int left_button;
int middle_button;
int right_button;
int mousewheel;
int mousex, mousey;

int develope_mode;								//enable development features:  0 :off,  1:  object creator,  2: for code developemnt

bool key_a;
bool key_d;
bool key_w;
bool key_r;
bool key_f;
bool key_k;
bool key_m;
bool key_x;
bool key_y;
bool key_s;
bool key_z;
bool key_up;
bool key_down;
bool key_left;
bool key_right;
bool key_space;

bool toggle_space;

double bounding_scale_down;						//see set_bounding_rect(..), applies a scale(fudge) to bounding rect to cater for poss of obj being rotate

float accel_grav;
float adj_angle;
int adj_line_width;
en_filter_window_type_tag adj_filter;
int adj_kernel_size;

int adj_offsx;
int adj_offsy;
float adj_scale;
bool adj_show_filtered;

bool b_highscore_ask;
float best_dy;


//unsigned char *scrn_bf_bkgd;

//Fl_Box *bx_image;
//Fl_JPEG_Image *jpg;


public:															//public var
int menu_hei;

Fl_Menu_Bar* menu;
int sizex, sizey;
//Fl_Offscreen offscr;
int bf0szx, bf0szy;

bool rebuild_filter_kernel;
int kern_size;														//odd gives a 1.0 peak
float kern0[ 256 ];
uint16_t kern0_16_t[256];

unsigned char *pixbf0;													//this holds the plot lines drawn at 4 times the req size
unsigned char *pixbf1;													//this holds the plot lines downsampled/filtered to normal size


float dbg_x1;
float dbg_y1;
float dbg_x2;
float dbg_y2;

//uint16_t *pixbf_r0 = 0;
//uint16_t *pixbf_g0 = 0;
//uint16_t *pixbf_b0 = 0;


bool scrn_regen;

int expertise_level;

bool pause;






vector<st_obj_tag> vobj;						//parent
st_obj_tag st_obj_ship_copy;

float time_sim;

double tframe;                                  //desired time between frames

float panx, pany;
//bool pan_auto;

bool zoom_immediately;
bool zoom_extents;
float zoom_extents_turn_on_time;
float zoom_extents_turn_on_time_cnt;
bool zoom_extents_goes_off;

bool zoom_auto;
float zoomx;
float zoomy;
//float zoom_lastx;
//float zoom_lasty;

int midx;
int midy;

//float nearest_dist_last;
int zoom_detent;
int zoom_detent_last;
float zoom_last;
float zoom_holdoff_time_cnt;



vector<st_pixel_tag> vpxl;						//used for debug
vector<st_pixel_tag> vpxl_cntr;					//used for debug
vector<st_line2_tag>vlin;						//holds coords for lines drawn in src image (4x oversized), used by 'filter_lines()'

float measured_tframe;
float fps;

bool first_draw;

int idx_ship;
int idx_ground;									//obj idx of ground
int idx_crosshair;								//used when creating/editing ship obj

int backgrnd_r = 255;
int backgrnd_g = 255;
int backgrnd_b = 255;
int line_r;
int line_g;
int line_b;
int plot_col_r;
int plot_col_g;
int plot_col_b;


int ship_rect_x1;
int ship_rect_y1;
int ship_rect_x2;
int ship_rect_y2;
int ship_rect_ww;
int ship_rect_hh;

int idx_obj_rect_overlap;
int idx_line_rect_overlap;

float thrust_wheel;

bool flame_on;
bool flame_flicker;

int view_x0, view_y0;
int view_x1, view_y1;

bool show_help;
bool show_debug_info;

bool zoom_state;

int lines_drawn;
int lines_erased;

float ship_vel_landing_crash;
float ship_crash_timeout;

float draw_ship_float_x;							//these are details of ship for the last draw that occurred
float draw_ship_float_y;							//as 'check_collision()' relies on drawn lines which are a frame behind, grab these vals in the 'draw()' function
float draw_ship_float_dx;
float draw_ship_float_dy;
float draw_panx;
float draw_pany;

bool animate_bounce;
float animate_bounce_time;
float animate_bounce_time_cnt;
float animate_bounce_dy;
int animate_bounce_collision_check_state;

bool taking_off_wait_for_a_draw_to_occur;	//this ensures that after a landing takeoff starts, at least one draw occurs, 
											//need this else 'check_collision()' may spuriously detect a second good landing as '..drawn_x1' vals are still holding previous frame lines

bool key_inhibit;
float exploding_time_key_inhibit;										//time for keyboard to be inactive, to allow some of explosion to play out
float exploding_time_key_inhibit_cnt;

en_game_state game_state;

float fuel_new_game;
float fuel;
float fuel_bonus;
float fuel_earnt_tot;
int landings_tot;
int landing_zones_avail;
int crashings_tot;
int crash_limit;
int score;

string landing_sreason;
float landing_x;									//inital touchdown pos, before bounce settling happens
float landing_y;
float landing_dx;
float landing_dy;
float landing_panx;
float landing_pany;
float landing_fuel_val;
float landing_rating_ratio;							//0.001-->0.99f		0.99 is a 'great landing'
int landing_rating_quantized;						//0-->4 used to access 's_landing_rating[]'


bool flash0;
float flash_time0;
float flash_cnt0;

bool flash1;
float flash_time1;
float flash_cnt1;

string sdesc_llem;
string sdesc_ground;


vector<st_highscore_tag> vhighscore;

bool b_draws_occurring;
int draws_missed;


public:											//public functions
llem_wnd( int xx, int yy, int wid, int hei, const char *label );
~llem_wnd();
void init( bool full_init );
void tick( float tframe_from_callback );
bool file_open_obj( unsigned int idx, int offx, int offy, string fname, bool clear_vsub, string &sdesc );
bool file_open_obj_old( unsigned int idx, string fname, float sclex, float scley );
bool file_save_obj( unsigned int idx, string fname, string sdesc );
void file_load_ship_state( string fname );
bool file_load_highscore( string fname );
bool file_save_highscore( string fname );

void ground_landing_restore_fuel_levels();
void ground_landing_zone_fuel_remove_digits();
void ground_landing_zone_colour_and_fuel_digits();
void zoom_set_detent( int zoom_detent_in );
float get_thrust();
void set_thrust( float thrust );
void set_pause( bool state );
void do_key( int key, bool state );

private:										//private functions
void draw();
void draw_objs( int only_this_idx, bool bkgd_only );
void draw_obj_fl_line( int idx, bool bkgd_only );
void draw_llem( int px, int py, int line_width );
void draw_line( int ix0, int iy0, int ix1, int iy1, unsigned char* bf, int sizex, int sizey );
void draw_help( int px, int py, int col );
int handle( int );
void add_user_ship( int shape, bool affected_by_grav, bool apply_dxdy, double scale, double xx, double yy, double dx, double dy, float time_to_live );
void add_ground( int shape, bool affected_by_grav, bool apply_dxdy, double scale, double xx, double yy, double dx, double dy, float time_to_live );
void add_crosshair( int shape, bool affected_by_grav, bool apply_dxdy, double scale, double xx, double yy, double dx, double dy, float time_to_live );
void add_rect( int xx, int yy, int ww, int hh, float scale );
void set_bounding_rect( int idx, double scalex, double scaley, bool include_sub_objs );
double rnd();
void setcolref( colref col );
void set_col( unsigned int col );
void line( int x1, int y1, int x2, int y2, int r, int g, int b, bool bkgd_only, bool dont_set_pixels, int dbg_line_idx );
//void line_zero_based( double x1, double y1, double x2, double y2 );
void rotate( float xx, float yy, float &x1, float &y1, float &x2, float &y2, float theta );
bool make_arcseg( vector<st_line_tag> &vo, int r, int g, int b, int segments, float rad, float start_ang, float stop_ang );
bool has_neighbours( int xx, int yy, int kern_size );
void get_pixel( int xx, int yy, int &r, int &g, int &b );
bool has_subpixels( int xx, int yy );
void set_pixel( int xx, int yy );
void set_pixel3( int xx, int yy );
void set_pixel2( int xx, int yy );
void draw_rect( int xx, int yy, int ww, int hh );
void aa_filter_block( int x1, int y1, int width, int height, float scle, int &dest_wid, int &dest_hei );
void draw_buf_to_wnd( unsigned char * bf, int posx, int posy, int sizex, int sizey );
void filter_lines();
void filter_lines_per_pixel();
void move_obj( float local_tframe );
void draw_obj_create_params(  int px, int py, int col );
void draw_status(  int px, int py, int col );
void obj_vertex_offset( int idx, int offx, int offy, float scalex, float scaley );
void set_rect( int x1, int y1, int x2, int y2, int col );
void set_rect( int x1, int y1, int x2, int y2, int r, int g, int b );
void set_line_update_flag( int idx, bool state );
void set_line_update_flag_if_in_rect( int idx, int rx1, int ry1, int ww, int hh, bool state );
void set_line_update_flag_if_in_rect_old( int idx, int rx1, int ry1, int ww, int hh, bool state );
void set_line_drawn_flag( int idx, bool state );
void draw_rect_src( int xx, int yy, int ww, int hh );
void set_plot_col( int r, int g, int b );
bool check_empty( int xx, int yy );
void set_obj_line_col( vector<st_line_tag> &vo, int r, int g, int b );
void set_obj_line_col_single( vector<st_line_tag> &vo, int idx_line, int r, int g, int b );
//bool obj_overlap( int idx0, int idx1 );
void do_key_obj_create( int key );
bool pressed_key();
float LiangBarsky_maxi(float arr[],int n);
float LiangBarsky_mini(float arr[], int n);
bool LiangBarsky_line_rect_intersect(float xmin, float ymin, float xmax, float ymax, float x1, float y1, float x2, float y2, int dbg_line_idx );
float check_near_ground( vector<float>vdist_considered_near, int &idx_line_closest );
//bool check_near_ground( float length_considered_near, int &idx_closest );
int check_collision( vector<int>&vship_idx, vector<int>&vground_idx, int &fuel_val );
int check_collision_old( vector<int>&vship_idx, vector<int>&vground_idx );
void ship_crash( string sreason );
void ship_landed( string sreason, int fuel_val );
void ship_landed_calc_rating_award_fuel();
void ship_restore( bool rnd_thrust, bool rnd_theta );
bool LineIntersect_onSegment( st_Point p, st_Point q, st_Point r );
int LineIntersect_orientation( st_Point p, st_Point q, st_Point r);
bool LineIntersect_doIntersect( st_Point p1, st_Point q1, st_Point p2, st_Point q2 );
void print_line_coord( unsigned int obj_idx, unsigned int line_idx );
void set_line_type_col( unsigned int obj_idx, unsigned int line_idx );
void calc_objs_line_midpoints( unsigned int obj_idx );
void explode_ship_init_params();
void explode_ship_init_line_params( int line_idx, float dx, float dy, float theta_inc );
void explode_ship_animate( float local_tframe );
void build_explosion_env( int smpl_srate );
void set_clipper_details( int menu_offy );
float get_obj_vel( unsigned int idx );
//void draw_landing_zone_text( unsigned int idx );
void zoom_detent_to_zoom( int zoom_detent_in, float &zoom_in );
void add_7_seg_num( int num, float posx, float posy, int r, int g, int b, vector<st_line_tag>  &vo );
bool check_obj_at_screen_edge( unsigned int idx, float obj_bounds_enlarge_factor, bool &left, bool &right, bool &top, bool &bott );
void crosshair_build();
int highscore_has_been_achieved( int score );
void highscore_enter();

};
