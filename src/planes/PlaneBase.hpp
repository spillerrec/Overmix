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
		T* data{ nullptr };
		
		unsigned dataSize() const{ return realsize.height() * line_width; }
		void clearContainer(){
			delete[] data;
			size = realsize = { 0,0 };
		}
		void copySettings( const PlaneBase<T>& other ){
			realsize = other.realsize;
			offset = other.offset;
			size = other.size;
			line_width = other.line_width;
		}
		void copyAndMove( PlaneBase<T>& other ){
			//NOTICE: delete anything in 'data' first
			copySettings( other );
			data = other.data;
			other.data = nullptr;
			other.clearContainer();
		}
		
		unsigned getOffset( unsigned x, unsigned y ) const
			{ return x + offset.x + (y + offset.y) * line_width; }
		
	public:
		PlaneBase() { }
		PlaneBase( Size<unsigned> size )
			:	realsize(size), size(size), line_width( size.width() ), data( new T[dataSize()] ) { }
		PlaneBase( unsigned w, unsigned h ) : PlaneBase( Size<unsigned>( w, h ) ) { }
		
	//Memory handling
		PlaneBase( const PlaneBase<T>& other ){
			copySettings( other );
			if( other.data ){
				data = new T[dataSize()];
				std::copy( other.data, other.data + dataSize(), data );
			}
		}
		PlaneBase( PlaneBase<T>&& other ){ copyAndMove( other ); }
		~PlaneBase() { delete[] data; }
		
		PlaneBase<T>& operator=( const PlaneBase<T>& other ){
			if( this != &other ){
				if( other.data ){
					//Try to reuse the old allocation if possible
					if( dataSize() != other.dataSize() || data == nullptr ){
						delete[] data;
						data = new T[other.dataSize()];
					}
					copySettings( other );
					std::copy( other.data, other.data + dataSize(), data );
				}
				else
					clearContainer();
			}
			return *this;
		}
		PlaneBase<T>& operator=( PlaneBase<T>&& other ){
			if( this != &other ){
				delete[] data;
				copyAndMove( other );
			}
			return *this;
		}
		
	//Status
		operator bool() const{ return valid(); }
		bool valid() const{ return data != nullptr && size.x != 0 && size.y != 0; }
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
		const T*       scan_line( unsigned y ) const{ return data + getOffset( 0, y ); } //TODO: !!!!!!!!
		      T*       scan_line( unsigned y )      { return data + getOffset( 0, y ); } //TODO: !!!!!!!!
		const T* const_scan_line( unsigned y ) const{ return data + getOffset( 0, y ); }
		
	//Resizing
		void crop( Point<unsigned> pos, Size<unsigned> newsize ){
			offset += pos;
			size = newsize;
		}
		Point<unsigned> getOffset() const{ return offset; }
		Size<unsigned> getRealSize() const{ return realsize; }
		
	//Drawing methods
		void fill( T value ){
			for( unsigned i=0; i<dataSize(); ++i )
				data[i] = value;
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
		
		
	//Iterators
	public:
		template<typename T2>
		class RowIt{
			private:
				T2* row;
				unsigned iy;
				unsigned w;
				
			public:
				RowIt( T2* row, unsigned iy, unsigned width ) : row(row), iy(iy), w(width) { }
				
				T2* line() const{ return row; }
				unsigned y() const{ return iy; }
				unsigned width() const{ return w; }
				
				T2& operator[]( int index ){ return row[index]; }
				T2* begin() const{ return row;     }
				T2* end()   const{ return row + w; }
		};
		
		template<typename T1, typename T2>
		class LineIt{
			private:
				T1* p;
				unsigned iy;
			
			public:
				LineIt( T1* p, unsigned iy ) : p(p), iy(iy) {}
				
				bool operator!=( const LineIt& other ) const { return iy != other.iy; }
				LineIt& operator++() {
					iy++;
					return *this;
				}
				RowIt<T2> operator*() { return { p->scan_line( iy ), iy, p->get_width() }; }
		};
		
	public:
		LineIt<      PlaneBase<T>,       T> begin()       { return { this, 0             }; }
		LineIt<const PlaneBase<T>, const T> begin() const { return { this, 0             }; }
		LineIt<      PlaneBase<T>,       T> end()         { return { this, size.height() }; }
		LineIt<const PlaneBase<T>, const T> end()   const { return { this, size.height() }; }
};

#endif