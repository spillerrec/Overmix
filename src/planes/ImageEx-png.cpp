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
#include "../utils/PlaneUtils.hpp"

#include <QIODevice>

#include <png.h>
#include <cstring>
#include <vector>

using namespace std;
using namespace Overmix;

struct MemStream{
	//TODO: kinda unnecessary, just pass dev*
	QIODevice* dev;
	MemStream( QIODevice& dev ) : dev(&dev) { }
	
	unsigned read( char* out, unsigned amount ){
		auto result = dev->read( out, amount );
		switch( result ){
			case -1: return 0;
			case  0: return amount;
			default: return result;
		}
	}
	
	static void read( png_structp png_ptr, png_bytep bytes_out, png_size_t lenght ){
		MemStream& stream = *reinterpret_cast<MemStream*>( png_get_io_ptr( png_ptr ) );
		if( stream.read( (char*)bytes_out, lenght ) != lenght )
		return; //Error!
	}
};

class ImgRow{
	private:
		std::vector<color_type*> rows;
		int pixels;
		unsigned width;
		
	public:
		ImgRow( ImageEx& img, unsigned iy, int colors, bool alpha )
			:	pixels( colors + (alpha ? 1 : 0) )
			,	width( img.get_width() ){
			rows.reserve( pixels );
			for( int i=0; i<colors; i++ )
				rows.push_back( img[i].scan_line( iy ) );
			if( alpha )
				rows.push_back( img.alpha_plane().scan_line( iy ) );
		}
		
		void read8( png_bytep row_pointer ){
			for( unsigned ix=0; ix<width; ix++ )
				for( int i=0; i<pixels; i++ )
					rows[i][ix] = color::from8bit( row_pointer[ix*pixels + i] );
		}
		
		void read16( png_bytep row_pointer ){
			auto in = reinterpret_cast<uint16_t*>( row_pointer );
			for( unsigned ix=0; ix<width; ix++ )
				for( int i=0; i<pixels; i++ )
					rows[i][ix] = color::from16bit( in[ix*pixels + i] );
		}
};

bool ImageEx::from_png( QIODevice& dev ){
	Timer t( "from_png" );
	//TODO: Reuse the RAII class in imgviewer?
	//Check signature
	unsigned char header[8];
	if( dev.read( reinterpret_cast<char*>(header), 8 ) != 8 )
		return false;
	if( png_sig_cmp( header, 0, 8 ) )
		return false;
	
	//Start initializing libpng
	auto png_ptr = png_create_read_struct( PNG_LIBPNG_VER_STRING, NULL, NULL, NULL );
	if( !png_ptr ){
		png_destroy_read_struct( &png_ptr, NULL, NULL );
		return false;
	}
	
	auto info_ptr = png_create_info_struct( png_ptr );
	if( !info_ptr ){
		png_destroy_read_struct( &png_ptr, NULL, NULL );
		return false;
	}
	
	MemStream stream( dev );
	png_set_read_fn( png_ptr, &stream, MemStream::read );
	png_set_sig_bytes( png_ptr, 8 );
	
	//Finally start reading
	png_read_png( png_ptr, info_ptr, PNG_TRANSFORM_SWAP_ENDIAN | PNG_TRANSFORM_EXPAND, NULL );
	png_bytep *row_pointers = png_get_rows( png_ptr, info_ptr );
	
	//Determine plane types
	auto color_type = png_get_color_type( png_ptr, info_ptr );
	bool as16bit = png_get_bit_depth( png_ptr, info_ptr ) > 8;
	bool alpha = color_type == PNG_COLOR_TYPE_GRAY_ALPHA || color_type == PNG_COLOR_TYPE_RGB_ALPHA;
	bool gray = color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA;
	int colors = gray ? 1 : 3;
	qCDebug(LogImageIO) << "Alpha:" << alpha;
	qCDebug(LogImageIO) << "Gray:" << gray;
	qCDebug(LogImageIO) << "16-bit:" << as16bit;
	
	unsigned height = png_get_image_height( png_ptr, info_ptr );
	unsigned width = png_get_image_width( png_ptr, info_ptr );
	
	for( int i=0; i<colors; i++ )
		planes.emplace_back( Plane{width, height} );
	if( alpha )
		alpha_plane() = Plane( width, height );
	transform = gray ? Transform::GRAY : Transform::RGB;
	transfer = Transfer::SRGB;
	
	for( unsigned iy=0; iy<height; iy++ ){
		ImgRow row( *this, iy, colors, alpha );
		if( as16bit )
			row.read16( row_pointers[iy] );
		else
			row.read8(  row_pointers[iy] );
	}
	
	
	png_destroy_read_struct( &png_ptr, &info_ptr, NULL );
	return true;
}

