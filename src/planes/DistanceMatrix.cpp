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

#include "DistanceMatrix.hpp"

#include <QColor>
#include <QImage>

#include <algorithm>

using namespace Overmix;


QImage DistanceMatrix::toQImage() const{
	auto comp = [](ImageOffset a, ImageOffset b){ return a.error < b.error; };
	auto min = std::min_element( matrix.allPixelsBegin(), matrix.allPixelsEnd(), comp )->error;
	auto max = std::max_element( matrix.allPixelsBegin(), matrix.allPixelsEnd(), comp )->error;
	auto color = [=](ImageOffset a){ return (a.error-min) / (max-min); };
	
	QImage img( matrix.get_width(), matrix.get_height(), QImage::Format_Indexed8 );
	QVector<QRgb> palette;
	for( unsigned i=0; i<256; i++ )
		palette << QColor::fromHsvF( (1.0 - (i/256.0))*240/360, 1.0, 1.0 ).rgb();
	img.setColorTable( palette );
	
	for( unsigned iy=0; iy<matrix.get_height(); iy++ ){
		auto row = matrix.scan_line( iy );
		auto out = img.scanLine( iy );
		for( unsigned ix=0; ix<matrix.get_width(); ix++ )
			out[ix] = color( row[ix] ) * 255;
	}
	
	return img;
}

