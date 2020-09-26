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
#include "../color.hpp"
#include "../debug.hpp"

#include <QtConcurrent>
#include <type_traits>

using namespace std;
using namespace Overmix;


template<typename Out, typename T>
struct EdgeLine{
	//Output
	Out* out;
	unsigned width;
	
	//Kernels
	unsigned size;
	const T *weights_x;
	const T *weights_y;
	unsigned div;
	
	//Operator
	typedef Out (*func_t)( const EdgeLine<Out, T>&, const color_type* );
	func_t func;
	
	//Input, with line_width as we need several lines
	const color_type* in;
	unsigned line_width;
};

template<typename T>
static T calculate_kernel( T *kernel, unsigned size, const color_type* in, unsigned line_width ){
	std::remove_cv_t<T> sum = 0;
	for( unsigned iy=0; iy<size; ++iy ){
		auto row = in + iy*line_width;
		for( unsigned ix=0; ix<size; ++ix, ++kernel, ++row )
			sum += *kernel * *row;
	}
	return sum;
}

template<typename Out, typename T>
static std::pair<T,T> calculate_direction( const EdgeLine<Out, T>& line, const color_type* in ){
	T sum_x = calculate_kernel( line.weights_x, line.size, in, line.line_width );
	T sum_y = calculate_kernel( line.weights_y, line.size, in, line.line_width );
	return std::make_pair(sum_x, sum_y);
}

template<typename T>
static color_type calculate_edge( const EdgeLine<color_type, T>& line, const color_type* in ){
	auto sums = calculate_direction<color_type, T>(line, in);
	T sum = std::abs( sums.first ) + std::abs( sums.second );
	sum /= line.div;
	return color::truncate( sum );
}

template<typename T>
static color_type calculate_zero_edge( const EdgeLine<color_type, T>& line, const color_type* in ){
	//TODO: improve
	int sum = std::max( calculate_kernel( line.weights_x, line.size, in, line.line_width ), 0 );
	sum /= line.div;
	return color::truncate( sum );
}

template<typename Out, typename T>
static void edge_line( const EdgeLine<Out, T>& line ){
	auto in = line.in;
	auto out = line.out;
	unsigned size_half = line.size/2;
	
	//Fill the start of the row with the same value
	auto first = line.func( line, in );
	for( unsigned ix=0; ix<size_half; ++ix )
		*(out++) = first;
	
	unsigned end = line.width - (line.size-size_half);
	for( unsigned ix=size_half; ix<end; ++ix, ++in )
		*(out++) = line.func( line, in );
	
	//Repeat the end with the same value
	auto last = *(out-1);
	for( unsigned ix=end; ix<line.width; ++ix )
		*(out++) = last;
}

template<typename PlaneType, typename T, typename T2>
PlaneType parallel_edge_line( const Plane& p, const vector<T>& weights_x, const vector<T>& weights_y, unsigned div, T2 func ){
	Timer t( "parallel_edge_line" );
	PlaneType out( p.getSize() );
	unsigned size = sqrt( weights_x.size() );
	
	//Calculate all y-lines
	using OutType = typename PlaneType::PixelType;
	std::vector<EdgeLine<OutType, T> > lines;
	for( unsigned iy=0; iy<p.get_height(); ++iy ){
		EdgeLine<OutType, T> line;
		line.out = out.scan_line( iy ).begin();
		line.width = p.get_width(); //TODO: investigate
		
		line.size = size;
		line.weights_x = weights_x.data();
		line.weights_y = weights_y.data();
		line.div = div;
		
		line.func = func;
		
		line.in = p.scan_line( std::min( std::max( int(iy-size/2), 0 ), int(p.get_height()-size-1) ) ).begin(); //Always stay inside
		line.line_width = p.get_line_width();
		
		lines.push_back( line );
	}
	
	QtConcurrent::blockingMap( lines, &edge_line<OutType, T> );
	return out;
}

Plane Plane::edge_zero_generic( const vector<int>& weights, unsigned div ) const{
	return parallel_edge_line<Plane, int, decltype(calculate_zero_edge<int>)>( *this, weights, vector<int>(), div, calculate_zero_edge<int> );
}

Plane Plane::edge_dm_generic( const vector<int>& weights_x, const vector<int>& weights_y, unsigned div ) const{
	return parallel_edge_line<Plane, int, decltype(calculate_edge<int>)>( *this, weights_x, weights_y, div, calculate_edge<int> );
}

PlaneBase<std::pair<int,int>> Plane::edge_dm_direction( const vector<int>& weights_x, const vector<int>& weights_y, unsigned div ) const{
	auto func = calculate_direction<std::pair<int,int>, int>;
	return parallel_edge_line<PlaneBase<std::pair<int,int>>, int, decltype(func)>( *this, weights_x, weights_y, div, func );
}
