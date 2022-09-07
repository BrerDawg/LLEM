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

//line_clip_code.cpp
//v1.01	  2021-oct-23	//	


#include "line_clip_code.h"



line_clip::line_clip()
{
clip_vportleft = -500;						//upward is positive, for: 'line_clip()', 'clip_poly()' algorithm
clip_vportright= 500;
clip_vporttop = 500;						//NOTE: 'clip_vporttop' here is in an increasing direction up the screen
clip_vportbot = -500;
}




//for cohen-sutherland line clip algorithm
int line_clip::compute_outcode_int( int xx, int yy )
{
int oc = 0;
 
if ( yy > clip_vporttop ) oc |= en_lineclip_top;
else if ( yy < clip_vportbot ) oc |= en_lineclip_bot;
 
 
if ( xx > clip_vportright ) oc |= en_lineclip_right;
else if ( xx < clip_vportleft ) oc |= en_lineclip_left;
 
return oc;
}





//for cohen-sutherland line clip algorithm
//returns 1 if part or all of line visible, else 0
bool line_clip::line_clip_int( int &x1_in, int &y1_in, int &x2_in, int &y2_in )
{
int accept;
int done;
int outcode1, outcode2;

accept = 0;
done = 0;

float x1 = x1_in;
float x2 = x2_in;
float y1 = y1_in;
float y2 = y2_in;

outcode1 = compute_outcode_int ( x1, y1 );
outcode2 = compute_outcode_int ( x2, y2 );
do
	{
	if (outcode1 == 0 && outcode2 == 0)
		{
		accept = 1;
		done = 1;
		}
else if (outcode1 & outcode2)
	{
	done = 1;
	}
else{
	float x, y;
	int outcode_ex = outcode1 ? outcode1 : outcode2;
	if (outcode_ex & en_lineclip_top)
		{
		x = x1 + (x2 - x1) * ( clip_vporttop - y1 ) / ( y2 - y1 );
		y = clip_vporttop;
		}

	else if ( outcode_ex & en_lineclip_bot )
			{
			x = x1 + ( x2 - x1 ) * ( clip_vportbot - y1 ) / ( y2 - y1 );
			y = clip_vportbot;
			}
	else if (outcode_ex & en_lineclip_right )
			{
			y = y1 + ( y2 - y1 ) * ( clip_vportright - x1 ) / ( x2 - x1 );
			x = clip_vportright;
			}
	else{
		y = y1 + ( y2 - y1 ) * ( clip_vportleft - x1 ) / ( x2 - x1 );
		x = clip_vportleft;
		}
	if ( outcode_ex == outcode1 )
		{
		x1 = x;
		y1 = y;
		outcode1 = compute_outcode_int( x1, y1 );
		}
	else
		{
		x2 = x;
		y2 = y;
		outcode2 = compute_outcode_int( x2, y2 );
		}
	}	
} while ( done == 0 );

x1_in = x1;
y1_in = y1;
x2_in = x2;
y2_in = y2;

return accept;
}
