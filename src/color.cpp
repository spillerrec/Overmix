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

#include <vector>

static std::vector<color_type> generate_gamma(){
	std::vector<color_type> temp( UINT16_MAX );
	
	for( int_fast32_t i=0; i<UINT16_MAX; i++ ){
		auto v = color::asDouble( i );
		v = color::linear2sRgb( color::ycbcr2linear( v ) );
		temp[i] = color::fromDouble( v );
	}
	
	return temp;
}

auto gamma_lookup = generate_gamma();

static double stretch( double val, int min_8, int max_8 ){
	double min = min_8 / 255.0, max = max_8 / 255.0;
	return (val-min) / (max-min);
}

static double limit( double val, int min_8, int max_8 ){
	double min = min_8 / 255.0, max = max_8 / 255.0;
	return val * (max-min) + min;
}

color color::ycbcrToRgb( double kr, double kg, double kb, bool gamma, bool swing ){
	double y = asDouble( r ), cb = asDouble( g ), cr = asDouble( b );
	
	//Remove foot- and head-room
	if( swing ){
		y  = stretch(  y, 16, 235 );
		cb = stretch( cb, 16, 240 );
		cr = stretch( cr, 16, 240 );
	}
	
	//Don't let it outside the allowed range
	y  = (y  < 0 ) ? 0 : (y  > 1 ) ? 1 : y;
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
	if( gamma )
		return color(
				gamma_lookup[ fromDouble(rr) ]
			,	gamma_lookup[ fromDouble(rg) ]
			,	gamma_lookup[ fromDouble(rb) ]
			,	a
			);
	
	return color( fromDouble(rr), fromDouble(rg), fromDouble(rb), a );
}


color color::rgbToYcbcr( double kr, double kg, double kb, bool gamma, bool swing ){
	double rr = asDouble( r ), rg = asDouble( g ), rb = asDouble( b );
	
	double y  = kr*rr + kg*rg + kb*rb;
	double cb = (rb-y) / (1-kb) * 0.5 + 0.5;
	double cr = (rr-y) / (1-kr) * 0.5 + 0.5;
	
	//add studio-swing
	if( swing ){
		y  = limit(  y, 16, 235 );
		cb = limit( cb, 16, 240 );
		cr = limit( cr, 16, 240 );
	}
	
	//TODO: gamma
	return color( fromDouble(y), fromDouble(cb), fromDouble(cr), a );
}
