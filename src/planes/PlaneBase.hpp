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

#ifndef PLANE_BASE_HPP
#define PLANE_BASE_HPP

#include <cstdio>
#include <vector>
#include <utility>

#include "../Geometry.hpp"

template<typename T>
class PlaneBase{
	protected:
		Size<unsigned> realsize{ 0, 0 };
		Point<unsigned> offset{ 0, 0 };
		Size<unsigned> size{ 0, 0 };
		unsigned line_width{ 0 };
		std::vector<T> data;
		
		unsigned getOffset( unsigned x, unsigned y ) const
			{ return x + offset.x + (y + offset.y) * line_width; }
		
	public:
		PlaneBase() { }
		PlaneBase( Size<unsigned> size )
			:	realsize(size), size(size), line_width( size.width() ), data( size.height() * line_width ) { }
		PlaneBase( unsigned w, unsigned h )
			:	PlaneBase( Size<unsigned>( w, h ) ) { }
		
	//Status
		operator bool() const{ return valid(); }
		bool valid() const{ return data.size() != 0; }
		unsigned get_height() const{ return size.height(); }
		unsigned get_width() const{ return size.width(); }
		unsigned get_line_width() const{ return line_width; }
		
		Size<unsigned> getSize() const{ return size; }
		
		bool equalSize( const PlaneBase& p ) const{
			return size == p.size;
		}
		
	//Pixel/Row query
		const T& pixel( Point<unsigned> pos        ) const{ return data[ getOffset( pos.x, pos.y ) ];       }
		void  setPixel( Point<unsigned> pos, T val )      {        data[ getOffset( pos.x, pos.y ) ] = val; }
		T* scan_line( unsigned y ) { return data.data() + getOffset( 0, y ); } //TODO: !!!!!!!!
		const T* const_scan_line( unsigned y ) const{ return data.data() + getOffset( 0, y ); }
		
	//Resizing
		void crop( Point<unsigned> pos, Size<unsigned> newsize ){
			offset += pos;
			size = newsize;
		}
		Point<unsigned> getOffset() const{ return offset; }
		Size<unsigned> getRealSize() const{ return realsize; }
		
	//Drawing methods
		void fill( T value ){
			for( auto& x : data )
				x = value;
		}
		void copy( int x, int y, const PlaneBase& from ){
			//TODO: check ranges
			unsigned range_x = min( size.width(), from.size.width()+x );
			unsigned range_y = min( size.height(), from.size.height()+y );
			for( unsigned iy=y; iy < range_y; iy++ ){
				T* dest = scan_line( iy );
				const T* source = from.scan_line( iy - y );
				for( unsigned ix=x; ix < range_x; ix++ )
					dest[ix] = source[ix-x];
			}
		}
};

#endif