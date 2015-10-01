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
#include <algorithm> //For min, max, and accumulate
#include <cstdlib> //For abs(int)
#include <cassert>
#include <utility>

#include "../color.hpp"

using namespace Overmix;

color_type Plane::min_value() const{
	color_type min = color::MAX_VAL;
	for( auto row : *this )
		for( auto val : row )
			min = std::min( min, val );
	return min;
}
color_type Plane::max_value() const{
	color_type max = color::MIN_VAL;
	for( auto row : *this )
		for( auto val : row )
			max = std::max( max, val );
	return max;
}
color_type Plane::mean_value() const{
	precision_color_type avg = color::BLACK;
	for( auto row : *this )
		avg += std::accumulate( row.begin(), row.end(), precision_color_type(color::BLACK) ) / row.width();
	return avg / get_height();
}

bool Plane::is_interlaced() const{
	double avg2_uneven = 0, avg2_even = 0;
	for( unsigned iy=0; iy<get_height()/4*4; iy+=4 ){
		auto row1 = scan_line( iy+0 );
		auto row2 = scan_line( iy+1 );
		auto row3 = scan_line( iy+2 );
		auto row4 = scan_line( iy+3 );
		
		unsigned long long line_avg_uneven = 0, line_avg_even = 0;
		for( unsigned ix=0; ix<get_width(); ++ix ){
			color_type diff_uneven = std::abs( row2[ix]-row1[ix] ) + std::abs( row4[ix]-row3[ix] );
			color_type diff_even   = std::abs( row3[ix]-row1[ix] ) + std::abs( row4[ix]-row2[ix] );
			line_avg_uneven += (unsigned long long)diff_uneven*diff_uneven;
			line_avg_even   += (unsigned long long)diff_even  *diff_even;
		}
		avg2_uneven += (double)line_avg_uneven / get_width();
		avg2_even   += (double)line_avg_even   / get_width();
	}
	avg2_uneven /= get_height()/2;
	avg2_uneven /= 0xFFFF;
	avg2_uneven /= 0xFFFF;
	avg2_even /= get_height()/2;
	avg2_even /= 0xFFFF;
	avg2_even /= 0xFFFF;
	
	qDebug( "interlace factor: %f > %f", avg2_uneven, avg2_even );
	return avg2_uneven > avg2_even;
}

void Plane::replace_line( const Plane &p, bool top ){
	if( get_height() != p.get_height() || get_width() != p.get_width() ){
		qWarning( "replace_line: Planes not equaly sized!" );
		return;
	}
	
	for( unsigned iy=(top ? 0 : 1); iy<get_height(); iy+=2 )
		for( auto val : makeZipRowIt( scan_line(iy), p.scan_line( iy ) ) )
			val.first = val.second;
}

void Plane::combine_line( const Plane &p, bool top ){
	if( get_height() != p.get_height() || get_width() != p.get_width() ){
		qWarning( "combine_line: Planes not equaly sized!" );
		return;
	}
	
	for( unsigned iy=(top ? 0 : 1); iy<get_height(); iy+=2 )
		for( auto val : makeZipRowIt( scan_line(iy), p.scan_line( iy ) ) )
			val.first = ( (precision_color_type)val.first + val.second ) / 2;
}

Plane Plane::normalize() const{
	return level( min_value(), max_value(), color::BLACK, color::WHITE, 1.0 );
}

Plane Plane::maxPlane( const Plane& p ) const{
	assert( getSize() == p.getSize() );
	auto out = *this;
	for( unsigned iy=0; iy<get_height(); ++iy )
		for( auto val : makeZipRowIt( out.scan_line(iy), p.scan_line( iy ) ) )
			val.first = std::max( val.first, val.second );
	return out;
}

Plane Plane::minPlane( const Plane& p ) const{
	assert( getSize() == p.getSize() );
	auto out = *this;
	for( unsigned iy=0; iy<get_height(); ++iy )
		for( auto val : makeZipRowIt( out.scan_line(iy), p.scan_line( iy ) ) )
			val.first = std::min( val.first, val.second );
	return out;
}

