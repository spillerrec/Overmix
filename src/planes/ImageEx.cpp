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
#include "dump/DumpPlane.hpp"
#include "../utils/PlaneUtils.hpp"

#include <QtConcurrentMap>
#include <QFileInfo>

#include <cassert>
#include <stdint.h>
#include <limits>
#include <vector>
#include <png.h>

using namespace std;

static const double DOUBLE_MAX = std::numeric_limits<double>::max();

void ImageEx::to_grayscale(){
	switch( type ){
		case GRAY: break;
		case RGB:
				//TODO: properly convert to grayscale
				while( planes.size() > 1 )
					planes.pop_back();
			break;
		case YUV:
				while( planes.size() > 1 )
					planes.pop_back();
			break;
	}
	type = GRAY;
}


static ImageEx yuvToRgb( const ImageEx& img ){
	//TODO: assert YUV
	ImageEx out( ImageEx::RGB );
	out.addPlane( Plane( ScaledPlane( img[0], img.getSize() )() ) );
	out.addPlane( Plane( ScaledPlane( img[1], img.getSize() )() ) );
	out.addPlane( Plane( ScaledPlane( img[2], img.getSize() )() ) );
	
	for( unsigned iy=0; iy<img.get_height(); iy++ ){
		ColorRow row( out, iy );
		for( unsigned ix=0; ix<img.get_width(); ix++ )
			row.set( ix, row[ix].rec709ToRgb() );
	}
	
	return out;
}

ImageEx ImageEx::toRgb() const{
	switch( type ){
		case GRAY:{
				ImageEx out( RGB );
				out.addPlane( Plane{ planes[0] } );
				out.addPlane( Plane{ planes[0] } );
				out.addPlane( Plane{ planes[0] } );
				return out;
			}
		
		case RGB: return *this;
		case YUV: return yuvToRgb( *this );
		default: return { }; //TODO: throw
	}
}

template<typename T> void copyLine( color_type* out, const T* in, unsigned width, double scale ){
	for( unsigned ix=0; ix<width; ++ix )
		out[ix] = color::fromDouble( in[ix] / scale );
}

void process_dump_line( color_type *out, const uint8_t* in, unsigned width, uint16_t depth ){
	double scale = pow( 2.0, depth ) - 1.0;
	
	if( depth <= 8 )
		copyLine( out, in, width, scale );
	else
		copyLine( out, reinterpret_cast<const uint16_t*>(in), width, scale );
}

bool ImageEx::read_dump_plane( QIODevice &dev ){
	DumpPlane dump_plane;
	if( !dump_plane.readHeader( dev ) )
		return false;
	
	planes.emplace_back( Size<unsigned>{ dump_plane.getWidth(), dump_plane.getHeight() } );
	//TODO: add assertion that width == line_width !!
	dump_plane.readData( dev, (uint16_t*)planes.back().scan_line(0), 14 ); //TODO: constant with bit depth
	
	
	//Convert data
//	for( auto&& row : planes.back() )
//		process_dump_line( row.line(), dump_plane.constScanline( row.y() ), row.width(), dump_plane.getDepth() );
	
	return true;
}

bool ImageEx::from_dump( QIODevice& dev ){
	planes.reserve( 3 );
	while( read_dump_plane( dev ) ); //Load all planes
//	planes.reserve( 1 ); //For benchmarking other stuff
//	read_dump_plane( dev );
	
	//Use last plane as Alpha
	auto amount = planes.size();
	if( amount == 2 || amount == 4 ){
		alpha = std::move( planes.back() );
		planes.pop_back();
		amount--;
	}
	
	//Find type and validate
	type = (amount == 1) ? GRAY : YUV;
	return amount < 4;
}

bool ImageEx::from_dump( QString path ){
	QFile f( path );
	if( !f.open( QIODevice::ReadOnly ) )
		return false;
	
	return from_dump( f );
}

