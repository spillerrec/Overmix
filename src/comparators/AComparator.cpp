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

#include "AComparator.hpp"

#include "../planes/Plane.hpp"
#include <QRect>

using namespace Overmix;

ImageOffset::ImageOffset( Point<double> distance, double error, const Plane& img1, const Plane& img2 ) : distance( distance ), error( error ) {
	overlap = calculate_overlap( distance.to<int>(), img1, img2 );
}

double ImageOffset::calculate_overlap( Point<> offset, const Plane& img1, const Plane& img2 ){
	QRect first( 0,0, img1.get_width(), img1.get_height() );
	QRect second( { offset.x, offset.y }, QSize(img2.get_width(), img2.get_height()) );
	QRect common = first.intersected( second );
	
	double area = first.width() * first.height();
	return (double)common.width() * common.height() / area;
}
