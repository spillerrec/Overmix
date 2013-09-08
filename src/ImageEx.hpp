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

#ifndef IMAGE_EX_HPP
#define IMAGE_EX_HPP

#include "Plane.hpp"
#include <QImage>
#include <algorithm>

class ImageEx{
	public:
		const static unsigned MAX_PLANES = 4;
		enum system{
			GRAY,
			RGB,
			YUV
		};
		enum YuvSystem{
			SYSTEM_KEEP,
			SYSTEM_REC601,
			SYSTEM_REC709
		};
		enum Settings{
			SETTING_NONE = 0x0,
			SETTING_DITHER = 0x1,
			SETTING_GAMMA = 0x2
		};
	
	private:
		bool initialized;
		Plane *planes[MAX_PLANES];
		system type;
		bool read_dump_plane( FILE *f, unsigned index );
		bool from_dump( const char* path );
		bool from_png( const char* path );
		bool from_qimage( const char* path );
		
	public:
		ImageEx( system type = RGB ) : type( type ){
			initialized = false;
			for( unsigned i=0; i<MAX_PLANES; i++ )
				planes[i] = 0;
		}
		~ImageEx(){
			for( unsigned i=0; i<MAX_PLANES; i++ )
				if( planes[i] )
					delete planes[i];
		}
		
		bool create( unsigned width, unsigned height, bool alpha=false );
		
		bool is_valid() const{ return initialized; }
		
		bool read_file( const char* path );
		Plane* alpha_plane() const{ return planes[MAX_PLANES-1]; }
		
		QImage to_qimage( YuvSystem system, unsigned setting=SETTING_NONE );
		
		unsigned get_width() const{
			unsigned width = 0;
			for( unsigned i=0; i<MAX_PLANES; ++i )
				if( planes[i] )
					width = std::max( planes[i]->get_width(), width );
			return width;
		}
		unsigned get_height() const{
			unsigned height = 0;
			for( unsigned i=0; i<MAX_PLANES; ++i )
				if( planes[i] )
					height = std::max( planes[i]->get_height(), height );
			return height;
		}
		
		system get_system() const{ return type; }
		
		
		double diff( const ImageEx& img, int x, int y ) const;
		bool is_interlaced() const;
		void replace_line( ImageEx& img, bool top );
		void combine_line( ImageEx& img, bool top );
		
		void scale_plane( unsigned index, unsigned width, unsigned height ){
			if( planes[index] ){
				Plane* prev = planes[index];
				planes[index] = prev->scale_cubic( width, height );
				delete prev;
			}
		}
		void scale( unsigned width, unsigned height ){
			for( unsigned i=0; i<MAX_PLANES; ++i )
				scale_plane( i, width, height );
		}
		void scale( double factor ){
			unsigned width = get_width() * factor + 0.5;
			unsigned height = get_height() * factor + 0.5;
			scale( width, height );
		}
		
		MergeResult best_vertical( const ImageEx& img, int level, double range, DiffCache *cache=nullptr ) const{
			return best_round( img, level, 0, range, cache );
		}
		MergeResult best_horizontal( const ImageEx& img, int level, double range, DiffCache *cache=nullptr ) const{
			return best_round( img, level, range, 0, cache );
		}
		MergeResult best_round( const ImageEx& img, int level, double range_x, double range_y, DiffCache *cache=nullptr ) const;
		
		Plane* operator[]( const unsigned index ) const{
			return planes[index];
		}
};

#endif