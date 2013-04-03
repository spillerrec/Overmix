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

#include "Image.hpp"

#include <QRect>
#include <vector>
#include <QtConcurrentMap>

#include <png.h>

using namespace std;

image::image( unsigned w, unsigned h ){
	height = h;
	width = w;
	data = new color[ h * w ];
}
image::image( QImage img ){
	height = img.height();
	width = img.width();
	data = new color[ height * width ];
	
	for( unsigned iy=0; iy<height; iy++ ){
		const QRgb *row = (const QRgb*)img.constScanLine( iy );
		color* row2 = scan_line( iy );
		for( unsigned ix=0; ix<width; ++ix, ++row, ++row2 )
			*row2 = color( *row );
	}
}

image::~image(){
	qDebug( "deleting image %p", this );
	if( data )
		delete[] data;
}

