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

#include "Plane.hpp"
#include <limits>

#include <QtConcurrent>
#include <QDebug>
#include <algorithm>

#include "../color.hpp"

using namespace std;
using namespace Overmix;

static const double DOUBLE_MAX = numeric_limits<double>::max();

using DiffAmount = pair<precision_color_type, double>;
struct Sum{
	double sum{ 0.0 };
	unsigned count{ 0 };
	double total{ 0.0 };
	
	void reduce( const DiffAmount add ){
		sum += add.first / add.second;
		total += add.second;
		count++;
	}
	double average() const{ return sum / count; }
};
struct Para{
	const color_type *c1;
	const color_type *c2;
	const color_type *a1;
	const color_type *a2;
	unsigned width;
	unsigned stride;
	static constexpr color_type epsilon = 10 / 255.0 * color::WHITE; //TODO: Ad-hoc constant, and perhaps a little high...
	
	Para( const color_type* c1, const color_type *c2, unsigned width, unsigned stride
		,	const color_type* a1=nullptr, const color_type *a2=nullptr )
		:	c1( c1 ), c2( c2 ), a1( a1 ), a2( a2 ), width( width ), stride( stride ) { }
	
	template<typename T>
	DiffAmount sum( T func ) const{
		//Calculates the sum of func applied on each i, but optimized for stride==1
		precision_color_type sum = 0;
		if( stride == 1 ) for( unsigned i=0; i<width; ++i       ) sum += (this->*func)( i );
		else              for( unsigned i=0; i<width; i+=stride ) sum += (this->*func)( i );
		return { sum, width / stride };
	}
	template<typename T, typename T2>
	DiffAmount sum( T func, T2 alpha_func ) const{
		//Calculates the sum of func applied on each i, but optimized for stride==1
		precision_color_type sum = 0;
		double alpha = 0;
		auto adder = [&]( unsigned i ){
			auto alpha_value = (this->*alpha_func)( i );
			sum += (this->*func)( i ) * alpha_value;
			alpha += alpha_value;
		};
		if( stride == 1 ) for( unsigned i=0; i<width; ++i       ) adder( i );
		else              for( unsigned i=0; i<width; i+=stride ) adder( i );
		return { sum, alpha };
	}
	
	color_type abs(     unsigned i ) const{ 
		auto val = color::asDouble( std::abs( c1[i] - c2[i] ) );
		return color::fromDouble( val*val );
	}
	color_type checked( unsigned i ) const{ auto val = abs( i ); return val > epsilon ? val : 0; }
	color_type alpha1(  unsigned i ) const{ return color::asDouble( a1[i] ); }
	color_type alpha2(  unsigned i ) const{ return color::asDouble( a2[i] ); }
	color_type alpha(   unsigned i ) const{ return alpha1(i) * alpha2(i); }
	
	template<typename T>
	DiffAmount diff_line( T func ) const{
		if( !a1 && !a2 )
			return sum( func );
		else if( a1 && a2 )
			return sum( func, &Para::alpha );
		else //We have exactly one alpha plane
			return sum( func, a1 ? &Para::alpha1 : &Para::alpha2 );
	}
};
static DiffAmount diff_alpha_line(      Para p ){ return p.diff_line( &Para::checked ); }
static DiffAmount fast_diff_alpha_line( Para p ){ return p.diff_line( &Para::abs ); }

double Plane::diff( const Plane& p, int x, int y, unsigned stride ) const{
	Plane empty;
	return diffAlpha( p, empty, empty, x, y, stride );
}

double Plane::diffAlpha( const Plane& p, const Plane& alpha, const Plane& alpha_p, int x, int y, unsigned stride, bool fast ) const{
	//Find edges
	int p1_top = y < 0 ? 0 : y;
	int p2_top = y > 0 ? 0 : -y;
	int p1_left = x < 0 ? 0 : x;
	int p2_left = x > 0 ? 0 : -x;
	unsigned width = min( get_width() - p1_left, p.get_width() - p2_left );
	unsigned height = min( get_height() - p1_top, p.get_height() - p2_top );
	
	//Initial offsets on the two planes
	//TODO: avoid using pointer math
	auto c1 =   scan_line( p1_top ).begin() + p1_left;
	auto c2 = p.scan_line( p2_top ).begin() + p2_left;
	auto a1 = alpha   ? alpha  .scan_line( p1_top ).begin() + p1_left : nullptr;
	auto a2 = alpha_p ? alpha_p.scan_line( p2_top ).begin() + p2_left : nullptr;
	
	
	//Calculate all the offsets for QtConcurrent::mappedReduced
	vector<Para> lines;
	lines.reserve( height );
	for( unsigned i=0; i<height; i+=stride ){
		lines.push_back( Para( c1+i%stride, c2+i%stride, width, stride, a1, a2 ) );
		c1 += line_width * stride;
		c2 += p.line_width * stride;
		a1 += a1 ? alpha  .line_width * stride : 0;
		a2 += a2 ? alpha_p.line_width * stride : 0;
	}
	
	auto diff_func = fast ? &fast_diff_alpha_line : &diff_alpha_line;
	Sum sum = QtConcurrent::blockingMappedReduced( lines, diff_func, &Sum::reduce );
//	Sum sum; for( auto& p : lines ) sum.reduce( diff_alpha_line( p ) );
	auto full_area = height * width / (stride*stride);
	return  ( full_area * 0.1 > sum.total ) ? std::numeric_limits<double>::max() : sum.average();
}


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



