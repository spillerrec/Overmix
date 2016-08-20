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

#include "PatternRemove.hpp"

#include "ImageEx.hpp"
#include "../color.hpp"

using namespace Overmix;

#include <QImage>

template<typename T>
class Average{
	private:
		T sum = { 0 };
		unsigned amount = { 0 };
		
	public:
		T operator()() const{ return sum / amount; }
		Average& operator+=( T add ){
			sum += add;
			amount++;
			return *this;
		}
};

Plane   Overmix::patternRemove( const Plane&   p  , Point<double> size ){
	auto mean = p.blur_box( size.x, size.y );
	
	//TODO:
	auto out = p;
	for( unsigned iy=0; iy<out.get_height(); iy++ ){
		auto row = out.scan_line( iy );
		auto m = mean.scan_line( iy );
		for( unsigned ix=0; ix<out.get_width(); ix++ ){
			row[ix] = color::fromDouble( (color::asDouble(row[ix]) - color::asDouble(m[ix]) + color::WHITE*0.5) / 2 );
		}
	}
	
	PlaneBase<Average<double>> pattern( size.ceil().to<unsigned>() );
	pattern.fill( Average<double>() );
	auto index = [&]( unsigned i, double size ){
		return unsigned( std::min( std::round( std::fmod( i, size ) ), std::floor(size) ) );
	};
	
	for( unsigned iy=0; iy<p.get_height(); iy++ ){
		auto row   = p      .scan_line(        iy           );
		auto row_p = pattern.scan_line( index( iy, size.y ) );
		
		for( unsigned ix=0; ix<p.get_width(); ix++ ){
			row_p[ index( ix, size.x ) ] += color::asDouble( row[ix] );
		}
	}
	
	/*
	Plane out_pattern( pattern.getSize() );
	for( unsigned iy=0; iy<out_pattern.get_height(); iy++ ){
		auto row_out = out_pattern.scan_line( iy );
		auto row_in  =     pattern.scan_line( iy );
		
		for( unsigned ix=0; ix<out_pattern.get_width(); ix++ )
			row_out[ix] = color::fromDouble( row_in[ix]() );
	}
	
	return out_pattern.normalize();
	//*/
	
	Plane full_pattern( p.getSize() );
	
	for( unsigned iy=0; iy<full_pattern.get_height(); iy++ ){
		auto row_out = full_pattern.scan_line(        iy           );
		auto row_in  =      pattern.scan_line( index( iy, size.y ) );
		
		for( unsigned ix=0; ix<full_pattern.get_width(); ix++ )
			row_out[ ix ] = color::fromDouble( row_in[ index( ix, size.x ) ]() );
	}
	
	return full_pattern.normalize();
}


ImageEx Overmix::patternRemove( const ImageEx& img, Point<double> size ){
	return ImageEx{ patternRemove( img[0], size ) }; //TODO: Do on each plane
}

