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

#include "../color.hpp"
#include "../debug.hpp"

#include "gwenview/iodevicejpegsourcemanager.h"
#include "jpeglib.h"

#include <vector>

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
				for( int i=0; i<rows.size(); i++ )
					rows[i][ix] = color::from8bit( row_pointer[ix*rows.size() + i] );
		}
};

bool ImageEx::from_jpeg( QIODevice& dev ){
	Timer t( "from_jpeg" );
	jpeg_decompress_struct cinfo;
	jpeg_error_mgr jerr;
	
	cinfo.err = jpeg_std_error( &jerr );
	jpeg_create_decompress( &cinfo );
	
	Gwenview::IODeviceJpegSourceManager::setup( &cinfo, &dev );
	jpeg_read_header( &cinfo, true );
	jpeg_start_decompress( &cinfo );
	
	Size<unsigned> size( cinfo.output_width, cinfo.output_height );
	switch( cinfo.out_color_components ){
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
		
		ImgRow row( *this, iy, cinfo.output_components );
		row.read8( arr );
	}
	
	jpeg_finish_decompress( &cinfo );
	jpeg_destroy_decompress( &cinfo );
	
	return true;
}