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
#include "../comparators/GradientPlane.hpp"
#include "basic/difference.hpp"
#include "PorterDuff.hpp"

#include <QFileInfo>
#include <stdexcept>

using namespace std;
using namespace Overmix;


class ColorRow{
	private:
		RowIt<color_type> r, g, b;
		
	public:
		ColorRow( ImageEx& img, int iy )
			:	r(img[0].scan_line(iy))
			,	g(img[1].scan_line(iy))
			,	b(img[2].scan_line(iy))
			{ }
		
		color operator[]( int i ) const
			{ return { r[i], g[i], b[i] }; }
		
		void set( int ix, color rgb ){
			r[ix] = rgb.r;
			g[ix] = rgb.g;
			b[ix] = rgb.b;
		}
};

static const double DOUBLE_MAX = std::numeric_limits<double>::max();

static Plane RgbToGrayscale(const Plane& r, const Plane& g, const Plane& b, const ColorSpace& /*color_space*/)
{
	Timer t( "RgbToGrayscale" );
	if (r.getSize() != g.getSize() || g.getSize() != b.getSize()) {
		qWarning("RgbToGrayscale failed, planes not consistent in size!");
		return {};
	}
	
	//TODO: Get those from color space
	double cr = 0.3;
	double cg = 0.59;
	double cb = 0.11;
	
	Plane out(r.getSize());
	for (unsigned y=0; y<r.get_height(); y++)
		for (unsigned x=0; x<r.get_width(); x++) 
			out[y][x] = color::truncateFullRange( r[y][x] * cr + g[y][x] * cg + b[y][x] * cb );
	
	return out;
}

void ImageEx::to_grayscale(){
	Timer t( "to_grayscale" );
	qWarning("Color space: %d", (int)color_space.transform());
	switch( color_space.transform() ){
		case Transform::RGB: {
			auto p = RgbToGrayscale(planes[0].p, planes[1].p, planes[2].p, color_space);
			planes.clear();
			addPlane( std::move(p) );
			break;
		}
		case Transform::GRAY:
		case Transform::YCbCr_601:
		case Transform::YCbCr_709:
		case Transform::JPEG:
		case Transform::BAYER: //TODO: 
				while( planes.size() > 1 )
					planes.pop_back();
			break;
		default: ; //TODO: throw
	}
	color_space = color_space.changed( Transform::GRAY );
}

ImageEx ImageEx::toRgb() const
	{ return toColorSpace( color_space.changed( Transform::RGB ) ); }


ImageEx ImageEx::toColorSpace( ColorSpace to ) const{
	Timer t( "ImageEx::toColorSpace" );
	ImageEx out( *this );
	
	//Special case for RGB to gray
	if( to.isGray() ){
		out.to_grayscale();
		return out;
	}
	
	//Special case for Bayer inputs
	if (color_space.isBayer()){
		//TODO: Do something more proper that aligns the channels!
		ImageEx copy(*this);
		std::swap(copy.planes[2], copy.planes[3]);
		copy.planes[1].p.mix(copy.planes[3].p); //Average the two green channels
		copy.planes.pop_back(); //Remove the second (non-averaged) green channel
		copy.color_space = color_space.changed(Transform::RGB);
		return copy.toColorSpace(to);
	}
	
	if( to.isBayer() )
		throw std::runtime_error( "Colorspace conversion to bayer not implemented!" );
	
	out.color_space = to;
	//Special case for GRAY to RGB
	if( color_space.isGray() && to.isRgb() ){
		//TODO: Transfer function not converted
		if( color_space.transfer() != to.transfer() )
			std::runtime_error( "Gray to RGB does not yet implement transfer function conversion" );
		
 		out.planes.push_back( out.planes[0] );
 		out.planes.push_back( out.planes[0] );
		return out;
	}
	
	if( color_space.components() != to.components() || to.components() != 3 )
		throw std::runtime_error( "Unsupported color space conversion" );
	
	//Upscale planes
	auto img_size = out.getSize();
	for( auto& info : out.planes )
		if( info.p.getSize() != img_size )
			info.p = info.p.scale_cubic( alpha_plane(), img_size );
	
	#pragma omp parallel for
	for( unsigned iy=0; iy<img_size.height(); iy++ ){
		ColorRow row( out, iy );
		for( unsigned ix=0; ix<img_size.width(); ix++ )
			row.set( ix, color_space.convert( row[ix], to ) );
	}
	
	return out;
}


