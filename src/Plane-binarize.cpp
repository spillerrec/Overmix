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

#include "Plane.hpp"
//#include <algorithm> //For min#include "color.hpp"

#include <QtConcurrent>
#include <QDebug>

using namespace std;

static void apply_threshold( const SimplePixel& pixel ){
	color_type threshold = *(color_type*)pixel.data;
	*pixel.row1 = *pixel.row1 > threshold ? color::MAX_VAL : color::MIN_VAL;
}

void Plane::binarize_threshold( color_type threshold ){
	for_each_pixel( &apply_threshold, &threshold );
}

void Plane::binarize_dither(){
	color_type threshold = (color::MAX_VAL - color::MIN_VAL) / 2;
	
	vector<double> errors( width+1, 0 );
	for( unsigned iy=0; iy<get_height(); ++iy ){
		color_type* row = scan_line( iy );
		for( unsigned ix=0; ix<get_width(); ++ix ){
			double wanted = row[ix] + errors[ix];
			color_type binary = wanted > threshold ? color::MAX_VAL : color::MIN_VAL;
			
			double error = wanted - binary;
			errors[ix] = error / 4;
			errors[ix+1] += error / 2;
			if( ix )
				errors[ix-1] += error / 2;
			row[ix] = binary;
		}
	}
}

