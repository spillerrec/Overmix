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

Plane QuantTable::degrade8x8( DctPlane& f, const Plane& p, Point<unsigned> pos ) const{
	//NOTE: it is zero-centered with 127.5, not 128 as JPEG specifies!
	f.initialize( p, pos, 255 );
	
	for( unsigned iy=0; iy<f.get_height(); iy++ ){
		auto row = f.scan_line( iy );
		auto quant = table.const_scan_line( iy );
		for( unsigned ix=0; ix<f.get_width(); ix++ )
			row[ix] = degradeCoeff( ix, iy, row[ix], quant[ix] );
	}
	
	return f.toPlane( 255 );
}

Plane QuantTable::degrade8x8Comp( DctPlane& f1, DctPlane& f2, const Plane& p1, const Plane& p2, Point<unsigned> pos ) const{
	f1.initialize( p1, pos, 255 );
	f2.initialize( p2, pos, 255 );
	
	for( unsigned iy=0; iy<f1.get_height(); iy++ ){
		auto row1 = f1.scan_line( iy );
		auto row2 = f2.scan_line( iy );
		auto quant = table.const_scan_line( iy );
		for( unsigned ix=0; ix<f1.get_width(); ix++ ){
		//	qDebug() << "coeff: " << row1[ix] << " - " << row2[ix];
			auto deg1 = quantize( ix, iy, row1[ix], quant[ix] );
			auto deg2 = quantize( ix, iy, row2[ix], quant[ix] );
		//	qDebug() << "deg: " << deg1 << " - " << deg2;
		//	qDebug() << "deg-scaled: " << deg1 * quant[ix] * scale(ix,iy) << " - " << deg2* quant[ix] * scale(ix,iy);
			if( deg1 == deg2 )
				row2[ix] = row1[ix]; //deg1 * quant[ix] * scale(ix,iy);
		}
	}
	
	return f2.toPlane( 255 );
}

Plane QuantTable::degrade( const Plane& p ) const{
	Timer t( "QuantTable::degrade" );
	DctPlane f( {8,8} );
	Plane out( p.getSize() );
	for( unsigned iy=0; iy<p.get_height(); iy+=8 )
		for( unsigned ix=0; ix<p.get_width(); ix+=8 ){
			auto test = degrade8x8( f, p, {ix,iy} );
			out.copy( test, {0,0}, {8,8}, {ix,iy} );
		}
	return out;
}

Plane QuantTable::degradeComp( const Plane& p1, const Plane& p2 ) const{
	Timer t( "QuantTable::degradeComp" );
	DctPlane f1( {8,8} );
	DctPlane f2( {8,8} );
	
	Plane out( p1.getSize() );
	for( unsigned iy=0; iy<p1.get_height(); iy+=8 )
		for( unsigned ix=0; ix<p1.get_width(); ix+=8 ){
			auto test = degrade8x8Comp( f1, f2, p1, p2, {ix,iy} );
			out.copy( test, {0,0}, {8,8}, {ix,iy} );
		}
	return out;
}

Plane JpegPlane::degrade( const Plane& p ) const{
	//auto out = p.scale_cubic( p.getSize() * sampling );
	return quant.degrade( p );
}

Plane JpegPlane::degradeComp( const Plane& p1, const Plane& p2 ) const{
	//auto out = p.scale_cubic( p.getSize() * sampling );
	return quant.degradeComp( p1, p2 );
}

ImageEx JpegDegrader::degrade( const ImageEx& img ) const{
	auto out = img;
	//TODO: change color space
	
	for( unsigned i=0; i<out.size(); i++ )
		out[i] = planes[i].degrade( out[i] );
	
	//TODO: change color space back
	
	return out;
}
