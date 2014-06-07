/*
	This file is part of Overmix.

	Overmix is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Overmix is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Overmix.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "color.hpp"


color color::yuvToRgb( double kr, double kg, double kb, bool gamma ){
	double y  = asDouble( r );
	double cb = asDouble( g );
	double cr = asDouble( b );
	
	//Remove foot- and head-room
	y = (y - (16 / 255.0)) / ( (255 - 16 - (255-235)) / 255.0 );
	cb = (cb - (16 / 255.0)) / ( (255 - 16 - (255-240)) / 255.0 );
	cr = (cr - (16 / 255.0)) / ( (255 - 16 - (255-240)) / 255.0 );
	
	//Don't let it outside the allowed range
	y = (y < 0 ) ? 0 : (y > 1 ) ? 1 : y;
	cb = (cb < 0 ) ? 0 : (cb > 1 ) ? 1 : cb;
	cr = (cr < 0 ) ? 0 : (cr > 1 ) ? 1 : cr;
	
	//Move chroma range
	cb -= 0.5;
	cr -= 0.5;
	
	//Convert to R'G'B'
	double rr = y + 2*(1-kr) * cr;
	double rb = y + 2*(1-kb) * cb;
	double rg = y - 2*kr*(1-kr)/kg * cr - 2*kb*(1-kb)/kg * cb;
	
	//Don't let it outside the allowed range
	//Should not happen, so we can probably remove this later
	rr = (rr < 0 ) ? 0 : (rr > 1 ) ? 1 : rr;
	rg = (rg < 0 ) ? 0 : (rg > 1 ) ? 1 : rg;
	rb = (rb < 0 ) ? 0 : (rb > 1 ) ? 1 : rb;
	
	//Gamma correction
	if( gamma ){
		rg = ycbcr2srgb( rg );
		rr = ycbcr2srgb( rr );
		rb = ycbcr2srgb( rb );
	}
	
	return color( fromDouble(rr), fromDouble(rg), fromDouble(rb), a );
}

