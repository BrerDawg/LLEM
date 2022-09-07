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

//freeverb_code.cpp

//freeverb implementation, a reverb arrangement by Jezar at Dreampoint

//refer: Schroeder Reverberator
//refer: Synthesis ToolKit in C++ (STK)



//v1.01		2021-dec-03			//simple/inefficient implementation, not coded with simd, set compiler optimizer to max

#include "freeverb_code.h"




freeverb_reverb::freeverb_reverb( unsigned int ref_id_in )
{
verbose = 0;
	
inited = 0;
ref_id = ref_id_in;
sample_rate = 48000;

bf_lp00 = 0;
bf_cb00 = 0;

bf_lp01 = 0;
bf_cb01 = 0;

bf_lp02 = 0;
bf_cb02 = 0;

bf_lp03 = 0;
bf_cb03 = 0;

bf_lp04 = 0;
bf_cb04 = 0;

bf_lp05 = 0;
bf_cb05 = 0;

bf_lp06 = 0;
bf_cb06 = 0;

bf_lp07 = 0;
bf_cb07 = 0;

bf_ap00 = 0;
bf_ap01 = 0;
bf_ap02 = 0;
bf_ap03 = 0;


bf_lp10 = 0;
bf_cb10 = 0;

bf_lp11 = 0;
bf_cb11 = 0;

bf_lp12 = 0;
bf_cb12 = 0;

bf_lp13 = 0;
bf_cb13 = 0;

bf_lp14 = 0;
bf_cb14 = 0;

bf_lp15 = 0;
bf_cb15 = 0;

bf_lp16 = 0;
bf_cb16 = 0;

bf_lp17 = 0;
bf_cb17 = 0;

bf_ap10 = 0;
bf_ap11 = 0;
bf_ap12 = 0;
bf_ap13 = 0;



room_size = 0.7f;
damping = 1.0f;
width = 1.0f;
effect_mix = 1.0f;

}



freeverb_reverb::~freeverb_reverb()
{
}





