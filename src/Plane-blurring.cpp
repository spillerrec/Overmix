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

#include "Plane.hpp"

#include <QtConcurrent>
#include <QDebug>

using namespace std;


struct WeightedSumLine{
	color_type *out;	//Output row
	color_type *in; //Input row, must be the top row weights affects
	unsigned width;
	unsigned line_width; //For input row
	
	//Only bounds-checked in the horizontal direction!
	double *weights;
	unsigned w_width;
	unsigned w_height; //How many input lines should be weighted, must not overflow!
	double full_sum;
	
	double calculate_sum( unsigned start=0 ) const{
		double sum = 0;
		for( unsigned iy=0; iy<w_height; ++iy )
			for( unsigned ix=start; ix<w_width; ++ix )
				sum += weights[ ix + iy*w_width ];
		return sum;
	}
	
	color_type weighted_sum( color_type* in ) const{
		double sum = 0;
		for( unsigned iy=0; iy<w_height; ++iy ){
			color_type *line = in + iy*line_width;
			for( unsigned ix=0; ix<w_width; ++ix, ++line )
				sum += weights[ ix + iy*w_width ] * (*line);
		}
		
		sum /= full_sum;
		return std::round( std::max( sum, 0.0 ) );
		
	}
	color_type weighted_sum( color_type* in, int cutting ) const{
		unsigned start = cutting > 0 ? 0 : -cutting;
		unsigned end = cutting > 0 ? w_width-cutting : w_width;
		
		double sum = 0;
		double w_sum = 0;
		for( unsigned iy=0; iy<w_height; ++iy ){
			color_type *line = in + iy*line_width;
			for( unsigned ix=start; ix<end; ++ix, ++line ){
				double w = weights[ ix + iy*w_width ];
				sum += w * (*line);
				w_sum += w;
			}
		}
		
		sum /= w_sum;
		return std::round( std::max( sum, 0.0 ) );
	}
};

void sum_line( const WeightedSumLine& line ){
	color_type *in = line.in;
	color_type *out = line.out;
	unsigned size_half = line.w_width/2;
	
	//Fill the start of the row with the same value
	for( unsigned ix=0; ix<size_half; ++ix )
		*(out++) = line.weighted_sum( in, (int)ix-size_half );
	
	unsigned end = line.width - (line.w_width-size_half);
	for( unsigned ix=size_half; ix<=end; ++ix, ++in )
		*(out++) = line.weighted_sum( in );
	
	//Repeat the end with the same value
	for( unsigned ix=end+1; ix<line.width; ++ix, ++in )
		*(out++) = line.weighted_sum( in, ix-end );
}

Plane Plane::weighted_sum( double *kernel, unsigned w_width, unsigned w_height ) const{
	if( !kernel || w_width == 0 || w_height == 0 )
		return Plane();
	
	Plane out( width, height );
	
	//Set default settings
	WeightedSumLine default_line;
	default_line.width = width;
	default_line.line_width = out.get_line_width();
	default_line.weights = kernel;
	default_line.w_width = w_width;
	default_line.w_height = w_height;
	default_line.full_sum = default_line.calculate_sum();
	
	int half_size = w_height / 2;
	std::vector<WeightedSumLine> lines;
	for( unsigned iy=0; iy<height; ++iy ){
		WeightedSumLine line = default_line;
		line.out = out.scan_line( iy );
		
		int top = iy - half_size;
		if( top < 0 ){
			//Cut stuff from top
			line.w_height += top; //Subtracts!
			line.weights += -top * line.w_width;
			line.in = scan_line( 0 );
			line.full_sum = line.calculate_sum();
		}
		else if( top+w_height >= height ){
			//Cut stuff from bottom
			line.in = scan_line( top );
			line.w_height = height - top;
			line.full_sum = line.calculate_sum();
		}
		else //Use defaults
			line.in = scan_line( top );
		
		lines.push_back( line );
	}
	
	QtConcurrent::blockingMap( lines, &sum_line );
	
	return out;
}

