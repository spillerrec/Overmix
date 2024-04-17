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

#ifndef PLANES_BASIC_ROTATION_HPP
#define PLANES_BASIC_ROTATION_HPP

#include <utility>

#include "../../Geometry.hpp"

namespace Overmix{
	class Plane;
}

namespace Overmix{ namespace Transformations{
	
	Rectangle<int> rotationEndSize( Size<unsigned> size, double radians, Point<double> scale={1.0, 1.0} );
	
	Plane rotation( const Plane& p1, double radians, Point<double> scale={1.0, 1.0} );
	Plane rotationAlpha( const Plane& p1, double radians, Point<double> scale={1.0, 1.0} );
	
} }

#endif
