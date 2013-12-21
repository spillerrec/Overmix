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


#include "DifferenceRender.hpp"
#include "Plane.hpp"
#include "color.hpp"

#include "MultiPlaneIterator.hpp"

static void diff_pixel( MultiPlaneLineIterator &it ){
	//Calculate sum
	unsigned amount = 0;
	precision_color_type sum = 0;
	for( unsigned i=1; i<it.size(); i++ ){
		if( it.valid( i ) ){
			sum += it[i];
			amount++;
		}
	}
	
	if( amount ){
		precision_color_type avg = sum / amount;
		
		//Calculate sum of the difference from average
		precision_color_type diff_sum = 0;
		for( unsigned i=1; i<it.size(); i++ ){
			if( it.valid( i ) ){
				unsigned long d = abs( avg - it[i] );
				diff_sum += d;
			}
		}
		
		it[0] = diff_sum / amount;
	}
}
DifferenceRender::pixel_func* DifferenceRender::pixel() const{
	return &diff_pixel;
}
