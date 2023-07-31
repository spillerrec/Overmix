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

#include <vector>
#include <utility>
#include <string>

#include "PlaneBase.hpp"

namespace Overmix{

typedef short color_type;


using Kernel = PlaneBase<double>;

enum class ScalingFunction{
		SCALE_NEAREST
	,	SCALE_LINEAR
	,	SCALE_MITCHELL
	,	SCALE_CATROM
	,	SCALE_SPLINE
	,	SCALE_LANCZOS_3
	,	SCALE_LANCZOS_5
	,	SCALE_LANCZOS_7
};

class Plane : public PlaneBase<color_type>{
	public:
		Plane() { }
		explicit Plane( Size<unsigned> size ) : PlaneBase( size ) { }
		explicit Plane( unsigned w, unsigned h ) : PlaneBase( w, h ) { }
		
		explicit Plane( const Plane& p ) : PlaneBase( p ) { }
		Plane( Plane&& p ) : PlaneBase( std::move(p) ) { }
		
		Plane& operator=( const Plane& p ){
			asBasePlane() = p;
			return *this;
		}
		Plane& operator=( Plane&& p ){
			asBasePlane() = std::move(p);
			return *this;
		}
		
		explicit Plane( const PlaneBase<color_type>& p ) : PlaneBase( p ) { }
		
		void save_png(std::string path) const;
		
	//Plane handling
		
	//Convert
		PlaneBase<double> toDouble() const;
		PlaneBase<uint8_t> to8Bit() const;
		PlaneBase<uint8_t> to8BitDither() const;
		
	//Queries
		color_type min_value() const;
		color_type max_value() const;
		color_type mean_value() const;
		double meanSquaredError( const Plane& other ) const;
		
	//Interlacing methods
		bool is_interlaced() const;
		bool is_interlaced( const Plane& previous ) const;
		void replace_line( const Plane &p, bool top );
		void combine_line( const Plane &p, bool top );
		
	//Overlays
	private:
		using PixelFunc1 = color_type (*)( color_type, void* );
		using PixelFunc2 = color_type (*)( color_type, color_type, void* );
		void for_each_pixel( Plane::PixelFunc1 f, void* data=nullptr );
		void for_each_pixel( const Plane& p, Plane::PixelFunc2 f, void* data=nullptr );
	public:
		void add( const Plane &p );
		void substract( const Plane &p );
		void difference( const Plane &p );
		void divide( const Plane &p );
		void multiply( const Plane &p );
		void mix( const Plane &p, double amount = 0.5 );
		Plane level( color_type limit_min, color_type limit_max
			,	color_type output_min, color_type output_max
			,	double gamma
			) const;
		Plane normalize() const;
		
		Plane minPlane( const Plane& p ) const;
		Plane maxPlane( const Plane& p ) const;
		
		Plane overlay( const Plane& p, const Plane& p_alpha, const Plane& this_alpha = {} ) const;
		
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
		static double catrom( double x ){ return cubic( 0.0, 0.5, x ); }
		static double spline( double x ){ return cubic( 1.0, 1.0, x ); }
		static double lancozs( double x, int a );
		static double lancozs3( double x ){ return lancozs( x, 3 ); }
		static double lancozs5( double x ){ return lancozs( x, 5 ); }
		static double lancozs7( double x ){ return lancozs( x, 7 ); }
		Plane scale_generic( Point<unsigned> size, double window, Filter f, Point<double> offset={0.0,0.0} ) const;
		Plane scale_generic_alpha( const Plane& alpha, Point<unsigned> size, double window, Filter f, Point<double> offset={0.0,0.0} ) const;
	public:
		Plane scale_nearest( Point<unsigned> size ) const;
		Plane scale_linear( const Plane& alpha, Point<unsigned> size, Point<double> offset={0.0,0.0} ) const{
			return scale_generic_alpha( alpha, size, 1.5, linear, offset );
		}
		Plane scale_cubic( const Plane& alpha, Point<unsigned> size, Point<double> offset={0.0,0.0} ) const{
			return scale_generic_alpha( alpha, size, 2.5, mitchell, offset );
		}
		Plane scale_lanczos3( const Plane& alpha, Point<unsigned> size, Point<double> offset={0.0,0.0} ) const
			{ return scale_generic_alpha( alpha, size, 3.5, lancozs3, offset ); }
		Plane scale_lanczos5( const Plane& alpha, Point<unsigned> size, Point<double> offset={0.0,0.0} ) const
			{ return scale_generic_alpha( alpha, size, 5.5, lancozs5, offset ); }
		Plane scale_lanczos7( const Plane& alpha, Point<unsigned> size, Point<double> offset={0.0,0.0} ) const
			{ return scale_generic_alpha( alpha, size, 7.5, lancozs7, offset ); }
		