bool freeverb_reverb::create( unsigned int sample_rate_in )
{
bool vb = verbose;

sample_rate = sample_rate_in;

if(vb) printf("freeverb_reverb::create() - ref_id %u, sample_rate %u\n", ref_id, sample_rate );

float srate_factor = sample_rate / 44100.0f;              //scale delayline sizes if srate is not 44K1Hz

int lpf_delay = -1;

//---------- left -------------
//---
//lpfs and combs
bf_lp00 = new float[1];
bf_lp00[0] = 0;

cb_siz00 = (1617 + lpf_delay) * srate_factor;
//printf("freeverb_reverb::create() - cb_siz00 %d\n", cb_siz00 );
//getchar();
bf_cb00 = new float[cb_siz00];
cb_wr00 = 0;
memset( bf_cb00, 0, cb_siz00*sizeof(float) );

bf_lp01 = new float[1];
bf_lp01[0] = 0;

cb_siz01 = (1557 + lpf_delay) * srate_factor;
bf_cb01 = new float[cb_siz01];
cb_wr01 = 0;
memset( bf_cb01, 0, cb_siz01*sizeof(float) );



bf_lp02 = new float[1];
bf_lp02[0] = 0;

cb_siz02 = (1491 + lpf_delay) * srate_factor;
bf_cb02 = new float[cb_siz02];
cb_wr02 = 0;
memset( bf_cb02, 0, cb_siz02*sizeof(float) );



bf_lp03 = new float[1];
bf_lp03[0] = 0;

cb_siz03 = (1422 + lpf_delay) * srate_factor;
bf_cb03 = new float[cb_siz03];
cb_wr03 = 0;
memset( bf_cb03, 0, cb_siz03*sizeof(float) );



bf_lp04 = new float[1];
bf_lp04[0] = 0;

cb_siz04 = (1356 + lpf_delay) * srate_factor;
bf_cb04 = new float[cb_siz04];
cb_wr04 = 0;
memset( bf_cb04, 0, cb_siz04*sizeof(float) );



bf_lp05 = new float[1];
bf_lp05[0] = 0;

cb_siz05 = (1277 + lpf_delay) * srate_factor;
bf_cb05 = new float[cb_siz05];
cb_wr05 = 0;
memset( bf_cb05, 0, cb_siz05*sizeof(float) );



bf_lp06 = new float[1];
bf_lp06[0] = 0;

cb_siz06 = (1188 + lpf_delay) * srate_factor;
bf_cb06 = new float[cb_siz06];
cb_wr06 = 0;
memset( bf_cb06, 0, cb_siz06*sizeof(float) );



bf_lp07 = new float[1];
bf_lp07[0] = 0;

cb_siz07 = (1116 + lpf_delay) * srate_factor;
bf_cb07 = new float[cb_siz07];
cb_wr07 = 0;
memset( bf_cb07, 0, cb_siz07*sizeof(float) );
//---




//---
//allpasses
ap_siz00 = 225 * srate_factor;
bf_ap00 = new float[ap_siz00];
ap_wr00 = 0;
memset( bf_ap00, 0, ap_siz00*sizeof(float) );

ap_siz01 = 556 * srate_factor;
bf_ap01 = new float[ap_siz01];
ap_wr01 = 0;
memset( bf_ap01, 0, ap_siz01*sizeof(float) );

ap_siz02 = 441 * srate_factor;
bf_ap02 = new float[ap_siz02];
ap_wr02 = 0;
memset( bf_ap02, 0, ap_siz02*sizeof(float) );

ap_siz03 = 341 * srate_factor;
bf_ap03 = new float[ap_siz03];
ap_wr03 = 0;
memset( bf_ap03, 0, ap_siz03*sizeof(float) );
//---
//--------------------------



//---------- right -------------
//---
int right_adj = 23;
//lpfs and combs
bf_lp10 = new float[1];
bf_lp10[0] = 0;

cb_siz10 = (1617 + lpf_delay) * srate_factor + right_adj;
bf_cb10 = new float[cb_siz10];
cb_wr10 = 0;
memset( bf_cb10, 0, cb_siz10*sizeof(float) );

bf_lp11 = new float[1];
bf_lp11[0] = 0;

cb_siz11 = (1557 + lpf_delay) * srate_factor + right_adj;
bf_cb11 = new float[cb_siz11];
cb_wr11 = 0;
memset( bf_cb11, 0, cb_siz11*sizeof(float) );



bf_lp12 = new float[1];
bf_lp12[0] = 0;

cb_siz12 = (1491 + lpf_delay) * srate_factor + right_adj;
bf_cb12 = new float[cb_siz12];
cb_wr12 = 0;
memset( bf_cb12, 0, cb_siz12*sizeof(float) );



bf_lp13 = new float[1];
bf_lp13[0] = 0;

cb_siz13 = (1422 + lpf_delay) * srate_factor + right_adj;
bf_cb13 = new float[cb_siz13];
cb_wr13 = 0;
memset( bf_cb13, 0, cb_siz13*sizeof(float) );



bf_lp14 = new float[1];
bf_lp14[0] = 0;

cb_siz14 = (1356 + lpf_delay) * srate_factor + right_adj;
bf_cb14 = new float[cb_siz14];
cb_wr14 = 0;
memset( bf_cb14, 0, cb_siz14*sizeof(float) );



bf_lp15 = new float[1];
bf_lp15[0] = 0;

cb_siz15 = (1277 + lpf_delay) * srate_factor + right_adj;
bf_cb15 = new float[cb_siz15];
cb_wr15 = 0;
memset( bf_cb15, 0, cb_siz15*sizeof(float) );



bf_lp16 = new float[1];
bf_lp16[0] = 0;

cb_siz16 = (1188 + lpf_delay) * srate_factor + right_adj;
bf_cb16 = new float[cb_siz16];
cb_wr16 = 0;
memset( bf_cb16, 0, cb_siz16*sizeof(float) );



bf_lp17 = new float[1];
bf_lp17[0] = 0;

cb_siz17 = (1116 + lpf_delay) * srate_factor + right_adj;
bf_cb17 = new float[cb_siz17];
cb_wr17 = 0;
memset( bf_cb17, 0, cb_siz17*sizeof(float) );
//---



//---
//allpasses
ap_siz10 = (225) * srate_factor + right_adj;
bf_ap10 = new float[ap_siz10];
ap_wr10 = 0;
memset( bf_ap10, 0, ap_siz10*sizeof(float) );

ap_siz11 = (556) * srate_factor + right_adj;
bf_ap11 = new float[ap_siz11];
ap_wr11 = 0;
memset( bf_ap11, 0, ap_siz11*sizeof(float) );

ap_siz12 = (441) * srate_factor + right_adj;
bf_ap12 = new float[ap_siz12];
ap_wr12 = 0;
memset( bf_ap12, 0, ap_siz12*sizeof(float) );

ap_siz13 = (341) * srate_factor + right_adj;
bf_ap13 = new float[ap_siz13];
ap_wr13 = 0;
memset( bf_ap13, 0, ap_siz13*sizeof(float) );
//---
//--------------------------

inited = 1;

return 1;
}