Plane Plane::blur_box( unsigned amount_x, unsigned amount_y ) const{
	unsigned size = ++amount_x * ++amount_y;
	double *kernel = new double[ size ];
	if( !kernel )
		return Plane();
	
	for( unsigned iy=0; iy<amount_y; ++iy )
		for( unsigned ix=0; ix<amount_x; ++ix )
			kernel[ ix + iy*amount_x ] = 1.0;
	
	Plane p = weighted_sum( kernel, amount_x, amount_y );
	delete[] kernel;
	return p;
}

const double PI = std::atan(1)*4;
static double gaussian1d( double dx, double devi ){
	double base = 1.0 / ( sqrt( 2*PI ) * devi );
	double power = -( dx*dx ) / ( 2*devi*devi );
	return base * exp( power );
}
static double gaussian2d( double dx, double dy, double devi_x, double devi_y ){
	return gaussian1d( dx, devi_x ) * gaussian1d( dy, devi_y );
}

Kernel Plane::gaussian_kernel( double deviation_x, double deviation_y ) const{
	//TODO: make sure deviation is positive
	
	//Init kernel
	Kernel kernel;
	kernel.width = std::ceil( 12*deviation_x );
	kernel.height = std::ceil( 12*deviation_y );
	kernel.values = new double[ kernel.width * kernel.height ];
	
	if( !kernel.values ){
		kernel.values = nullptr;
		return kernel;
	}
	
	//Fill kernel
	double half_x = kernel.width/2.0;
	double half_y = kernel.height/2.0;
	for( unsigned iy=0; iy<kernel.height; ++iy )
		for( unsigned ix=0; ix<kernel.width; ++ix )
			kernel.values[ ix + iy*kernel.width ] = gaussian2d( ix-half_x, iy-half_y, deviation_x, deviation_y );
	
	return kernel;
}

Plane Plane::blur_gaussian( unsigned amount_x, unsigned amount_y ) const{
	const double scaling = 0.33; //TODO: pixel to deviation
	Kernel kernel = gaussian_kernel( amount_x*scaling, amount_y*scaling );
	if( !kernel.values )
		return Plane();
	
	Plane p = weighted_sum( kernel.values, kernel.width, kernel.height );
	delete[] kernel.values;
	return p;
}


Plane Plane::deconvolve_rl( double amount, unsigned iterations ) const{
	//Create point spread function
	//NOTE: It is symmetric, so we don't need a flipped one
	Kernel psf = gaussian_kernel( amount, amount );
	if( !psf.values )
		return Plane();
	//TODO: Make destructor in Kernel!
	
	//TODO: Make non-symmetric kernels work!
	/*
	double half_x = psf.height/2.0;
	for( unsigned iy=0; iy<psf.height; iy++ )
		for( unsigned ix=0; ix<psf.width; ix++ ){
			psf.values[iy*psf.width + ix] = 1.0;// gaussian1d( iy-half_x, 8.0/12 );
		}*/
//	for( unsigned i=0; i<psf.width*psf.height; i++ )
//		psf.values[i] = 1.0 / (psf.width*psf.height);
	//return estimate->weighted_sum( psf.values, psf.width, psf.height );
	
	Plane blurred( this->weighted_sum( psf.values, psf.width, psf.height ) );
	Plane estimate( blurred ); //NOTE: some use just plain 0.5 as initial estimate
	//TODO: Why did we initialy use *this, and then overwrite it with blurred?
	Plane copy( *this );// blurred ); //*///TODO: make subtract/divide/etc take const input
	for( unsigned i=0; i<iterations; ++i ){
		Plane est_psf = estimate.weighted_sum( psf.values, psf.width, psf.height );
		est_psf.divide( copy ); //This is observed / est_psf
		Plane error_est = est_psf.weighted_sum( psf.values, psf.width, psf.height );
		//TODO: oh shit, what happened to error_est?
		estimate.multiply( est_psf );
	}
	
	delete psf.values;
	return estimate;
}
