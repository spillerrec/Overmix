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


#include "AverageAligner.hpp"
#include "SimpleRender.hpp"

QPointF AverageAligner::min_point() const{
	if( images.size() == 0 )
		return QPointF(0,0);
	
	QPointF min = images[0].pos;
	for( auto img : images ){
		if( img.pos.x() < min.x() )
			min.setX( img.pos.x() );
		if( img.pos.y() < min.y() )
			min.setY( img.pos.y() );
	}
	
	return min;
}

void AverageAligner::align( AProcessWatcher* watcher ){
	if( count() == 0 )
		return;
	
	raw = true;
	
	if( watcher )
		watcher->set_total( count() );
	
 	images[0].pos = QPointF( 0,0 );
	for( unsigned i=1; i<count(); i++ ){
		if( watcher )
			watcher->set_current( i );
		
		ImageEx* img = SimpleRender( SimpleRender::FOR_MERGING ).render( *this, i );
		if( !img )
			qFatal( "NoOOO" );
		
		ImageOffset offset = find_offset( *((*img)[0]), *(images[i].image) );
		images[i].pos = QPointF( offset.distance_x, offset.distance_y ) + min_point();
		
		delete img;
	}
	
	raw = false;
}

