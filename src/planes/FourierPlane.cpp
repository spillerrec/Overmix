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

#include "FourierPlane.hpp"

#include "../color.hpp"
#include "../debug.hpp"

#include <algorithm>
#include <cmath>
#include <QDebug>
#include <fftw3.h>

using namespace std;



FourierPlane::FourierPlane( const Plane& p ) : PlaneBase( p.get_width() / 2 + 1, p.get_height() ){
	real_width = p.get_width();
	scaling = 1.0 / (real_width * get_height());
	vector<double> raw( p.get_width() * p.get_height() );
	
	fftw_plan plan = fftw_plan_dft_r2c_2d( p.get_height(), p.get_width(), raw.data(), (fftw_complex*)data, FFTW_ESTIMATE );
	
	//Fill data
	for( unsigned iy=0; iy<p.get_height(); iy++ ){
		auto row = p.const_scan_line( iy );
		for( unsigned ix=0; ix<p.get_width(); ix++ )
			raw[iy*p.get_width()+ix] = color::asDouble( row[ix] );
	}
	
	fftw_execute( plan );
	
	fftw_destroy_plan( plan );
}

Plane FourierPlane::asPlane() const{
	//Find maximum value
	double max_real = 0.0;
	for( unsigned iy=0; iy<get_height(); iy++ ){
		auto in = const_scan_line( iy );
		for( unsigned ix=0; ix<get_width(); ix++ )
			max_real = max( max_real, abs( in[ix] ) );
	}
	
	Plane output( get_width(), get_height() );
	
	double scale = 100000;
	auto half_size = get_height() / 2;
	for( unsigned iy=0; iy<get_height(); iy++ ){
		auto in = const_scan_line( iy < half_size ? iy + half_size : iy - half_size );
		auto out = output.scan_line( iy );
		for( unsigned ix=0; ix<get_width(); ix++ )
			out[ix] = color::fromDouble( log( abs( in[ix] ) / max_real * scale + 1 ) / log( scale + 1 ) );
	}
	
	return output;
}

Plane FourierPlane::toPlaneInvalidate() const{
	//Convert
	vector<double> raw( real_width * get_height() );
	fftw_plan plan = fftw_plan_dft_c2r_2d( get_height(), real_width, (fftw_complex*)data, raw.data(), FFTW_ESTIMATE );
	fftw_execute( plan );
	fftw_destroy_plan( plan );
	
	//Fill data into plane
	Plane p( real_width, get_height() );
	for( unsigned iy=0; iy<p.get_height(); iy++ ){
		auto row = p.scan_line( iy );
		for( unsigned ix=0; ix<p.get_width(); ix++ ){
			auto val = raw[iy*p.get_width()+ix] * scaling;//(real_width * get_height());
			row[ix] = color::fromDouble( max( min( val, 1.0 ), 0.0 ) );
		}
	}
	return p;
}

void FourierPlane::debugResolution( string path ) const{
	debug::CsvFile csv( path );
	csv.add( "Res" ).add( "height" ).stop();
	
	auto half_size = get_height() / 2;
	
	unsigned stride = 10;
	for( unsigned i=0; i<half_size; i++ ){
		
		double sum = 0.0;
		
		for( unsigned j=0; j<stride; j++, i++ ){
			auto row1 = const_scan_line( i );
			auto row2 = const_scan_line( get_height() - i - 1 );
			for( unsigned ix=0; ix<get_width(); ix++ )
				sum += abs( row1[ix] ) + abs( row2[ix] );
		}
		
		csv.add( to_string( i*2 ) ).add( sum );
		
		csv.stop();
	}
	
}

FourierPlane FourierPlane::reduce( unsigned w, unsigned h ) const{
	FourierPlane out( w/2+1, h );
	out.real_width = w;
	out.scaling = scaling;
	out.fill( std::complex<double>( 0, 0 ) );
	
	for( unsigned iy=0; iy<h/2; iy++ ){
		auto in1 = const_scan_line( iy );
		auto in2 = const_scan_line( get_height() - iy - 1 );
		auto out1 = out.scan_line( iy );
		auto out2 = out.scan_line( h - iy - 1 );
		for( unsigned ix=0; ix<out.get_width(); ix++ ){
			out1[ix] = in1[ix];
			out2[ix] = in2[ix];
		}
	}
	
	return out;
}

void FourierPlane::remove( unsigned w, unsigned h ){
	unsigned small_width = w / 2 + 1;
	
	//Remove from vertical
	unsigned y_start = (get_height()-h)/2;
	for( unsigned iy=y_start; iy<y_start+h; iy++ ){
		auto row = scan_line( iy );
		for( unsigned ix=0; ix<small_width; ix++ )
			row[ix] = complex<double>( 0, 0 );
	}
	
	//Remove from horizontal
	for( unsigned iy=0; iy<get_height(); iy++ ){
		auto row = scan_line( iy );
		for( unsigned ix=small_width; ix<get_width(); ix++ )
			row[ix] = complex<double>( 0, 0 );
	}
	
	/*
	//Soften edges
	unsigned amount = min( small_width, h/2 ) / 2;
	for( unsigned i=0; i<amount; i++ ){
		double factor = (double)i / amount;
		
		auto row1 = scan_line( y_start-1 - i );
		auto row2 = scan_line( y_start+h + i );
		for( unsigned ix=0; ix<small_width; ix++ ){
			row1[ix] *= factor;
			row2[ix] *= factor;
		}
		
		for( unsigned iy=0; iy<get_height(); iy++ )
			scan_line( iy )[small_width-1 - i] *= factor;
	}
	*/
}


const double PI = std::atan(1)*4;
static double gaussian1d( double dx, double devi ){
	double base = 1.0;// / ( sqrt( 2*PI ) * devi );
	double power = -( dx*dx ) / ( 2*devi*devi );
	return base * exp( power );
}
static double gaussian2d( double dx, double dy, double devi_x, double devi_y ){
	return gaussian1d( dx, devi_x ) * gaussian1d( dy, devi_y );
}

void FourierPlane::blur( double dev_x, double dev_y ){
	for( unsigned iy=0; iy<get_height(); iy++ ){
		auto row = scan_line( iy );
		int y = (iy < get_height()/2) ? iy : get_height() - iy - 1;
		for( unsigned ix=0; ix<get_width(); ix++ )
			row[ix] *= gaussian2d( ix, y, dev_x, dev_y );//polar( abs( row[ix] ) * gaussian2d( ix, y, dev_x, dev_y ), arg( row[ix] ) );
	}
}