static DumpPlane toDumpPlane( const Plane& plane, unsigned depth ){
	bool multi_byte = depth > 8;
	auto power = std::pow( 2, depth ) - 1;
	vector<uint8_t> data;
	data.reserve( plane.get_width() * plane.get_height() * (multi_byte?2:1) );
	
	for( auto row : plane )
		for( auto pixel : row ){
			if( multi_byte ){
				uint16_t val = color::asDouble( pixel ) * power;
				data.push_back( val & 0x00FF );
				data.push_back( (val & 0xFF00) >> 8 );
			}
			else
				data.push_back( color::asDouble( pixel ) * power );
	}
	
	return DumpPlane( plane.get_width(), plane.get_height(), depth, data );
}

bool ImageEx::saveDump( QIODevice& dev, unsigned depth, bool compression ) const{
	auto method = compression ? DumpPlane::LZMA : DumpPlane::NONE;
	for( auto& plane : planes )
		if( !toDumpPlane( plane, depth ).write( dev, method ) )
			return false;
	
	if( alpha )
		if( !toDumpPlane( alpha, depth ).write( dev, method ) )
			return false;
	
	return true;
}

bool ImageEx::saveDump( QString path, unsigned depth ) const{
	QFile f( path );
	if( !f.open( QIODevice::WriteOnly ) )
		return false;
	
	return saveDump( f, depth, true );
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
	png_read_png( png_ptr, info_ptr, PNG_TRANSFORM_STRIP_16  | PNG_TRANSFORM_STRIP_ALPHA | PNG_TRANSFORM_EXPAND, NULL );
	png_bytep *row_pointers = png_get_rows( png_ptr, info_ptr );
	
	unsigned height = png_get_image_height( png_ptr, info_ptr );
	unsigned width = png_get_image_width( png_ptr, info_ptr );
	
	for( int i=0; i<3; i++ )
		planes.emplace_back( width, height );
	
	for( unsigned iy=0; iy<height; iy++ ){
		color_type* r = planes[0].scan_line( iy );
		color_type* g = planes[1].scan_line( iy );
		color_type* b = planes[2].scan_line( iy );
		for( unsigned ix=0; ix<width; ix++ ){
			r[ix] = color::from8bit( row_pointers[iy][ix*3 + 0] );
			g[ix] = color::from8bit( row_pointers[iy][ix*3 + 1] );
			b[ix] = color::from8bit( row_pointers[iy][ix*3 + 2] );
		}
	}
	
	type = RGB;
	
	return true;
	
	//TODO: cleanup libpng
}


bool ImageEx::from_qimage( QString path ){
	QImage img( path );
	if( img.isNull() )
		return false;
	
	type = RGB;
	Point<unsigned> size( img.size() );
	
	if( img.hasAlphaChannel() )
		alpha = Plane( size );
	img = img.convertToFormat( QImage::Format_ARGB32 );
	
	for( int i=0; i<3; i++ )
		planes.emplace_back( size );
	
	for( unsigned iy=0; iy<size.height(); ++iy ){
		auto r = planes[0].scan_line( iy );
		auto g = planes[1].scan_line( iy );
		auto b = planes[2].scan_line( iy );
		auto a = alpha ? alpha.scan_line( iy ) : nullptr;
		
		auto in = (const QRgb*)img.constScanLine( iy );
		
		for( unsigned ix=0; ix<size.width(); ++ix, ++in ){
			*(r++) = color::from8bit( qRed( *in ) );
			*(g++) = color::from8bit( qGreen( *in ) );
			*(b++) = color::from8bit( qBlue( *in ) );
			if( a )
				*(a++) = color::from8bit( qAlpha( *in ) );
		}
	}
	
	return true;
}

bool ImageEx::read_file( QString path ){
	QString ext = QFileInfo( path ).suffix();
	if( ext == "dump" )
		return from_dump( path );
	//TODO: actually make png loading useful, by providing 16-bit loading, and fix const char* interface
//	if( ext == "png" )
//		return from_png( path );
	
	return from_qimage( path );
}

struct PlanesIt{
	std::vector<ScaledPlane> planes;
	std::vector<const color_type*> rows;
	