//make sure 'process()' is not still being called
void freeverb_reverb::destroy()
{
bool vb = verbose;
if(vb) printf("freeverb_reverb::destroy() - ref_id %u\n", ref_id );

if( !inited ) return;

inited = 0;

//----
if( bf_lp00 ) delete[] bf_lp00;
bf_lp00 = 0;

if( bf_cb00 ) delete[] bf_cb00;
bf_cb00 = 0;


if( bf_lp01 ) delete[] bf_lp01;
bf_lp01 = 0;

if( bf_cb01 ) delete[] bf_cb01;
bf_cb01 = 0;


if( bf_lp02 ) delete[] bf_lp02;
bf_lp02 = 0;

if( bf_cb02 ) delete[] bf_cb02;
bf_cb02 = 0;


if( bf_lp03 ) delete[] bf_lp03;
bf_lp03 = 0;

if( bf_cb03 ) delete[] bf_cb03;
bf_cb03 = 0;


if( bf_lp04 ) delete[] bf_lp04;
bf_lp04 = 0;

if( bf_cb04 ) delete[] bf_cb04;
bf_cb04 = 0;


if( bf_lp05 ) delete[] bf_lp05;
bf_lp05 = 0;

if( bf_cb05 ) delete[] bf_cb05;
bf_cb05 = 0;


if( bf_lp06 ) delete[] bf_lp06;
bf_lp06 = 0;

if( bf_cb06 ) delete[] bf_cb06;
bf_cb06 = 0;


if( bf_lp07 ) delete[] bf_lp07;
bf_lp07 = 0;

if( bf_cb07 ) delete[] bf_cb07;
bf_cb07 = 0;
//----

//----
if( bf_ap00 ) delete[] bf_ap00;
bf_ap00 = 0;

if( bf_ap01 ) delete[] bf_ap01;
bf_ap01 = 0;

if( bf_ap02 ) delete[] bf_ap02;
bf_ap02 = 0;

if( bf_ap03 ) delete[] bf_ap03;
bf_ap03 = 0;
//----




//----
if( bf_lp10 ) delete[] bf_lp10;
bf_lp10 = 0;

if( bf_cb10 ) delete[] bf_cb10;
bf_cb10 = 0;


if( bf_lp11 ) delete[] bf_lp11;
bf_lp11 = 0;

if( bf_cb11 ) delete[] bf_cb11;
bf_cb11 = 0;


if( bf_lp12 ) delete[] bf_lp12;
bf_lp12 = 0;

if( bf_cb12 ) delete[] bf_cb12;
bf_cb12 = 0;


if( bf_lp13 ) delete[] bf_lp13;
bf_lp13 = 0;

if( bf_cb13 ) delete[] bf_cb13;
bf_cb13 = 0;


if( bf_lp14 ) delete[] bf_lp14;
bf_lp14 = 0;

if( bf_cb14 ) delete[] bf_cb14;
bf_cb14 = 0;


if( bf_lp15 ) delete[] bf_lp15;
bf_lp15 = 0;

if( bf_cb15 ) delete[] bf_cb15;
bf_cb15 = 0;


if( bf_lp16 ) delete[] bf_lp16;
bf_lp16 = 0;

if( bf_cb16 ) delete[] bf_cb16;
bf_cb16 = 0;


if( bf_lp17 ) delete[] bf_lp17;
bf_lp17 = 0;

if( bf_cb17 ) delete[] bf_cb17;
bf_cb17 = 0;
//----

//----
if( bf_ap10 ) delete[] bf_ap10;
bf_ap00 = 0;

if( bf_ap11 ) delete[] bf_ap11;
bf_ap01 = 0;

if( bf_ap12 ) delete[] bf_ap12;
bf_ap02 = 0;

if( bf_ap13 ) delete[] bf_ap13;
bf_ap03 = 0;
//----


}








