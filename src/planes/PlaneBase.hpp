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

#include "../Geometry.hpp"

#include <cassert>
#include <memory>

namespace Overmix{

//Array iterator
template<typename T2>
class RowIt{
	private:
		T2* row;
		unsigned w;
		
	public:
		RowIt( T2* row, unsigned width ) : row(row), w(width) { }
		unsigned width() const{ return w; }
		
		T2& operator[]( int index ) const{ return row[index]; }
		T2* begin() const{ return row;     }
		T2* end()   const{ return row + w; }
};

template<typename T1, typename T2>
class ZipRowIt{
	private:
		RowIt<T1> a;
		RowIt<T2> b;
		
		struct Iterator{
			T1* a;
			T2* b;
			Iterator( T1* a, T2* b ) : a(a), b(b) { }
			
			std::pair<T1&,T2&> operator*() { return { *a, *b }; }
			Iterator& operator++() {
				a++; b++;
				return *this;
			}
			
			bool operator!=( const Iterator& other ) const
				{ return a != other.a && b != other.b; }
		};
		
	public:
		ZipRowIt( RowIt<T1> a, RowIt<T2> b ) : a(a), b(b)
			{ assert( a.width() == b.width() ); }
		auto width() const{ return a.width(); }
		
		Iterator begin() const { return { a.begin(), b.begin() }; }
		Iterator end()   const { return { a.end(),   b.end()   }; }
		std::pair<T1&,T2&> operator[]( int index ) { return { a[index], b[index] }; }
};

template<typename T1, typename T2>
auto makeZipRowIt( RowIt<T1> a, RowIt<T2> b )
	{ return ZipRowIt<T1,T2>( a, b ); }

template<typename T>
class PlaneBase{
	protected:
		Size<unsigned> realsize{ 0, 0 };
		Point<unsigned> offset{ 0, 0 };
		Size<unsigned> size{ 0, 0 };
		unsigned line_width{ 0 };
		std::unique_ptr<T[]> data;
		
		unsigned dataSize() const{ return realsize.height() * line_width; }
		void copySettings( const PlaneBase<T>& other ){
			realsize = other.realsize;
			offset = other.offset;
			size = other.size;
			line_width = other.line_width;
		}
		void copyAndMove( PlaneBase<T>& other ){
			copySettings( other );
			data = std::move( other.data );
			other.size = other.realsize = { 0,0 };
		}
		
		unsigned getOffset( unsigned x, unsigned y ) const
			{ return x + offset.x + (y + offset.y) * line_width; }
		
	public:
		PlaneBase() { }
		PlaneBase( Size<unsigned> size )
			:	realsize(size), size(size), line_width( size.width() )
			,	data( std::make_unique<T[]>( dataSize() ) ) { }
		PlaneBase( unsigned w, unsigned h ) : PlaneBase( Size<unsigned>( w, h ) ) { }
		
	//Memory copying
		PlaneBase( const PlaneBase<T>& other ){
			copySettings( other );
			if( other.data ){
				data = std::make_unique<T[]>( dataSize() );
				std::copy( other.data.get(), other.data.get() + dataSize(), data.get() );
			}
		}
		
		PlaneBase<T>& operator=( const PlaneBase<T>& other ){
			if( this != &other ){
				if( other.data ){
					//Try to reuse the old allocation if possible
					if( dataSize() != other.dataSize() || !data ) //TODO: require dataSize() to be 0 if !data
						data = std::make_unique<T[]>( other.dataSize() );
					std::copy( other.data.get(), other.data.get() + other.dataSize(), data.get() );
				}
				else
					data = {};
				
				copySettings( other );
			}
			return *this;
		}
		
	//Memory moving
		PlaneBase( PlaneBase<T>&& other ){ copyAndMove( other ); }
		PlaneBase<T>& operator=( PlaneBase<T>&& other ){
			if( this != &other )
				copyAndMove( other );
			return *this;
		}
		
	//Status
		operator bool() const{ return valid(); }
		bool valid() const{ return data && size.x != 0 && size.y != 0; }
		unsigned get_height() const{ return size.height(); }
		unsigned get_width() const{ return size.width(); }
		unsigned get_line_width() const{ return line_width; }
		
		Size<unsigned> getSize() const{ return size; }
		
	//Pixel/Row query
		const T& pixel( Point<unsigned> pos        ) const{ return data[ getOffset( pos.x, pos.y ) ];       }
		void  setPixel( Point<unsigned> pos, T val )      {        data[ getOffset( pos.x, pos.y ) ] = val; }
		RowIt<const T> scan_line( unsigned y ) const{ return { data.get() + getOffset( 0, y ), get_width() }; }
		RowIt<T>       scan_line( unsigned y )      { return { data.get() + getOffset( 0, y ), get_width() }; }
		
	//Resizing
		void crop( Point<unsigned> pos, Size<unsigned> newsize ){
			offset += pos;
			size = newsize;
		}
		Point<unsigned> getOffset() const{ return offset; }
		Size<unsigned> getRealSize() const{ return realsize; }
		
	//Drawing methods
		void fill( T value ){
			for( auto row : *this )
				for( auto& val : row )
					val = value;
		}
		void copy( const PlaneBase& from, Point<unsigned> pos, Size<unsigned> size, Point<unsigned> to ){
			//TODO: check offsets
			auto range_this =      getSize().min(to + size) - to;
			auto range_from = from.getSize().min(pos+ size) - pos;
			auto range = range_this.min( range_from );
			for( unsigned iy=0; iy < range.height(); iy++ ){
				auto dest = scan_line( iy+to.y );
				auto source = from.scan_line( iy + pos.y );
				for( unsigned ix=0; ix < range.width(); ix++ )
					dest[ix+to.x] = source[ix+pos.x];
			}
		}
		
	//Transformations
		void flipHor(){
			for( auto row : *this )
				for( unsigned ix=0; ix<row.width()/2; ix++ )
					std::swap( row[ix], row[get_width()-ix-1] );
		}
		void flipVer(){
			for( unsigned iy=0; iy<get_height()/2; iy++ )
				for( auto rows : makeZipRowIt( scan_line( iy ), scan_line( get_height()-iy-1 ) ) )
					std::swap( rows.first, rows.second );
		}
		
		void truncate( T min, T max ){
			for( auto row : *this )
				for( auto& val : row )
					val = std::min( std::max( val, min ), max );
		}
		
		template<typename T2>
		PlaneBase<T2> to() const{
			PlaneBase<T2> out( getSize() );
			for( unsigned iy=0; iy<get_height(); iy++ )
				for( auto rows : makeZipRowIt( out.scan_line(iy), scan_line(iy) ) )
					rows.first = T2( rows.second );
			return out;
		}
		
	//Iterators
	public:
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
				RowIt<T2> operator*() { return p->scan_line( iy ); }
		};
		
	public:
		LineIt<      PlaneBase<T>,       T> begin()       { return { this, 0             }; }
		LineIt<const PlaneBase<T>, const T> begin() const { return { this, 0             }; }
		LineIt<      PlaneBase<T>,       T> end()         { return { this, size.height() }; }
		LineIt<const PlaneBase<T>, const T> end()   const { return { this, size.height() }; }
};

}

#endif