	public:
		void add( const Plane& p, Size<int> size )
			{ planes.emplace_back( p, size ); }
		void next_x(){ for( auto& row : rows ) row++; }
		void prepare_row( int iy ){
			rows.clear();
			for( auto& plane : planes )
				rows.push_back( plane().const_scan_line( iy ) );
		}
		
		auto at( int c ) const{ return *(rows[c]); }
		
		color gray(){   return { at(0), at(0), at(0) }; }
		color gray_a(){ return { at(0), at(0), at(0), at(1) }; }
		color rgb(){    return { at(0), at(1), at(2) }; }
		color rgb_a(){  return { at(0), at(1), at(2), at(3) }; }
};

QImage ImageEx::to_qimage( YuvSystem system, unsigned setting ) const{
	if( planes.size() == 0 || !planes[0] )
		return QImage();
	
	//Settings
	bool dither = setting & SETTING_DITHER;
	bool gamma = setting & SETTING_GAMMA;
	bool is_yuv = (type == YUV) && (system != SYSTEM_KEEP);
	
	//Create iterator
	auto img_size = getSize();
	PlanesIt it;
	for( auto& p : planes )
		it.add( p, img_size );
	if( alpha_plane() )
		it.add( alpha_plane(), img_size );
	
	//Fetch with alpha
	auto pixel = ( type == GRAY )
		?	( alpha_plane() ? &PlanesIt::gray_a : &PlanesIt::gray )
		:	( alpha_plane() ? &PlanesIt::rgb_a  : &PlanesIt::rgb  );
	
	
	//Create image
	QImage img(	img_size.width(), img_size.height()
		,	( alpha_plane() ) ? QImage::Format_ARGB32 : QImage::Format_RGB32
		);
	img.fill(0);
	
	
	vector<color> line( img_size.width()+1, color( 0,0,0,0 ) );
	for( unsigned iy=0; iy<img_size.height(); iy++ ){
		QRgb* row = (QRgb*)img.scanLine( iy );
		it.prepare_row( iy );
		for( unsigned ix=0; ix<img_size.width(); ix++, it.next_x() ){
			color p = (it.*pixel)();
			if( is_yuv ){
				if( system == SYSTEM_REC709 )
					p = p.rec709ToRgb( gamma );
				else
					p = p.rec601ToRgb( gamma );
			}
			
			if( dither )
				p += line[ix];
			
			const double scale = color::WHITE / 255.0;
			color rounded = p / scale;
			
			if( dither ){
				color err = p - ( rounded * scale );
				line[ix] = err / 4;
				line[ix+1] += err / 2;
				if( ix )
					line[ix-1] += err / 4;
			}
			
			rounded.trunc( 255 );
			row[ix] = qRgba( rounded.r, rounded.g, rounded.b, rounded.a );
		}
	}
	
	return img;
}


double ImageEx::diff( const ImageEx& img, int x, int y ) const{
	//Check if valid
	if( !is_valid() || !img.is_valid() )
		return DOUBLE_MAX;
	
	return planes[0].diff( img[0], x, y );
}

bool ImageEx::is_interlaced() const{
	return planes[0].is_interlaced();
}
void ImageEx::replace_line( ImageEx& img, bool top ){
	for( unsigned i=0; i<max(size(), img.size()); ++i )
		planes[i].replace_line( img[i], top );
	if( alpha && img.alpha_plane() )
		alpha.replace_line( img.alpha_plane(), top );
}
void ImageEx::combine_line( ImageEx& img, bool top ){
	for( unsigned i=0; i<max(size(), img.size()); ++i )
		planes[i].combine_line( img[i], top );
	if( alpha && img.alpha_plane() )
		alpha.combine_line( img.alpha_plane(), top );
}

Point<unsigned> ImageEx::crop( unsigned left, unsigned top, unsigned right, unsigned bottom ){
	Point<unsigned> pos( left, top );
	Size<unsigned> decrease( right + left, top + bottom );
	
	if( type == YUV ){
		planes[0].crop( pos*2, planes[0].getSize() - decrease*2 );
		planes[1].crop( pos,   planes[1].getSize() - decrease   );
		planes[2].crop( pos,   planes[2].getSize() - decrease   );
		if( alpha )
			alpha.crop( pos*2, alpha.getSize() - decrease*2 );
		return pos*2;
	}
	else{
		for( auto& plane : planes )
			plane.crop( pos, plane.getSize() - decrease );
		if( alpha )
			alpha.crop( pos, alpha.getSize() - decrease );
		return pos;
	}
};

