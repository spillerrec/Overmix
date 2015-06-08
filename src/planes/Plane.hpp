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

#ifndef PLANE_HPP
#define PLANE_HPP

#include <QList>
#include <cstdio>
#include <vector>
#include <utility>

#include "PlaneBase.hpp"

typedef unsigned short color_type;
typedef std::pair<Point<>, double> MergeResult;

class DiffCache{
	private:
		struct Cached{
			int x;
			int y;
			double diff;
			unsigned precision;
		};
		QList<Cached> cache;
		
	public:
		double get_diff( int x, int y, unsigned precision ) const;
		void add_diff( int x, int y, double diff, unsigned precision );
};
struct SimplePixel{
	color_type *row1;
	color_type *row2;
	unsigned width;
	void (*f)( const SimplePixel& );
	void *data;
};
struct Kernel{
	//TODO: This should be a plane
	unsigned width;
	unsigned height;
	std::vector<double> values;
	Kernel( unsigned width, unsigned height ) : width(width), height(height){
		values.reserve( width*height );
	}
};

enum class ScalingFunction{
		SCALE_NEAREST
	,	SCALE_LINEAR
	,	SCALE_MITCHELL
	,	SCALE_SPLINE
	,	SCALE_LANCZOS
};

class Plane : public PlaneBase<color_type>{
	public:
		Plane() { }
		Plane( Size<unsigned> size ) : PlaneBase( size ) { }
		Plane( unsigned w, unsigned h ) : PlaneBase( w, h ) { }
		
		Plane( const Plane& p ) : PlaneBase( p ) { }
		Plane( Plane&& p ) : PlaneBase( std::move(p) ) { }
		
		Plane& operator=( const Plane& p ){
			*(PlaneBase<color_type>*)this = p;
			return *this;
		}
		Plane& operator=( Plane&& p ){
			*(PlaneBase<color_type>*)this = std::move(p);
			return *this;
		}
		
		static const unsigned long MAX_VAL;
		
	//Plane handling
		
	//Queries
		color_type min_value() const;
		color_type max_value() const;
		color_type mean_value() const;
		
	//Interlacing methods
		bool is_interlaced() const;
		void replace_line( const Plane &p, bool top );
		void combine_line( const Plane &p, bool top );
		
	//Overlays
	private:
		void for_each_pixel( void (*f)( const SimplePixel& ), void *data=nullptr );
		void for_each_pixel( Plane &p, void (*f)( const SimplePixel& ), void *data=nullptr );
	public:
		void add( Plane &p );
		void substract( Plane &p );
		void difference( Plane &p );
		void divide( Plane &p );
		void multiply( Plane &p );
		Plane level( color_type limit_min, color_type limit_max
			,	color_type output_min, color_type output_max
			,	double gamma
			) const;
		Plane normalize() const;
		
		Plane minPlane( const Plane& p ) const;
		Plane maxPlane( const Plane& p ) const;
		
	//Difference
		double diff( const Plane& p, int x, int y, unsigned stride=1 ) const;
		double diffAlpha( const Plane& p, const Plane& alpha, const Plane& alpha_p, int x, int y, unsigned stride=1, bool fast=true ) const;
		MergeResult best_round_sub( const Plane& p, const Plane& a1, const Plane& a2, int level, int left, int right, int top, int bottom, DiffCache *cache, bool fast ) const;
		
	//Binarization
		//TODO: find threshold methods: average, otsu?
		void binarize_threshold( color_type threshold );
		void binarize_adaptive( unsigned amount, color_type threshold );
		void binarize_dither();
		Plane dilate( int size ) const;
		
	//Scaling
	public:
		typedef double (*Filter)( double );
	private:
		static double cubic( double b, double c, double x );
		static double linear( double x );
		static double mitchell( double x ){ return cubic( 1.0/3, 1.0/3, x ); }
		static double spline( double x ){ return cubic( 1.0, 1.0, x ); }
		Plane scale_generic( Point<unsigned> size, double window, Filter f, Point<double> offset={0.0,0.0} ) const;
	public:
		Plane scale_nearest( Point<unsigned> size ) const;
		Plane scale_linear( Point<unsigned> size, Point<double> offset={0.0,0.0} ) const{
			return scale_generic( size, 1, linear, offset );
		}
		Plane scale_cubic( Point<unsigned> size, Point<double> offset={0.0,0.0} ) const{
			return scale_generic( size, 2, mitchell, offset );
		}
		Plane scale_lanczos( Point<unsigned> size, Point<double> offset={0.0,0.0} ) const;
		
		Plane scale_select( Point<unsigned> size, ScalingFunction scaling, Point<double> offset={0.0,0.0} ) const{
			switch( scaling ){
				case ScalingFunction::SCALE_NEAREST : return scale_nearest( size );
				case ScalingFunction::SCALE_LINEAR  : return scale_linear(  size, offset );
				case ScalingFunction::SCALE_MITCHELL: return scale_cubic(   size, offset );
				case ScalingFunction::SCALE_SPLINE  : return scale_generic( size, 2, spline, offset );
			//	case ScalingFunction::SCALE_LANCZOS : return scale_lanczos( size, offset );
				default: return Plane();
			}
		}
		
	//Edge-detection
	private:
		Plane edge_zero_generic( std::vector<int> weights, unsigned div ) const;
		Plane edge_dm_generic( std::vector<int> weights_x, std::vector<int> weights_y, unsigned div ) const;
		
	public:
		Plane edge_robert() const{
			return edge_dm_generic( { 0,1, -1,0 }, { 1,0, 0,-1 }, 1 );
		}
		Plane edge_sobel() const{
			return edge_dm_generic( { -1,0,1, -2,0,2, -1,0,1 }, { 1,2,1, 0,0,0, -1,-2,-1 }, 4 );
		}
		Plane edge_prewitt() const{
			return edge_dm_generic( { -1,0,1, -1,0,1, -1,0,1 }, { 1,1,1, 0,0,0, -1,-1,-1 }, 3 );
		}
		Plane edge_laplacian() const{
			return edge_zero_generic( { -1,-1,-1, -1,8,-1, -1,-1,-1 }, 1 );
		}
		Plane edge_laplacian_large() const{
			return edge_zero_generic( {
					 0, 0,-1, 0, 0,
					 0,-1,-2,-1, 0,
					-1,-2,16,-2,-1, 
					 0,-1,-2,-1, 0,
					 0, 0,-1, 0, 0
				}, 2 );
		}
		
	//Blurring
	private:
		Plane weighted_sum( Kernel &kernel ) const;
		Kernel gaussian_kernel( double deviation_x, double deviation_y ) const;
	public:
		Plane blur_box( unsigned amount_x, unsigned amount_y ) const;
		Plane blur_gaussian( unsigned amount_x, unsigned amount_y ) const;
		
	//De-blurring
		Plane deconvolve_rl( double amount, unsigned iterations ) const;
};

#endif