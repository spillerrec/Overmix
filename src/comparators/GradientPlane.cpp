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

#include "GradientPlane.hpp"

#include <limits>

#include <QtConcurrent>

#include "../planes/Plane.hpp"
#include "AComparator.hpp" //For ImageOffset

using namespace Overmix;


static const double DOUBLE_MAX = std::numeric_limits<double>::max();

//TODO: these two are brutally simple, improve?
double DiffCache::get_diff( int x, int y, unsigned precision ) const{
	for( auto c : cache )
		if( c.x == x && c.y == y && c.precision <= precision ){
		//	qDebug( "Reusing diff: %dx%d with %.2f", x, y, c.diff );
			return c.diff;
		}
	return -1;
}
void DiffCache::add_diff( int x, int y, double diff, unsigned precision ){
	Cached c = { x, y, diff, precision };
	cache.emplace_back( c );
}

GradientCheck::GradientCheck( Size<unsigned> size1, Size<unsigned> size2, double width_scale, double height_scale, Point<double> hint, int lvl ) : hint(hint){
	//TODO: Hint is now broken since it might not be centered
	auto limitLeftTop = Size<int>(1,1) - size2.to<int>();
	auto limitBottomRight = size1.to<int>() - 1;
	Point<double> scale(width_scale, height_scale);
	
	auto leftTop = limitLeftTop.max( limitLeftTop * scale + hint );
	auto bottomRight = limitBottomRight.min( limitBottomRight * scale + hint );
	left = leftTop.x;
	top  = leftTop.y;
	right  = bottomRight.x;
	bottom = bottomRight.y;
	level = lvl;
}

double GradientPlane::getDifference( int x, int y, double precision ) const{
	auto local = settings;
	local.stride = precision; //TODO: double?
	local.stride = std::max(1u, local.stride);
	return Difference::simpleAlpha( p1, p2, a1, a2, {x, y}, local );
}


struct img_comp{
	GradientPlane& plane;
	GradientCheck area;
	int h_middle;
	int v_middle;
	double diff{ -1.0 };
	double precision;
	bool diff_set{ false }; //TODO: name is missleading, interface unclear as well
	
	img_comp( GradientPlane& plane, int hm, int vm, double diff, GradientCheck area={}, double p=1 )
		:	plane(plane), area(std::move(area))
		,	h_middle( hm ), v_middle( vm )
		,	diff(diff)
		,	precision( p )
		{ diff_set = diff >= 0; }
	void do_diff(){
		if( !diff_set )
			diff = plane.getDifference( h_middle, v_middle, precision );
		//TODO: set to true?
	}
	
	ImageOffset result() const{
		if( area.level > 0 )
			return plane.findMinimum( area );
		else
			return ImageOffset( {double(h_middle), double(v_middle)}, diff, 1 ); //TODO: calculate overlap
	}
	
	double checkedPercentage(){
		//Find edges
		int x = h_middle;
		int y = v_middle;
		//TODO: Geometry?
		int p1_top  = y < 0 ? 0 : y;
		int p1_left = x < 0 ? 0 : x;
		int p2_top  = y > 0 ? 0 : -y;
		int p2_left = x > 0 ? 0 : -x;
		unsigned width = std::min( plane.p1.get_width() - p1_left, plane.p2.get_width() - p2_left );
		unsigned height = std::min( plane.p1.get_height() - p1_top, plane.p2.get_height() - p2_top );
		return width * height;
	}
	void increasePrecision( double max_checked ){
		precision = std::max( precision / (max_checked / checkedPercentage()), 1.0 );
	}
};


ImageOffset GradientPlane::findMinimum( GradientCheck area ){
	std::vector<img_comp> comps;
	//TODO: Move to GradientCheck, and document
	int amount = area.level*2 + 2;
	comps.reserve( amount * amount );
	double h_offset = (double)(area.right - area.left) / amount;
	double v_offset = (double)(area.bottom - area.top) / amount;
	auto level = area.level > 1 ? area.level-1 : 1;
	
	if( h_offset < 1 && v_offset < 1 ){
		//Handle trivial step
		//Check every diff in the remaining area
		for( int ix=area.left; ix<=area.right; ix++ )
			for( int iy=area.top; iy<=area.bottom; iy++ )
				comps.emplace_back( *this, ix, iy, cache.get_diff( ix, iy, 1 ) );
	}
	else{
		//Make sure we will not do the same position multiple times
		double h_add = ( h_offset < 1 ) ? 1 : h_offset;
		double v_add = ( v_offset < 1 ) ? 1 : v_offset;
		
		double prec_offset = std::min( h_offset, v_offset );
		if( h_offset == 0 || v_offset == 0 )
			prec_offset = std::max( h_offset, v_offset );
		double precision = std::sqrt(prec_offset);
		
		for( double iy=area.top+v_offset; iy<=area.bottom; iy+=v_add )
			for( double ix=area.left+h_offset; ix<=area.right; ix+=h_add ){
				int x = ( ix < 0.0 ) ? ceil( ix-0.5 ) : floor( ix+0.5 );
				int y = ( iy < 0.0 ) ? ceil( iy-0.5 ) : floor( iy+0.5 );
				
				//Avoid right-most case. Can't be done in the loop
				//as we always want it to run at least once.
				if( ( x == area.right && x != area.left ) || ( y == area.bottom && y != area.top ) )
					continue;
				
				//Calculate sub-area
				GradientCheck sub_area{
						(int)floor( ix - h_offset ), (int)ceil( ix + h_offset )
					,	(int)floor( iy - v_offset ), (int)ceil( iy + v_offset )
					,	level
				};
				
				//Create and add
				auto diff = cache.get_diff( x, y, precision );
				comps.emplace_back( *this, x, y, diff, sub_area, precision );
			}
	}
	
	//Find maximal checked area, and re-evaluate precision
	double max_checked = max_element( comps.begin(), comps.end(), []( img_comp i1, img_comp i2 ){
			return i1.checkedPercentage() < i2.checkedPercentage();
		} )->checkedPercentage();
	for( auto& comp : comps )
		comp.increasePrecision( max_checked );
	
	//Calculate diffs
	QtConcurrent::map( comps, [](img_comp& comp){ comp.do_diff(); } ).waitForFinished();
//	for( auto& comp : comps ) comp.do_diff();
	
	//Find best comp
	const img_comp* best = nullptr;
	double best_diff = DOUBLE_MAX;
	
	for( auto &c : comps ){
		if( c.diff < best_diff ){
			best = &c;
			best_diff = best->diff;
		}
		
		//Add to cache
		if( !c.diff_set )
			cache.add_diff( c.h_middle, c.v_middle, c.diff, c.precision );
	}
	
	if( !best )
		throw std::runtime_error( "GradientPlane::findMinimum: no result to continue on!!" );
	
	return best->result();
}

