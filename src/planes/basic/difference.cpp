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

#include "difference.hpp"

#include <algorithm>

#include "../Plane.hpp"
#include "../PlaneExcept.hpp"


using namespace std;
using namespace Overmix;

using DiffAmount = pair<precision_color_type, double>;
struct Sum{
	double sum{ 0.0 };
	double total{ 0 };
	unsigned count{ 0 };
	
	double average() const{ return sum / count; }
	Sum& operator+=( DiffAmount add ){
		sum += add.first / add.second;
		total += add.second;
		count++;
		return *this;
	}
	
	Sum& operator+=( const Sum& other ){
		sum   += other.sum;
		total += other.total;
		count += other.count;
		return *this;
	}
};

#pragma omp declare reduction(SumAdd: Sum: \
	omp_out += omp_in) 

struct Para{
	const color_type *c1;
	const color_type *c2;
	const color_type *a1;
	const color_type *a2;
	unsigned width;
	unsigned stride;
	color_type epsilon;
	
	Para( const color_type* c1, const color_type *c2, unsigned width, unsigned stride, color_type epsilon
		,	const color_type* a1=nullptr, const color_type *a2=nullptr )
		:	c1( c1 ), c2( c2 ), a1( a1 ), a2( a2 ), width( width ), stride( stride ), epsilon(epsilon) { }
	
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
		auto val = color::asDouble( distance_L1_checked( i ) );
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


double Difference::simple( const Plane& p1, const Plane& p2, Point<int> offset, SimpleSettings s ){
	Plane empty;
	return simpleAlpha( p1, p2, empty, empty, offset, s );
}


double Difference::simpleAlpha( const Plane& p1, const Plane& p2, const Plane& alpha1, const Plane& alpha2, Point<int> offset, SimpleSettings s ){
	planeSizeEqual( "Difference::simpleAlpha", p1, p2 );
	if( alpha1 )
		planeSizeEqual( "Difference::simpleAlpha", p1, alpha1 );
	if( alpha2 )
		planeSizeEqual( "Difference::simpleAlpha", p2, alpha2 );
	
	//Find edges
	int p1_top  = offset.y < 0 ? 0 :  offset.y;
	int p2_top  = offset.y > 0 ? 0 : -offset.y;
	int p1_left = offset.x < 0 ? 0 :  offset.x;
	int p2_left = offset.x > 0 ? 0 : -offset.x;
	unsigned width  = min( p1.get_width()  - p1_left, p2.get_width()  - p2_left );
	unsigned height = min( p1.get_height() - p1_top,  p2.get_height() - p2_top  );
	
	//Initial offsets on the two planes
	//TODO: avoid using pointer math
	auto c1 = p1.scan_line( p1_top ).begin() + p1_left;
	auto c2 = p2.scan_line( p2_top ).begin() + p2_left;
	auto a1 = alpha1 ? alpha1.scan_line( p1_top ).begin() + p1_left : nullptr;
	auto a2 = alpha2 ? alpha2.scan_line( p2_top ).begin() + p2_left : nullptr;
	
	
	//Calculate all the offsets for QtConcurrent::mappedReduced
	vector<Para> lines;
	lines.reserve( height );
	for( unsigned i=0; i<height; i+=s.stride ){
		//Use i%s.stride to offset each line so we don't start the same place every time
		lines.push_back( Para( c1+i%s.stride, c2+i%s.stride, width, s.stride, s.epsilon, a1, a2 ) );
		c1 += p1.get_line_width() * s.stride;
		c2 += p2.get_line_width() * s.stride;
		a1 += a1 ? alpha1.get_line_width() * s.stride : 0;
		a2 += a2 ? alpha2.get_line_width() * s.stride : 0;
		//TODO: Avoid get_line_width?
	}
	
	Sum sum;
	if( s.epsilon == 0 ){
		#pragma omp parallel for reduction(SumAdd:sum)
		for( unsigned i=0; i<lines.size(); i++ )
			if( s.use_l2 )
				sum += lines[i].diff_line( &Para::distance_L2 );
			else
				sum += lines[i].diff_line( &Para::distance_L1 );
	}
	else{
		#pragma omp parallel for reduction(SumAdd:sum)
		for( unsigned i=0; i<lines.size(); i++ )
			if( s.use_l2 )
				sum += lines[i].diff_line( &Para::distance_L2_checked );
			else
				sum += lines[i].diff_line( &Para::distance_L1_checked );
	}
	
	//If our stride causes too much reducin
	//NOTE: This seems risky, as it might cause us to disregard results? Maybe we should return NAN?
	auto full_area = height * width / (s.stride*s.stride);
	//TODO: configure this
	return  ( full_area * 0.1 > sum.total ) ? std::numeric_limits<double>::max() : sum.average();
}

