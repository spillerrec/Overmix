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

#include <QImage>

#include <cassert>
#include <vector>

using namespace std;


bool ImageEx::from_qimage( QIODevice& dev, QString ext ){
	Timer t( "from_qimage" );
	QImage img;
	if( !img.load( &dev, ext.toLocal8Bit().constData() ) )
		return false;
	
	transform = Transform::RGB;
	transfer  = Transfer::SRGB;
	Point<unsigned> size( img.size() );
	
	if( img.hasAlphaChannel() )
		alpha = Plane( size );
	img = img.convertToFormat( QImage::Format_ARGB32 );
	
	for( int i=0; i<3; i++ )
		planes.emplace_back( size );
	
	for( unsigned iy=0; iy<size.height(); ++iy ){
		auto r = planes[0].p.scan_line( iy );
		auto g = planes[1].p.scan_line( iy );
		auto b = planes[2].p.scan_line( iy );
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
	Timer t( "to_qimage" );
	if( planes.size() == 0 || !planes[0].p )
		return QImage();
	
	//Settings
	bool dither = setting & SETTING_DITHER;
	bool gamma = setting & SETTING_GAMMA;
	bool is_yuv = isYCbCr() && (system != SYSTEM_KEEP);
	//TODO: be smarter now we have access to the color info!
	
	//Create iterator
	auto img_size = getSize();
	PlanesIt it;
	for( auto& info : planes )
		it.add( info.p, img_size );
	if( alpha_plane() )
		it.add( alpha_plane(), img_size );
	
	//Fetch with alpha
	auto pixel = ( transform == Transform::GRAY )
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
				if( transform == Transform::YCbCr_709 )
					p = p.rec709ToRgb( gamma );
				else if( transform == Transform::YCbCr_601 )
					p = p.rec601ToRgb( gamma );
				else if( transform == Transform::JPEG )
					p = p.jpegToRgb( false );
					
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

