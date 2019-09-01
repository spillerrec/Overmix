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


#include "Mosaic.hpp"

#include "../ImageEx.hpp"
#include "../../color.hpp"

#include <cmath>
#include <limits>

using namespace Overmix;
constexpr double PI = 3.14159265;

static auto edgeDetect(const ImageEx& input){
	PlaneBase<double> out(input.getSize());
	out.fill(0.0);
	
	for(unsigned i=0; i<input.size(); i++){
		auto edges = input[i].edge_sobel_direction();
	
		double maxStrenght = 0.0;
		for( unsigned y=0; y<input.get_height(); y++ )
			for( unsigned x=0; x<input.get_width(); x++ )
				maxStrenght = std::max(maxStrenght, (double)std::max(std::abs(edges[y][x].first), std::abs(edges[y][x].second)));
		
		for( unsigned y=0; y<input.get_height(); y++ ){
			for( unsigned x=0; x<input.get_width(); x++ ){
				auto dirs = edges[y][x];
				/*double angle = atan2(dirs.second, dirs.first) / PI * 180;
				if(angle < 0)
					angle += 360;
				double distance_1 = std::abs(  90.0 - angle );
				double distance_2 = std::abs( 270.0 - angle );
				double distance = std::min(distance_1, distance_2);
				double edge = (90 - distance) / distance;
				*/
				double edgeMax = std::max(std::abs(dirs.first), std::abs(dirs.second));
				double edgeMin = std::min(std::abs(dirs.first), std::abs(dirs.second));
				double strength = edgeMax / maxStrenght;
				double singleEdge = double(edgeMax - edgeMin) / edgeMax;
				
				//out[y][x] = std::max(out[y][x], edge * strength);
				out[y][x] = std::max(out[y][x], singleEdge);
			}
		}
	}
	
	double maxValue = 0.0;
	for( unsigned y=0; y<input.get_height(); y++ )
		for( unsigned x=0; x<input.get_width(); x++ )
			maxValue = std::max(maxValue, out[y][x]);
		
	for( unsigned y=0; y<input.get_height(); y++ )
		for( unsigned x=0; x<input.get_width(); x++ )
			out[y][x] = out[y][x] / maxValue;
	
	return out;
}
ImageEx Mosaic::detect( const ImageEx& input ){
	auto out = edgeDetect( input );
	
	return ImageEx(out.map([](auto value){ return color::fromDouble(value); } ));
}