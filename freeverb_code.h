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

//freeverb_code.h
//---- v1.01

#ifndef freeverb_code_h
#define freeverb_code_h

#include "GCProfile.h"

using namespace std;





class freeverb_reverb
{
private:
bool inited;

unsigned int sample_rate;
unsigned int ref_id;

//left channel
float *bf_lp00;					//lpf
float *bf_cb00;					//comb filter
int cb_siz00;
int cb_wr00;

float *bf_lp01;
float *bf_cb01;	
int cb_siz01;
int cb_wr01;

float *bf_lp02;
float *bf_cb02;	
int cb_siz02;
int cb_wr02;

float *bf_lp03;
float *bf_cb03;	
int cb_siz03;
int cb_wr03;

float *bf_lp04;
float *bf_cb04;	
int cb_siz04;
int cb_wr04;

float *bf_lp05;
float *bf_cb05;	
int cb_siz05;
int cb_rd05;
int cb_wr05;

float *bf_lp06;
float *bf_cb06;	
int cb_siz06;
int cb_wr06;

float *bf_lp07;
float *bf_cb07;	
int cb_siz07;
int cb_wr07;

float *bf_ap00;					//all pass
int ap_siz00;
int ap_wr00;

float *bf_ap01;
int ap_siz01;
int ap_wr01;

float *bf_ap02;
int ap_siz02;
int ap_wr02;

float *bf_ap03;
int ap_siz03;
int ap_wr03;



//right channel
float *bf_lp10;					//lpf
float *bf_cb10;					//comb filter
int cb_siz10;
int cb_wr10;

float *bf_lp11;
float *bf_cb11;
int cb_siz11;
int cb_wr11;

float *bf_lp12;
float *bf_cb12;
int cb_siz12;
int cb_wr12;

float *bf_lp13;
float *bf_cb13;
int cb_siz13;
int cb_wr13;

float *bf_lp14;
float *bf_cb14;
int cb_siz14;
int cb_wr14;

float *bf_lp15;
float *bf_cb15;
int cb_siz15;
int cb_wr15;

float *bf_lp16;
float *bf_cb16;
int cb_siz16;
int cb_wr16;

float *bf_lp17;
float *bf_cb17;
int cb_siz17;
int cb_wr17;

float *bf_ap10;					//all pass
int ap_siz10;
int ap_rd10;
int ap_wr10;

float *bf_ap11;
int ap_siz11;
int ap_wr11;

float *bf_ap12;
int ap_siz12;
int ap_wr12;

float *bf_ap13;
int ap_siz13;
int ap_wr13;

public:
bool verbose;
float room_size;				//0.0-->1.0, high is larger room
float damping; 					//0.0-->1.0, higher add more filtering
float width;					//0.0-->1.0, 1.0 sounds more natural
float effect_mix;				//0.0-->1.0,  0.0 is fully dry,  1.0 is fully wet


public:
freeverb_reverb( unsigned int ref_id_in );
~freeverb_reverb();

bool create( unsigned int sample_rate_in );
void destroy();
void process( float &f0, float &f1 );
void process( float fi0, float fi1, float &fo0, float &fo1 );
void process_block( float *bf0, float *bf1, unsigned int bfsiz );
};



#endif
