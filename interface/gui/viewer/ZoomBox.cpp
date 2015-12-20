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

#include "ZoomBox.hpp"

#include "qrect_extras.h"

using namespace std;

bool ZoomBox::change_scale( double new_level, QPoint keep_on ){
	if( new_level == zoom )
		return false;
	
	auto old_size = size();
	zoom = new_level;
	
	if( !old_size.isEmpty() ) //Don't divide with old_size if it is 0
		position = keep_on + (QPointF(position-keep_on) * size() / old_size).toPoint();
	
	return true;
}


bool ZoomBox::change_content( QSize content, bool keep_zoom ){
	if( this->content == content && keep_zoom )
		return false;
	
	this->content = content;
	zoom = 0;
	position = {};
	return true;
}

void ZoomBox::restrict( QSize view ){
	//Make sure it doesn't leave the view-port
	QSize diff = view - size();
	
	//TODO: the following is repeated for y, avoid this
	if( diff.width() >= 0 )
		position.setX( diff.width() / 2.0 + 0.5 );
	else{
		position.setX( min( 0, position.x() ) );
		if( position.x() + size().width() < view.width() )
			position.setX( view.width() - size().width() );
	}
	
	if( diff.height() >= 0 )
		position.setY( diff.height() / 2.0 + 0.5 );
	else{
		position.setY( min( 0, position.y() ) );
		if( position.y() + size().height() < view.height() )
			position.setY( view.height() - size().height() );
	}
}

bool ZoomBox::resize( QSize view, bool downscale_only, bool upscale_only, bool keep_aspect ){
	if( content.isEmpty() )
		return false;
	
	QRect old( position, size() );
	double scaling_x = (double) view.width() / content.width();
	double scaling_y = (double) view.height() / content.height();
	
	//Prevent certain types of scaling
	if( downscale_only ){
		scaling_x = min( scaling_x, 1.0 );
		scaling_y = min( scaling_y, 1.0 );
	}
	if( upscale_only ){
		scaling_x = max( scaling_x, 1.0 );
		scaling_y = max( scaling_y, 1.0 );
	}
	
	//TODO: non-aspect ratio not supported!
	if( keep_aspect ){
		if( scaling_y < scaling_x )
			scaling_x = scaling_y;
		else
			scaling_y = scaling_x;
	}
	
	//Apply scaling
	zoom = fromScale( scaling_x );
	
	restrict( view );
	return old != QRect( position, size() );
}


bool ZoomBox::move( QPoint offset ){
	position += offset;
	return !offset.isNull();
}