bool ImageEx::read_file( QString path ){
	QFile f( path );
	if( !f.open( QIODevice::ReadOnly ) )
		return false;
	
	auto ext = QFileInfo( path ).suffix().toLower();
	if( ext == "fff" || ext == "dng" || ext == "arw" )
		return from_libraw( f );
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
		return {};
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
	
	return ImageEx{ std::move(out) };
}


double ImageEx::diff( const ImageEx& img, int x, int y ) const{
	//Check if valid
	if( !is_valid() || !img.is_valid() )
		return DOUBLE_MAX;
	
	//TODO: Why no alpha?
	return Difference::simple( planes[0].p, img[0], {x, y} );
}

bool ImageEx::is_interlaced() const{
	return planes[0].p.is_interlaced();
}
bool ImageEx::is_interlaced( const ImageEx& previous ) const{
	if( !previous )
		return is_interlaced();
	
	return planes[0].p.is_interlaced( previous[0] );
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
	if( color_space.isYCbCr() ){
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
	auto& cropped = planes[ color_space.isYCbCr() ? 1 : 0 ].p;
	return { cropped.getOffset(), cropped.getRealSize() - cropped.getSize() - cropped.getOffset() };
}

void ImageEx::copyFrom( const ImageEx& source, Point<unsigned> source_pos, Size<unsigned> source_size, Point<unsigned> to_pos ){
	//Validate
	if( size() != source.size() )
		throw std::runtime_error( "ImageEx::copyFrom() - not the same number of image planes" );
	for( unsigned c=0; c<size(); c++ )
		if( planeScale(c) != source.planeScale(c) )
			throw std::runtime_error( "ImageEx::copyFrom() - subplanes have different scales" );
	
	auto resize = [&](const Plane& p){
			Plane out( getSize() );
			out.fill( color::BLACK );
			out.copy( p, source_pos, source_size, to_pos );
			return out;
		};
	//TODO: Handle sub-sampling
	
	PorterDuff duffer( resize(source.alpha), alpha );
	for( unsigned c=0; c<size(); c++ )
		planes[c].p = duffer.over( resize( source.planes[c].p ), planes[c].p );
	
	alpha = duffer.overAlpha();
}

ImageEx Overmix::deVlcImage( const ImageEx& img ){
	Timer t( "deVlcImage" );
	if( !img.getColorSpace().isRgb() )
		return {};
	
	ImageEx out( img.getColorSpace().changed( Transform::YCbCr_709 ) );
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
			auto row_in1 = out[c].scan_line( iy*2   );
			auto row_in2 = out[c].scan_line( iy*2+1 );
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

ImageEx ImageEx::deconvolve_rl( Point<double> amount, unsigned iterations ) const{
	auto creep = ImageEx::fromFile( "creep.png" ); //TODO: Get as input
	auto get_creep = [&]( int c ){ return creep.is_valid() ? &creep.planes[c].p : nullptr; };
	
	auto out( *this );
	if( color_space.isYCbCr() )
		out.planes[0].p = out.planes[0].p.deconvolve_rl( amount, iterations, get_creep( 0 ), 0.5 );
	else
		for( unsigned c=0; c<size(); c++ )
			out.planes[c].p = out.planes[c].p.deconvolve_rl( amount, iterations, get_creep( c ), 0.5 );
	return out;
}
