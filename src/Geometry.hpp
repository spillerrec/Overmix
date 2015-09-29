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

#ifndef GEOMETRY_HPP
#define GEOMETRY_HPP

#include <cmath>
#include <QPoint>
#include <QPointF>
#include <QSize>
#include <QSizeF>

namespace Overmix{

template<typename T=int>
struct Point{
	T x{ 0 };
	T y{ 0 };
	
	T& width(){ return x; }
	T& height(){ return y; }
	T width() const{ return x; }
	T height() const{ return y; }
	
	Point() { }
	Point( T x, T y ) : x(x), y(y) { }
	
	template<typename T2>
	Point( const Point<T2>& p ) : x( p.x ), y( p.y ) { }
	
	Point<T>( QPoint  p ) : x( p.x()     ), y( p.y()      ) { }
	Point<T>( QPointF p ) : x( p.x()     ), y( p.y()      ) { }
	Point<T>( QSize   p ) : x( p.width() ), y( p.height() ) { }
	Point<T>( QSizeF  p ) : x( p.width() ), y( p.height() ) { }
	
	double lenght() const{ return std::sqrt( x*x + y*y ); }
	double manhattanLength() const{ return x+y; }
	
	template<typename T2>
	bool operator==( const Point<T2>& p ) const{ return x == p.x && y == p.y; }
	
	template<typename T2>
	bool operator!=( const Point<T2>& p ) const{ return x != p.x || y != p.y; }
	
	template<typename T2>
	Point<T2> to() const{ return { static_cast<T2>(x), static_cast<T2>(y) }; }
	
	Point<T> max( Point<T> other ) const { return { std::max( x, other.x ), std::max( y, other.y ) } ; }
	Point<T> min( Point<T> other ) const { return { std::min( x, other.x ), std::min( y, other.y ) } ; }
	
	Point<int> abs()     const{ return { (int)std::abs( x ),   (int)std::abs( y )   }; }
	Point<int> floor()   const{ return { (int)std::floor( x ), (int)std::floor( y ) }; }
	Point<int> ceil()    const{ return { (int)std::ceil( x ),  (int)std::ceil( y )  }; }
	Point<int> round()   const{ return { (int)std::round( x ), (int)std::round( y ) }; }
	Point<int> trunc()   const{ return { (int)std::trunc( x ), (int)std::trunc( y ) }; }
	Point<double> sqrt() const{ return { std::sqrt( x ),  std::sqrt( y )  }; }
	
	template<typename T2>
	Point<T> pow( T2 power ) const
		{ return { std::pow( x, power ), std::sqrt( y, power ) }; }
		
	template<typename T2>
	Point<T> pow( const Point<T2>& power ) const
		{ return { std::pow( x, power.x ), std::sqrt( y, power.y ) }; }
	
	
	//Math with Point<T2>
	template<typename T2>
	Point<T>& operator+=( const Point<T2>& other ){
		x += other.x;
		y += other.y;
		return *this;
	}
	template<typename T2>
	Point<T>& operator-=( const Point<T2>& other ){
		x -= other.x;
		y -= other.y;
		return *this;
	}
	template<typename T2>
	Point<T>& operator*=( const Point<T2>& other ){
		x *= other.x;
		y *= other.y;
		return *this;
	}
	template<typename T2>
	Point<T>& operator/=( const Point<T2>& other ){
        x /= other.x;
        y /= other.y;
		return *this;
	}
	
	//TODO: make return type dependent on the arguments
	template<typename T2>
	Point<T> operator+( const Point<T2>& other ) const
		{ return Point<T>( *this ) += other; }
	
	template<typename T2>
	Point<T> operator-( const Point<T2>& other ) const
		{ return Point<T>( *this ) -= other; }
	
	template<typename T2>
	Point<T> operator*( const Point<T2>& other ) const
		{ return Point<T>( *this ) *= other; }
	
	template<typename T2>
	Point<T> operator/( const Point<T2>& other ) const
		{ return Point<T>( *this ) /= other; }
	
	//Math with scala
	template<typename T2>
	Point<T>& operator+=( T2 other ){
		x += other;
		y += other;
		return *this;
	}
	template<typename T2>
	Point<T>& operator-=( T2 other ){
		x -= other;
		y -= other;
		return *this;
	}
	template<typename T2>
	Point<T>& operator*=( T2 other ){
		x *= other;
		y *= other;
		return *this;
	}
	template<typename T2>
	Point<T>& operator/=( T2 other ){
		x /= other;
		y /= other;
		return *this;
	}
	
	//TODO: make return type dependent on the arguments
	template<typename T2>
	Point<T> operator+( T2 other ) const
		{ return Point<T>( x + other, y + other ); }
	
	template<typename T2>
	Point<T> operator-( T2 other ) const
		{ return Point<T>( x - other, y - other ); }
	
	template<typename T2>
	Point<T> operator*( T2 other ) const
		{ return Point<T>( x * other, y * other ); }
	
	template<typename T2>
	Point<T> operator/( T2 other ) const
		{ return Point<T>( x / other, y / other ); }
	
};

template<typename T=int>
using Size=Point<T>;

template<typename T=int>
struct Rectangle{
	Point<T> pos;
	Size<T> size;
	
	Point<T> endPos() const{ return pos + size; }
	
	Rectangle( Point<T> pos, Size<T> size ) : pos(pos), size(size) { }
	
	bool intersects( const Rectangle<T>& other ) const{
		return other.pos.x <= endPos().x && other.endPos().x >= pos.x
		   &&  other.pos.y <= endPos().y && other.endPos().y >= pos.y;
	}
};

}

#endif
