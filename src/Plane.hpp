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

#include <QPoint>
#include <QList>
#include <cstdio>
#include <utility>

typedef int color_type;
typedef std::pair<QPoint,double> MergeResult;

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
	double* values;
};

class Plane{
	private:
		unsigned height;
		unsigned width;
		unsigned line_width;
		color_type *data;
		
	public:
		Plane() : height(0), width(0), line_width(0), data(nullptr) { }
		Plane( unsigned w, unsigned h );
		~Plane();
		
		Plane( Plane&& p )
			:	height(p.height), width(p.width), line_width(p.line_width), data(p.data)
			{ p.data = nullptr; }
		
		Plane& operator=( const Plane& p );
		Plane& operator=( Plane&& p ){
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
		
		static const unsigned long MAX_VAL;
		
	//Plane handling
		Plane( const Plane& p );
		
	//Status
		operator bool() const{ return valid(); }
		bool valid() const{ return data != nullptr; }
		unsigned get_height() const{ return height; }
		unsigned get_width() const{ return width; }
		unsigned get_line_width() const{ return line_width; }
		
	//Pixel/Row query
		color_type& pixel( unsigned x, unsigned y ) const{ return data[ x + y*line_width ]; }
		color_type* scan_line( unsigned y ) const{ return data + y*line_width; }
		const color_type* const_scan_line( unsigned y ) const{ return scan_line( y ); }
		
	//Queries
		color_type min_value() const;
		color_type max_value() const;
		color_type mean_value() const;
		
	//Drawing methods
		void fill( color_type value );
		void copy( int x, int y, const Plane& from );
		
	//Interlacing methods
		bool is_interlaced() const;
		void replace_line( const Plane &p, bool top );
		void combine_line( const Plane &p, bool top );
		
	//Overlays
	private:
		void for_each_pixel( void (*f)( const SimplePixel& ), void *data=nullptr );
		void for_each_pixel( Plane &p, void (*f)( const SimplePixel& ), void *data=nullptr );
	public:
		void substract( Plane &p );
		void divide( Plane &p );
		void multiply( Plane &p );
		Plane level( color_type limit_min, color_type limit_max
			,	color_type output_min, color_type output_max
			,	double gamma
			) const;
		Plane normalize() const;
		
	//Difference
		double diff( const Plane& p, int x, int y, unsigned stride=1 ) const;
		MergeResult best_round_sub( const Plane& p, int level, int left, int right, int top, int bottom, DiffCache *cache ) const;
		
	//Cropping
		Plane crop( unsigned x, unsigned y, unsigned width, unsigned height ) const;
		
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
		Plane scale_generic( unsigned wanted_width, unsigned wanted_height, double window, Filter f ) const;
	public:
		Plane scale_nearest( unsigned wanted_width, unsigned wanted_height ) const;
		Plane scale_linear( unsigned wanted_width, unsigned wanted_height ) const{
			return scale_generic( wanted_width, wanted_height, 1, linear );
		}
		Plane scale_cubic( unsigned wanted_width, unsigned wanted_height ) const{
			return scale_generic( wanted_width, wanted_height, 2, mitchell );
		}
		Plane scale_lanczos( unsigned wanted_width, unsigned wanted_height ) const;
		
	//Edge-detection
	private:
		Plane edge_zero_generic( int *weights, unsigned size, unsigned div ) const;
		Plane edge_dm_generic( int *weights_x, int *weights_y, unsigned size, unsigned div ) const;
		
	public:
		Plane edge_robert() const{
			int kx[] = { 0,1, -1,0 };
			int ky[] = { 1,0, 0,-1 };
			return edge_dm_generic( kx, ky, 2, 1 );
		}
		Plane edge_sobel() const{
			int kx[] = { -1,0,1, -2,0,2, -1,0,1 };
			int ky[] = { 1,2,1, 0,0,0, -1,-2,-1 };
			return edge_dm_generic( kx, ky, 3, 4 );
		}
		Plane edge_prewitt() const{
			int kx[] = { -1,0,1, -1,0,1, -1,0,1 };
			int ky[] = { 1,1,1, 0,0,0, -1,-1,-1 };
			return edge_dm_generic( kx, ky, 3, 3 );
		}
		Plane edge_laplacian() const{
			int k[] = { -1,-1,-1, -1,8,-1, -1,-1,-1 };
			return edge_zero_generic( k, 3, 1 );
		}
		Plane edge_laplacian_large() const{
			int k[] = {
					 0, 0,-1, 0, 0,
					 0,-1,-2,-1, 0,
					-1,-2,16,-2,-1, 
					 0,-1,-2,-1, 0,
					 0, 0,-1, 0, 0
				};
			return edge_zero_generic( k, 5, 2 );
		}
		
	//Blurring
	private:
		Plane weighted_sum( double *kernel, unsigned w_width, unsigned w_height ) const; //TODO: change to kernel
		Kernel gaussian_kernel( double deviation_x, double deviation_y ) const;
	public:
		Plane blur_box( unsigned amount_x, unsigned amount_y ) const;
		Plane blur_gaussian( unsigned amount_x, unsigned amount_y ) const;
		
	//De-blurring
		Plane deconvolve_rl( double amount, unsigned iterations ) const;
};

#endif