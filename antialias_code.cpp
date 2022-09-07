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

//antialias_code.cpp
//v1.01	  2020-nov-08	//	


#include "antialias_code.h"


#define cn_deg2rad ( M_PI / 180 )
#define twopi ( M_PI * 2.0 )
#define cn_bytes_per_pixel 3


//fast_mgraph fgph3;

extern vector<float> fgph_vx;
extern vector<float> fgph_vy0;
extern vector<float> fgph_vy1;
extern vector<float> fgph_vy2;








int kern_size = 15;														//odd gives a 1.0 peak
float kern0[ 256 ];
unsigned char *pixbf0 = 0;
unsigned char *pixbf1 = 0;


/*
void plot_mgraph0_aa()
{
fgph_vx.clear();
fgph_vy0.clear();
fgph_vy1.clear();

string s1 = fgph3.label();
s1 += " - plot_mgraph0() - filter kernel";
fgph3.copy_label( s1.c_str() );
fgph3.position( 10, 130 );
fgph3.scale_y( 1.0, 1.0, 1.0, 0.1 );
fgph3.shift_y( 0.0, -0.1, -0.10, -0.2 );
fgph3.font_size( 9 );
fgph3.set_sig_dig( 2 );
fgph3.sample_rect_hints_distancex = 0;
fgph3.sample_rect_hints_distancey = 0;

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
fgph3.plotxy( fgph_vx, fgph_vy0, "drkr", "ofw", "drkb", "blk", "pc srate" );	//3 traces cols, bkgd col, axis col, text col, and trace colour coded labels, see defined colours: 'user_col'

}
*/









bool filter_kernel( en_filter_window_type_tag wnd_type, float *w, int N )
{
int ilow;



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

		default:
			printf( "filter_fir_windowed() - unknown filter window type(wnd): %u\n", wnd_type );
			return 0;
		break;
        }
	}
	
return 1;
}






