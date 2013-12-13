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

typedef int color_type;

struct color{
	color_type r;
	color_type g;
	color_type b;
	color_type a;
	
	const static color_type MAX_VAL = 256*256-1;
	const static color_type MIN_VAL = 0;
	
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
		color( color_type r, color_type g, color_type b, color_type a = 255*256 ){
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
			r = qRed( c ) * 256;
			g = qGreen( c ) * 256;
			b = qBlue( c ) * 256;
			a = qAlpha( c ) * 256;
		//	linearize();
		}
	
	private:
		static color_type sRgb2linear( color_type value ){
			double v = value / (double)(255*256);
			v = ( v <= 0.04045 ) ? v / 12.92 : std::pow( (v+0.055)/1.055, 2.4 );
			return (color_type)( v *255*256 +0.5 );
		}
		static color_type linear2sRgb( color_type value ){
			double v = value / (double)(255*256);
			v = ( v <= 0.0031308 ) ? 12.92 * v : 1.055*std::pow( v, 1.0/2.4 ) - 0.055;
			return (color_type)( v *255*256 +0.5 );
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
		
		static void test(){
			for( int i=0; i<=255*256; i++ ){
				color_type i2 = linear2sRgb( sRgb2linear( i ) );
				if( i != i2 )
					qDebug( "didn't match: %d %d", i, i2 );
			}
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
	
	color& operator+=( const int &rhs ){
		r += rhs;
		g += rhs;
		b += rhs;
		a += rhs;
		return *this;
	}
	
	color& operator*=( const int &rhs ){
		r *= rhs;
		g *= rhs;
		b *= rhs;
		a *= rhs;
		return *this;
	}
	
	color& operator/=( const int &rhs ){
		r /= rhs;
		g /= rhs;
		b /= rhs;
		a /= rhs;
		return *this;
	}
	
	color& operator<<=( const int &rhs ){
		r <<= rhs;
		g <<= rhs;
		b <<= rhs;
		a <<= rhs;
		return *this;
	}
	
	color& operator>>=( const int &rhs ){
		r >>= rhs;
		g >>= rhs;
		b >>= rhs;
		a >>= rhs;
		return *this;
	}
	
	const color operator+( const color &other ) const{
		return color(*this) += other;
	}
	const color operator+( const int &other ) const{
		return color(*this) += other;
	}
	const color operator-( const color &other ) const{
		return color(*this) -= other;
	}
	const color operator*( const int &other ) const{
		return color(*this) *= other;
	}
	const color operator/( const int &other ) const{
		return color(*this) /= other;
	}
	const color operator<<( const int &other ) const{
		return color(*this) <<= other;
	}
	const color operator>>( const int &other ) const{
		return color(*this) >>= other;
	}
};


class ColorAvg{
	private:
		unsigned r;
		unsigned g;
		unsigned b;
		unsigned a;
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