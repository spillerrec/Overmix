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
#include "MultiPlaneIterator.hpp"
#include "color.hpp"

#include <limits>
#include <png.h>
#include <QtConcurrentMap>


static const double DOUBLE_MAX = std::numeric_limits<double>::max();

bool ImageEx::read_dump_plane( FILE *f, unsigned index ){
	if( !f )
		return false;
	
	unsigned width, height;
	fread( &width, sizeof(unsigned), 1, f );
	fread( &height, sizeof(unsigned), 1, f );
	
	Plane* p = planes[index] = new Plane( width, height );
	if( !p )
		return false;
	
	unsigned depth;
	fread( &depth, sizeof(unsigned), 1, f );
	unsigned byte_count = (depth + 7) / 8;
	
	unsigned char* buffer = new unsigned char[ width*byte_count ];
	
	
	for( unsigned iy=0; iy<height; iy++ ){
		color_type *row = p->scan_line( iy );
		fread( buffer, byte_count, width, f );
		if( byte_count == 1 )
			for( unsigned ix=0; ix<width; ++ix, ++row )
				*row = (color_type)(buffer[ix]) << (16 - depth);
		else
			for( unsigned ix=0; ix<width; ++ix, ++row )
				*row = (color_type)((short int*)buffer)[ix] << (16 - depth);
	}
	
	delete[] buffer;
	return true;
}

bool ImageEx::from_dump( const char* path ){
	FILE *f = fopen( path, "rb" );
	if( !f )
		return false;
	
	bool result = true;
	result &= read_dump_plane( f, 0 );
	result &= read_dump_plane( f, 1 );
	result &= read_dump_plane( f, 2 );
	
	fclose( f );
	
	type = YUV;
	
	return result;
}

static int read_chunk_callback_thing( png_structp, png_unknown_chunkp ){
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
	
	type = RGB;
	
	return true;
	
	//TODO: cleanup libpng
}


bool ImageEx::read_file( const char* path ){
	if( !from_png( path ) )
		return initialized = from_dump( path );
	return initialized = true;
}

bool ImageEx::create( unsigned width, unsigned height, bool alpha ){
	if( initialized )
		return false;
	
	unsigned amount;
	switch( type ){
		case GRAY: amount = 1; break;
		case RGB:
		case YUV: amount = 3; break;
		
		default: return false;
	}
	
	for( unsigned i=0; i<amount; i++ )
		if( !( planes[i] = new Plane( width, height ) ) )
			return false;
	
	if( alpha )
		if( !( planes[MAX_PLANES-1] = new Plane( width, height ) ) )
			return false;
	
	return initialized = true;
}

QImage ImageEx::to_qimage( YuvSystem system, unsigned setting ){
	if( !planes || !planes[0] )
		return QImage();
	
	//Settings
	bool dither = setting & SETTING_DITHER;
	bool gamma = setting & SETTING_GAMMA;
	bool is_yuv = (type == YUV) && (system != SYSTEM_KEEP);
	
	//Create iterator
	std::vector<PlaneItInfo> info;
	info.push_back( PlaneItInfo( planes[0], 0,0 ) );
	if( type != GRAY ){
		info.push_back( PlaneItInfo( planes[1], 0,0 ) );
		info.push_back( PlaneItInfo( planes[2], 0,0 ) );
	}
	if( alpha_plane() )
		info.push_back( PlaneItInfo( alpha_plane(), 0,0 ) );
	MultiPlaneIterator it( info );
	it.iterate_all();
	
	//Fetch with alpha
	color (MultiPlaneIterator::*pixel)() const = NULL;
	
	if( type == GRAY )
		pixel = ( alpha_plane() ) ? &MultiPlaneIterator::gray_alpha : &MultiPlaneIterator::gray;
	else
		pixel = ( alpha_plane() ) ? &MultiPlaneIterator::pixel_alpha : &MultiPlaneIterator::pixel;
	
	
	color *line = new color[ get_width()+1 ];
	
	//Create image
	QImage img(	it.width(), it.height()
		,	( alpha_plane() ) ? QImage::Format_ARGB32 : QImage::Format_RGB32
		);
	img.fill(0);
	
	//TODO: select function
	
	for( unsigned iy=0; iy<it.height(); iy++, it.next_line() ){
		QRgb* row = (QRgb*)img.scanLine( iy );
		for( unsigned ix=0; ix<it.width(); ix++, it.next_x() ){
			color p = (it.*pixel)();
			if( is_yuv ){
				if( system == SYSTEM_REC709 )
					p = p.rec709_to_rgb( gamma );
				else
					p = p.rec601_to_rgb( gamma );
			}
			
			if( dither )
				p += line[ix];
			
			color rounded = (p) / 256;
			
			if( dither ){
				color err = p - ( rounded * 256 );
				line[ix] = err / 4;
				line[ix+1] += err / 2;
				if( ix )
					line[ix-1] += err / 4;
			}
			
			//rounded.trunc( 255 );
			row[ix] = qRgba( rounded.r, rounded.g, rounded.b, rounded.a );
		}
	}
	
	delete[] line;
	
	return img;
}


