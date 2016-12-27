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
#include "ColorSpace.hpp"
#include <numeric>
#include <vector>

class QImage;
class QIODevice;
class QString;

namespace Overmix{

class ImageEx{
	public:
		struct PlaneInfo{
			Plane p;
			Size<int> subsampling;
			PlaneInfo( Plane&& p, Size<int> sub={0,0} )
				:	p(std::move(p)), subsampling(sub) {}
		};
		
		enum Settings{
			SETTING_NONE = 0x0,
			SETTING_DITHER = 0x1,
			SETTING_GAMMA = 0x2
		};
	
	private:
		std::vector<PlaneInfo> planes;
		Plane alpha;
		ColorSpace color_space;
		bool read_dump_plane( QIODevice& dev );
		bool from_dump(   QIODevice& dev );
		bool from_jpeg(   QIODevice& dev, class JpegDegrader* deg=nullptr );
		bool from_png(    QIODevice& dev );
		bool from_qimage( QIODevice& dev, QString ext );
		
	public:
		void addPlane( Plane&& p ){ if( p.valid() ) planes.emplace_back( std::move(p) ); }
		
		ImageEx() : color_space( Transform::UNKNOWN, Transfer::UNKNOWN ) { }
		explicit ImageEx( ColorSpace s ) : color_space(s) { planes.reserve(s.components()); }
		explicit ImageEx( Plane p ) : ImageEx( { Transform::GRAY, Transfer::UNKNOWN } )
			{ addPlane( std::move(p) ); } //TODO: transfer function!
		ImageEx( Plane p, Plane a ) : ImageEx( p ) { alpha = a; }
		
		unsigned size() const{ return planes.size(); }
		Size<double> planeScale( unsigned c ) const{ return planes[c].p.getSize() / getSize(); }
		void to_grayscale();
		ImageEx toRgb() const;
		ImageEx toColorSpace( ColorSpace to ) const;
		
		template<typename... Args>
		void apply( Plane (Plane::*func)( Args... ) const, Args... args ){
			if( color_space.isYCbCr() )
				planes[0].p = (planes[0].p.*func)( args... );
			else
				applyAll( false, func, args... );
		}
		template<typename... Args>
		void applyAll( bool do_alpha, Plane (Plane::*func)( Args... ) const, Args... args ){
			for( auto& info : planes )
				info.p = (info.p.*func)( args... );
			if( do_alpha && alpha )
				alpha = (alpha.*func)( args... );
		}
		
		template<typename... Args>
		ImageEx copyApply( Args... args ) const{
			ImageEx temp( *this );
			temp.apply( args... );
			return temp;
		}
		
		template<typename... Args>
		ImageEx copyApplyAll( Args... args ) const{
			ImageEx temp( *this );
			temp.applyAll( args... );
			return temp;
		}
		
		bool is_valid() const{ return planes.size() > 0; }
		operator bool() const{ return is_valid(); }
		
		bool read_file( QString path );
		static ImageEx fromFile( QString path );
		static JpegDegrader getJpegDegrader( QString path );
		
		bool saveDump( QIODevice& dev, unsigned depth, bool compression ) const;
		bool saveDump( QString path, unsigned depth=10 ) const;
		Plane& alpha_plane(){ return alpha; }
		const Plane& alpha_plane() const{ return alpha; }
		
		ImageEx flatten() const; 
		QImage to_qimage( unsigned setting=SETTING_NONE ) const;
		
		Point<unsigned> getSize() const{
			return std::accumulate( planes.begin(), planes.end(), Point<unsigned>( 0, 0 )
				,	[]( auto& acc, auto& info ){ return acc.max( info.p.getSize() ); } );
		}
		unsigned get_width()  const{ return getSize().width(); }
		unsigned get_height() const{ return getSize().height(); }
		
		Rectangle<unsigned> getCrop() const;
		
		ColorSpace getColorSpace() const{ return color_space; }
		
		
		double diff( const ImageEx& img, int x, int y ) const;
		bool is_interlaced() const;
		bool is_interlaced( const ImageEx& previous ) const;
		void replace_line( ImageEx& img, bool top );
		void combine_line( ImageEx& img, bool top );
		
		void copyFrom( const ImageEx& source, Point<unsigned> source_pos, Size<unsigned> size, Point<unsigned> to_pos );
		
		void scale( Point<unsigned> size, ScalingFunction scaling=ScalingFunction::SCALE_MITCHELL ){
			for( auto& info : planes )
				info.p = info.p.scale_select( size, scaling );
			if( alpha_plane() )
				alpha_plane() = alpha_plane().scale_select( size, scaling );
		}
		void scaleFactor( Size<double> factor, ScalingFunction scaling=ScalingFunction::SCALE_MITCHELL ){
			for( auto& info : planes )
				info.p = info.p.scale_select( ( info.p.getSize() * factor ).round(), scaling );
			if( alpha_plane() )
				alpha_plane() = alpha_plane().scale_select( ( alpha_plane().getSize() * factor ).round(), scaling );
		}
		Point<unsigned> crop( unsigned left, unsigned top, unsigned right, unsigned bottom );
		void crop( Point<unsigned> offset, Size<unsigned> size );
		
		MergeResult best_vertical( const ImageEx& img, int level, double range, DiffCache *cache=nullptr ) const{
			return best_round( img, level, 0, range, cache );
		}
		MergeResult best_horizontal( const ImageEx& img, int level, double range, DiffCache *cache=nullptr ) const{
			return best_round( img, level, range, 0, cache );
		}
		MergeResult best_round( const ImageEx& img, int level, double range_x, double range_y, DiffCache *cache=nullptr ) const;
		
		Plane& operator[]( unsigned index ){ return planes[index].p; }
		const Plane& operator[]( unsigned index ) const{ return planes[index].p; }
};

QImage setQImageAlpha( QImage img, const Plane& alpha );
ImageEx deVlcImage( const ImageEx& img );

}

#endif
