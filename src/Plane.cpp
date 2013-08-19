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
#include <algorithm> //For min
#include <cstdint> //For abs(int) and uint*_t

#include <QtConcurrent>
#include <QDebug>

const unsigned long Plane::MAX_VAL = 0xFFFFFFFF;

Plane::Plane( unsigned w, unsigned h ){
	height = h;
	width = w;
	line_width = w;
	data = new color_type[ h * line_width ];
}

Plane::~Plane(){
	if( data )
		delete[] data;
}

bool Plane::is_interlaced() const{
	double avg2 = 0;
	for( unsigned iy=0; iy<get_height()/2*2; ){
		color_type *row1 = scan_line( iy++ );
		color_type *row2 = scan_line( iy++ );
		
		unsigned long line_avg = 0;
		for( unsigned ix=0; ix<get_width(); ++ix ){
			color_type diff = row2[ix] > row1[ix] ? row2[ix] - row1[ix] : row1[ix] - row2[ix];
			line_avg += (unsigned long)diff*diff;
		}
		avg2 += (double)line_avg / get_width();
	}
	avg2 /= get_height()/2;
	avg2 /= 0xFFFF;
	avg2 /= 0xFFFF;
	
	qDebug( "interlace factor: %f", avg2 );
	return avg2 > 0.0015; //NOTE: Based on experiments, not reliable!
}

void Plane::replace_line( Plane &p, bool top ){
	if( get_height() != p.get_height() || get_width() != p.get_width() ){
		qWarning( "replace_line: Planes not equaly sized!" );
		return;
	}
	
	for( unsigned iy=(top ? 0 : 1); iy<get_height(); iy+=2 ){
		color_type *row1 = scan_line( iy );
		color_type *row2 = p.scan_line( iy );
		
		for( unsigned ix=0; ix<get_width(); ++ix )
			row1[ix] = row2[ix];
	}
}

void Plane::combine_line( Plane &p, bool top ){
	if( get_height() != p.get_height() || get_width() != p.get_width() ){
		qWarning( "combine_line: Planes not equaly sized!" );
		return;
	}
	
	for( unsigned iy=(top ? 0 : 1); iy<get_height(); iy+=2 ){
		color_type *row1 = scan_line( iy );
		color_type *row2 = p.scan_line( iy );
		
		for( unsigned ix=0; ix<get_width(); ++ix )
			row1[ix] = ( (unsigned)row1[ix] + row2[ix] ) / 2;
	}
}


struct Sum{
	uint64_t total;
	Sum() : total( 0 ) { }
	void reduce( const uint64_t add ){ total += add; }
};
struct Para{
	color_type *c1;
	color_type *c2;
	unsigned width;
	unsigned stride;
	Para( color_type* c1, color_type *c2, unsigned width, unsigned stride )
		:	c1( c1 ), c2( c2 ), width( width ), stride( stride )
		{ }
};
static uint64_t diff_2_line( Para p ){
	uint64_t sum = 0;
	for( color_type* end=p.c1+p.width; p.c1<end; p.c1+=p.stride, p.c2+=p.stride )
		sum += std::abs( *p.c1 - *p.c2 );
	
	return sum;
}

double Plane::diff( const Plane& p, int x, int y, unsigned stride ) const{
	//Find edges
	int p1_top = y < 0 ? 0 : y;
	int p2_top = y > 0 ? 0 : -y;
	int p1_left = x < 0 ? 0 : x;
	int p2_left = x > 0 ? 0 : -x;
	unsigned width = std::min( get_width() - p1_left, p.get_width() - p2_left );
	unsigned height = std::min( get_height() - p1_top, p.get_height() - p2_top );
	
	//Initial offsets on the two planes
	color_type* c1 = scan_line( p1_top ) + p1_left;
	color_type* c2 = p.scan_line( p2_top ) + p2_left;
	
	//Calculate all the offsets for QtConcurrent::mappedReduced
	std::vector<Para> lines;
	lines.reserve( height );
	for( unsigned i=0; i<height; i+=stride ){
		lines.push_back( Para( c1+i%stride, c2+i%stride, width, stride ) );
		c1 += line_width;
		c2 += p.line_width;
	}
	
	Sum sum = QtConcurrent::blockingMappedReduced( lines, &diff_2_line, &Sum::reduce );
	return sum.total / (double)( height * width / (stride*stride) );
}


Plane* Plane::scale_nearest( unsigned wanted_width, unsigned wanted_height, double offset_x, double offset_y ) const{
	if( offset_x <= -1.0 || offset_x >= 1.0 || offset_y <= -1.0 || offset_y >= 1.0 )
		return 0;
	
	Plane *scaled = new Plane( wanted_width, wanted_height );
	if( !scaled || scaled->is_invalid() )
		return 0;
	
	for( unsigned iy=0; iy<wanted_height; iy++ ){
		color_type* row = scaled->scan_line( iy );
		for( unsigned ix=0; ix<wanted_width; ix++ ){
			double pos_x = ((double)ix / wanted_width) * width;
			double pos_y = ((double)iy / wanted_height) * height;
			
			row[ix] = pixel( floor( pos_x ), floor( pos_y ) );
		}
	}
	
	return scaled;
}


Plane* Plane::scale_linear( unsigned wanted_width, unsigned wanted_height, double offset_x, double offset_y ) const{
	if( offset_x <= -1.0 || offset_x >= 1.0 || offset_y <= -1.0 || offset_y >= 1.0 )
		return 0;
	
	Plane *scaled = new Plane( wanted_width, wanted_height );
	if( !scaled || scaled->is_invalid() )
		return 0;
	
	return 0; //Not implemented
}


Plane* Plane::scale_bilinear( unsigned wanted_width, unsigned wanted_height, double offset_x, double offset_y ) const{
	if( offset_x <= -1.0 || offset_x >= 1.0 || offset_y <= -1.0 || offset_y >= 1.0 )
		return 0;
	
	Plane *scaled = new Plane( wanted_width, wanted_height );
	if( !scaled || scaled->is_invalid() )
		return 0;
	
	return 0; //Not implemented
}


Plane* Plane::scale_cubic( unsigned wanted_width, unsigned wanted_height, double offset_x, double offset_y ) const{
	if( offset_x <= -1.0 || offset_x >= 1.0 || offset_y <= -1.0 || offset_y >= 1.0 )
		return 0;
	
	Plane *scaled = new Plane( wanted_width, wanted_height );
	if( !scaled || scaled->is_invalid() )
		return 0;
	
	return 0; //Not implemented
}


Plane* Plane::scale_lanczos( unsigned wanted_width, unsigned wanted_height, double offset_x, double offset_y ) const{
	if( offset_x <= -1.0 || offset_x >= 1.0 || offset_y <= -1.0 || offset_y >= 1.0 )
		return 0;
	
	Plane *scaled = new Plane( wanted_width, wanted_height );
	if( !scaled || scaled->is_invalid() )
		return 0;
	
	return 0; //Not implemented
}

