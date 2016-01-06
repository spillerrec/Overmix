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

using namespace Overmix;

static color_type apply_threshold( color_type in, void* data ){
	color_type threshold = *(color_type*)data;
	return in > threshold ? color::WHITE : color::BLACK;
}

void Plane::binarize_threshold( color_type threshold )
	{ for_each_pixel( &apply_threshold, &threshold ); }

static color_type adaptive_threshold( color_type row1, color_type row2, void* data ){
	color_type c = *(color_type*)data;
	color_type threshold = row2 - c; //TODO: This could be an issue if color_type is unsigned!
	return row1 > threshold ? color::WHITE : color::BLACK;
}
void Plane::binarize_adaptive( unsigned amount, color_type threshold ){
	Plane blurred = blur_box( amount, amount );
	for_each_pixel( blurred, &adaptive_threshold, &threshold );
}

void Plane::binarize_dither(){
	color_type threshold = (color::WHITE - color::BLACK) / 2 + color::BLACK;
	
	std::vector<double> errors( size.width()+1, 0 );
	for( auto row : *this )
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

Plane Plane::dilate( int size ) const{
	Plane output( *this );
	if( size <= 0 )
		return output;
	
	output.for_each_pixel( blur_box( size, size ), [](color_type row1, color_type row2, void*){
			return ( row1 > color::WHITE*0.5 && row2 >= color::WHITE )
				?	color::WHITE : color::BLACK;
		} );
	return output;
}

