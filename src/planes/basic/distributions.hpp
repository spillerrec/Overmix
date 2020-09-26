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

#ifndef PLANES_DISTRIBUTIONS_HPP
#define PLANES_DISTRIBUTIONS_HPP

#include <boost/math/constants/constants.hpp>
#include <cmath>
#include <numeric>

namespace Overmix{
	class Plane;
}

namespace Overmix{ namespace Distributions{
	
	inline double linear( double x ){
		x = abs( x );
		return ( x <= 1.0 ) ? 1.0 - x : 0.0;
	}
	
	inline double cubic( double b, double c, double x ){
		x = abs( x );
		
		if( x < 1 )
			return
					(12 - 9*b - 6*c)/6 * x*x*x
				+	(-18 + 12*b + 6*c)/6 * x*x
				+	(6 - 2*b)/6
				;
		else if( x < 2 )
			return
					(-b - 6*c)/6 * x*x*x
				+	(6*b + 30*c)/6 * x*x
				+	(-12*b - 48*c)/6 * x
				+	(8*b + 24*c)/6
				;
		else
			return 0;
	}
	
	inline double lancozs( double x, int a ){
		auto pi = boost::math::constants::pi<double>();
		if( x >= a )
			return 0;
		else if( std::abs(x) < std::numeric_limits<double>::epsilon() )
			return 1;
		else
			return (a*std::sin(pi*x)*std::sin(pi*x/2)) / (pi*pi*x*x);
	}
	
} }

#endif
