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

#include "MultiScaleComparator.hpp"
#include "../color.hpp"
#include "../planes/basic/difference.hpp"

#include <QRect>

#include <array>
#include <algorithm>

using namespace Overmix;


Plane simple2xDownscale( const Plane& img )
{
	if( img.get_width() < 2 || img.get_height() < 2 )
		return {};
	
	Plane out( img.getSize() / 2 );
	
	for( unsigned y=0; y<out.get_height(); y++ )
		for( unsigned x=0; x<out.get_width(); x++ ){
			auto pos = Point<>(x,y) * 2;
			out[y][x] = ((precision_color_type)
				  img[pos.y  ][pos.x  ]
				+ img[pos.y  ][pos.x+1]
				+ img[pos.y+1][pos.x  ]
				+ img[pos.y+1][pos.x+1]
				) / 4;
		}
	
	return out;
}


ImageOffset MultiScaleComparator::findOffset( const Plane& img1, const Plane& img2, const Plane& alpha1, const Plane& alpha2 ) const{
	//Check of end of recursion, this is our starting point
	if( !img1 || !img2 )
		return {};
	
	//Find the offset in one resolution step lower
	auto down = [](const Plane& p){ return simple2xDownscale( p ); };
	//auto down = [](const Plane& p){ return p.scale_cubic( {}, p.getSize()/2 ); };
	auto result = findOffset( down(img1), down(img2), down(alpha1), down(alpha2) );
	auto baseOffset = result.distance * 2;
	
	//Check all offsets
	std::array<Point<int>,4> offsets = {Point<int>{0,0}, Point<int>{0,1}, Point<int>{1,0}, Point<int>{1,1}};
	std::array<double,4> errors;
	for( int i=0; i<4; i++ )
		errors[i] = Difference::simpleAlpha(img1, img2, alpha1, alpha2, baseOffset + offsets[i]);
	//Handle invalid comparisions at the lowest resolution
	for(auto& error : errors)
		if( !std::isnormal(error) )
			error = std::numeric_limits<double>::max();
	
	//Find and return the best offset
	auto pos = std::distance(errors.begin(), std::min_element(errors.begin(), errors.end()));
	return {baseOffset + offsets[pos], errors[pos], img1, img2};
}

