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

#include "rotation.hpp"

#include "../Plane.hpp"
#include "../PlaneExcept.hpp"


using namespace std;
using namespace Overmix;


std::pair<Plane, Plane> Transformations::rotation( const Plane& p, const Plane& alpha, double radians, double scale ){
	if( alpha )
		planeSizeEqual( "Difference::simpleAlpha", p, alpha );
	
	return {};
	/*
	auto max_size = std::max( p.get_width(), p.get_height() );
	Point out_size( max_size, max_size );
	Point center = out_size / 2;
	Point plane_center = Point( p.get_width(), p.get_height() ) / 2;
	
	auto forward = [&](int x, int y){
		
	};
	
	for( int iy=0; iy<out_size.height(); iy++ )
		for( int ix=0; ix<out_size.height(); ix++ ){
			auto [x, y] = forward( ix, iy );
			
			int x1 = std::max(x, 0.0);
			int y1 = std::max(y, 0.0);
			//TODO: interpolate
			
		}*/
}

