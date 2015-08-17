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

#include "../debug.hpp"
#include "../utils/PlaneUtils.hpp"

#include <QFileInfo>


using namespace std;

static const double DOUBLE_MAX = std::numeric_limits<double>::max();

void ImageEx::to_grayscale(){
	Timer t( "to_grayscale" );
	switch( transform ){
		case Transform::RGB:
				//TODO: properly convert to grayscale
				while( planes.size() > 1 )
					planes.pop_back();
			break;
		case Transform::GRAY:
		case Transform::YCbCr_601:
		case Transform::YCbCr_709:
		case Transform::JPEG:
				while( planes.size() > 1 )
					planes.pop_back();
			break;
		default: ; //TODO: throw
	}
	transform = Transform::GRAY;
}


static ImageEx yuvToRgb( const ImageEx& img ){
	//TODO: assert YUV
	ImageEx out( Transform::RGB );
	out.addPlane( Plane( ScaledPlane( img[0], img.getSize() )() ) );
	out.addPlane( Plane( ScaledPlane( img[1], img.getSize() )() ) );
	out.addPlane( Plane( ScaledPlane( img[2], img.getSize() )() ) );
	
	for( unsigned iy=0; iy<img.get_height(); iy++ ){
		ColorRow row( out, iy );
		for( unsigned ix=0; ix<img.get_width(); ix++ )
			row.set( ix, row[ix].rec709ToRgb() );
		//TODO: use color info
	}
	
	return out;
}

ImageEx ImageEx::toRgb() const{
	Timer t( "toRgb" );
	switch( transform ){
		case Transform::GRAY:{
				ImageEx out( Transform::RGB );
				//TODO: replicate transfer
				out.addPlane( Plane{ planes[0].p } );
				out.addPlane( Plane{ planes[0].p } );
				out.addPlane( Plane{ planes[0].p } );
				return out;
			}
		
		case Transform::RGB: return *this;
		
		case Transform::YCbCr_601:
		case Transform::YCbCr_709:
		case Transform::JPEG:
			return yuvToRgb( *this );
		default: return { }; //TODO: throw
	}
}


bool ImageEx::read_file( QString path ){
	QFile f( path );
	if( !f.open( QIODevice::ReadOnly ) )
		return false;
	
	auto ext = QFileInfo( path ).suffix().toLower();
	if( ext == "dump" )
		return from_dump( f );
	if( ext == "png" )
		return from_png( f );
	if( ext == "jpg" || ext == "jpeg" )
		return from_jpeg( f );
	
	return from_qimage( f, ext );
}

ImageEx ImageEx::fromFile( QString path ){
	ImageEx temp;
	if( !temp.read_file( path ) )
		temp.planes.clear();
	return temp;
}

ImageEx ImageEx::flatten() const{
	Timer t( "ImageEx::flatten()" );
	//Find dimensions and create plane
	unsigned height = get_height(), width = 0;
	for( auto& info : planes )
		width += info.p.get_width();
	if( alpha_plane() )
		width += alpha_plane().get_width();
	
	Plane out( width, height );
	out.fill( color::BLACK );
	
	//Fill output-plane, one plane at a time
	unsigned x_offset = 0;
	for( auto& info : planes ){
		out.copy( info.p, {0,0}, info.p.getSize(), {x_offset,0} );
		x_offset += info.p.get_width();
	}
	if( alpha_plane() )
		out.copy( alpha_plane(), {0,0}, alpha_plane().getSize(), {x_offset,0} );
	
	return out;
}


double ImageEx::diff( const ImageEx& img, int x, int y ) const{
	//Check if valid
	if( !is_valid() || !img.is_valid() )
		return DOUBLE_MAX;
	
	return planes[0].p.diff( img[0], x, y );
}

bool ImageEx::is_interlaced() const{
	return planes[0].p.is_interlaced();
}
void ImageEx::replace_line( ImageEx& img, bool top ){
	for( unsigned i=0; i<max(size(), img.size()); ++i )
		planes[i].p.replace_line( img[i], top );
	if( alpha && img.alpha_plane() )
		alpha.replace_line( img.alpha_plane(), top );
}
void ImageEx::combine_line( ImageEx& img, bool top ){
	for( unsigned i=0; i<max(size(), img.size()); ++i )
		planes[i].p.combine_line( img[i], top );
	if( alpha && img.alpha_plane() )
		alpha.combine_line( img.alpha_plane(), top );
}

Point<unsigned> ImageEx::crop( unsigned left, unsigned top, unsigned right, unsigned bottom ){
	Point<unsigned> pos( left, top );
	Size<unsigned> decrease( right + left, top + bottom );
	
	//TODO: use subsampling
	if( transform == Transform::YCbCr_709 ){
		planes[0].p.crop( pos*2, planes[0].p.getSize() - decrease*2 );
		planes[1].p.crop( pos,   planes[1].p.getSize() - decrease   );
		planes[2].p.crop( pos,   planes[2].p.getSize() - decrease   );
		if( alpha )
			alpha.crop( pos*2, alpha.getSize() - decrease*2 );
		return pos*2;
	}
	else{
		for( auto& info : planes )
			info.p.crop( pos, info.p.getSize() - decrease );
		if( alpha )
			alpha.crop( pos, alpha.getSize() - decrease );
		return pos;
	}
};

void ImageEx::crop( Point<unsigned> offset, Size<unsigned> size ){
	auto real_size = getSize();
	for( auto& info : planes ){
		auto scale = info.p.getSize().to<double>() / real_size.to<double>();
		info.p.crop( offset*scale, size*scale );
	}
	if( alpha )
		alpha.crop( offset, size );
}

Rectangle<unsigned> ImageEx::getCrop() const{
	if( planes.size() == 0 )
		return { {0,0}, {0,0} };
	
	//TODO: fix cropping with subsampling
	auto& cropped = planes[ (transform == Transform::YCbCr_709) ? 1 : 0 ].p;
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
	
	return planes[0].p.best_round_sub(
			img[0], alpha_plane(), img.alpha_plane(), level
		,	((int)1 - (int)img.get_width()) * range_x, ((int)get_width() - 1) * range_x
		,	((int)1 - (int)img.get_height()) * range_y, ((int)get_height() - 1) * range_y
		,	cache, true
		);
}

ImageEx deVlcImage( const ImageEx& img ){
	Timer t( "deVlcImage" );
	if( img.getTransform() != Transform::RGB )
		return {};
	
	ImageEx out( Transform::YCbCr_709 );
	//TODO: transfer?
	for( int i=0; i<3; i++ )
		out.addPlane( Plane( img[i] ) );
	
	for( unsigned iy=0; iy<img.get_height(); iy++ ){
		ColorRow row( out, iy );
		for( unsigned ix=0; ix<img.get_width(); ix++ )
			row.set( ix, row[ix].rgbToYcbcr( 0.299, 0.587, 0.114, false, true ) );
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

