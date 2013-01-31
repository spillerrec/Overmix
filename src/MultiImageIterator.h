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

#ifndef MULTI_IMAGE_ITERATOR_H
#define MULTI_IMAGE_ITERATOR_H

#include <QImage>
#include <QPoint>

#include <utility>
#include <vector>

#include "color.h"

typedef std::pair<int,int> Line;

class MultiImageIterator{
	private:
		const std::vector<QImage> &imgs;
		const std::vector<QPoint> &pos;
		
		std::vector<const QRgb*> lines;
		std::vector<Line> line_width;
		std::vector<color> values;
		int current_x;
		int current_y;
		int left;
		
	private:
		void new_y( int y );
		void new_x( int x );
		void fill_values();
	
	public:
		MultiImageIterator( const std::vector<QImage> &images, const std::vector<QPoint> &points, int x=0, int y=0 );
		
		void next_y(){ new_y( current_y + 1 ); }
		void next_x(){ new_x( current_x + 1 ); }
		void next_line(){
			current_x = left; //We can do this as new_y() updates this as well
			next_y();
		}
		
	public:
		color average();
		color simple_filter( unsigned threshould );
		color simple_slide( unsigned threshould );
};

#endif