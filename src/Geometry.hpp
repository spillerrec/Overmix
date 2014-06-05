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
#include <QSize>
#include <QSizeF>
#include <QPoint>
#include <QPointF>

template<typename T>
struct Point{
	T x{ 0 };
	T y{ 0 };
	
	Point() { }
	Point( T x, T y ) : x(x), y(y) { }
	
	template<typename T2>
	Point( Point<T2> p ) : x( p.x ), y( p.y ) { }
	
	Point<T>( QPoint p ) : x( p.x() ), y( p.y() ) { }
	Point<T>( QPointF p ) : x( p.x() ), y( p.y() ) { }
	
	double lenght() const{ return std::sqrt( x*x + y*y ); }
	double manhattanLength() const{ return x+y; }
	
	template<typename T2>
	bool operator==( const Point<T2>& p ) const{
		return x == p.x && y == p.y;
	}
	
	template<typename T2>
	operator Point<T2>() const{ return Point<T2>( x, y ); }
	//TODO: cast to Size<T2>
	
	Point<int> abs() const{
		return Point<T>( std::abs( x ), std::abs( y ) );
	}
	Point<int> floor() const{
		return Point<T>( std::floor( x ), std::floor( y ) );
	}
	Point<int> ceil() const{
		return Point<T>( std::ceil( x ), std::ceil( y ) );
	}
	Point<int> round() const{
		return Point<T>( std::round( x ), std::round( y ) );
	}
	Point<int> trunc() const{
		return Point<T>( std::trunc( x ), std::trunc( y ) );
	}
	Point<double> sqrt() const{
		return Point<T>( std::sqrt( x ), std::sqrt( y ) );
	}
	
	template<typename T2>
	Point<T> pow( T2 power ) const{
		return Point<T>( std::pow( x, power ), std::sqrt( y, power ) );
	}
	template<typename T2>
	Point<T> pow( const Point<T2>& power ) const{
		return Point<T>( std::pow( x, power.x ), std::sqrt( y, power.y ) );
	}
	
	
	//Math with Point<T2>
	template<typename T2>
	Point<T>& operator+=( const Point<T2>& other ){
		x += other.x;
		y += other.y;
		return *this;
	}
	template<typename T2>
	Point<T>& operator-=( const Point<T2>& other ){
		x += other.x;
		y += other.y;
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
		x *= other.x;
		y *= other.y;
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
	Point<T>& operator+=( T2 other ) const{
		x += other;
		y += other;
		return *this;
	}
	template<typename T2>
	Point<T>& operator-=( T2 other ) const{
		x += other;
		y += other;
		return *this;
	}
	template<typename T2>
	Point<T>& operator*=( T2 other ) const{
		x *= other;
		y *= other;
		return *this;
	}
	template<typename T2>
	Point<T>& operator/=( T2 other ) const{
		x *= other;
		y *= other;
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



template<typename T>
struct Size{
	T width{ 0 };
	T height{ 0 };
	
	Size() { }
	Size( T width, T height ) : width(width), height(height) { }
	
	template<typename T2>
	Size( Size<T2> p ) : width( p.width ), height( p.height ) { }
	
	Size<T>( QSize p ) : width( p.width() ), height( p.height() ) { }
	Size<T>( QSizeF p ) : width( p.width() ), height( p.height() ) { }
	
	double lenght() const{ return std::sqrt( width*width + height*height ); }
	double manhattanLength() const{ return width+height; }
	
	template<typename T2>
	bool operator==( const Size<T2>& p ) const{
		return width == p.width && height == p.height;
	}
	
	template<typename T2>
	operator Point<T2>() const{ return Point<T2>( width, height ); }
	template<typename T2>
	operator Size<T2>() const{ return Size<T2>( width, height ); }
	
	Size<int> abs() const{
		return Size<T>( std::abs( width ), std::abs( height ) );
	}
	Size<int> floor() const{
		return Size<T>( std::floor( width ), std::floor( height ) );
	}
	Size<int> ceil() const{
		return Size<T>( std::ceil( width ), std::ceil( height ) );
	}
	Size<int> round() const{
		return Size<T>( std::round( width ), std::round( height ) );
	}
	Size<int> trunc() const{
		return Size<T>( std::trunc( width ), std::trunc( height ) );
	}
	Size<double> sqrt() const{
		return Size<T>( std::sqrt( width ), std::sqrt( height ) );
	}
	
	template<typename T2>
	Size<T> pow( T2 power ) const{
		return Size<T>( std::pow( width, power ), std::sqrt( height, power ) );
	}
	template<typename T2>
	Size<T> pow( const Size<T2>& power ) const{
		return Size<T>( std::pow( width, power.width ), std::sqrt( height, power.height ) );
	}
	
	
	//Math with Size<T2>
	template<typename T2>
	Size<T>& operator+=( const Size<T2>& other ){
		width += other.width;
		height += other.height;
		return *this;
	}
	template<typename T2>
	Size<T>& operator-=( const Size<T2>& other ){
		width += other.width;
		height += other.height;
		return *this;
	}
	template<typename T2>
	Size<T>& operator*=( const Size<T2>& other ){
		width *= other.width;
		height *= other.height;
		return *this;
	}
	template<typename T2>
	Size<T>& operator/=( const Size<T2>& other ){
		width *= other.width;
		height *= other.height;
		return *this;
	}
	
	//TODO: make return type dependent on the arguments
	template<typename T2>
	Size<T> operator+( const Size<T2>& other ) const
		{ return Size<T>( *this ) += other; }
	
	template<typename T2>
	Size<T> operator-( const Size<T2>& other ) const
		{ return Size<T>( *this ) -= other; }
	
	template<typename T2>
	Size<T> operator*( const Size<T2>& other ) const
		{ return Size<T>( *this ) *= other; }
	
	template<typename T2>
	Size<T> operator/( const Size<T2>& other ) const
		{ return Size<T>( *this ) /= other; }
	
	//Math with scala
	template<typename T2>
	Size<T>& operator+=( T2 other ) const{
		width += other;
		height += other;
		return *this;
	}
	template<typename T2>
	Size<T>& operator-=( T2 other ) const{
		width += other;
		height += other;
		return *this;
	}
	template<typename T2>
	Size<T>& operator*=( T2 other ) const{
		width *= other;
		height *= other;
		return *this;
	}
	template<typename T2>
	Size<T>& operator/=( T2 other ) const{
		width *= other;
		height *= other;
		return *this;
	}
	
	//TODO: make return type dependent on the arguments
	template<typename T2>
	Size<T> operator+( T2 other ) const
		{ return Size<T>( width + other, height + other ); }
	
	template<typename T2>
	Size<T> operator-( T2 other ) const
		{ return Size<T>( width - other, height - other ); }
	
	template<typename T2>
	Size<T> operator*( T2 other ) const
		{ return Size<T>( width * other, height * other ); }
	
	template<typename T2>
	Size<T> operator/( T2 other ) const
		{ return Size<T>( width / other, height / other ); }
	
};

#endif