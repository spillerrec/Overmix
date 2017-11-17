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

#include <QtConcurrent>
#include <QDebug>
#include <algorithm>

#include "../color.hpp"
#include "../comparators/GradientPlane.hpp"

using namespace std;
using namespace Overmix;

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
	
	color_type distance_L1( unsigned i ) const{ 
		return std::abs( c1[i] - c2[i] );
	}
	color_type distance_L1_checked( unsigned i ) const{
		//Ignore small differences
		auto val = distance_L1( i );
		return val > epsilon ? val-epsilon : 0;
	}
	
	color_type distance_L2( unsigned i ) const{
		auto val = color::asDouble( distance_L1( i ) );
		return color::fromDouble( val * val );
	}
	
	color_type distance_L2_checked( unsigned i ) const{
		auto val = color::asDouble( distance_L1( i ) );
		//Ignore small differences
		val = val > epsilon ? val-epsilon : 0;
		return color::fromDouble( val * val );
	}
	
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
static DiffAmount diff_alpha_line(      Para p ){ return p.diff_line( &Para::distance_L2_checked ); }
static DiffAmount fast_diff_alpha_line( Para p ){ return p.diff_line( &Para::distance_L2 ); }

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

MergeResult Plane::best_round_sub( const Plane& p, const Plane& a1, const Plane& a2, int level, int left, int right, int top, int bottom, bool fast ) const{
	GradientPlane gradient( *this, p, a1, a2, fast );
	return gradient.findMinimum( { left, right, top, bottom } );
}

