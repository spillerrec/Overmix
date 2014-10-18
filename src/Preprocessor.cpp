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


#include "Preprocessor.hpp"

static bool round_comp( double x, double y )
	{ return std::fabs( x - y ) < std::numeric_limits<double>::epsilon(); }

void Preprocessor::processFile( ImageEx& img ){
	/* /De-telecine
	if( detelecine ){
		auto frame = detelecine->process( &img );
		if( !frame ){
			img = *frame;
			delete frame;
		}
	}
	//*/
	
	//Crop
	if( crop_left != 0 || crop_right != 0 || crop_top != 0 || crop_bottom != 0 )
		img.crop( crop_left, crop_top, crop_right, crop_bottom );
	
	//Deconvolve
	if( deviation > 0.0009 && dev_iterations > 0 )
		img.apply( &Plane::deconvolve_rl, deviation, dev_iterations );
	
	//Scale
	if( !round_comp( scale_x , 1.0 ) || !round_comp( scale_y, 1.0 ) || scale_chroma ) //TODO: scale_chroma?
		img.scaleFactor( scale_x, scale_y );
}