struct img_comp{
	const Plane& img1;
	const Plane& img2;
	const Plane& a1;
	const Plane& a2;
	bool fast;
	int h_middle;
	int v_middle;
	double diff{ -1.0 };
	int level;
	int left;
	int right;
	int top;
	int bottom;
	double precision;
	bool diff_set{ false }; //TODO: name is missleading, interface unclear as well
	
	img_comp( const Plane& image1, const Plane& image2, const Plane& a1, const Plane& a2, bool fast, int hm, int vm, int lvl=0, int l=0, int r=0, int t=0, int b=0, double p=1 )
		:	img1( image1 ), img2( image2 )
		,	a1( a1 ), a2( a2 )
		,	fast(fast)
		,	h_middle( hm ), v_middle( vm )
		,	level( lvl )
		,	left( l ), right( r )
		,	top( t ), bottom( b )
		,	precision( p )
		{ }
	void do_diff(){
		if( !diff_set )
			diff = img1.diffAlpha( img2, a1, a2, h_middle, v_middle, precision, fast );
	}
	void set_diff( double new_diff ){
		diff = new_diff;
		if( diff >= 0 )
			diff_set = true;
	}
	
	MergeResult result( DiffCache *cache ) const{
		if( level > 0 )
			return img1.best_round_sub( img2, a1, a2, level, left, right, top, bottom, cache, fast );
		else
			return MergeResult( {h_middle, v_middle}, diff);
	}
	
	double checkedPercentage(){
		//Find edges
		int x = h_middle;
		int y = v_middle;
		int p1_top = y < 0 ? 0 : y;
		int p2_top = y > 0 ? 0 : -y;
		int p1_left = x < 0 ? 0 : x;
		int p2_left = x > 0 ? 0 : -x;
		unsigned width = min( img1.get_width() - p1_left, img2.get_width() - p2_left );
		unsigned height = min( img1.get_height() - p1_top, img2.get_height() - p2_top );
		return width * height;
	}
	void increasePrecision( double max_checked ){
		precision = max( precision / (max_checked / checkedPercentage()), 1.0 );
	}
};

MergeResult Plane::best_round_sub( const Plane& p, const Plane& a1, const Plane& a2, int level, int left, int right, int top, int bottom, DiffCache *cache, bool fast ) const{
//	qDebug( "Round %d: %d,%d x %d,%d", level, left, right, top, bottom );
	std::vector<img_comp> comps;
	int amount = level*2 + 2;
	double h_offset = (double)(right - left) / amount;
	double v_offset = (double)(bottom - top) / amount;
	level = level > 1 ? level-1 : 1;
	
	if( h_offset < 1 && v_offset < 1 ){
		//Handle trivial step
		//Check every diff in the remaining area
		for( int ix=left; ix<=right; ix++ )
			for( int iy=top; iy<=bottom; iy++ ){
				img_comp t( *this, p, a1, a2, fast, ix, iy );
				t.set_diff( cache->get_diff( ix, iy, 1 ) );
				comps.push_back( t );
			}
	}
	else{
		//Make sure we will not do the same position multiple times
		double h_add = ( h_offset < 1 ) ? 1 : h_offset;
		double v_add = ( v_offset < 1 ) ? 1 : v_offset;
		
		double prec_offset = min( h_offset, v_offset );
		if( h_offset == 0 || v_offset == 0 )
			prec_offset = max( h_offset, v_offset );
		double precision = sqrt(prec_offset);
		
		for( double iy=top+v_offset; iy<=bottom; iy+=v_add )
			for( double ix=left+h_offset; ix<=right; ix+=h_add ){
				int x = ( ix < 0.0 ) ? ceil( ix-0.5 ) : floor( ix+0.5 );
				int y = ( iy < 0.0 ) ? ceil( iy-0.5 ) : floor( iy+0.5 );
				
				//Avoid right-most case. Can't be done in the loop
				//as we always want it to run at least once.
				if( ( x == right && x != left ) || ( y == bottom && y != top ) )
					continue;
				
				//Create and add
				img_comp t(
						*this, p, a1, a2, fast, x, y, level
					,	floor( ix - h_offset ), ceil( ix + h_offset )
					,	floor( iy - v_offset ), ceil( iy + v_offset )
					,	precision
					);
				
				t.set_diff( cache->get_diff( x, y, t.precision ) );
				
				comps.push_back( t );
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
	const img_comp* best = NULL;
	double best_diff = DOUBLE_MAX;
	
	for( auto &c : comps ){
		if( c.diff < best_diff ){
			best = &c;
			best_diff = best->diff;
		}
		
		//Add to cache
		if( !c.diff_set )
			cache->add_diff( c.h_middle, c.v_middle, c.diff, c.precision );
	}
	
	if( !best ){
		qDebug( "ERROR! no result to continue on!!" );
		return MergeResult( {0,0}, DOUBLE_MAX );
	}
	
	return best->result( cache );
}

