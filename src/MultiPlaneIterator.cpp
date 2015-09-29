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
#include <algorithm>

using namespace Overmix;


MultiPlaneLineIterator::MultiPlaneLineIterator(
		int y, int left, int right, const std::vector<PlaneItInfo> &infos, void *data
	)
	:	x( left ), right( right ), data( data ){
		
		//TODO: estimated reserve for vector?
		lines.reserve( infos.size() );
		
		for( PlaneItInfo info : infos ){
			int local_y = y - info.y;
			if( info.check_y( local_y ) ){
				color_type* row = info.p.scan_line( local_y );
				
				lines.push_back( PlaneLine( row
						,	row + info.p.get_width()
						,	row + ( x - info.x ) )
						);
			}
		}
	}

void MultiPlaneIterator::iterate( int x, int y, int right, int bottom ){
	this->bottom = bottom;
	this->right = right;
	new_y( top = y );
	new_x( left = x );
}

//Find minimum and maximum for info using the accessor item
#define info_min( item ) (std::min_element( infos.begin(), infos.end(), \
	[]( const PlaneItInfo& i1, const PlaneItInfo& i2 ){ return i1.item < i2.item;  } )->item)
#define info_max( item ) (std::max_element( infos.begin(), infos.end(), \
	[]( const PlaneItInfo& i1, const PlaneItInfo& i2 ){ return i1.item < i2.item;  } )->item)

/** Prepares iteration for the whole range
 *  @return true if all images covers the whole width of the resulting image */
bool MultiPlaneIterator::iterate_all(){
	iterate( info_min( x ), info_min( y ), info_max( right() ), info_max( bottom() ) );
	return info_max( x ) == left && info_min( right() ) == right;
}

/** Prepares iteration for the subset which is covered by all images */
void MultiPlaneIterator::iterate_shared()
	{ iterate( info_max( x ), info_max( y ), info_min( right() ), info_min( bottom() ) ); }


void MultiPlaneIterator::new_x( int x ){
	this->x = x;
	
	for( auto& info : infos ){
		int local_x = x - info.x;
		info.row = ( info.row_start && info.check_x( local_x ) ) ? info.row_start + local_x : 0;
	}
}


void MultiPlaneIterator::new_y( int y ){
	this->y = y;
	
	for( auto& info : infos ){
		int local_y = y - info.y;
		info.row_start = info.check_y( local_y ) ? info.p.scan_line( local_y ) : 0;
		info.row = 0;
	}
}

