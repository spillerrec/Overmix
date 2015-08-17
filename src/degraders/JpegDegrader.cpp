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
#include "../color.hpp"
#include "../planes/FourierPlane.hpp"
#include "../planes/ImageEx.hpp"

constexpr int DCTSIZE = 8;

QuantTable::QuantTable() : table( DCTSIZE, DCTSIZE ) { table.fill( 1 ); }
QuantTable::QuantTable( uint16_t* input ) : table( DCTSIZE, DCTSIZE ) {
	for( unsigned iy=0; iy<DCTSIZE; iy++ ){
		auto row = table.scan_line( iy );
		for( unsigned ix=0; ix<DCTSIZE; ix++ )
			row[ix] = 1.0 / (input[iy*DCTSIZE + ix] * scale(ix,iy));
	}
}

unsigned QuantTable::degrade8x8Comp( DctPlane& f1, DctPlane& f2, const Plane& p1, const Plane& p2, Point<unsigned> pos ) const{
	f1.initialize( p1, pos, 255 );
	f2.initialize( p2, pos, 255 );
	unsigned change = 0;
	
	for( unsigned iy=0; iy<f1.get_height(); iy++ ){
		auto row1 = f1.scan_line( iy );
		auto row2 = f2.scan_line( iy );
		auto quant = table.const_scan_line( iy );
		for( unsigned ix=0; ix<f1.get_width(); ix++ ){
			auto deg1 = quantize( row1[ix], quant[ix] );
			auto deg2 = quantize( row2[ix], quant[ix] );
			if( deg1 == deg2 )
				row2[ix] = row1[ix];
			else{
				auto shift = ( row1[ix] > row2[ix] ) ? +0.45 : -0.45;
				row2[ix] = (deg2 + shift)/quant[ix];
					
				change++;
			}
		}
	}
	
	return change;
}

static bool hasChange( const Plane& mask, Point<unsigned> pos ){
	auto size = mask.getSize().min( pos + Size<unsigned>(8,8) ) - pos;
	for( unsigned iy=0; iy<size.height(); iy++ ){
		auto row = mask.scan_line( iy + pos.y );
		for( unsigned ix=0; ix<size.width(); ix++ )
			if( row[ix+pos.x] > 0 )
				return true;
	}
	
	return false;
}

Plane QuantTable::degradeComp( const Plane& mask, const Plane& p1, const Plane& p2, unsigned& change ) const{
//	Timer t( "QuantTable::degradeComp" );
	DctPlane f1( {8,8} );
	DctPlane f2( {8,8} );
	
	Plane out( p1 );
	Plane mask2( mask );
	mask2.fill( color::BLACK );
	for( unsigned iy=0; iy<p1.get_height(); iy+=8 )
		for( unsigned ix=0; ix<p1.get_width(); ix+=8 ){
			if( hasChange( mask, {ix, iy} ) ){
				auto local = degrade8x8Comp( f1, f2, p1, p2, {ix,iy} );
				if( local != 0 )
					f2.toPlane( out, {ix,iy}, 255 );
				change += local;
			}
		}
	
	return out;
}

Plane JpegPlane::degradeComp( const Plane& mask, const Plane& p1, const Plane& p2, unsigned& change ) const{
	//auto out = p.scale_cubic( p.getSize() * sampling );
	return quant.degradeComp( mask, p1, p2, change );
}
