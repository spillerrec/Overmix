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
#include <vector>
#include <utility>
#include <QDebug>
#include <png++/png.hpp>

#include "../color.hpp"
#include "basic/difference.hpp"
#include "../comparators/GradientPlane.hpp"

using namespace Overmix;

PlaneBase<double> Plane::toDouble() const{
	return map( color::asDouble );
}
PlaneBase<uint8_t> Plane::to8Bit() const{
	return map( color::as8bit );
}
PlaneBase<uint8_t> Plane::to8BitDither() const{
	std::vector<color_type> line( get_width()+1, 0 );
	
	PlaneBase<uint8_t> out( getSize() );
	for( unsigned iy=0; iy<get_height(); iy++ ){
		auto row_out = out.scan_line( iy );
		auto row_in  =     scan_line( iy );
		for( unsigned ix=0; ix<get_width(); ix++ ){
			auto p = color::truncate( row_in[ix] + line[ix] );
			
			row_out[ix] = color::as8bit( p );
			
			color_type err = p - color::from8bit( row_out[ix] );
			line[ix] = err / 4;
			line[ix+1] += err / 2;
			if( ix )
				line[ix-1] += err / 4;
		}
	}
	return out;
}

void Plane::save_png(std::string path) const{
	if( get_width()==0 || get_height() == 0 )
		throw std::logic_error("Tried to " + path + " with invalid dimensions " + std::to_string(get_width()) + "x" + std::to_string(get_height()));
	
	png::image< png::gray_pixel_16 > image(get_width(), get_height());
	
	for( unsigned iy=0; iy<get_height(); iy++ ){
		auto line = scan_line( iy );
		for( unsigned ix=0; ix<get_width(); ix++ )
			image[iy][ix] = line[ix] << 2;
	}
	
	image.write(path);
}

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
	Difference::SimpleSettings settings; //TODO: configure?
	auto frame1 = everySecond( *this, true  );
	auto frame2 = everySecond( *this, false );
	
	GradientPlane gradient( frame1, frame2, {}, {}, settings );
	auto offset = gradient.findMinimum( { 0, 0, -10, 10, 20 } ).distance; //TODO: configure
	
	double diff_normal    = Difference::simple( frame1, frame2, {0, 0}, settings );
	double diff_interlace;
	if( offset.y == 0 || offset.y == 1 )
		diff_interlace = Difference::simple( frame1, frame1, {0, 1}, settings )/2 + Difference::simple( frame2, frame2, {0, 1}, settings )/2;
	else
		diff_interlace = Difference::simple( frame1, frame2, offset, settings );
		
	return diff_interlace < diff_normal*0.95;
}

bool Plane::is_interlaced( const Plane& previous ) const{
	Difference::SimpleSettings settings; //TODO: configure?
	if( !previous )
		return is_interlaced();
	
	auto frame_old = everySecond( previous, false );
	
	auto frame1 = everySecond( *this, true  );
	auto frame2 = everySecond( *this, false );
	
	auto diff_previous = Difference::simple( frame1, frame_old, {0, 0} );
	auto diff_normal   = Difference::simple( frame1, frame2,    {0, 0} );
	
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
	auto out = Plane(*this);
	for( unsigned iy=0; iy<get_height(); ++iy )
		for( auto val : makeZipRowIt( out.scan_line(iy), p.scan_line( iy ) ) )
			val.first = std::max( val.first, val.second );
	return out;
}

Plane Plane::minPlane( const Plane& p ) const{
	assert( getSize() == p.getSize() );
	auto out = Plane(*this);
	for( unsigned iy=0; iy<get_height(); ++iy )
		for( auto val : makeZipRowIt( out.scan_line(iy), p.scan_line( iy ) ) )
			val.first = std::min( val.first, val.second );
	return out;
}

