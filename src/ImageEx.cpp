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
	
	planes[0] = new Plane( width, height );
	planes[1] = new Plane( width, height );
	planes[2] = new Plane( width, height );
	infos[0] = new PlaneInfo( *planes[0], 0,0, 1 );
	infos[1] = new PlaneInfo( *planes[1], 0,0, 1 );
	infos[2] = new PlaneInfo( *planes[2], 0,0, 1 );
	
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

bool ImageEx::create( unsigned width, unsigned height ){
	if( initialized )
		return false;
	
	for( int i=0; i<4; i++ ){
		if( !( planes[i] = new Plane( width, height ) ) )
			return false;
		if( !( infos[i] = new PlaneInfo( *planes[i], 0,0,1 ) ) )
			return false;
	}
	
	return initialized = true;
}

QImage ImageEx::to_qimage( bool dither ){
	if( !planes || !planes[0] )
		return QImage();
	
	//Create iterator
	std::vector<PlaneItInfo> info;
	info.push_back( PlaneItInfo( planes[0], 0,0 ) );
	info.push_back( PlaneItInfo( planes[1], 0,0 ) );
	info.push_back( PlaneItInfo( planes[2], 0,0 ) );
	if( alpha_plane() )
		info.push_back( PlaneItInfo( alpha_plane(), 0,0 ) );
	MultiPlaneIterator it( info );
	it.iterate_all();
	
	//Fetch with alpha
	color (MultiPlaneIterator::*pixel)() const = ( alpha_plane() )
		?	&MultiPlaneIterator::pixel : &MultiPlaneIterator::pixel_alpha;
	
	
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
			if( type == YUV )
				p = p.rec709_to_rgb();
			
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
	
	//Prepare iterator
	std::vector<PlaneItInfo> info;
	info.push_back( PlaneItInfo( planes[0], 0,0 ) );
	info.push_back( PlaneItInfo( &img[0].p, x,y ) );
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
		avg = it.for_all_lines_combine<Average>(
				[](MultiPlaneLineIterator &it) -> Average{
					Average avg( 0, 0 );
					
					//if( it[2] > 127*256 ){
						avg.first += it.diff( 0, 1 );
						avg.second++;
					//}
					
					return avg;
				}
			,	avg, sum
			);
	}
	else if( info.size() == 4 ){
		avg = it.for_all_lines_combine<Average>(
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
		avg = it.for_all_lines_combine<Average>(
				[](MultiPlaneLineIterator &it) -> Average{
					return Average( it.diff( 0, 1 ), 1 );
				}
			,	avg, sum
			);
	}
	
	return avg.second ? (double)avg.first / avg.second : DOUBLE_MAX;
}



struct img_comp{
	ImageEx* img1;
	ImageEx* img2;
	int h_middle;
	int v_middle;
	double diff;
	int level;
	int left;
	int right;
	int top;
	int bottom;
	
	img_comp( ImageEx& image1, ImageEx& image2, int hm, int vm, int lvl=0, int l=0, int r=0, int t=0, int b=0 ){
		img1 = &image1;
		img2 = &image2;
		h_middle = hm;
		v_middle = vm;
		diff = -1;
		level = lvl;
		left = l;
		right = r;
		top = t;
		bottom = b;
	}
	void do_diff( int x, int y ){
		if( diff < 0 )
			diff = img1->diff( *img2, x, y );
	}
	
	MergeResult result() const{
		if( level > 0 )
			return img1->best_round_sub( *img2, level, left, right, h_middle, top, bottom, v_middle, diff );
		else
			return MergeResult(QPoint( h_middle, v_middle ),diff);
	}
};

static void do_diff_center( img_comp& comp ){
	comp.do_diff( comp.h_middle, comp.v_middle );
}

MergeResult ImageEx::best_round( ImageEx& img, int level, double range_x, double range_y ){
	//Bail if invalid settings
	if(	level < 1
		||	( range_x < 0.0 || range_x > 1.0 )
		||	( range_y < 0.0 || range_y > 1.0 )
		||	!is_valid()
		||	!img.is_valid()
		)
		return MergeResult(QPoint(),DOUBLE_MAX);
	
	//Starting point is the one where both images are centered on each other
	int x = ( (int)get_width() - img.get_width() ) / 2;
	int y = ( (int)get_height() - img.get_height() ) / 2;
	
	return best_round_sub(
			img, level
		,	((int)1 - (int)img.get_width()) * range_x, ((int)get_width() - 1) * range_x, x
		,	((int)1 - (int)img.get_height()) * range_y, ((int)get_height() - 1) * range_y, y
		,	diff( img, x,y )
		);
}

MergeResult ImageEx::best_round_sub( ImageEx& img, int level, int left, int right, int h_middle, int top, int bottom, int v_middle, double diff ){
//	qDebug( "Round %d: %d,%d,%d x %d,%d,%d at %.2f", level, left, h_middle, right, top, v_middle, bottom, diff );
	QList<img_comp> comps;
	int amount = level*2 + 2;
	double h_offset = (double)(right - left) / amount;
	double v_offset = (double)(bottom - top) / amount;
	level = level > 1 ? level-1 : 1;
	
	if( h_offset < 1 && v_offset < 1 ){
		//Handle trivial step
		//Check every diff in the remaining area
		for( int ix=left; ix<=right; ix++ )
			for( int iy=top; iy<=bottom; iy++ ){
				img_comp t( *this, img, ix, iy );
				if( ix == h_middle && iy == v_middle )
					t.diff = diff;
				comps << t;
			}
	}
	else{
		//Make sure we will not do the same position multiple times
		double h_add = ( h_offset < 1 ) ? 1 : h_offset;
		double v_add = ( v_offset < 1 ) ? 1 : v_offset;
		
		for( double iy=top+v_offset; iy<=bottom; iy+=v_add )
			for( double ix=left+h_offset; ix<=right; ix+=h_add ){
				int x = ( ix < 0.0 ) ? ceil( ix-0.5 ) : floor( ix+0.5 );
				int y = ( iy < 0.0 ) ? ceil( iy-0.5 ) : floor( iy+0.5 );
				
				//Avoid right-most case. Can't be done in the loop
				//as we always want it to run at least once.
				if( ( x == right && x != left ) || ( y == bottom && y != top ) )
					continue;
				
				//Create and add
				img_comp t(
						*this, img, x, y, level
					,	floor( ix - h_offset ), ceil( ix + h_offset )
					,	floor( iy - v_offset ), ceil( iy + v_offset )
					);
				
				if( x == h_middle && y == v_middle )
					t.diff = diff; //Reuse old diff
				
				comps << t;
			}
	}
	
	//Calculate diffs
	QtConcurrent::map( comps, do_diff_center ).waitForFinished();
	
	//Find best comp
	const img_comp* best = NULL;
	double best_diff = DOUBLE_MAX;
	
	for( int i=0; i<comps.size(); i++ ){
		if( comps.at(i).diff < best_diff ){
			best = &comps.at(i);
			best_diff = best->diff;
		}
	}
	
	if( !best ){
		qDebug( "ERROR! no result to continue on!!" );
		return MergeResult(QPoint(),DOUBLE_MAX);
	}
	
	return best->result();
}



