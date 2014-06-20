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

#include <algorithm>
#include <cmath>
#include <QDebug>
#include <fftw3.h>

using namespace std;



FourierPlane::FourierPlane( const Plane& p ) : PlaneBase( p.get_width() / 2 + 1, p.get_height() ){
	real_width = p.get_width();
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
		for( unsigned ix=0; ix<p.get_width(); ix++ )
			row[ix] = color::fromDouble( raw[iy*p.get_width()+ix] / (real_width * get_height()) );
	}
	return p;
}