void ImageEx::crop( Point<unsigned> offset, Size<unsigned> size ){
	auto real_size = getSize();
	for( auto& plane : planes ){
		auto scale = plane.getSize().to<double>() / real_size.to<double>();
		plane.crop( offset*scale, size*scale );
	}
	if( alpha )
		alpha.crop( offset, size );
}

Rectangle<unsigned> ImageEx::getCrop() const{
	if( planes.size() == 0 )
		return { {0,0}, {0,0} };
	
	auto& cropped = planes[ (type == YUV) ? 1 : 0 ];
	return { cropped.getOffset(), cropped.getRealSize() - cropped.getSize() - cropped.getOffset() };
}

MergeResult ImageEx::best_round( const ImageEx& img, int level, double range_x, double range_y, DiffCache *cache ) const{
	//Bail if invalid settings
	if(	level < 1
		||	( range_x < 0.0 || range_x > 1.0 )
		||	( range_y < 0.0 || range_y > 1.0 )
		||	!is_valid()
		||	!img.is_valid()
		)
		return MergeResult({0,0}, DOUBLE_MAX);
	
	//Make sure cache exists
	DiffCache temp;
	if( !cache )
		cache = &temp;
	
	return planes[0].best_round_sub(
			img[0], alpha_plane(), img.alpha_plane(), level
		,	((int)1 - (int)img.get_width()) * range_x, ((int)get_width() - 1) * range_x
		,	((int)1 - (int)img.get_height()) * range_y, ((int)get_height() - 1) * range_y
		,	cache, true
		);
}

static QRgb setQRgbAlpha( QRgb in, int alpha )
	{ return qRgba( qRed(in), qGreen(in), qBlue(in), alpha ); }

QImage setQImageAlpha( QImage img, const Plane& alpha ){
	if( !alpha )
		return img;
	
	assert( img.width() == (int)alpha.get_width() );
	assert( img.height() == (int)alpha.get_height() );
	
	img = img.convertToFormat( QImage::Format_ARGB32 );
	for( int iy=0; iy<img.height(); ++iy ){
		auto out = (QRgb*)img.scanLine( iy );
		auto in = alpha.const_scan_line( iy );
		for( int ix=0; ix<img.width(); ++ix )
			out[ix] = setQRgbAlpha( out[ix], color::as8bit( in[ix] ) );
	}
	
	return img;
}

ImageEx deVlcImage( const ImageEx& img ){
	if( img.get_system() != ImageEx::RGB )
		return {};
	
	ImageEx out( ImageEx::YUV );
	for( int i=0; i<3; i++ )
		out.addPlane( Plane( img[i] ) );
	
	for( unsigned iy=0; iy<img.get_height(); iy++ ){
		ColorRow row( out, iy );
		for( unsigned ix=0; ix<img.get_width(); ix++ )
			row.set( ix, row[ix].rgbToYuv( 0.299, 0.587, 0.114, false ) );
	}
	
	//Downscale chroma
	for( int c=1; c<3; c++ ){
		Plane downscaled( out[c].getSize() / 2 );
		
		for( unsigned iy=0; iy<downscaled.get_height(); iy++ ){
			auto row_in1 = out[c].const_scan_line( iy*2   );
			auto row_in2 = out[c].const_scan_line( iy*2+1 );
			auto row_out = downscaled.  scan_line( iy     );
			for( unsigned ix=0; ix<downscaled.get_width(); ix++ )
				row_out[ix] =
					(	row_in1[ix*2+0]
					+	row_in1[ix*2+1]
					+	row_in2[ix*2+0]
					+	row_in2[ix*2+1]
					) / 4;
		}
		
		out[c] = downscaled;
	}
	
	//TODO: check consistency?
	
	return out;
}

