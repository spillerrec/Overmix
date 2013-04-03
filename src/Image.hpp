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

#ifndef IMAGE_H
#define IMAGE_H

#include "color.hpp"

#include <QPoint>
#include <QImage>

class image{
	private:
		unsigned height;
		unsigned width;
		color *data;
		
	public:
		image( unsigned w, unsigned h );
		image( QImage img );
		~image();
		
		bool is_invalid() const{ return height == 0 || width == 0; }
		unsigned get_height() const{ return height; }
		unsigned get_width() const{ return width; }
		
		color& pixel( unsigned x, unsigned y ) const{ return data[ x + y*width ]; }
		color* scan_line( unsigned y ) const{ return data + y*width; }
		color* raw() const{ return data; }
		
};

#endif