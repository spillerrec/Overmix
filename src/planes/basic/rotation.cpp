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

#include "../../color.hpp"


using namespace std;
using namespace Overmix;

class TransformMatrix {
	private:
		double x0, x1;
		double y0, y1;
		Point<double> scale;
		
	public:
		TransformMatrix( double radians, Point<double> scale ) {
			radians = 3.14*2-radians;
			this->scale = Point<double>(1.0, 1.0)/scale;
			// TODO: Support scaling
			x0 =  std::cos(radians);// * scale.x;
			x1 = -std::sin(radians);// * scale.y;
			y0 =  std::sin(radians);// * scale.x;
			y1 =  std::cos(radians);// * scale.y;
		}
		
		Point<double> operator()(Point<double> pos){
			//pos *= scale;
			return {
				pos.x * x0 + pos.y * x1,
				pos.x * y0 + pos.y * y1
			};
		}
};


Rectangle<int> Transformations::rotationEndSize( Size<unsigned> size, double radians, Point<double> scale ){
	//size /= scale;
	TransformMatrix forward(3.14*2-radians, Point<double>(1.0, 1.0));
	auto p0 = Point<double>(0, 0);
	auto p1 = forward(Point<double>(0, size.y));
	auto p2 = forward(Point<double>(size.x, size.y));
	auto p3 = forward(Point<double>(size.x, 0));
	
	auto start = p0.min(p1).min(p2).min(p3).floor();
	auto end   = p0.max(p1).max(p2).max(p3).ceil();
	return Rectangle<int>( start, end - start );
}

Plane Transformations::rotation( const Plane& p, double radians, Point<double> scale ){
	auto area = rotationEndSize( p.getSize(), radians, scale );
	TransformMatrix forward( radians, scale );
	
	Plane out(area.size);
	
	for( int iy=0; iy<out.get_height(); iy++ )
		for( int ix=0; ix<out.get_width(); ix++ ){
			auto pos = forward( Point<double>(ix, iy) + area.pos );
			
			auto clamped = pos.max({0,0}).min(p.getSize()-1);
			auto base0 = clamped.floor();
			auto delta = clamped - base0;
			auto base1 = (base0 + 1).min(p.getSize()-1);
			
			auto v00 = p[base0.y][base0.x];
			auto v01 = p[base0.y][base1.x];
			auto v10 = p[base1.y][base0.x];
			auto v11 = p[base1.y][base1.x];
			
			auto mix = [](auto a, auto b, double x){ return a * (1.0-x) + b * x; };
			out[iy][ix] = mix(
				mix(v00, v01, delta.x),
				mix(v10, v11, delta.x),
				delta.y);
		}
	
	return out;
}

Plane Transformations::rotationAlpha( const Plane& p, double radians, Point<double> scale ){
	auto area = rotationEndSize( p.getSize(), radians, scale );
	TransformMatrix forward( radians, scale );
	
	Plane out(area.size);
	
	for( int iy=0; iy<out.get_height(); iy++ )
		for( int ix=0; ix<out.get_width(); ix++ ){
			auto pos = forward( Point<double>(ix, iy) + area.pos ).round();
			auto posClamped = pos.max({0,0}).min(p.getSize()-1);
			
			out[iy][ix] = (pos == posClamped) ? color::WHITE : color::BLACK;
		}
	
	return out;
}

