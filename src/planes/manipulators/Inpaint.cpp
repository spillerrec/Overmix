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


#include "Inpaint.hpp"

#include "../ImageEx.hpp"
#include "../../color.hpp"

#include <limits>

using namespace Overmix;


Plane Inpaint::simple( const Plane& input, const Plane& alpha ){
	constexpr int radius = 20;
	
	Plane out( input.getSize() );
	
	for( unsigned y=0; y<input.get_height(); y++ ){
		for( unsigned x=0; x<input.get_width(); x++ ){
			if( alpha[y][x] > 0 ){
				out[y][x] = input[y][x];
				continue;
			}
			
			int distance = std::numeric_limits<int>::max();
			color_type value = 0;
			unsigned start_x = unsigned( std::max(0, int(x)-radius) );
			unsigned start_y = unsigned( std::max(0, int(y)-radius) );
			unsigned end_x = std::min(unsigned(x+radius), input.get_width());
			unsigned end_y = std::min(unsigned(y+radius), input.get_height());
			for( unsigned dy=start_y; dy<end_y; dy++ )
				for( unsigned dx=start_x; dx<end_x; dx++ )
					if( alpha[dy][dx] > 0 ){
						int x_dist = (int)x - dx;
						int y_dist = (int)y - dy;
						auto current_dist = /*std::sqrt(*/x_dist*x_dist + y_dist*y_dist;//);
						if( current_dist < distance ){
							distance = current_dist;
							value = input[dy][dx];
						}
					}
			
			out[y][x] = value;
		}
	}
	
	return out;
}
ImageEx Inpaint::simple( const ImageEx& input ){
	ImageEx out( input );
	
	for(unsigned c=0; c<input.size(); c++)
		out[c] = simple( input[c], input.alpha_plane() );
	
	//TODO: Alpha
	out.alpha_plane() = {};
	
	return out;
}