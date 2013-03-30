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
#include "Plane.hpp"
#include "Image.hpp"

#include <png.h>

bool ImageEx::initialize_size( unsigned size ){
	if( initialized || planes || infos )
		return false;
	
	planes = new Plane*[ size ];
	infos = new PlaneInfo*[ size ];
	
	return planes && infos;
}

bool ImageEx::read_dump_plane( FILE *f, unsigned index ){
	if( !f )
		return false;
	
	unsigned width, height;
	fread( &width, sizeof(unsigned), 1, f );
	fread( &height, sizeof(unsigned), 1, f );
	
	Plane* p = new Plane( width, height );
	planes[index] = p;
	if( !p )
		return false;
	
	if( index < 1 )
		infos[index] = new PlaneInfo( *p, 0,0, 1 );
	else
		infos[index] = new PlaneInfo( *p, 1,1, 2 );
	if( !infos )
		return false;
	
	unsigned depth;
	fread( &depth, sizeof(unsigned), 1, f );
	unsigned byte_count = (depth + 7) / 8;
	
	for( unsigned iy=0; iy<height; iy++ ){
		color_type *row = p->scan_line( iy );
		for( unsigned ix=0; ix<width; ++ix, ++row ){
			fread( row, byte_count, 1, f );
			*row <<= 16 - depth;
		}
	}
	
	return true;
}

bool ImageEx::from_dump( const char* path ){
	FILE *f = fopen( path, "rb" );
	if( !f || !initialize_size( 3 ) )
		return false;
	
	bool result = true;
	result &= read_dump_plane( f, 0 );
	result &= read_dump_plane( f, 1 );
	result &= read_dump_plane( f, 2 );
	
	fclose( f );
	return result;
}

static int read_chunk_callback_thing( png_structp ptr, png_unknown_chunkp chunk ){
	return 0;
}
bool ImageEx::from_png( const char* path ){
	FILE *f = fopen( path, "rb" );
	if( !f )
		return false;
	
	//Read signature
	unsigned char header[8];
	fread( &header, 1, 8, f );
	
	//Check signature
	if( png_sig_cmp( header, 0, 8 ) ){
		fclose( f );
		return false;
	}
	
	//Start initializing libpng
	png_structp png_ptr = png_create_read_struct( PNG_LIBPNG_VER_STRING, NULL, NULL, NULL );
	if( !png_ptr ){
		png_destroy_read_struct( &png_ptr, NULL, NULL );
		fclose( f );
		return false;
	}
	
	png_infop info_ptr = png_create_info_struct( png_ptr );
	if( !info_ptr ){
		png_destroy_read_struct( &png_ptr, NULL, NULL );
		fclose( f );
		return false;
	}
	
	png_infop end_info = png_create_info_struct( png_ptr );
	if( !end_info ){
		png_destroy_read_struct( &png_ptr, &info_ptr, NULL );
		fclose( f );
		return false;
	}
	
	png_init_io( png_ptr, f );
	png_set_sig_bytes( png_ptr, 8 );
	png_set_read_user_chunk_fn( png_ptr, NULL, read_chunk_callback_thing );
	
	//Finally start reading
	png_read_png( png_ptr, info_ptr, PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_EXPAND, NULL );
	png_bytep *row_pointers = png_get_rows( png_ptr, info_ptr );
	
	unsigned height = png_get_image_height( png_ptr, info_ptr );
	unsigned width = png_get_image_width( png_ptr, info_ptr );
	
	initialize_size( 3 );
	
	planes[0] = new Plane( width, height );
	planes[1] = new Plane( width, height );
	planes[2] = new Plane( width, height );
	
	if( !planes[0] || !planes[1] || !planes[2] )
		return false;
	
	for( unsigned iy=0; iy<height; iy++ ){
		color_type* r = planes[0]->scan_line( iy );
		color_type* g = planes[1]->scan_line( iy );
		color_type* b = planes[2]->scan_line( iy );
		for( unsigned ix=0; ix<width; ix++ ){
			r[ix] = row_pointers[iy][ix*3 + 0] * 256;
			g[ix] = row_pointers[iy][ix*3 + 1] * 256;
			b[ix] = row_pointers[iy][ix*3 + 2] * 256;
		}
	}
	
	return true;
	
	//TODO: cleanup libpng
}


bool ImageEx::read_file( const char* path ){
	if( !from_png( path ) )
		return from_dump( path );
	return true;
}

image* ImageEx::to_image(){
	if( !planes || !planes[0] )
		return NULL;
	
	image* img = new image( planes[0]->get_width(), planes[0]->get_height() );
	if( !img )
		return NULL;
	
	for( int iy=0; iy<img->get_height(); iy++ ){
		color* row = img->scan_line( iy );
		color_type* org_row = planes[0]->scan_line( iy );
		for( int ix=0; ix<img->get_width(); ix++ ){
			color_type val = org_row[ix];
			row[ix] = color( val, val, val );
		}
	}
	
	return img;
}