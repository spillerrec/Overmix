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
#include "../color.hpp"

#include <QtConcurrent>
#include <QDebug>

using namespace std;

static void apply_threshold( const SimplePixel& pixel ){
	color_type threshold = *(color_type*)pixel.data;
	*pixel.row1 = *pixel.row1 > threshold ? color::WHITE : color::BLACK;
}

void Plane::binarize_threshold( color_type threshold ){
	for_each_pixel( &apply_threshold, &threshold );
}

static void adaptive_threshold( const SimplePixel& pixel ){
	color_type c = *(color_type*)pixel.data;
	color_type threshold = *pixel.row2 - c;
	*pixel.row1 = *pixel.row1 > threshold ? color::WHITE : color::BLACK;
}
void Plane::binarize_adaptive( unsigned amount, color_type threshold ){
	Plane blurred = blur_box( amount, amount );
	for_each_pixel( blurred, &adaptive_threshold, &threshold );
}

void Plane::binarize_dither(){
	color_type threshold = (color::WHITE - color::BLACK) / 2 + color::BLACK;
	
	vector<double> errors( width+1, 0 );
	for( unsigned iy=0; iy<get_height(); ++iy ){
		color_type* row = scan_line( iy );
		for( unsigned ix=0; ix<get_width(); ++ix ){
			double wanted = row[ix] + errors[ix];
			color_type binary = wanted > threshold ? color::WHITE : color::BLACK;
			
			double error = wanted - binary;
			errors[ix] = error / 4;
			errors[ix+1] += error / 2;
			if( ix )
				errors[ix-1] += error / 2;
			row[ix] = binary;
		}
	}
}

Plane Plane::dilate( int size ) const{
	Plane blurred = blur_box( size, size );
	Plane copy(blurred);
	blurred.for_each_pixel( copy, [](const SimplePixel& pixel){
			if( *pixel.row2 > color::WHITE*0.5 )
				*pixel.row1 = *pixel.row1 < color::WHITE ? color::BLACK : color::WHITE;
			else
				*pixel.row1 = color::BLACK;
		} );
	return blurred;
}

