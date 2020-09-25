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
using namespace Overmix;


bool ImageEx::from_qimage( QIODevice& dev, QString ext ){
	Timer t( "from_qimage" );
	QImage img;
	if( !img.load( &dev, ext.toLocal8Bit().constData() ) )
		return false;
	
	Point<unsigned> size( img.size() );
	if( img.hasAlphaChannel() )
		alpha = Plane( size );
	
#if QT_VERSION >= 0x050500
	if( img.format() == QImage::Format_Grayscale8 ){
		color_space = { Transform::GRAY, Transfer::SRGB };
		planes.emplace_back( size );
		
		for( unsigned iy=0; iy<size.height(); ++iy ){
			auto out = planes[0].p.scan_line( iy ).begin();
			auto in = img.constScanLine( iy );
			
			for( unsigned ix=0; ix<size.width(); ++ix )
				out[ix] = color::from8bit( in[ix] );
		}
	}
	else{
#endif
		color_space = { Transform::RGB, Transfer::SRGB };
		img = img.convertToFormat( QImage::Format_ARGB32 );
		
		for( int i=0; i<3; i++ )
			planes.emplace_back( size );
		
		for( unsigned iy=0; iy<size.height(); ++iy ){
			auto r = planes[0].p.scan_line( iy ).begin();
			auto g = planes[1].p.scan_line( iy ).begin();
			auto b = planes[2].p.scan_line( iy ).begin();
			auto a = alpha ? alpha.scan_line( iy ).begin() : nullptr;
			
			auto in = (const QRgb*)img.constScanLine( iy );
			
			for( unsigned ix=0; ix<size.width(); ++ix, ++in ){
				*(r++) = color::from8bit( qRed( *in ) );
				*(g++) = color::from8bit( qGreen( *in ) );
				*(b++) = color::from8bit( qBlue( *in ) );
				if( a )
					*(a++) = color::from8bit( qAlpha( *in ) );
			}
		}
#if QT_VERSION >= 0x050500
	}
#endif
	
	return true;
}


QImage ImageEx::to_qimage( bool use_dither ) const{
	Timer t( "to_qimage" );
	if( planes.size() == 0 || !planes[0].p )
		return QImage();
	
	auto to8bit = [use_dither]( Plane p )
		{ return use_dither ? p.to8BitDither() : p.to8Bit(); };
	
	if( color_space.isGray() && !alpha_plane() )
	{
		QImage img( get_width(), get_height(), QImage::Format_Grayscale8 );
		
		auto p = to8bit( planes[0].p );
		#pragma omp parallel for
		for( unsigned iy=0; iy<get_height(); iy++ ){
			auto out = (uint8_t*)img.scanLine( iy );
			auto line = p.scan_line( iy );
			
			for( unsigned ix=0; ix<get_width(); ix++ )
				out[ix] = line[ix];
		}
		
		return img;
	}
	
	//Convert colors
	//TODO: Support grayscale output?
	ImageEx rgb = toColorSpace( { Transform::RGB, Transfer::SRGB } );
	
	//Convert to 8-bit range
	std::vector<PlaneBase<uint8_t>> planar( 4 );
	#pragma omp parallel for
	for( unsigned c=0; c<rgb.planes.size(); c++ )
		planar[c] = to8bit( rgb.planes[c].p );
	//Convert or add default alpha plane
	if( rgb.alpha_plane() )
		planar[3] = rgb.alpha_plane().to8Bit(); //NOTE: Dither on alpha is probably not a good idea
	else{
		planar[3] = PlaneBase<uint8_t>{ getSize() };
		planar[3].fill( 255 );
	}
	
	
	//Create image
	auto qimg_format = rgb.alpha_plane() ? QImage::Format_ARGB32 : QImage::Format_RGB32;
	QImage img(	get_width(), get_height(), qimg_format );
	
	#pragma omp parallel for
	for( unsigned iy=0; iy<get_height(); iy++ ){
		auto out = (QRgb*)img.scanLine( iy );
		auto r = planar[0].scan_line( iy );
		auto g = planar[1].scan_line( iy );
		auto b = planar[2].scan_line( iy );
		auto a = planar[3].scan_line( iy ); //TODO: We can just use an empty row for this if no alpha
		
		for( unsigned ix=0; ix<get_width(); ix++ )
			out[ix] = qRgba( r[ix], g[ix], b[ix], a[ix] );
	}
	
	return img;
}



static QRgb setQRgbAlpha( QRgb in, int alpha )
	{ return qRgba( qRed(in), qGreen(in), qBlue(in), alpha ); }

QImage Overmix::setQImageAlpha( QImage img, const Plane& alpha ){
	if( !alpha )
		return img;
	
	assert( img.width() == (int)alpha.get_width() );
	assert( img.height() == (int)alpha.get_height() );
	
	img = img.convertToFormat( QImage::Format_ARGB32 );
	for( int iy=0; iy<img.height(); ++iy ){
		auto out = (QRgb*)img.scanLine( iy );
		auto in  = alpha.scan_line( iy );
		for( int ix=0; ix<img.width(); ++ix )
			out[ix] = setQRgbAlpha( out[ix], color::as8bit( in[ix] ) );
	}
	
	return img;
}

