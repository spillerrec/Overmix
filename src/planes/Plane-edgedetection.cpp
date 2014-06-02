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

#include <QtConcurrent>
#include <QDebug>

using namespace std;


template<typename T>
struct EdgeLine{
	//Output
	color_type* out;
	unsigned width;
	
	//Kernels
	unsigned size;
	T *weights_x;
	T *weights_y;
	unsigned div;
	
	//Operator
	typedef color_type (*func_t)( const EdgeLine<T>&, color_type* );
	func_t func;
	
	//Input, with line_width as we need several lines
	color_type* in;
	unsigned line_width;
};

template<typename T>
static T calculate_kernel( T *kernel, unsigned size, color_type* in, unsigned line_width ){
	T sum = 0;
	for( unsigned iy=0; iy<size; ++iy ){
		color_type *row = in + iy*line_width;
		for( unsigned ix=0; ix<size; ++ix, ++kernel, ++row )
			sum += *kernel * *row;
	}
	return sum;
}

template<typename T>
static color_type calculate_edge( const EdgeLine<T>& line, color_type* in ){
	using namespace std;
	
	int sum_x = calculate_kernel( line.weights_x, line.size, in, line.line_width );
	int sum_y = calculate_kernel( line.weights_y, line.size, in, line.line_width );
	int sum = abs( sum_x ) + abs( sum_y );
	sum /= line.div;
	return color::truncate( sum );
}

template<typename T>
static color_type calculate_zero_edge( const EdgeLine<T>& line, color_type* in ){
	using namespace std;
	//TODO: improve
	int sum = max( calculate_kernel( line.weights_x, line.size, in, line.line_width ), 0 );
	sum /= line.div;
	return color::truncate( sum );
}

template<typename T>
static void edge_line( const EdgeLine<T>& line ){
	color_type *in = line.in;
	color_type *out = line.out;
	unsigned size_half = line.size/2;
	
	//Fill the start of the row with the same value
	color_type first = line.func( line, in );
	for( unsigned ix=0; ix<size_half; ++ix )
		*(out++) = first;
	
	unsigned end = line.width - (line.size-size_half);
	for( unsigned ix=size_half; ix<end; ++ix, ++in )
		*(out++) = line.func( line, in );
	
	//Repeat the end with the same value
	color_type last = *(out-1);
	for( unsigned ix=end; ix<line.width; ++ix )
		*(out++) = last;
}


template<typename T, typename T2>
Plane parallel_edge_line( const Plane& p, vector<T> weights_x, vector<T> weights_y, unsigned div, T2 func ){
	Plane out( p.get_width(), p.get_height() );
	unsigned size = sqrt( weights_x.size() );
	
	//Calculate all y-lines
	std::vector<EdgeLine<T> > lines;
	for( unsigned iy=0; iy<p.get_height(); ++iy ){
		EdgeLine<T> line;
		line.out = out.scan_line( iy );
		line.width = p.get_width();
		
		line.size = size;
		line.weights_x = weights_x.data();
		line.weights_y = weights_y.data();
		line.div = div;
		
		line.func = func;
		
		line.in = p.scan_line( std::min( std::max( int(iy-size/2), 0 ), int(p.get_height()-size-1) ) ); //Always stay inside
		line.line_width = p.get_line_width();
		
		lines.push_back( line );
	}
	
	QtConcurrent::blockingMap( lines, &edge_line<T> );
	return out;
}

Plane Plane::edge_zero_generic( vector<int> weights, unsigned div ) const{
	return parallel_edge_line( *this, weights, vector<int>(), div, calculate_zero_edge<int> );
}

Plane Plane::edge_dm_generic( vector<int> weights_x, vector<int> weights_y, unsigned div ) const{
	return parallel_edge_line( *this, weights_x, weights_y, div, calculate_edge<int> );
}
