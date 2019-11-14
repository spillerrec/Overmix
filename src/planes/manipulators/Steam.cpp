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


#include "Steam.hpp"

#include "../ImageEx.hpp"
#include "../../color.hpp"

#include <limits>

using namespace Overmix;


Plane Steam::detect( const Plane& input, const Plane& alpha ){
	constexpr int radius = 50;
	
	Plane out( input.getSize() );
	
	for( unsigned y=0; y<input.get_height(); y++ ){
		for( unsigned x=0; x<input.get_width(); x++ ){
			
			color_type low = std::numeric_limits<color_type>::max();
			color_type high = std::numeric_limits<color_type>::min();
			
			unsigned start_x = unsigned( std::max(0, int(x)-radius) );
			unsigned start_y = unsigned( std::max(0, int(y)-radius) );
			unsigned end_x = std::min(unsigned(x+radius), input.get_width());
			unsigned end_y = std::min(unsigned(y+radius), input.get_height());
			for( unsigned dy=start_y; dy<end_y; dy++ )
				for( unsigned dx=start_x; dx<end_x; dx++ )
					//if( alpha[dy][dx] > 0 ) //TODO: Check if alpha exists
					{
						low  = std::min(low,  input[dy][dx]);
						high = std::max(high, input[dy][dx]);
					}
			
			//Blur input for bright area factor?
			out[y][x] = color::fromDouble((1.0-color::asDouble(high-low)) * color::asDouble(input[y][x]));
		}
	}
	
	return out;
}
ImageEx Steam::detect( const ImageEx& input ){
	ImageEx out( input );
	out.to_grayscale(); //Ignore colors for now, perhaps use max in future instead?
	
	for(unsigned c=0; c<out.size(); c++)
		out[c] = detect( out[c], out.alpha_plane() );
	
	//TODO: Alpha
	out.alpha_plane() = {};
	
	return out;
}