//the form of the comb filter including its one pole lowpass filter (total: 1x8 in parallel for each channel)

//                  .------> lpfc_outxx
//             (v0) |
//                  |                            (v1)       (zxx)
//      in0>-- add -'---> Zcomb >---- b0 -->add ---> Zlpf >---.
//              ^                            ^                |               
//              |                            |                v
//              |                            |                |
//              |                            ^  one pole lfp  |
//              ^                            |                |              
//              |    comb filter             |                |
//              |                            |                |   
//              |                            |..<.. -a1 ..<....
//              |                                             |
//              |                                             |
//              '---------<------- ff <-------------<---------'
//
//  ff is feedback value governed by: room size
//  b0 and a0 are the one pole lpf coeffs governed by: damping factor





//the form of the all pass filter (total: 1x4 in series for each channel )

//gg is fixed at 0.5
//
//                  .---------------> -1 ------->------.
//                  |                                  |
//                  |                                  |
//                  |                                  |
//                  | (v0)                             v 
//      Xin>-- add -'---> Zallpass >---.--> 1+gg ---->add ---> ap_outxx
//              ^                      |               
//              |                      v
//              |                      |
//              ^                      |              
//              |                      |
//              '------<--- gg ----<---'




//for impulse debug purposes
//int fvb_cnt = 0;
//float fvb_ip = 0.5;