double ImageEx::diff( const ImageEx& img, int x, int y ) const{
	//Check if valid
	if( !is_valid() || !img.is_valid() )
		return DOUBLE_MAX;
	
	return planes[0]->diff( *(img[0]), x, y );
	
	//Prepare iterator
	std::vector<PlaneItInfo> info;
	info.push_back( PlaneItInfo( planes[0], 0,0 ) );
	info.push_back( PlaneItInfo( img[0], x,y ) );
	if( alpha_plane() )
		info.push_back( PlaneItInfo( alpha_plane(), 0,0 ) );
	if( img.alpha_plane() )
		info.push_back( PlaneItInfo( img.alpha_plane(), x,y ) );
	
	MultiPlaneIterator it( info );
	it.iterate_shared();
	
	typedef std::pair<unsigned long long,unsigned> Average;
	Average avg( 0, 0 );
	
	auto sum = [](Average w1, Average w2){
			w1.first += w2.first;
			w1.second += w2.second;
			return w1;
		};
	
	
	if( info.size() == 3 ){
		/*
		avg = it.for_all_pixels_combine<Average>(
				[](MultiPlaneLineIterator &it) -> Average{
					Average avg( 0, 1 );
					
					//if( it[2] > 127*256 ){
						avg.first += abs( (int)it[0] - it[1] );//it.diff( 0, 1 );
					//	avg.second++;
					//}
					
					return avg;
				}
			,	avg, sum
			);
		/*/
		avg = it.for_all_lines_combine<Average>(
				[](MultiPlaneLineIterator &it) -> Average{
					Average avg( 0, it.left() + 1 );
					
					color_type *i1 = &it[0];
					color_type *i2 = &it[1];
				//	color_type *i3 = &it[2];
					avg.first += abs( (int_fast16_t)(*i1 - *i2) );
					while( it.valid() ){
						i1++; i2++;// i3++;
						avg.first += abs( (int_fast16_t)(*i1 - *i2) );
					}
					
					return avg;
				}
			,	avg, sum
			);
		//*/
	}
	else if( info.size() == 4 ){
		avg = it.for_all_pixels_combine<Average>(
				[](MultiPlaneLineIterator &it) -> Average{
					Average avg( 0, 0 );
					
					if( it[2] > 127*256 && it[3] > 127*256 ){
						avg.first += it.diff( 0, 1 );
						avg.second++;
					}
					
					return avg;
				}
			,	avg, sum
			);
	}
	else{
		avg = it.for_all_pixels_combine<Average>(
				[](MultiPlaneLineIterator &it) -> Average{
					return Average( it.diff( 0, 1 ), 1 );
				}
			,	avg, sum
			);
	}
	
	return avg.second ? (double)avg.first / avg.second : DOUBLE_MAX;
}

bool ImageEx::is_interlaced() const{
	return planes[0]->is_interlaced();
}
void ImageEx::replace_line( ImageEx& img, bool top ){
	for( unsigned i=0; i<4; ++i )
		if( planes[i] ) //TODO: check if img has plane
			planes[i]->replace_line( *(img[i]), top );
}
void ImageEx::combine_line( ImageEx& img, bool top ){
	for( unsigned i=0; i<4; ++i )
		if( planes[i] ) //TODO: check if img has plane
			planes[i]->combine_line( *(img[i]), top );
}


MergeResult ImageEx::best_round( const ImageEx& img, int level, double range_x, double range_y, DiffCache *cache ) const{
	//Bail if invalid settings
	if(	level < 1
		||	( range_x < 0.0 || range_x > 1.0 )
		||	( range_y < 0.0 || range_y > 1.0 )
		||	!is_valid()
		||	!img.is_valid()
		)
		return MergeResult(QPoint(),DOUBLE_MAX);
	
	//Make sure cache exists
	DiffCache temp;
	if( !cache )
		cache = &temp;
	
	return planes[0]->best_round_sub(
			*(img[0]), level
		,	((int)1 - (int)img.get_width()) * range_x, ((int)get_width() - 1) * range_x
		,	((int)1 - (int)img.get_height()) * range_y, ((int)get_height() - 1) * range_y
		,	cache
		);
}


