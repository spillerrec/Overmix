/*
	This file is part of AnimeRaster.

	AnimeRaster is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	AnimeRaster is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with AnimeRaster.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "JpegImage.hpp"
#include "JpegWrapper.hpp"
#include "FourierPlane.hpp"

#include <QIODevice>

using namespace Overmix;


void JpegBlock::fillFromRaw( const Overmix::PlaneBase<double>& input, Overmix::Point<unsigned> pos, const QuantBlock& quant ){
	for( unsigned iy=0; iy<DCTSIZE; iy++ ){
		auto in = input.scan_line( pos.y + iy );
		for( unsigned ix=0; ix<DCTSIZE; ix++ )
			table[iy][ix] = in[ix] / quant[iy][ix] / scale( ix, iy );
	}
}

void JpegBlock::fillDctPlane( DctPlane& dct, const QuantBlock& quant ) const{
		for( unsigned iy=0; iy<DCTSIZE; iy++ ){
		auto out = dct  .scan_line( iy );
		for( unsigned ix=0; ix<DCTSIZE; ix++ )
			out[ix] = table[iy][ix] * quant[iy][ix] * scale( ix, iy );
	}
}

JpegImage Overmix::from_jpeg( QIODevice& dev ){
	JpegImage img;
	
	JpegDecompress jpeg;
	jpeg.setDevice( dev );
	jpeg.readHeader();
	
	jpeg.cinfo.raw_data_out = true;
	auto v_ptr = jpeg_read_coefficients( &jpeg.cinfo );
	
	
	for( int ic=0; ic<jpeg.numComponents(); ic++ ){
		JpegPlane p( jpeg.blockCount( ic ), { jpeg.cinfo.comp_info[ic].quant_table->quantval } );
		
		auto blockarr = jpeg.cinfo.mem->access_virt_barray( (j_common_ptr)&jpeg.cinfo, v_ptr[ic], 0, 1, false );
		for( unsigned iy=0; iy<p.get_height(); iy++ )
			for( unsigned ix=0; ix<p.get_width(); ix++ )
				p.scan_line(iy)[ix] = JpegBlock{ blockarr[iy][ix] };
		
		img.planes.emplace_back( std::move( p ) );
	}
	
	return img;
}


Plane JpegPlane::toPlane() const{
	Plane out( getSize() * DCTSIZE );
	DctPlane dct( {DCTSIZE, DCTSIZE} );
	
	for( unsigned iy=0; iy<get_height(); iy++ ){
		for( unsigned ix=0; ix<get_width(); ix++ ){
			scan_line( iy )[ix].fillDctPlane( dct, quant );
			dct.toPlane( out, { DCTSIZE*ix, DCTSIZE*iy }, 255 );
		}
	}
	
	return out;
}