		Plane scale_select( const Plane& alpha, Point<unsigned> size, ScalingFunction scaling, Point<double> offset={0.0,0.0} ) const{
			switch( scaling ){
				case ScalingFunction::SCALE_NEAREST : return scale_nearest(       size );
				case ScalingFunction::SCALE_LINEAR  : return scale_linear(        alpha, size, offset );
				case ScalingFunction::SCALE_MITCHELL: return scale_cubic(         alpha, size, offset );
				case ScalingFunction::SCALE_CATROM  : return scale_generic_alpha( alpha, size, 2.5, catrom, offset );
				case ScalingFunction::SCALE_SPLINE  : return scale_generic_alpha( alpha, size, 2.5, spline, offset );
				case ScalingFunction::SCALE_LANCZOS_3 : return scale_lanczos3(    alpha, size, offset );
				case ScalingFunction::SCALE_LANCZOS_5 : return scale_lanczos5(    alpha, size, offset );
				case ScalingFunction::SCALE_LANCZOS_7 : return scale_lanczos7(    alpha, size, offset );
				default: return Plane();
			}
		}
		
	//Edge-detection
	private:
		using Weights = std::vector<int>;
		Plane edge_zero_generic( const Weights& weights, unsigned div ) const;
		Plane edge_dm_generic( const Weights& weights_x, const Weights& weights_y, unsigned div ) const;
		PlaneBase<std::pair<int,int>> edge_dm_direction( const Weights& weights_x, const Weights& weights_y, unsigned div ) const;
		
	public:
		Plane edge_robert() const{
			return edge_dm_generic( Weights{ 0,1, -1,0 }, Weights{ 1,0, 0,-1 }, 1 );
		}
		Plane edge_sobel() const{
			return edge_dm_generic( Weights{ -1,0,1, -2,0,2, -1,0,1 }, Weights{ 1,2,1, 0,0,0, -1,-2,-1 }, 4 );
		}
		PlaneBase<std::pair<int,int>> edge_sobel_direction() const{
			return edge_dm_direction( Weights{ -1,0,1, -2,0,2, -1,0,1 }, Weights{ 1,2,1, 0,0,0, -1,-2,-1 }, 4 );
		}
		Plane edge_prewitt() const{
			return edge_dm_generic( Weights{ -1,0,1, -1,0,1, -1,0,1 }, Weights{ 1,1,1, 0,0,0, -1,-1,-1 }, 3 );
		}
		Plane edge_laplacian_ex(double sigma, double k, int size) const;
		Plane edge_laplacian() const{
			return edge_laplacian_ex(0.5, 3.0, 2);
			return edge_zero_generic( Weights{ -1,-1,-1, -1,8,-1, -1,-1,-1 }, 1 );
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
		
		Plane edge_guassian(double sigma_low, double sigma_high, double amount) const;
		
	//Blurring
	private:
		Plane weighted_sum( const Kernel &kernel ) const;
		Kernel gaussian_kernel( double deviation_x, double deviation_y ) const;
	public:
		Plane blur_box( unsigned amount_x, unsigned amount_y ) const;
		Plane blur_gaussian( double amount_x, double amount_y ) const;
		
	//De-blurring
		Plane deconvolve_rl( Point<double> amount, unsigned iterations, Plane* creep_plane=nullptr, double creep_amount=0.0 ) const;
};

}

#endif
