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

#include "BruteForceComparator.hpp"
#include "../planes/Plane.hpp"

#include <QDebug>
#include <QRect>
#include <limits>

using namespace Overmix;


ImageOffset BruteForceComparator::findOffset( const Plane& img1, const Plane& img2, const Plane& a1, const Plane& a2, Point<double> hint ) const{
	Point<double> moves{ method == AlignMethod::VER ? 0.0 : movement
	                   , method == AlignMethod::HOR ? 0.0 : movement
	                   };
	
	//Calculate range
	auto max = (moves * img1.getSize()).to<int>();
	auto min = Point<>()-max; //TODO:
	qDebug() << "BruteForce: " << max.x << ", " << max.y;
	
	//Find minimum error in range
	ImageOffset result;
	result.error = std::numeric_limits<double>::max();
	for( int x = min.x; x<=max.x; x++ ) //TODO: Make Point<int> iterator
		for( int y = min.y; y<=max.y; y++ ){
			auto error = Difference::simpleAlpha( img1, img2, a1, a2, {x, y}, settings );
			qDebug() << x << ", " << y << ", " << error;
			if( error < result.error )
				result = ImageOffset( Point<double>(x, y), error, img1, img2 );
		}
	
	return result;
}

