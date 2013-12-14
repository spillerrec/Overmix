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

#ifndef COLOR_H
#define COLOR_H

#include <QCoreApplication>
#include <QColor>
#include <cmath>
#include <limits>

typedef int color_type;
typedef long long precision_color_type;

struct color{
	color_type r;
	color_type g;
	color_type b;
	color_type a;
	
	constexpr static color_type WHITE = std::numeric_limits<color_type>::max()/2;
	constexpr static color_type BLACK = 0;
	constexpr static color_type MIN_VAL = std::numeric_limits<color_type>::min();
	constexpr static color_type MAX_VAL = std::numeric_limits<color_type>::max();
	
	constexpr static double as_double( color_type value ){
		return value / (double)WHITE;
	}
	constexpr static color_type from_double( double value ){
		return value * WHITE;
	}
	constexpr static unsigned char as_8bit( color_type value ){
		return as_double( value ) * 255;
	}
	constexpr static color_type from_8bit( unsigned char value ){
		return from_double( value / 255.0 );
	}
	
	static color_type truncate( precision_color_type value ){
		return std::min( std::max( value, (precision_color_type)color::MIN_VAL ), (precision_color_type)color::MAX_VAL );
	}
	
	public:
		//TODO: we do not handle the case of preallocated storage!
		void* operator new( size_t size ){
			return std::calloc( 1, size );
		}
		void* operator new[]( size_t size ){
			return std::calloc( size, 1 );
		}
		void operator delete( void* p )  { std::free( p ); }
		void operator delete[]( void* p ){ std::free( p ); }
		
	
		color() { } //Initialized by new/delete
		color( color_type r, color_type g, color_type b, color_type a = WHITE ){
			this->r = r;
			this->g = g;
			this->b = b;
			this->a = a;
		}
		color( color* c ){
			r = c->r;
			g = c->g;
			b = c->b;
			a = c->a;
		}
		color( QRgb c ){
			r = from_8bit( qRed( c ) );
			g = from_8bit( qGreen( c ) );
			b = from_8bit( qBlue( c ) );
			a = from_8bit( qAlpha( c ) );
		//	linearize();
		}
	
	private:
		static color_type sRgb2linear( color_type value ){
			double v = as_double( value );
			v = ( v <= 0.04045 ) ? v / 12.92 : std::pow( (v+0.055)/1.055, 2.4 );
			return from_double( v );
		}
		static color_type linear2sRgb( color_type value ){
			double v = as_double( value );
			v = ( v <= 0.0031308 ) ? 12.92 * v : 1.055*std::pow( v, 1.0/2.4 ) - 0.055;
			return from_double( v );
		}
		static double ycbcr2srgb( double v ){
			//rec. 601 and 709
			v = ( v < 0.08125 ) ? 1.0/4.5 * v : std::pow( (v+0.099)/1.099, 1.0/0.45 );
			v = ( v <= 0.0031308 ) ? 12.92 * v : 1.055*std::pow( v, 1.0/2.4 ) - 0.055;
			return v;
		}
	
	
	public:
		void linearize(){
			r = sRgb2linear( r );
			g = sRgb2linear( g );
			b = sRgb2linear( b );
			a = sRgb2linear( a );
		}
		void sRgb(){
			r = linear2sRgb( r );
			g = linear2sRgb( g );
			b = linear2sRgb( b );
			a = linear2sRgb( a );
		}
		
		color yuv_to_rgb( double kr, double kg, double kb, bool gamma );
		
		color rec601_to_rgb( bool gamma=true ){
			return yuv_to_rgb( 0.299, 0.587, 0.114, gamma );
		}
		color rec709_to_rgb( bool gamma=true ){
			return yuv_to_rgb( 0.2126, 0.7152, 0.0722, gamma );
		}
		
	void clear(){
		r = b = g = a = 0;
	}
	
	void trunc( color_type max ){
		r = ( r > max ) ? max : r;
		g = ( g > max ) ? max : g;
		b = ( b > max ) ? max : b;
		a = ( a > max ) ? max : a;
	}
	
	color difference( color c ){
		c.diff( *this );
		return c;
	}
	void diff( color c ){
		r = ( c.r > r ) ? c.r - r : r - c.r;
		g = ( c.g > g ) ? c.g - g : g - c.g;
		b = ( c.b > b ) ? c.b - b : b - c.b;
		a = ( c.a > a ) ? c.a - a : a - c.a;
	}
	
	color_type gray(){
		//This function corresponds to qGray()
		return ( r*11 + g*16 + b*5 ) / 32;
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


class ColorAvg{
	private:
		precision_color_type r;
		precision_color_type g;
		precision_color_type b;
		precision_color_type a;
		unsigned amount;
		
	public:
		ColorAvg(){
			r = g = b = a = 0;
			amount = 0;
		}
		
		unsigned size() const{ return amount; }
		
		ColorAvg& operator+=( const color &rhs ){
			r += rhs.r;
			g += rhs.g;
			b += rhs.b;
			a += rhs.a;
			++amount;
			return *this;
		}
		
		ColorAvg& operator-=( const color &rhs ){
			r -= rhs.r;
			g -= rhs.g;
			b -= rhs.b;
			a -= rhs.a;
			--amount;
			return *this;
		}
		
		color get_color() const{
			if( amount )
				return color(
						r / amount
					,	g / amount
					,	b / amount
					,	a / amount
					);
			else
				return color();
		}
		
		operator color(){ return get_color(); }
};

#endif