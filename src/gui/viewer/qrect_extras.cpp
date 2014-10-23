/*
	This file is part of imgviewer.

	imgviewer is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	imgviewer is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with imgviewer.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "qrect_extras.h"

QRect constrain( QRect outer, QRect inner, bool keep_aspect ){
	QRect result( inner );
	
	//Constrain size
	if( result.width() > outer.width() )
		result.setWidth( outer.width() );
	if( result.height() > outer.height() )
		result.setHeight( outer.height() );
	
	//Fix aspect
	if( keep_aspect ){
		double old_aspect = (double)inner.width() / (double)inner.height();
		double new_aspect = (double)result.width() / (double)result.height();
		
		//Calculate new height/width so that result becomes smaller
		if( old_aspect < new_aspect )
			result.setWidth( result.height() * old_aspect + 0.5 );
		else
			result.setHeight( result.width() / old_aspect + 0.5 );
	}
	
	//Check if there is room for it in outer / constrain position
	if( !outer.contains( result ) ){
		//If inner is to the upper left of outer, move to corner of outer
		if( result.x() < outer.x() )
			result.setX( outer.x() );
		if( result.y() < outer.y() )
			result.setY( outer.y() );
		
		//If inner sticks out of outer, move towards center
		int outer_right = outer.x() + outer.width();
		int outer_bottom = outer.y() + outer.height();
		if( result.x() + result.width() > outer_right )
			result.moveLeft( outer_right - result.width() );
		if( result.y() + result.height() > outer_bottom )
			result.moveTop( outer_bottom - result.height() );
	}
	
	return result;
}

QPoint contrain_point( QRect outer, QPoint inner ){
	if( outer.x() > inner.x() )
		inner.setX( outer.x() );
	else if( outer.x() + outer.width() < inner.x() )
		inner.setX( outer.x() + outer.width() );
	
	if( outer.y() > inner.y() )
		inner.setY( outer.y() );
	else if( outer.y() + outer.height() < inner.y() )
		inner.setY( outer.y() + outer.height() );
	return inner;
}
