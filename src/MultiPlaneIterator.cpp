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



MultiPlaneLineIterator::MultiPlaneLineIterator(
		int y, int left, int right, const std::vector<PlaneItInfo> &infos, void *data
	)
	:	x( left ), right( right ), data( data ){
		
		//TODO: estimated reserve for vector?
		lines.reserve( infos.size() );
		
		for( PlaneItInfo info : infos ){
			int local_y = y - info.y;
			if( info.check_y( local_y ) ){
				color_type* row = info.p->scan_line( local_y );
				
				lines.push_back( PlaneLine( row
						,	row + info.p->get_width()
						,	row + ( x - info.x ) )
						);
			}
		}
	}

bool MultiPlaneIterator::iterate_all(){
	int min = INT_MAX, max = INT_MIN;
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
		
		max = info.x > max ? info.x : max;
		min = min > r ? r : min;
	}
	
	new_y( top = y );
	new_x( left = x );
	
	return max == left && min == right;
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

