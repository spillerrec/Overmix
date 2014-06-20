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
	unsigned amount = p.get_width() * p.get_height();
	vector<double> raw( amount );
	
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
	double max_imag = 0.0, max_real = 0.0;
	
	for( unsigned iy=0; iy<get_height(); iy++ ){
		auto in = const_scan_line( iy );
		for( unsigned ix=0; ix<get_width(); ix++ ){
			max_real = max( max_real, abs( in[ix] ) );
			max_imag = max( max_imag, abs( in[ix].imag() ) );
		}
	}
	
	Plane output( get_width()*2, get_height() );
	
	for( unsigned iy=0; iy<get_height(); iy++ ){
		auto in = const_scan_line( iy );
		auto out = output.scan_line( iy );
		for( unsigned ix=0; ix<get_width(); ix++ ){
			out[ix] = color::fromDouble( abs( in[ix] ) / max_real );
			out[ix+get_width()] = color::fromDouble( abs( in[ix].imag() ) / max_imag );
		}
	}
	
	return output;
}

