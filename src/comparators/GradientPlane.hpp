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

#ifndef GRADIENT_PLANE_HPP
#define GRADIENT_PLANE_HPP

#include "AComparator.hpp"
#include "../Geometry.hpp"
#include "../planes/basic/difference.hpp"
#include <vector>

namespace Overmix{
	
class Plane;

class DiffCache{
	private:
		struct Cached{
			int x;
			int y;
			double diff;
			unsigned precision;
		};
		std::vector<Cached> cache;
		
	public:
		double get_diff( int x, int y, unsigned precision ) const;
		void add_diff( int x, int y, double diff, unsigned precision );
};

struct GradientCheck{
	int left   { 0 };
	int right  { 0 };
	int top    { 0 };
	int bottom { 0 };
	int level  { 1 };
	
	GradientCheck(){ }
	GradientCheck( int l, int r, int t, int b, int lvl=1 )
		:	left(l), right(r), top(t), bottom(b), level(lvl) { }
	
	GradientCheck( Size<unsigned> size, double width_scale, double height_scale, int lvl=1 );
};

class GradientPlane{
	public:
		const Plane& p1;
		const Plane& p2;
		const Plane& a1;
		const Plane& a2;
	private:
		DiffCache cache;
		Difference::SimpleSettings settings;
		
	public:
		GradientPlane( const Plane& p1, const Plane& p2, const Plane& a1, const Plane& a2, Difference::SimpleSettings settings={} )
			: p1(p1), p2(p2), a1(a1), a2(a2), settings(settings) { }
		
		double getDifference( int x, int y, double precision ) const;
		
		struct ImageOffset findMinimum( GradientCheck area );
};


}

#endif
