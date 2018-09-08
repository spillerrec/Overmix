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

#ifndef SIMPLE_COMPARATOR_BASE_HPP
#define SIMPLE_COMPARATOR_BASE_HPP

#include "AComparator.hpp"
#include "../planes/basic/difference.hpp"

namespace Overmix{
	
class Plane;

class SimpleComparatorBase : public AComparator{
	public:
		Difference::SimpleSettings settings;
		
	public:
		SimpleComparatorBase( Difference::SimpleSettings settings = {} )
			: settings(settings) { }
		
		double findError( const Plane& p1, const Plane& p2, const Plane& a1, const Plane& a2, double x, double y ) const override{
			auto pos = Point<int>( x, y ).round();
			return Difference::simpleAlpha( p1, p2, a1, a2, pos, settings );
		}
};


}

#endif
