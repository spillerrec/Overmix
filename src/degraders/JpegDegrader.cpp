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

#include "JpegDegrader.hpp"

#include "../debug.hpp"
#include "../planes/FourierPlane.hpp"
#include "../planes/ImageEx.hpp"

constexpr int DCTSIZE = 8;

QuantTable::QuantTable() : table( DCTSIZE, DCTSIZE ) { table.fill( 1 ); }
QuantTable::QuantTable( uint16_t* input ) : table( DCTSIZE, DCTSIZE ) {
	for( unsigned iy=0; iy<DCTSIZE; iy++ ){
		auto row = table.scan_line( iy );
		for( unsigned ix=0; ix<DCTSIZE; ix++ )
			row[ix] = input[iy*DCTSIZE + ix];
	}
}

Plane QuantTable::degrade8x8( const Plane& p ) const{
	DctPlane f( p, 255 );
	//NOTE: it is zero-centered with 127.5, not 128 as JPEG specifies!
	
	for( unsigned iy=0; iy<f.get_height(); iy++ ){
		auto row = f.scan_line( iy );
		auto quant = table.const_scan_line( iy );
		for( unsigned ix=0; ix<f.get_width(); ix++ )
			row[ix] = degradeCoeff( ix, iy, row[ix], quant[ix] );
	}
	
	return f.toPlane( 255 );
}
Plane QuantTable::degrade( const Plane& p ) const{
	Timer t( "QuantTable::degrade" );
	Plane out( p.getSize() );
	for( unsigned iy=0; iy<p.get_height(); iy+=8 )
		for( unsigned ix=0; ix<p.get_width(); ix+=8 ){
			Plane test( 8, 8 );
			test.copy( p, {ix,iy}, {8,8}, {0,0} );
			test = degrade8x8( test );
			out.copy( test, {0,0}, {8,8}, {ix,iy} );
		}
	return out;
}

Plane JpegPlane::degrade( const Plane& p ) const{
	auto out = p.scale_cubic( p.getSize() * sampling );
	return quant.degrade( p );
}

ImageEx JpegDegrader::degrade( const ImageEx& img ) const{
	auto out = img;
	//TODO: change color space
	
	for( unsigned i=0; i<out.size(); i++ )
		out[i] = planes[i].degrade( out[i] );
	
	//TODO: change color space back
	
	return out;
}
