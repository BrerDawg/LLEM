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



//rt_code.cpp
//v1.01	2022-aug-25



#include "rt_code.h"



#define twopi (2.0*M_PI)



extern mystr tim1;
extern rtaud rta;																//rtaudio
extern st_rtaud_arg_tag st_rta_arg;											//this is used in audio proc callback to work out chan in/out counts

extern int srate;
extern int framecnt;

extern float time_per_sample;
extern bool audio_started;
extern int audio_source;

extern bool mute;

extern st_iir_2nd_order_tag iir0;
extern st_iir_2nd_order_tag iir1;
extern st_iir_2nd_order_tag iir2;
extern st_iir_2nd_order_tag iir20;
extern st_iir_2nd_order_tag iir30;
extern st_iir_2nd_order_tag iir40;

extern float gain_iir0;
extern float gain_iir1;
extern float gain_iir2;
extern float gain_iir20;
extern float gain_iir30;
extern float gain_iir40;

extern freeverb_reverb *fvb;


extern float *revb_bf0;
extern float *revb_bf1;
extern int revb_size0;
extern int revb_size1;
extern int revb_rd0;
extern int revb_wr0;
extern int revb_rd1;
extern int revb_wr1;

extern float *revb_bf10;
extern float *revb_bf11;
extern int revb_size10;
extern int revb_size11;
extern int revb_rd10;
extern int revb_wr10;
extern int revb_rd11;
extern int revb_wr11;

extern audio_formats af0;
extern uint64_t audptr;
extern bool aud_loaded;
extern double dfract_audptr;
extern float play_speed;

extern bool grph_bf_loaded;	
extern float grph_bf0[];
extern float grph_bf1[];
extern float grph_bf2[];

extern float audio_gain;
extern float audio_thrust_gain;
extern float audio_thrust_gain_slewed;
//extern float audio_thrust_gain_slewed_zeroed;
extern float audio_explosion_gain;

extern int pref_audio_bass_boost;
extern int pref_audio_gain;
extern int pref_audio_reverb_on;
extern int pref_audio_reverb_room_size;
extern int pref_audio_reverb_damping;
extern int pref_audio_reverb_width;
extern int pref_audio_reverb_dry_wet;
extern int pref_audio_bounce_gain;
extern int pref_audio_explosion_gain;
extern int pref_audio_fryingpan_plosive_gain;
extern int pref_distortion_gain;


extern st_envlp_tag st_envlp0[];
extern st_envlp_tag st_envlp1[];
extern st_envlp_tag st_envlp2[];

extern float bounce_amp;

int proc_cnt = 0;
float freq0 =  framecnt * 1.0;
float freq1 = srate / 24;
float freq2 = 30;					//for bounce sound
float theta0 = 0;
float theta1 = 0;
float theta2 = 0;
float theta0_inc;
float theta1_inc;
float theta2_inc;

extern void env_inc( st_envlp_tag *st );

extern int iflag_nan;

//generates a rnd num between -1.0 -> 1.0
double rnd()
{
double drnd =  (double)( RAND_MAX / 2 - rand() ) / (double)( RAND_MAX / 2 );

return drnd;
}




int pulse_cnt = 0;
bool pulse = 0;

float pole00 = 0;					//voltage controlled filter
float pole01 = 0;
float pole02 = 0;
float pole03 = 0;
   
float polein00 = 0;
float polein01 = 0;
float polein02 = 0;
float polein03 = 0;

float pole10 = 0;
float pole11 = 0;
float pole12 = 0;
float pole13 = 0;
   
float polein10 = 0;
float polein11 = 0;
float polein12 = 0;
float polein13 = 0;




