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

#ifndef COLOR_HPP
#define COLOR_HPP

#include <algorithm>
#include <cstdint>
#include <cmath>
#include <limits>

namespace Overmix{

typedef short color_type;
typedef int precision_color_type;

struct color{
	color_type r;
	color_type g;
	color_type b;
	color_type a;
	
	constexpr static color_type WHITE = std::numeric_limits<color_type>::max()/2;
	constexpr static color_type BLACK = 0;
	constexpr static color_type MIN_VAL = std::numeric_limits<color_type>::min();
	constexpr static color_type MAX_VAL = std::numeric_limits<color_type>::max();
	
	template<typename T>
	constexpr static color_type truncate( T value )
		{ return std::min( std::max( value, (T)BLACK ), (T)WHITE ); }
	
	template<typename T>
	constexpr static color_type truncateFullRange( T value )
		{ return std::min( std::max( value, (T)MIN_VAL ), (T)MAX_VAL ); }
	
	constexpr static double asDouble( color_type value ){
		return value / (double)WHITE;
	}
	constexpr static color_type fromDouble( double value ){
		return truncateFullRange( value * WHITE + 0.5 );
	}
	constexpr static unsigned char as8bit( color_type value ){
		return asDouble( value ) * 255;
	}
	constexpr static uint16_t as16bit( color_type value ){
		return asDouble( value ) * 0xFFFF;
	}
	constexpr static color_type from8bit( unsigned char value ){
		return fromDouble( value / 255.0 );
	}
	constexpr static color_type from16bit( uint16_t value ){
		return fromDouble( value / double(std::numeric_limits<uint16_t>::max()) );
	}
	
	public:
		color( color_type r, color_type g, color_type b, color_type a = WHITE )
			:	r(r), g(g), b(b), a(a) { }
	
	public:
		static double sRgb2linear( double v ){
			return ( v <= 0.04045 ) ? v / 12.92 : std::pow( (v+0.055)/1.055, 2.4 );
		}
		static double linear2sRgb( double v ){
			return ( v <= 0.0031308 ) ? 12.92 * v : 1.055*std::pow( v, 1.0/2.4 ) - 0.055;
		}
		static double ycbcr2linear( double v ){
			//rec. 601 and 709
			return ( v < 0.08125 ) ? 1.0/4.5 * v : std::pow( (v+0.099)/1.099, 1.0/0.45 );
		}
	
	
	public:
		color ycbcrToRgb( double kr, double kg, double kb, bool gamma, bool swing );
		color rgbToYcbcr( double kr, double kg, double kb, bool gamma, bool swing );
		
		color jpegToRgb( bool gamma=true )
			{ return ycbcrToRgb( 0.299,  0.587,  0.114,  gamma, false ); }
		color rec601ToRgb( bool gamma=true )
			{ return ycbcrToRgb( 0.299,  0.587,  0.114,  gamma, true  ); }
		color rec709ToRgb( bool gamma=true )
			{ return ycbcrToRgb( 0.2126, 0.7152, 0.0722, gamma, true  ); }
		
	void clear(){
		r = b = g = a = 0;
	}
	
	void trunc( color_type max ){
		r = ( r > max ) ? max : r;
		g = ( g > max ) ? max : g;
		b = ( b > max ) ? max : b;
		a = ( a > max ) ? max : a;
	}
	
	color& operator+=( const color &rhs ){
		r += rhs.r;
		g += rhs.g;
		b += rhs.b;
		a += rhs.a;
		return *this;
	}
	
	color& operator-=( const color &rhs ){
		r -= rhs.r;
		g -= rhs.g;
		b -= rhs.b;
		a -= rhs.a;
		return *this;
	}
	
	color& operator+=( const color_type &rhs ){
		r += rhs;
		g += rhs;
		b += rhs;
		a += rhs;
		return *this;
	}
	
	color& operator*=( const color_type &rhs ){
		r *= rhs;
		g *= rhs;
		b *= rhs;
		a *= rhs;
		return *this;
	}
	
	color& operator/=( const color_type &rhs ){
		r /= rhs;
		g /= rhs;
		b /= rhs;
		a /= rhs;
		return *this;
	}
	
	const color operator+( const color &other ) const{
		return color(*this) += other;
	}
	const color operator+( const color_type &other ) const{
		return color(*this) += other;
	}
	const color operator-( const color &other ) const{
		return color(*this) -= other;
	}
	const color operator*( const color_type &other ) const{
		return color(*this) *= other;
	}
	const color operator/( const color_type &other ) const{
		return color(*this) /= other;
	}
};

}

#endif
