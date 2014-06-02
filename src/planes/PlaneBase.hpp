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
#include <utility>

template<typename T>
class PlaneBase{
	protected:
		unsigned width{ 0 };
		unsigned height{ 0 };
		unsigned line_width{ 0 };
		T *data{ nullptr };
		
	public:
		PlaneBase() { }
		PlaneBase( unsigned w, unsigned h )
			:	width( w ), height( h ), line_width( w ) {
			data = new T[ h * line_width ];
		}
		~PlaneBase(){ delete data; }
		
		PlaneBase( PlaneBase&& p )
			:	width(p.width), height(p.height), line_width(p.line_width), data(p.data)
			{ p.data = nullptr; }
		
		PlaneBase& operator=( const PlaneBase& p ){
			delete data; //TODO: optimize if all sizes are equal
			height = p.height;
			width = p.width;
			line_width = p.line_width;
			
			unsigned size = height * line_width;
			if( size > 0 ){
				data = new T[ size ];
				memcpy( data, p.data, sizeof(T)*size );
			}
			else
				data = nullptr;
			return *this;
		}
		
		PlaneBase& operator=( PlaneBase&& p ){
			if( this != &p ){
				height = p.height;
				width = p.width;
				line_width = p.line_width;
				
				delete data;
				data = p.data;
				p.data = nullptr;
			}
			return *this;
		}
		
	//Plane handling
		PlaneBase( const PlaneBase& p )
			:	width(p.width), height(p.height), line_width(p.line_width) {
			unsigned size = height * line_width;
			if( size > 0 ){
				data = new T[ size ];
				memcpy( data, p.data, sizeof(T)*size );
			}
			else
				data = nullptr;
		}
		
	//Status
		operator bool() const{ return valid(); }
		bool valid() const{ return data != nullptr; }
		unsigned get_height() const{ return height; }
		unsigned get_width() const{ return width; }
		unsigned get_line_width() const{ return line_width; }
		
		bool equalSize( const PlaneBase& p ) const{
			return width == p.width && height == p.height;
		}
		
	//Pixel/Row query
		T& pixel( unsigned x, unsigned y ) const{ return data[ x + y*line_width ]; }
		T* scan_line( unsigned y ) const{ return data + y*line_width; }
		const T* const_scan_line( unsigned y ) const{ return scan_line( y ); }
		
	//Drawing methods
		void fill( T value ){
			for( unsigned iy=0; iy<get_height(); ++iy ){
				T* row = scan_line( iy );
				for( unsigned ix=0; ix<get_width(); ++ix )
					row[ix] = value;
			}
		}
		void copy( int x, int y, const PlaneBase& from ){
			//TODO: check ranges
			unsigned range_x = min( width, from.width+x );
			unsigned range_y = min( height, from.height+y );
			for( unsigned iy=y; iy < range_y; iy++ ){
				T* dest = scan_line( iy );
				const T* source = from.scan_line( iy - y );
				for( unsigned ix=x; ix < range_x; ix++ )
					dest[ix] = source[ix-x];
			}
		}
};

#endif