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

#ifndef PLANES_BASIC_DIFFERENCE_HPP
#define PLANES_BASIC_DIFFERENCE_HPP

#include "../../color.hpp"
#include "../../Geometry.hpp"

namespace Overmix{
	class Plane;
}

namespace Overmix{ namespace Difference{
	struct SimpleSettings{
		unsigned   stride;
		bool       fast;    ///Do not use epsilon
		color_type epsilon; ///Ignore differences below this level
		//TODO: Switch between L1 and L2
		
		SimpleSettings() : stride(1), fast(true), epsilon(0) { }
	};
	
	double simple(      const Plane& p1, const Plane& p2,                                           Point<int> offset, SimpleSettings s={} );
	double simpleAlpha( const Plane& p1, const Plane& p2, const Plane& alpha1, const Plane& alpha2, Point<int> offset, SimpleSettings s={} );
} }

#endif
