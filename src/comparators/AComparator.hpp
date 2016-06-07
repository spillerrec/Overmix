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

#ifndef A_COMPARATOR_HPP
#define A_COMPARATOR_HPP

#include "../Geometry.hpp"
#include "../utils/PlaneUtils.hpp"

namespace Overmix{

class Plane;

struct ImageOffset{
	Point<double> distance;
	double error;
	double overlap;
	ImageOffset() : distance( 0,0 ), error( -1 ), overlap( -1 ) { }
	ImageOffset( Point<double> distance, double error, double overlap )
		: distance(distance), error(error), overlap(overlap) { }
	bool isValid() const{ return overlap >= 0.0; }
	
	ImageOffset reverse() const{ return { {-distance.x, -distance.y}, error, overlap }; }
};

class AComparator{
	public:
		virtual ModifiedPlane process( const Plane& plane ){ return { plane }; }
		virtual ImageOffset findOffset( const Plane& img1, const Plane& img2, const Plane& a1, const Plane& a2 ) const = 0;
};

}

#endif
