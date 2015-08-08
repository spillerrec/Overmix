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

#include "../color.hpp"

#include <QtConcurrent>
#include <QDebug>
#include <cmath>

using namespace std;


template<typename color_type>
struct WeightedSumLine{
	color_type *out;	//Output row
	const color_type *in; //Input row, must be the top row weights affects
	unsigned width;
	unsigned line_width; //For input row
	
	//Only bounds-checked in the horizontal direction!
	const double *weights;
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
	
	color_type weighted_sum( const color_type* in ) const{
		double sum = 0;
		for( unsigned iy=0; iy<w_height; ++iy ){
			auto line = in + iy*line_width;
			for( unsigned ix=0; ix<w_width; ++ix, ++line )
				sum += weights[ ix + iy*w_width ] * (*line);
		}
		
		sum /= full_sum;
	//	return std::round( std::max( sum, 0.0 ) );
		return sum;
		
	}
	color_type weighted_sum( const color_type* in, int cutting ) const{
		unsigned start = cutting > 0 ? 0 : -cutting;
		unsigned end = cutting > 0 ? w_width-cutting : w_width;
		
		double sum = 0;
		double w_sum = 0;
		for( unsigned iy=0; iy<w_height; ++iy ){
			auto line = in + iy*line_width;
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

template<typename color_type>
void sum_line_template( const WeightedSumLine<color_type>& line ){
	auto in = line.in;
	auto out = line.out;
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

void sum_line( const WeightedSumLine<color_type>& line )
	{ sum_line_template( line ); }
	
void sum_line_double( const WeightedSumLine<double>& line )
	{ sum_line_template( line ); }

template<typename T>
std::vector<WeightedSumLine<T>> prepare_weighted_sum(
		const PlaneBase<T>& in, PlaneBase<T>& out, const Kernel& kernel ){
	auto size = out.getSize();
	
	//Set default settings
	WeightedSumLine<T> default_line;
	default_line.width = size.width();
	default_line.line_width = in.get_line_width();
	default_line.weights = kernel.const_scan_line(0); //TODO: line_width ignored!
	default_line.w_width = kernel.get_width();
	default_line.w_height = kernel.get_height();
	default_line.full_sum = default_line.calculate_sum();
	
	int half_size = kernel.get_height() / 2;
	std::vector<WeightedSumLine<T>> lines;
	for( unsigned iy=0; iy<size.height(); ++iy ){
		auto line = default_line;
		line.out = out.scan_line( iy );
		
		int top = iy - half_size;
		if( top < 0 ){
			//Cut stuff from top
			line.w_height += top; //Subtracts!
			line.weights += -top * line.w_width;
			line.in = in.const_scan_line( 0 );
			line.full_sum = line.calculate_sum();
		}
		else if( top+kernel.get_height() >= size.height() ){
			//Cut stuff from bottom
			line.in = in.const_scan_line( top );
			line.w_height = size.height() - top;
			line.full_sum = line.calculate_sum();
		}
		else //Use defaults
			line.in = in.const_scan_line( top );
		
		lines.push_back( line );
	}
	
	return lines;
}

PlaneBase<double> convolve( const PlaneBase<double>& in, const Kernel& kernel ){
	if( !kernel.valid() )
		return {};
	
	PlaneBase<double> out( in.getSize() );
	
	auto lines = prepare_weighted_sum( in, out, kernel );
	QtConcurrent::blockingMap( lines, &sum_line_double );
	
	return out;
}

Plane Plane::weighted_sum( const Kernel &kernel ) const{
	if( !kernel.valid() )
		return Plane();
	
	Plane out( size );
	
	auto lines = prepare_weighted_sum( *this, out, kernel );
	QtConcurrent::blockingMap( lines, &sum_line );
	
	return out;
}

Plane Plane::blur_box( unsigned amount_x, unsigned amount_y ) const{
	Kernel kernel( amount_x, amount_y );
	kernel.fill( 1.0 );
	return weighted_sum( kernel );
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
	auto uneven = []( double val ){ return int(std::ceil(val))/2*2+1; };
	Kernel kernel( uneven( 12.5*deviation_x ), uneven( 12.5*deviation_y ) );
	
	//Fill kernel
	auto half = (kernel.getSize() - 1).to<double>() / 2.0;
	for( unsigned iy=0; iy<kernel.get_height(); ++iy ){
		auto row = kernel.scan_line( iy );
		for( unsigned ix=0; ix<kernel.get_width(); ++ix )
			row[ix] = gaussian2d( ix-half.x, iy-half.y, deviation_x, deviation_y );
	}
	
	qDebug() << "Kernel size: " << kernel.get_width() << "x" << kernel.get_height();
	return kernel;
}

Plane Plane::blur_gaussian( double amount_x, double amount_y ) const{
	const double scaling = 0.33/2; //TODO: pixel to deviation
	auto kernel = gaussian_kernel( amount_x*scaling, amount_y*scaling );
	return weighted_sum( kernel );
}

using DPlane = PlaneBase<double>;
static DPlane divide2( const DPlane& left, const DPlane& right ){
	auto copy = left;
	auto rl=begin(copy);
	auto rr=begin(right);
	for( ; rl!=end(copy); ++rl, ++rr ){
		auto pl=begin(*rl);
		auto pr=begin(*rr);
		for( ; pl!=end(*rl); ++pl, ++pr )
			*pl = *pl / *pr;
	}
	return copy;
}
static DPlane multiply2( const DPlane& left, const DPlane& right ){
	auto copy = left;
	auto rl=begin(copy);
	auto rr=begin(right);
	for( ; rl!=end(copy); ++rl, ++rr ){
		auto pl=begin(*rl);
		auto pr=begin(*rr);
		for( ; pl!=end(*rl); ++pl, ++pr )
			*pl = *pl * *pr;
	}
	return copy;
}
static double change( const DPlane& left, const DPlane& right ){
	double amount = 0;
	auto rl=begin(left);
	auto rr=begin(right);
	for( ; rl!=end(left); ++rl, ++rr ){
		auto pl=begin(*rl);
		auto pr=begin(*rr);
		for( ; pl!=end(*rl); ++pl, ++pr )
			amount += std::abs(*pl - *pr);
	}
	return amount;
}


Plane Plane::deconvolve_rl( double amount, unsigned iterations ) const{
	//The iteration step in Richardson–Lucy deconvolution:
	//New_Est = Est * ( observed / (Est * psf) * flipped_psf );
	//Where Est=estimation, New_Est replaces Est in next iteration
	
	//Create point spread function
	Kernel psf = gaussian_kernel( amount, amount );
	Kernel flipped( psf );
	flipped.flipHor();
	flipped.flipVer();
	qDebug() << "kernel diff: " << change(psf, flipped);
	
	//TODO: Make non-symmetric kernels work!
	/*
	double half_x = psf.height/2.0;
	for( unsigned iy=0; iy<psf.height; iy++ )
		for( unsigned ix=0; ix<psf.width; ix++ ){
			psf.values[iy*psf.width + ix] = 1.0;// gaussian1d( iy-half_x, 8.0/12 );
		}*/
//	for( unsigned i=0; i<psf.width*psf.height; i++ )
//		psf.values[i] = 1.0 / (psf.width*psf.height);
	//return estimate->weighted_sum( psf );
	
	
	auto observed = to<double>();
	auto estimate = observed;
	for( unsigned i=0; i<iterations; ++i ){
		auto est_psf = convolve( estimate, psf );
		auto error_est = convolve( divide2( observed, est_psf ), flipped );
		
		auto new_estimate = multiply2( estimate, error_est );
		//Don't allow imaginary colors!
		new_estimate.truncate( color::BLACK, color::WHITE );
		
		qDebug() << "Deconvolve change: " << i+1 << " - " << change(new_estimate, estimate) / color::WHITE;
		estimate = new_estimate;
		
	}
	
	return estimate.to<color_type>();
}
