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


#include "debug.hpp"

void debug::make_slide( QImage image, QString dir, double scale ){
	int height = image.height() - 720;
	
	for( int iy=0; iy<height; iy+=6 ){
		QImage current = image.copy( 0, iy, image.width(), 720 ).scaled( round(image.width()*scale), round(720*scale), Qt::IgnoreAspectRatio, Qt::SmoothTransformation );
		current.save( (dir + "/%1.png" ).arg( iy/6 + 1, 4, 10, QChar('0')) );
	}
}


void debug::make_low_res( QImage image, QString dir, unsigned scale, unsigned amount ){
	if( scale == 0 )
		return;
	
	if( amount == 0 )
		amount = scale*scale;
	
	unsigned new_width = image.width() - scale*2;
	unsigned new_height = image.height() - scale*2;
	for( unsigned iy=0; iy<scale; ++iy )
		for( unsigned ix=0; ix<scale; ++ix ){
			QImage frame = image
				.copy( ix, iy, new_width, new_height )
				.scaled( new_width / scale, new_height / scale, Qt::IgnoreAspectRatio, Qt::SmoothTransformation )
				;
			frame.save( (dir + "/%1x%2.png").arg( ix ).arg( iy ) );
		}
}