void freeverb_reverb::process( float &f0, float &f1 )
{
if( !inited ) return;


float b0;					//lpf
float a1;					//lpf
float ff = 0.84;			//comb feedback




float damp = damping * 0.4;                                   			//scale damping

b0 = 1.0f - damp;                                       			 	//set one pole lpf response using damping
a1 = -damp;


float scale_room = 0.28;
float offset_room = 0.7;
float room = room_size * scale_room + offset_room;             			//scale room

ff = room;

float gg = 0.5f;
float one_plus_gg = 1.0f + gg;

float ipgain = 0.015;

float in0 = f0;
float in1 = f1;


float v0, v1;

//printf("ff %f b0 %f a1 %f\n", ff, b0, a1 );

//in0 = 1.0;

//in0 = fvb_ip;
//in1 = fvb_ip;

float mono = ( in0 + in1 ) * ipgain;

//------------------------- left ---------------------------
//----------
//8 lowpass comb filters
float lpf_out00 = mono + ff * bf_lp00[0];						//v0
v1 = bf_cb00[cb_wr00] * b0 + (-a1 * bf_lp00[0]);
bf_lp00[0] = v1;
bf_cb00[cb_wr00] = lpf_out00;


/*
//impulse testing
if( fvb_cnt < 2000 )
	{
printf("%03d %f %f\n", fvb_cnt, fvb_ip, (float)v0 );
		
	}
fvb_ip = 0.0f;
*/

float lpf_out01 = mono + ff * bf_lp01[0];						//v0
v1 = bf_cb01[cb_wr01] * b0 + (-a1 * bf_lp01[0]);
bf_lp01[0] = v1;
bf_cb01[cb_wr01] = lpf_out01;


float lpf_out02 = mono + ff * bf_lp02[0];						//v0
v1 = bf_cb02[cb_wr02] * b0 + (-a1 * bf_lp02[0]);
bf_lp02[0] = v1;
bf_cb02[cb_wr02] = lpf_out02;


float lpf_out03 = mono + ff * bf_lp03[0];						//v0
v1 = bf_cb03[cb_wr03] * b0 + (-a1 * bf_lp03[0]);
bf_lp03[0] = v1;
bf_cb03[cb_wr03] = lpf_out03;


float lpf_out04 = mono + ff * bf_lp04[0];						//v0
v1 = bf_cb04[cb_wr04] * b0 + (-a1 * bf_lp04[0]);
bf_lp04[0] = v1;
bf_cb04[cb_wr04] = lpf_out04;


float lpf_out05 = mono + ff * bf_lp05[0];						//v0
v1 = bf_cb05[cb_wr05] * b0 + (-a1 * bf_lp05[0]);
bf_lp05[0] = v1;
bf_cb05[cb_wr05] = lpf_out05;


float lpf_out06 = mono + ff * bf_lp06[0];						//v0
v1 = bf_cb06[cb_wr06] * b0 + (-a1 * bf_lp06[0]);
bf_lp06[0] = v1;
bf_cb06[cb_wr06] = lpf_out06;


float lpf_out07 = mono + ff * bf_lp07[0];						//v0
v1 = bf_cb07[cb_wr07] * b0 + (-a1 * bf_lp07[0]);
bf_lp07[0] = v1;
bf_cb07[cb_wr07] = lpf_out07;
//----------




//----------
//4 allpass filters
v0 = bf_ap00[ap_wr00] * gg + lpf_out00 + lpf_out01 + lpf_out02 + lpf_out03 + lpf_out04 + lpf_out05 + lpf_out06 + lpf_out07;

float ap_out00 = bf_ap00[ap_wr00] * one_plus_gg + (-v0);
bf_ap00[ap_wr00] = v0;




v0 = bf_ap01[ap_wr01] * gg + ap_out00;

float ap_out01 = bf_ap01[ap_wr01] * one_plus_gg + (-v0);
bf_ap01[ap_wr01] = v0;



v0 = bf_ap02[ap_wr02] * gg + ap_out01;

float ap_out02 = bf_ap02[ap_wr02] * one_plus_gg + (-v0);
bf_ap02[ap_wr02] = v0;



v0 = bf_ap03[ap_wr03] * gg + ap_out02;

float ap_out03 = bf_ap03[ap_wr03] * one_plus_gg + (-v0);
bf_ap03[ap_wr03] = v0;
//----------


//----------------------------------------------------------



//------------------------- right ---------------------------
//----------
//8 lowpass comb filters
float lpf_out10 = mono + ff * bf_lp10[0];						//v0
v1 = bf_cb10[cb_wr10] * b0 + (-a1 * bf_lp10[0]);
bf_lp10[0] = v1;
bf_cb10[cb_wr10] = lpf_out10;


float lpf_out11 = mono + ff * bf_lp11[0];						//v0
v1 = bf_cb11[cb_wr11] * b0 + (-a1 * bf_lp11[0]);
bf_lp11[0] = v1;
bf_cb11[cb_wr11] = lpf_out11;


float lpf_out12 = mono + ff * bf_lp12[0];						//v0
v1 = bf_cb12[cb_wr12] * b0 + (-a1 * bf_lp12[0]);
bf_lp12[0] = v1;
bf_cb12[cb_wr12] = lpf_out12;


float lpf_out13 = mono + ff * bf_lp13[0];						//v0
v1 = bf_cb13[cb_wr13] * b0 + (-a1 * bf_lp13[0]);
bf_lp13[0] = v1;
bf_cb13[cb_wr13] = lpf_out13;


float lpf_out14 = mono + ff * bf_lp14[0];						//v0
v1 = bf_cb14[cb_wr14] * b0 + (-a1 * bf_lp14[0]);
bf_lp14[0] = v1;
bf_cb14[cb_wr14] = lpf_out14;


float lpf_out15 = mono + ff * bf_lp15[0];						//v0
v1 = bf_cb15[cb_wr15] * b0 + (-a1 * bf_lp15[0]);
bf_lp15[0] = v1;
bf_cb15[cb_wr15] = lpf_out15;


float lpf_out16 = mono + ff * bf_lp16[0];						//v0
v1 = bf_cb16[cb_wr16] * b0 + (-a1 * bf_lp16[0]);
bf_lp16[0] = v1;
bf_cb16[cb_wr16] = lpf_out16;


float lpf_out17 = mono + ff * bf_lp17[0];						//v0
v1 = bf_cb17[cb_wr17] * b0 + (-a1 * bf_lp17[0]);
bf_lp17[0] = v1;
bf_cb17[cb_wr17] = lpf_out17;
//----------



//----------
//4 allpass filters
v0 = bf_ap10[ap_wr10] * gg + lpf_out10 + lpf_out11 + lpf_out12 + lpf_out13 + lpf_out14 + lpf_out15 + lpf_out16 + lpf_out17;

float ap_out10 = bf_ap10[ap_wr10] * one_plus_gg + (-v0);
bf_ap10[ap_wr10] = v0;



v0 = bf_ap11[ap_wr11] * gg + ap_out10;

float ap_out11 = bf_ap11[ap_wr11] * one_plus_gg + (-v0);
bf_ap11[ap_wr11] = v0;



v0 = bf_ap12[ap_wr12] * gg + ap_out11;

float ap_out12 = bf_ap12[ap_wr12] * one_plus_gg + (-v0);
bf_ap12[ap_wr12] = v0;



v0 = bf_ap13[ap_wr13] * gg + ap_out12;

float ap_out13 = bf_ap13[ap_wr13] * one_plus_gg + (-v0);
bf_ap13[ap_wr13] = v0;


//----------
//----------------------------------------------------------


//this is from stk FreeVerb::update()

float scale_wet = 3;
float scale_dry = 2;

float wet = scale_wet * effect_mix;
float dry = scale_dry * ( 1.0 - effect_mix );

wet /= ( wet + dry );
dry /= ( wet + dry );

float wet0 = wet * ( width / 2.0 + 0.5 );
float wet1 = wet * ( 1.0 - width )  / 2.0;


v0 = ap_out03 * wet0 + ap_out13 * wet1 + in0 * dry;
v1 = ap_out13 * wet0 + ap_out03 * wet1 + in1 * dry;


f0 = v0;
f1 = v1;




//---- inc indexes -----
//combs
cb_wr00++;
if( cb_wr00 >= cb_siz00 ) cb_wr00 = 0;

cb_wr01++;
if( cb_wr01 >= cb_siz01 ) cb_wr01 = 0;


cb_wr02++;
if( cb_wr02 >= cb_siz02 ) cb_wr02 = 0;


cb_wr03++;
if( cb_wr03 >= cb_siz03 ) cb_wr03 = 0;


cb_wr04++;
if( cb_wr04 >= cb_siz04 ) cb_wr04 = 0;


cb_wr05++;
if( cb_wr05 >= cb_siz05 ) cb_wr05 = 0;


cb_wr06++;
if( cb_wr06 >= cb_siz06 ) cb_wr06 = 0;


cb_wr07++;
if( cb_wr07 >= cb_siz07 ) cb_wr07 = 0;


//all passes
ap_wr00++;
if( ap_wr00 >= ap_siz00 ) ap_wr00 = 0;

ap_wr01++;
if( ap_wr01 >= ap_siz01 ) ap_wr01 = 0;

ap_wr02++;
if( ap_wr02 >= ap_siz02 ) ap_wr02 = 0;

ap_wr03++;
if( ap_wr03 >= ap_siz03 ) ap_wr03 = 0;
//--------




//---- inc indexes -----
//combs
cb_wr10++;
if( cb_wr10 >= cb_siz10 ) cb_wr10 = 0;

cb_wr11++;
if( cb_wr11 >= cb_siz11 ) cb_wr11 = 0;


cb_wr12++;
if( cb_wr12 >= cb_siz12 ) cb_wr12 = 0;


cb_wr13++;
if( cb_wr13 >= cb_siz13 ) cb_wr13 = 0;


cb_wr14++;
if( cb_wr14 >= cb_siz14 ) cb_wr14 = 0;


cb_wr15++;
if( cb_wr15 >= cb_siz15 ) cb_wr15 = 0;


cb_wr16++;
if( cb_wr16 >= cb_siz16 ) cb_wr16 = 0;


cb_wr17++;
if( cb_wr17 >= cb_siz17 ) cb_wr17 = 0;


//all passes
ap_wr10++;
if( ap_wr10 >= ap_siz10 ) ap_wr10 = 0;

ap_wr11++;
if( ap_wr11 >= ap_siz11 ) ap_wr11 = 0;

ap_wr12++;
if( ap_wr12 >= ap_siz12 ) ap_wr12 = 0;

ap_wr13++;
if( ap_wr13 >= ap_siz13 ) ap_wr13 = 0;
//--------


//fvb_cnt++;		//for impulse testing
}










void freeverb_reverb::process( float fi0, float fi1, float &fo0, float &fo1 )
{
if( !inited ) return;

process( fi0, fi1 );
fo0 = fi0;
fo1 = fi1;
}




//yet to be tested
void freeverb_reverb::process_block( float *bf0, float *bf1, unsigned int bfsiz )
{
if( !inited ) return;

if( bfsiz == 0 ) return;

for( int i = 0; i < bfsiz; i++ )
	{
	float f0 = bf0[i];
	float f1 = bf1[i];
	process( f0, f1 );
	bf0[i] = f0;
	bf1[i] = f1;
	}

}



