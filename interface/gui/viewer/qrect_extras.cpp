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
	//Constrain size
	QSize newSize = inner.size().boundedTo( outer.size() );
	
	//Fix aspect ratio
	if( keep_aspect ){
		double old_aspect = (double)  inner.width() / (double)  inner.height();
		double new_aspect = (double)newSize.width() / (double)newSize.height();
		
		//Calculate new height/width so that newSize becomes smaller
		if( old_aspect < new_aspect )
			newSize.setWidth( newSize.height() * old_aspect + 0.5 );
		else
			newSize.setHeight( newSize.width() / old_aspect + 0.5 );
	}
	
	//Constrain position, by using that: pos+size < o.pos+o.size, thus pos < o.pos+o.size - size
	return { contrain_point( { outer.topLeft(), outer.size()-newSize }, inner.topLeft() ) , newSize };
}

QPoint contrain_point( QRect outer, QPoint inner ){
	auto bottom = outer.topLeft() + toQPoint( outer.size() );
	//     |     constrain if above outer      |       constrain if below outer       | already inside
	return { outer.x() > inner.x() ? outer.x() : (bottom.x() < inner.x() ? bottom.x() : inner.x())
	       , outer.y() > inner.y() ? outer.y() : (bottom.y() < inner.y() ? bottom.y() : inner.y()) };
}
