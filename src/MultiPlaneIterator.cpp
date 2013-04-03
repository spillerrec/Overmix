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
#include <QObject>


MultiPlaneIterator::MultiPlaneIterator( std::vector<PlaneItInfo> info, unsigned w, unsigned h, int x, int y )
	:	infos( info ){
	width = w;
	height = h;
	left = x;
	
	new_y( y );
	new_x( x );
	
	qDebug( "Multi (%p): size %d, w %d, h %d, x %d, y %d", this, info.size(), w, h, x, y );
	for( int i=0; i<info.size(); i++ )
		qDebug( "\tPlane: x %d, y %d", infos[i].x, infos[i].y );
}


void MultiPlaneIterator::new_x( int x ){
	current_x = x;
	
	for( unsigned i=0; i<infos.size(); i++ ){
		PlaneItInfo& info( infos[i] );
		int local_x = info.x + x;
		info.row = ( info.row_start && info.check_x( local_x ) ) ? info.row_start + local_x : 0;
	}
//	qDebug( "new_x: %d", x );
}


void MultiPlaneIterator::new_y( int y ){
	current_y = y;
	
	for( unsigned i=0; i<infos.size(); i++ ){
		PlaneItInfo& info( infos[i] );
		int local_y = info.y + y;
		info.row_start = info.check_y( local_y ) ? info.p->scan_line( local_y ) : 0;
		info.row = 0;
	}
//	qDebug( "new_y: %d", y );
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
	else
		qDebug( "Missing amount, have %d loaded", infos.size() );
}