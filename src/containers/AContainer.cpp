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


#include "AContainer.hpp"

#include "../ImageEx.hpp"

const Plane& AContainer::plane( unsigned img_index, unsigned p_index ) const{
	//TODO: Get rid of this
	return image( img_index )[p_index];
}

QRect AContainer::size() const{
	QRectF total;
	
	for( unsigned i=0; i<count(); i++ )
		total = total.united( QRectF( pos(i), QSizeF( plane(i).get_width(), plane(i).get_height() ) ) );
	
	//Round so that we only increase size
	total.setLeft( floor( total.left() ) );
	total.setTop( floor( total.top() ) );
	total.setRight( ceil( total.right() ) );
	total.setBottom( ceil( total.bottom() ) );
	//TODO: just return a QRectF and let the caller deal with the rounding
	
	return total.toRect();
}

QPointF AContainer::minPoint() const{
	if( count() == 0 )
		return QPointF(0,0);
	
	QPointF min = pos( 0 );
	for( unsigned i=0; i<count(); i++ ){
		if( pos(i).x() < min.x() )
			min.setX( pos(i).x() );
		if( pos(i).y() < min.y() )
			min.setY( pos(i).y() );
	}
	
	return min;
}

void AContainer::resetPosition(){
	for( unsigned i=0; i<count(); i++ )
		setPos( i, { 0.0, 0.0 } );
}

void AContainer::offsetAll( double dx, double dy ){
	QPointF offset( dx, dy );
	for( unsigned i=0; i<count(); i++ )
		setPos( i, pos( i ) + offset );
}
