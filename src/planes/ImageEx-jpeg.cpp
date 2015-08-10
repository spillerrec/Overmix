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

#include "ImageEx.hpp"

#include "FourierPlane.hpp"

#include "../color.hpp"
#include "../debug.hpp"

#include "gwenview/iodevicejpegsourcemanager.h"
#include "jpeglib.h"

#include <vector>
//#include <QImage>

class ImgRow{
	private:
		std::vector<color_type*> rows;
		unsigned width;
		
	public:
		ImgRow( ImageEx& img, unsigned iy, int colors )
			:	width( img.get_width() ){
			rows.reserve( colors );
			for( int i=0; i<colors; i++ )
				rows.push_back( img[i].scan_line( iy ) );
		}
		
		void read8( unsigned char* row_pointer ){
			for( unsigned ix=0; ix<width; ix++ )
				for( unsigned i=0; i<rows.size(); i++ )
					rows[i][ix] = color::from8bit( row_pointer[ix*rows.size() + i] );
		}
};

class QuantTable{
	private:
		Plane table; //TODO: type?
		
		double scale( int ix, int iy ) const
			{ return 2 * 4 * (ix==0?sqrt(2):1) * (iy==0?sqrt(2):1); }
			//NOTE: 4 is defined by JPEG, 2
		
		double degradeCoeff( int ix, int iy, double coeff, double quant ) const{
			auto factor = quant * scale(ix,iy);
			return int( coeff / factor ) * factor; //TODO: proper round!
		}
		
	public:
		QuantTable() : table( DCTSIZE, DCTSIZE ) { table.fill( 1 ); }
		QuantTable( UINT16* input ) : table( DCTSIZE, DCTSIZE ) {
			for( unsigned iy=0; iy<DCTSIZE; iy++ ){
				auto row = table.scan_line( iy );
				for( unsigned ix=0; ix<DCTSIZE; ix++ )
					row[ix] = input[iy*DCTSIZE + ix];
			}
		}
		
		Plane degrade8x8( const Plane& p ) const;
		
		Plane degrade( const Plane& p ) const;
};

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

template<typename T>
QString outputDCT( T* dct ){
	QString out;
	for( unsigned i=0; i<64; i++ )
		out += QString::number( dct[i] ) + " ";
	return out;
}

bool ImageEx::from_jpeg( QIODevice& dev ){
	Timer t( "from_jpeg" );
	jpeg_decompress_struct cinfo;
	jpeg_error_mgr jerr;
	
	cinfo.err = jpeg_std_error( &jerr );
	jpeg_create_decompress( &cinfo );
	
	Gwenview::IODeviceJpegSourceManager::setup( &cinfo, &dev );
	jpeg_read_header( &cinfo, true );
	
	/*
	auto v_ptr = jpeg_read_coefficients( &cinfo );
	auto blockarr = cinfo.mem->access_virt_barray( (j_common_ptr)&cinfo, v_ptr[0], 0, 1, false );
	for( unsigned j=0; j<5; j++ ){
		auto block = blockarr[0][j];
		QString out;
		for( unsigned i=0; i<64; i++ )
			out += QString::number( block[i] ) + " ";
		qDebug() << out;
	}
	/*/
	jpeg_start_decompress( &cinfo );
	
	Size<unsigned> size( cinfo.output_width, cinfo.output_height );
	switch( cinfo.out_color_components ){ //jpeg_color_space, JCS_YCbCr GRAYSCALE, RGB
		case 1: type = GRAY; qCDebug(LogImageIO) << "Gray-scale JPEG"; break;
		case 3: type = RGB; break;
		default: qCWarning(LogImageIO) << "Unknown components count:" << cinfo.out_color_components;
	}
	
	
	for( int i=0; i<cinfo.output_components; i++ )
		planes.emplace_back( size );
	
	std::vector<unsigned char> buffer( cinfo.output_components*size.width() );
	for( unsigned iy=0; iy<size.height(); iy++ ){
		auto arr = buffer.data(); //TODO: Why does it need the address of the pointer?
		jpeg_read_scanlines( &cinfo, &arr, 1 );
		
		ImgRow( *this, iy, cinfo.output_components ).read8( arr );
	}
	
//	QuantTable quant( cinfo.comp_info[0].quant_table->quantval );
//	ImageEx( quant.degrade( ImageEx::fromFile("out-clean/00.png" )[0] ) ).to_qimage( SYSTEM_REC709 ).save( "Test.png" );
	
	jpeg_finish_decompress( &cinfo );//*/
	jpeg_destroy_decompress( &cinfo );
	
	return true;
}