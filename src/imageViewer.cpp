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

#include "imageViewer.h"

#include <QPoint>
#include <QSize>
#include <QRect>

#include <QImage>
#include <QImageReader>

#include <QPainter>
#include <QTimer>

#include <QMouseEvent>
#include <QWheelEvent>

#include <math.h>


imageViewer::imageViewer( QWidget* parent ): QWidget( parent ){
	image_cache = NULL;
	
	background = QPalette().color( QPalette::Window );	//::Window for default
	
	//Auto scale default settings
	auto_scale_on = true;
	auto_aspect_ratio = true;
	auto_downscale_only = true;
	auto_upscale_only = false;
	
	shown_pos = QPoint( 0,0 );
	shown_zoom_level = 0;
}


QSize imageViewer::frame_size(){
	return ( image_cache ) ? image_cache->size() : QSize(0,0);
}


void imageViewer::auto_scale( QSize img ){
	QSize widget = size();
	
	if( auto_scale_on ){
		double scaling_x = (double) widget.width() / img.width();
		double scaling_y = (double) widget.height() / img.height();
		double img_aspect = (double) img.width() / img.height();
		
		//Prevent certain types of scaling
		if( auto_downscale_only ){
			if( scaling_x > 1 )
				scaling_x = 1;
			if( scaling_y > 1 )
				scaling_y = 1;
		}
		if( auto_upscale_only ){
			if( scaling_x < 1 )
				scaling_x = 1;
			if( scaling_y < 1 )
				scaling_y = 1;
		}
		
		//Keep aspect ratio
		if( auto_aspect_ratio ){
			int res_width = scaling_x * img.width();
			int res_height = scaling_y * img.height();
			double new_aspect = (double)res_width / res_height;
			
			if( img_aspect < new_aspect )
				scaling_x = scaling_y;
			else
				scaling_y = scaling_x;
		}
		
		//Apply scaling
		shown_size.setWidth( scaling_x * img.width() );
		shown_size.setHeight( scaling_y * img.height() );
		
		//Position image
		QSize temp = ( widget - shown_size ) / 2;
		shown_pos = QPoint( temp.width(), temp.height() );
		
		setCursor( Qt::ArrowCursor );
	}
	else{
		//Calculate zoom to 2^(x/2) which is then rounded to nearest 0.5
		//At negative x values: zoom^-1
		double zoom, exp = shown_zoom_level * 0.5;
		if( exp < 0 )
			zoom = 1 / ( ( (int)(2*pow( 2, -exp ) + 0.5) ) * 0.5 );
		else
			zoom = ( (int)(2*pow( 2, exp ) + 0.5) ) * 0.5;
		
		shown_size = img * zoom;
		
		//Make sure it doesn't leave the screen
		QSize diff = widget - shown_size;
		if( diff.width() >= 0 )
			shown_pos.setX( diff.width() / 2 );
		else{
			if( shown_pos.x() > 0 )
				shown_pos.setX( 0 );
			if( shown_pos.x() + shown_size.width() < widget.width() )
				shown_pos.setX( widget.width() - shown_size.width() );
		}
		
		if( diff.height() >= 0 )
			shown_pos.setY( diff.height() / 2 );
		else{
			if( shown_pos.y() > 0 )
				shown_pos.setY( 0 );
			if( shown_pos.y() + shown_size.height() < widget.height() )
				shown_pos.setY( widget.height() - shown_size.height() );
		}
		
		//TODO: don't show cursor if image can't be moved!
		if( mouse_active )
			setCursor( Qt::ClosedHandCursor );
		else
			setCursor( Qt::OpenHandCursor );
	}
}




void imageViewer::change_image( QImage *new_image, bool delete_old ){
	if( image_cache ){
		if( delete_old )
			delete image_cache;
	}
	
	image_cache = new_image;
	
	shown_pos = QPoint( 0,0 );
	shown_zoom_level = 0;
	
	update();
}

#include <QBrush>


void imageViewer::paintEvent( QPaintEvent *event ){
	if( !image_cache )
		return;
	
	//Everything when fine, start drawing the image
	QSize img_size = image_cache->size();
	auto_scale( img_size );
	
	QPainter painter( this );
	if( img_size.width()*1.5 >= shown_size.width() )
		painter.setRenderHints( QPainter::SmoothPixmapTransform, true );
	
	QSize current_size = size();
	painter.fillRect( QRect( QPoint(0,0), current_size ), QBrush( background ) );
	painter.drawImage( QRect( shown_pos, shown_size ), *image_cache );
}

QSize imageViewer::sizeHint() const{
	if( image_cache )
		return image_cache->size();
	
	return QSize();
}





void imageViewer::mousePressEvent( QMouseEvent *event ){
	if( event->button() & Qt::RightButton ){
		auto_scale_on = !auto_scale_on;
		update();
		return;
	}
	
	if( event->button() & Qt::LeftButton ){
		if( auto_scale_on )
			return;
		
		setCursor( Qt::ClosedHandCursor );
		mouse_active = true;
		mouse_last_pos = event->pos();
		update();
	}
}


void imageViewer::mouseMoveEvent( QMouseEvent *event ){
	if( !mouse_active )
		return;
	
	shown_pos -= mouse_last_pos - event->pos();
	mouse_last_pos = event->pos();
	
	update();
}


void imageViewer::mouseReleaseEvent( QMouseEvent *event ){
	if( mouse_active )
		setCursor( Qt::OpenHandCursor );
	
	mouse_active = false;
}


void imageViewer::wheelEvent( QWheelEvent *event ){
	int amount = event->delta() / 8;
	QPoint pos = event->pos();
	
	if( amount > 0 )
		shown_zoom_level++;
	else if( amount < 0 )
		shown_zoom_level--;
		
	update();
}




