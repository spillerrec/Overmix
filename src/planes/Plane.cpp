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
#include <algorithm> //For min, max
#include <numeric> //accumulate
#include <cstdlib> //For abs(int)
#include <cassert>
#include <utility>
#include <QDebug>

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

double Plane::meanSquaredError( const Plane& other ) const{
	assert( getSize() == other.getSize() );
	double mean = 0.0;
	for( unsigned iy=0; iy<get_height(); iy++ ){
		double local_mean = 0.0;
		for( auto row : makeZipRowIt( scan_line(iy), other.scan_line(iy) ) ){
			auto err = color::asDouble( row.first ) - color::asDouble( row.second );
			local_mean += err*err;
		}
		mean += local_mean / get_width();
	}
	return mean / get_height();
}

static Plane everySecond( const Plane& p, bool even=true ){
	//TODO: assert that height is even
	auto offset = even ? 0 : 1;
	
	Plane out( p.get_width(), p.get_height()/2 );
	for( unsigned iy=0; iy<out.get_height(); iy++ )
		for( auto pix : makeZipRowIt( out.scan_line( iy ), p.scan_line( iy*2+offset ) ) )
			pix.first = pix.second;
	return out;
}

bool Plane::is_interlaced() const{
	auto frame1 = everySecond( *this, true  );
	auto frame2 = everySecond( *this, false );
	
	DiffCache cache;
	auto offset = frame1.best_round_sub( frame2, {}, {}, 20, 0, 0, -10, 10, &cache, false ).first;
	
	double diff_normal    = frame1.diff( frame2, 0, 0 );
	double diff_interlace;
	if( offset.y == 0 || offset.y == 1 )
		diff_interlace = frame1.diff( frame1, 0, 1 )/2 + frame2.diff( frame2, 0, 1 )/2;
	else
		diff_interlace = frame1.diff( frame2, offset.x, offset.y );
		
	return diff_interlace < diff_normal*0.95;
}

bool Plane::is_interlaced( const Plane& previous ) const{
	if( !previous )
		return is_interlaced();
	
	auto frame_old = everySecond( previous, false );
	
	auto frame1 = everySecond( *this, true  );
	auto frame2 = everySecond( *this, false );
	
	auto diff_previous = frame1.diff( frame_old, 0, 0 );
	auto diff_normal   = frame1.diff( frame2,    0, 0 );
	
	return diff_previous < diff_normal*0.95;
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

void Plane::copyAlpha(
		Plane& out_alpha
	,	const Plane& source, const Plane& source_alpha
	,	Point<unsigned> source_pos, Size<unsigned> source_size
	,	Point<unsigned> to_pos
	){
	//Make sure all planes are equal size
	assert( getSize() == source.getSize() );
	assert( getSize() == out_alpha );
	assert( getSize() == source_alpha.getSize() );
	
	//Source-over algorithm:
	// value = a_src * src + a_dest * (1 - a_src) * dest
	// alpha = a_src       + a_dest * (1 - a_src)

	//End positions in source image
	auto end_x = std::min( get_width() , to_pos.x + source_size.width()  );
	auto end_y = std::min( get_height(), to_pos.y + source_size.height() );

	//Iterate over each pixel
	for( unsigned iy=to_pos.y; iy<end_y; iy++ ){
		//Apply offsets
		auto a_src  = out_alpha[ iy ];
		auto   src  = (*this)  [ iy ];
		auto a_dest = source      [ iy - to_pos.y ].begin() - to_pos.x;
		auto   dest = source_alpha[ iy - to_pos.y ].begin() - to_pos.x;
		//TODO: sub-rect iterator?
		
		for( unsigned ix=to_pos.y; ix<end_x; ix++ ){
			// a_dest * (1 - a_src)
			auto da = color::asDouble(a_dest[ix]) * (1 - color::asDouble(a_src[ix]) );
			
			src[ix] = color::fromDouble(
					color::asDouble(src[ix]) * color::asDouble(a_src[ix])
				+  da * color::asDouble(dest[ix])
				);
			
			a_src[ix] = color::fromDouble( color::asDouble(a_src[ix]) + da );
		}
	}
}

