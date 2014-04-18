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
#include <vector>

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
		bool initialized{ false };
		std::vector<Plane> planes;
		Plane alpha;
		system type;
		bool read_dump_plane( FILE *f );
		bool from_dump( const char* path );
		bool from_png( const char* path );
		bool from_qimage( const char* path );
		
	public:
		ImageEx( system type = RGB ) : type( type ){ }
		ImageEx( Plane p ) : type( GRAY ){
			planes.push_back( p );
		}
		
		unsigned size() const{ return planes.size(); }
		void to_grayscale();
		
		bool create( unsigned width, unsigned height, bool alpha=false );
		
		template<typename... Args>
		void apply_operation( Plane* (Plane::*func)( Args... ) const, Args... args ){
			//TODO: reconsider this-
			//TODO: remove pointer stuff
			if( type == YUV || type == GRAY )
				planes[0] = *( (planes[0].*func)( args... ) );
			else
				for( auto& plane : planes )
					plane = *( (plane.*func)( args... ) );
		}
		
		bool is_valid() const{ return initialized; }
		
		bool read_file( const char* path );
		Plane& alpha_plane(){ return alpha; }
		const Plane& alpha_plane() const{ return alpha; }
		
		QImage to_qimage( YuvSystem system, unsigned setting=SETTING_NONE );
		
		unsigned get_width() const{
			if( planes.size() == 0 )
				return 0;
			return std::max_element( planes.begin(), planes.end()
				,	[]( const Plane& p1, const Plane& p2 ){ return p1.get_width() < p2.get_width(); } )->get_width();
		}
		unsigned get_height() const{
			if( planes.size() == 0 )
				return 0;
			return std::max_element( planes.begin(), planes.end()
				,	[]( const Plane& p1, const Plane& p2 ){ return p1.get_height() < p2.get_height(); } )->get_height();
		}
		
		system get_system() const{ return type; }
		
		
		double diff( const ImageEx& img, int x, int y ) const;
		bool is_interlaced() const;
		void replace_line( ImageEx& img, bool top );
		void combine_line( ImageEx& img, bool top );
		
		void scale( unsigned width, unsigned height ){
			for( auto& plane : planes )
				plane = *plane.scale_cubic( width, height );
		}
		void scale( double factor ){
			unsigned width = get_width() * factor + 0.5;
			unsigned height = get_height() * factor + 0.5;
			scale( width, height );
		}
		void crop( unsigned left, unsigned top, unsigned right, unsigned bottom );
		
		MergeResult best_vertical( const ImageEx& img, int level, double range, DiffCache *cache=nullptr ) const{
			return best_round( img, level, 0, range, cache );
		}
		MergeResult best_horizontal( const ImageEx& img, int level, double range, DiffCache *cache=nullptr ) const{
			return best_round( img, level, range, 0, cache );
		}
		MergeResult best_round( const ImageEx& img, int level, double range_x, double range_y, DiffCache *cache=nullptr ) const;
		
		Plane& operator[]( unsigned index ){ return planes[index]; }
		const Plane& operator[]( unsigned index ) const{ return planes[index]; }
};

#endif