//-------------------- realtime audio proc -----------------------------
int cb_audio_proc_rtaudio( void *bf_out, void *bf_in, int frames, double streamTime, RtAudioStreamStatus status, void *arg_in )
{
bool vb = 1;

double dt = tim1.time_passed( tim1.ns_tim_start );
tim1.time_start( tim1.ns_tim_start );

if( !(proc_cnt % 300) )
	{
	if(vb) printf( "cb_audio_proc_rtaudio() - dt: %f, frames: %d, iflag_nan: %d\n", dt, frames, iflag_nan );
	
	}
proc_cnt++;
	

if ( status ) std::cout << "cb_audio_proc_rtaudio() - Stream over/underflow detected." << std::endl;



float *bfin = (float *) bf_in;											//depends on which 'audio_format' data type, refer: 'RtAudioFormat'
float *bfout = (float *) bf_out;


st_rtaud_arg_tag *arg = (st_rtaud_arg_tag*) arg_in;

//st_osc_params_tag *usr = (st_osc_params_tag*) arg->usr_ptr;


float freq_fit_in_framecnt = 1.0 / (framecnt * time_per_sample );		//freq that gives 1 exact cycle in framecnt

freq0 = freq_fit_in_framecnt * 10;
freq1 = freq_fit_in_framecnt * 20;

theta0_inc = freq0 * twopi / srate;
theta1_inc = freq1 * twopi / srate;
theta2_inc = freq2 * twopi / srate;


float f0 = 0;
float f1 = 0;

if( pref_audio_bounce_gain < 0 ) pref_audio_bounce_gain = 0;
if( pref_audio_bounce_gain > 100 ) pref_audio_bounce_gain = 100;
float prf_audio_bounce_gain = (pref_audio_bounce_gain / 100.0f);


if( pref_audio_explosion_gain < 0 ) pref_audio_explosion_gain = 0;
if( pref_audio_explosion_gain > 150 ) pref_audio_explosion_gain = 150;
float prf_audio_explosion_gain = (pref_audio_explosion_gain / 100.0f);


if( pref_audio_gain < 0 ) pref_audio_gain = 0;
if( pref_audio_gain > 200 ) pref_audio_gain = 200;
float prf_audio_gain = audio_gain * (pref_audio_gain / 100.0f);

if( pref_audio_reverb_room_size < 0 ) pref_audio_reverb_room_size = 0;
if( pref_audio_reverb_room_size > 100 ) pref_audio_reverb_room_size = 100;
fvb->room_size = (pref_audio_reverb_room_size / 100.0f);


if( pref_audio_reverb_damping < 0 ) pref_audio_reverb_damping = 0;
if( pref_audio_reverb_damping > 100 ) pref_audio_reverb_damping = 100;
fvb->damping = (pref_audio_reverb_damping / 100.0f);


if( pref_audio_reverb_width < 0 ) pref_audio_reverb_width = 0;
if( pref_audio_reverb_width > 100 ) pref_audio_reverb_width = 100;
fvb->width = (pref_audio_reverb_width / 100.0f);


if( pref_audio_reverb_dry_wet < 0 ) pref_audio_reverb_dry_wet = 0;
if( pref_audio_reverb_dry_wet > 100 ) pref_audio_reverb_dry_wet = 100;
fvb->effect_mix = (pref_audio_reverb_dry_wet / 100.0f);


if( pref_audio_fryingpan_plosive_gain < 0 ) pref_audio_fryingpan_plosive_gain = 0;
if( pref_audio_fryingpan_plosive_gain > 150 ) pref_audio_fryingpan_plosive_gain = 150;
float prf_audio_fryingpan_plosive_gain = (pref_audio_fryingpan_plosive_gain / 100.0f);


if( pref_distortion_gain < 0 ) pref_distortion_gain = 0;
if( pref_distortion_gain > 150 ) pref_distortion_gain = 150;
float prf_distortion_gain = (pref_distortion_gain / 100.0f);

bool bcheck_nan = 1;


int pinterleve = 0;
for ( int i = 0; i < framecnt; i++ )
	{
	pulse_cnt++;
	
	if( pulse_cnt > 48000/2 )
		{
		pulse = 1;
		if( pulse_cnt >= 48000 )
			{
			pulse_cnt = 0;
			}	
		}
	else{
		pulse = 0;
		}

	
	if( audio_thrust_gain_slewed < audio_thrust_gain ) audio_thrust_gain_slewed += 0.001f;
	if( audio_thrust_gain_slewed > audio_thrust_gain ) audio_thrust_gain_slewed -= 0.001f;

//	audio_thrust_gain_slewed_zeroed = audio_thrust_gain_slewed;
//	if( (audio_thrust_gain_slewed_zeroed >= -0.00001f) && ( audio_thrust_gain_slewed_zeroed <= 0.00001f ) ) audio_thrust_gain_slewed_zeroed = 0.0f;	//ensure there is a zero val, helps avoid plosive noise leaking through when plosive distortion is set high
/*
	if( audio_source == 0 )
		{
		float famp = 0.5;
		f0 = famp * cosf( theta0 );
		f1 = famp * cosf( theta1 );

		theta0 += theta0_inc;
		if( theta0 >= twopi ) theta0 -= twopi;

		theta1 += theta1_inc;
		if( theta1 >= twopi ) theta1 -= twopi;
		}

	if( audio_source == 1 )
		{
		float famp = 0.2;
		f0 = famp * rnd();
		f1 = famp * rnd();

		theta0 += theta0_inc;
		if( theta0 >= twopi ) theta0 -= twopi;

		theta1 += theta1_inc;
		if( theta1 >= twopi ) theta1 -= twopi;
		}
*/


	if( audio_source == 2 )
		{
		//--- bounce ---
		float bounce_f0 = 0;
		float bounce_f1 = 0;
		if( 1 )
			{
			bounce_f0 = cosf( theta2 ) * prf_audio_bounce_gain * bounce_amp * st_envlp2->cur_amp;
			bounce_f1 = bounce_f0;

			theta2 += theta2_inc;
			if( theta2 >= twopi ) theta2 -= twopi;
			}



		if( 0 )
			{
			float famp = 0.5;
			f0 = famp * cosf( theta0 );
			f1 = famp * cosf( theta1 );

			theta0 += theta0_inc;
			if( theta0 >= twopi ) theta0 -= twopi;

			theta1 += theta1_inc;
			if( theta1 >= twopi ) theta1 -= twopi;
			}

	if( 0 )
		{
		if( aud_loaded )
			{
			float famp = 0.2;
			f0 = famp * af0.pch0[audptr];
			f1 = famp * af0.pch1[audptr];
			audptr++;
			if( audptr >= af0.sizech0 ) audptr = 0;
			}
		}



	if( 1 )
		{
//		float famp = audio_thrust_gain_slewed_zeroed;
		float famp = 1.0f;

		float fnoise0 = rnd()*0.5;
		float fnoise1 = rnd()*0.5;

		float fexpl0;
		float fexpl1;
		float fthrust_lf0;
		float fthrust_lf1;
		float fthrust_frypan0;
		float fthrust_frypan1;
		float fdist0;
		float fdist1;

		if( pulse )
			{
//			fnoise0 = 0;
//			fnoise1 = 0;
			}

		float fsrc0 = famp * fnoise0;
		float fsrc1 = famp * fnoise1;

		float fsum0;
		float fsum1;


//-----	thrust lowpass filter - lf
		f0 = fsrc0;
		f1 = fsrc1;
		
		filter_iir_2nd_order_2ch( f0, f1, iir0 );
//-----	

		fsum0 = f0 * gain_iir0;
		fsum1 = f1 * gain_iir0;

//-----	thrust lowpass filter - hf
		f0 = fsrc0;
		f1 = fsrc1;
		
		filter_iir_2nd_order_2ch( f0, f1, iir1 );
//-----	

		fsum0 += f0 * gain_iir1;
		fsum1 += f1 * gain_iir1;



//-----	thrust subsonic filter
		f0 = fsrc0;
		f1 = fsrc1;
		filter_iir_2nd_order_2ch( f0, f1, iir2 );
		
		if( pref_audio_bass_boost )
			{
			if( audio_thrust_gain_slewed >= 0.5f )						//only bring in this filter at higher thrust levels
				{
				float gain_lf = (audio_thrust_gain_slewed - 0.5f);
				
				fsum0 += f0 * 9.5f * gain_lf * gain_iir2;
				fsum1 += f1 * 9.5f * gain_lf * gain_iir2;
				}
			}
//-----		

		f0 = fsum0;
		f1 = fsum1;

		fthrust_lf0 = f0;
		fthrust_lf1 = f1;

/*
		if( bcheck_nan )
			{
			if( std::isnan(fthrust_lf0) )
				{
				fthrust_lf0 = 0;
				iflag_nan |= 1;
		//		printf("std::isnan with 'fthrust_lf0'\n" );
				}

			if( std::isnan(fthrust_lf1) )
				{
				fthrust_lf1 = 0;
				iflag_nan |= 2;
		//		printf("std::isnan with 'fthrust_lf1'\n" );
				}
			}
*/


		float fclamp_lev = 1.0f;
		float fnoise_clamp0 = fnoise0;
		float fnoise_clamp1 = fnoise1;

/*
		if( fnoise_clamp0 > fclamp_lev) fnoise_clamp0 = fclamp_lev;				
		if( fnoise_clamp0 < -fclamp_lev) fnoise_clamp0 = -fclamp_lev;

		if( fnoise_clamp1 > fclamp_lev) fnoise_clamp1 = fclamp_lev;
		if( fnoise_clamp1 < -fclamp_lev) fnoise_clamp1 = -fclamp_lev;
*/

		fclamp_lev = 1.0f;
		float fnoise_clamp3 = fnoise0;
		float fnoise_clamp4 = fnoise1;	
/*
		if( fnoise_clamp3 > fclamp_lev) fnoise_clamp3 = fclamp_lev;				
		if( fnoise_clamp3 < -fclamp_lev) fnoise_clamp3 = -fclamp_lev;

		if( fnoise_clamp4 > fclamp_lev) fnoise_clamp4 = fclamp_lev;
		if( fnoise_clamp4 < -fclamp_lev) fnoise_clamp4 = -fclamp_lev;
*/




//------thrust frying pan noise gen/vcf -------

		float svf_in = 25.5f*fnoise_clamp0;
		if( svf_in > 1.5f ) svf_in = 1.5f;
		if( svf_in < -0.9f ) svf_in = -0.9f;


		float fc = (0.8f + 0.67f*fnoise_clamp3 );						//filter cutoff, lower is more filtering
//		if( fc > 10.5f ) fc = 10.5f;
//		if( fc < 0.2f ) fc = 0.2f;
		float res = svf_in;
		float f = fc * 1.16f;
		if( f > 1.0f ) f = 1.0f;
//		if( f < 1e-1f ) f = 1e-1f;
		float fb = res  * ( 1.0f - 0.15f * f * f );                  	//filter resonance


		svf_in -= pole03 * fb;                                      		//feedback
		svf_in *= 0.35013f * ( f * f ) * ( f * f );

	   
		pole00 = svf_in + 0.3f * polein00 + ( 1.0f - f) * pole00;
		polein00 = svf_in;
		
		pole01 = pole00 + 0.3f * polein01 + ( 1.0f - f) * pole01;
		polein01 = pole00;

		pole02 = pole01 + 0.3f * polein02 + ( 1.0f - f) * pole02;
		polein02 = pole01;

		pole03 = pole02 + 0.3f * polein03 + ( 1.0f - f) * pole03;
		polein03 = pole02;

//-----------------------------------------


//------thrust frying pan noise gen/vcf -------
//http://www.musicdsp.org/archive.php?classid=3#196

//Moog VCF, variation 2

//Type : 24db resonant lowpass
//References : CSound source code, Stilson/Smith CCRMA paper., Timo Tossavainen (?) version

//fc = cutoff, nearly linear [0,1] -> [0, fs/2]
//res = resonance [0, 4] -> [no resonance, self-oscillation]

		svf_in = 25.5f*fnoise_clamp1;
		if( svf_in > 1.5f ) svf_in = 1.5f;
		if( svf_in < -0.9f ) svf_in = -0.9f;

		fc = (0.8f + 0.67f*fnoise_clamp4);								//filter cutoff, lower is more filtering
//		if( fc > 10.5f ) fc = 10.5f;
//		if( fc < 0.2f ) fc = 0.2f;
		res = svf_in;
		f = fc * 1.16f;
		if( f > 1.0f ) f = 1.0f;
//		if( f < 1e-1f ) f = 1e-1f;
		fb = res  * ( 1.0f - 0.15f * f * f );                  			//filter resonance


		svf_in -= pole13 * fb;                                      		//feedback
		svf_in *= 0.35013f * ( f * f ) * ( f * f );


		pole10 = svf_in + 0.3f * polein10 + ( 1.0f - f) * pole10;
		polein10 = svf_in;
		
		pole11 = pole10 + 0.3f * polein11 + ( 1.0f - f) * pole11;
		polein11 = pole10;

		pole12 = pole11 + 0.3f * polein12 + ( 1.0f - f) * pole12;
		polein12 = pole11;

		pole13 = pole12 + 0.3f * polein13 + ( 1.0f - f) * pole13;
		polein13 = pole12;

		fthrust_frypan0 = pole03*0.2f;
		fthrust_frypan1 = pole13*0.2f;
		
		
//		if( fthrust_frypan0 > 0.99f) fthrust_frypan0 = 0.99f;										//clipper
//		if( fthrust_frypan0 < -0.99f) fthrust_frypan0 = -0.99f;

//		if( fthrust_frypan1 > 0.99f) fthrust_frypan1 = 0.99f;
//		if( fthrust_frypan1 < -0.99f) fthrust_frypan1 = -0.99f;
//-----------------------------------------

/*
if( bcheck_nan )
	{
	if( std::isnan(fthrust_frypan0) )
		{
		pole00 = 0;
		pole01 = 0;
		pole02 = 0;
		pole03 = 0;
		fthrust_frypan0 = 0;
//		printf("std::isnan with 'fthrust_frypan0'\n" );
		iflag_nan |= 4;
		}
	
	if( std::isnan(fthrust_frypan1) )
		{
		pole10 = 0;
		pole11 = 0;
		pole12 = 0;
		pole13 = 0;
		fthrust_frypan1 = 0;
//		printf("std::isnan with 'fthrust_frypan1'\n" );
		iflag_nan |= 8;
		}
	}
*/



//-------------- thrust frypan distortion ---------------
//refer: amsynth
		float fdist_in = fthrust_frypan0*audio_thrust_gain_slewed;
//		float compress = 1.05f + 0.3 *audio_thrust_gain_slewed;
		float compress = 0.85f + 0.5 * prf_distortion_gain;
		
		if( compress < 0.1f ) compress = 0.1f;
		if( compress > 1.45f ) compress = 1.45f;
		
		float pwr = 1.0f - compress;
		if ( pwr == 0.0f ) pwr = 0.01f;

		float fsign;
		if( fdist_in < 0.0f ) fsign = -1.0f;
		else fsign = 1.0f;

		fdist_in *= fsign;
		
		fdist_in = 0.1*powf( fdist_in, pwr );
		
		fdist0 = fdist_in;
//-----------------------------------------


//-------------- thrust frypan distortion ---------------
		fdist_in = fthrust_frypan1*audio_thrust_gain_slewed;//15.6;

		pwr = 1.0f - compress;
		if ( pwr == 0 ) pwr = 0.01f;

		if( fdist_in < 0.0f ) fsign = -1.0f;
		else fsign = 1.0f;

		fdist_in *= fsign;

		fdist_in = 0.1*powf( fdist_in, pwr );

		fdist1 = fdist_in;
//-----------------------------------------

/*
if( bcheck_nan )
	{
	if( std::isnan(fdist0) )
		{		
		fdist0 = 0;
		iflag_nan |= 16;
//		printf("std::isnan with 'fdist0'\n" );
		}

	if( std::isnan(fdist1) )
		{
		fdist1 = 0;
		iflag_nan |= 32;
//		printf("std::isnan with 'fdist1'\n" );
		}
	}
*/


		fthrust_frypan0 += fdist0;
		fthrust_frypan1 += fdist1;

		fthrust_frypan0 *= prf_audio_fryingpan_plosive_gain;
		fthrust_frypan1 *= prf_audio_fryingpan_plosive_gain;


//-----	frying pan noise filter -------
		fthrust_frypan0 *= audio_thrust_gain_slewed;
		fthrust_frypan1 *= audio_thrust_gain_slewed;

		filter_iir_2nd_order_2ch( fthrust_frypan0, fthrust_frypan1, iir40 );	//frying pan noise filter
//-------------------------------------

/*
if( bcheck_nan )
	{
	if( std::isnan(audio_thrust_gain_slewed) )
		{		
		audio_thrust_gain_slewed = 0;
		iflag_nan |= 64;
//		printf("std::isnan with 'audio_thrust_gain_slewed'\n" );
		}

	if( std::isnan(audio_thrust_gain_slewed) )
		{
		audio_thrust_gain_slewed = 0;
		iflag_nan |= 128;
//		printf("std::isnan with 'audio_thrust_gain_slewed'\n" );
		}
	}
*/



if( bcheck_nan )
	{
	if( std::isnan(fthrust_frypan0) )
		{
		fthrust_frypan0 = 0;
		iir40.delay0[0]= 0;
		iir40.delay0[1]= 0;
		iflag_nan |= 256;
//		printf("std::isnan with 'fthrust_frypan0-2'\n" );
		}

	if( std::isnan(fthrust_frypan1) )
		{
		
		fthrust_frypan1 = 0;
		iir40.delay1[0]= 0;
		iir40.delay1[1]= 0;
		iflag_nan |= 512;
//		printf("std::isnan with 'fthrust_frypan1-2'\n" );
		}
	}




//-----	explosion	

		fsum0 = pole03*0.5 + fnoise0*0.3;
		fsum1 = pole13*0.5 + fnoise1*0.3;

		filter_iir_2nd_order_2ch( fsum0, fsum1, iir20 );					//pass higher freq

		f0 = prf_audio_explosion_gain * audio_explosion_gain * 0.7 * st_envlp0->cur_amp * fsum0 * gain_iir20;
		f1 = prf_audio_explosion_gain * audio_explosion_gain * 0.7 * st_envlp0->cur_amp * fsum1 * gain_iir20;


		fsum0 = fnoise0;
		fsum1 = fnoise1;

		filter_iir_2nd_order_2ch( fsum0, fsum1, iir30 );				//pass lower freq

		f0 += prf_audio_explosion_gain * audio_explosion_gain * 0.8 * st_envlp1->cur_amp * fsum0 * gain_iir30;
		f1 += prf_audio_explosion_gain * audio_explosion_gain * 0.8 * st_envlp1->cur_amp * fsum1 * gain_iir30;

		
		env_inc( st_envlp0 );
		env_inc( st_envlp1 );
		env_inc( st_envlp2 );

		fexpl0 = f0;
		fexpl1 = f1;
		
//-----




		f0 = (fthrust_lf0 + fthrust_frypan0)*audio_thrust_gain_slewed;	//thrust mix and gain
		f1 = (fthrust_lf1 + fthrust_frypan1)*audio_thrust_gain_slewed;

		f0 += bounce_f0 + fexpl0;										//mix
		f1 += bounce_f1 + fexpl1;

/*
if( bcheck_nan )
	{
	if( std::isnan(fexpl0) )
		{		
		fexpl0 = 0;
		iflag_nan |= 1024;
//		printf("std::isnan with 'fexpl0'\n" );
		}
	
	if( std::isnan(fexpl1) )
		{		
		fexpl1 = 0;
		iflag_nan |= 2048;
//		printf("std::isnan with 'fexpl1'\n" );
		}

	if( std::isnan(bounce_f0) )
		{		
		bounce_f0 = 0;
		iflag_nan |= 4096;
//		printf("std::isnan with 'bounce_f0'\n" );
		}
	
	if( std::isnan(bounce_f1) )
		{		
		bounce_f1 = 0;
		iflag_nan |= 8192;
//		printf("std::isnan with 'bounce_f1'\n" );
		}


	if( std::isnan(f0) )
		{		
		f0 = 0;
		iflag_nan |= 16384;
//		printf("std::isnan with 'pre reverb f0'\n" );
		}
	
	if( std::isnan(f1) )
		{		
		f1 = 0;
		iflag_nan |= 32768;
//		printf("std::isnan with 'pre reverb f1'\n" );
		}
	}
*/


		if( f0 > 0.99f) f0 = 0.99f;										//clipper
		if( f0 < -0.99f) f0 = -0.99f;

		if( f1 > 0.99f) f1 = 0.99f;
		if( f1 < -0.99f) f1 = -0.99f;


		if( pref_audio_reverb_on ) fvb->process( f0, f1 );

		f0 *= prf_audio_gain;											//master gain
		f1 *= prf_audio_gain;

		if( f0 > 0.99f) f0 = 0.99f;										//clipper
		if( f0 < -0.99f) f0 = -0.99f;

		if( f1 > 0.99f) f1 = 0.99f;
		if( f1 < -0.99f) f1 = -0.99f;

//f0 = fthrust_frypan0*prf_audio_gain;
//f1 = fthrust_frypan1*prf_audio_gain;


		}


if( bcheck_nan )
	{
	if( std::isnan(f0) )
		{
		f0 = 0;
		iflag_nan |= 65536;
//		printf("std::isnan with 'f0'\n" );
		}

	if( std::isnan(f1) )
		{
		f1 = 0;
		iflag_nan |= 131072;
//		printf("std::isnan with 'f1'\n" );
		}
	}




	if( mute )
		{
		f0 = 0;
		f1 = 0;
		}




/*

float frevb_in0 = f0;
float frevb_in1 = f1;


//-----	reverb0

//		fsum0 = revb_bf0[revb_rd0];
//		fsum1 = revb_bf1[revb_rd1];
		fsum0 = revb_bf10[revb_rd10];
		fsum1 = revb_bf11[revb_rd11];
		
		revb_bf0[revb_wr0] = fsum0*0.2f + frevb_in0;
		revb_bf1[revb_wr1] = fsum1*0.3f + frevb_in1;
		
		f0 = fsum0;
		f1 = fsum1;
		
		revb_rd0 += 1;
		if( revb_rd0 >= revb_size0 ) revb_rd0 = 0;
		revb_rd1 += 1;
		if( revb_rd1 >= revb_size1 ) revb_rd1 = 0;

		revb_wr0 += 1;
		if( revb_wr0 >= revb_size0 ) revb_wr0 = 0;
		revb_wr1 += 1;
		if( revb_wr1 >= revb_size1 ) revb_wr1 = 0;
//-----		

//-----	reverb10

		fsum0 = revb_bf10[revb_rd10];
		fsum1 = revb_bf11[revb_rd11];
		
		revb_bf10[revb_wr10] = fsum0*0.2f + frevb_in0;
		revb_bf11[revb_wr11] = fsum1*0.3f + frevb_in1;
		
		f0 += fsum0;
		f1 += fsum1;
		
		revb_rd10 += 1;
		if( revb_rd10 >= revb_size10 ) revb_rd10 = 0;
		revb_rd11 += 1;
		if( revb_rd11 >= revb_size11 ) revb_rd11 = 0;

		revb_wr10 += 1;
		if( revb_wr10 >= revb_size10 ) revb_wr10 = 0;
		revb_wr11 += 1;
		if( revb_wr11 >= revb_size11 ) revb_wr11 = 0;
//-----		

*/



//		filter_iir_2nd_order( f1, iir1 );
		}

/*
	if( grph_bf_loaded == 0 )
		{
		grph_bf0[ i ] = f0 + 0.5;
		grph_bf1[ i ] = f1 + 0.5;
		}
*/
	bfout[ pinterleve ] = f0;
	pinterleve++;
	bfout[ pinterleve ] = f1;
	pinterleve++;

	}

if( grph_bf_loaded == 0 ) grph_bf_loaded = 1;

return 0;
}

