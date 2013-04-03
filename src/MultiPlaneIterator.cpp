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


#include "MultiPlaneIterator.hpp"

#include <cmath>
#include <climits>

void MultiPlaneIterator::iterate_all(){
	x = y = INT_MAX;
	bottom = right = INT_MIN;
	for( unsigned i=0; i<infos.size(); i++ ){
		PlaneItInfo& info( infos[i] );
		int r = info.x + info.p->get_width() - 1;
		int b = info.y + info.p->get_height() - 1;
		
		x = info.x < x ? info.x : x;
		y = info.y < y ? info.y : y;
		right = right < r ? r : right;
		bottom = bottom < b ? b : bottom;
	}
	
	new_y( top = y );
	new_x( left = x );
}

void MultiPlaneIterator::iterate_shared(){
	x = y = INT_MIN;
	bottom = right = INT_MAX;
	for( unsigned i=0; i<infos.size(); i++ ){
		PlaneItInfo& info( infos[i] );
		int r = info.x + info.p->get_width() - 1;
		int b = info.y + info.p->get_height() - 1;
		
		x = info.x > x ? info.x : x;
		y = info.y > y ? info.y : y;
		right = right > r ? r : right;
		bottom = bottom > b ? b : bottom;
	}
	
	new_y( top = y );
	new_x( left = x );
}


void MultiPlaneIterator::new_x( int x ){
	this->x = x;
	
	for( unsigned i=0; i<infos.size(); i++ ){
		PlaneItInfo& info( infos[i] );
		int local_x = x - info.x;
		info.row = ( info.row_start && info.check_x( local_x ) ) ? info.row_start + local_x : 0;
	}
}


void MultiPlaneIterator::new_y( int y ){
	this->y = y;
	
	for( unsigned i=0; i<infos.size(); i++ ){
		PlaneItInfo& info( infos[i] );
		int local_y = y - info.y;
		info.row_start = info.check_y( local_y ) ? info.p->scan_line( local_y ) : 0;
		info.row = 0;
	}
}

void MultiPlaneIterator::write_average(){
	unsigned avg = 0, amount = 0;
	
	for( unsigned i=1; i<infos.size(); i++ ){
		if( infos[i].row ){
			avg += *infos[i].row;
			amount++;
		}
	}
	
	if( amount )
		*infos[0].row = avg / amount;
}