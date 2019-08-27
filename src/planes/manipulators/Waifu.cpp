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


#include "Waifu.hpp"

#include <w2xconv.h>
#include "../ImageEx.hpp"
#include "../../color.hpp"
#include <stdexcept>

using namespace Overmix;



class WaifuBuffer{
	private:
		Size<unsigned> size;
		int components;
		std::unique_ptr<float[]> buf;
	
	public:
		WaifuBuffer( Size<unsigned> size, int components )
			: size(size), components(components)
			, buf( std::make_unique<float[]>( size.width() * size.height() * components ) )
		{ }
		WaifuBuffer( Size<unsigned> size, int components, double scale )
			: WaifuBuffer( size*scale, components )
		{ }
		
		unsigned stride() const
			{ return components; }
		unsigned bytesPerLine() const
			{ return stride() * size.width() * sizeof(float); }
		unsigned char* getData()
			{ return (unsigned char*)buf.get(); }
		Size<unsigned> getSize() const{ return size; }
		
		float& operator[]( int index )
			{ return buf[index]; }
};

static void readPlane( WaifuBuffer& buffer, int offset, const Plane& p ){
	//Write a plane into a float buffer
	int pos = offset;
	for( unsigned iy=0; iy<p.get_height(); iy++ ){
		auto row = p[iy];
		for( unsigned ix=0; ix<p.get_width(); ix++ ){
			buffer[pos] = color::asDouble( row[ix] );
			pos += buffer.stride();
		}
	}
}
static Plane writePlane( WaifuBuffer& buffer, int offset ){
	Plane p( buffer.getSize() );
	
	int pos = offset;
	for( unsigned iy=0; iy<p.get_height(); iy++ ){
		auto row = p[iy];
		for( unsigned ix=0; ix<p.get_width(); ix++ ){
			row[ix] = color::fromDouble( buffer[ pos ] );
			pos += buffer.stride();
		}
	}
	
	return p;
}

Waifu::Waifu( double scale, int denoise, const char* model_dir )
	:	scale(scale), denoise(denoise)
{
	conv = w2xconv_init( W2XCONV_GPU_AUTO, 0, true );
	if( !conv )
		throw std::runtime_error( "Waifu (w2xconv) could not be initialized" );
	
	auto local_dir = model_dir;
	if( !local_dir )
		local_dir = WAIFU_MODEL_DIR;
	
	if( w2xconv_load_models( conv, local_dir ) < 0 )
		throw std::runtime_error( "Could not load Waifu2x models" );
}

Waifu::~Waifu(){
	w2xconv_fini( conv );
	conv = nullptr;
}

ImageEx Waifu::processRgb( const ImageEx& input ){
	WaifuBuffer in ( input.getSize(), 3        );
	WaifuBuffer out( input.getSize(), 3, scale );
	
	//Write image to input buffer
	readPlane( in, 0, input[0] );
	readPlane( in, 1, input[1] );
	readPlane( in, 2, input[2] );
	
	w2xconv_convert_rgb_f32( conv
		,	out.getData(), out.bytesPerLine()
		,	in .getData(), in .bytesPerLine()
		,	input.get_width(), input.get_height()
		,	denoise, scale
		,	0 //TODO: unknown 'blockSize' argument
		);
	
	//Read image from output buffer
	ImageEx output( { Transform::RGB, Transfer::SRGB } );
	for( int i=0; i<3; i++ )
		output.addPlane( writePlane( out, i ) );
	return output;
}

ImageEx Waifu::processYuv( const ImageEx& input ){
	return processRgb(input.toRgb());
	
	WaifuBuffer in ( input.getSize(), 3        );
	WaifuBuffer out( input.getSize(), 3, scale );
	
	//Write image to input buffer
	readPlane( in, 0, input[0] );
	readPlane( in, 1, input[1] ); //TODO: Scale chroma as needed
	readPlane( in, 2, input[2] );
	
	/* TODO: Why no size parameter??
	w2xconv_convert_yuv( conv
		,	out.getData(), out.bytesPerLine()
		,	in .getData(), in .bytesPerLine()
		,	input.get_width(), input.get_height()
		,	denoise, scale
		,	0 //TODO: unknown 'blockSize' argument
		);//*/
	
	//Read image from output buffer
	ImageEx output( input.getColorSpace() );
	for( int i=0; i<3; i++ )
		output.addPlane( writePlane( out, i ) );
	return output;
}

Plane Waifu::process( const Plane& input ) {
	WaifuBuffer in ( input.getSize(), 1        );
	WaifuBuffer out( input.getSize(), 1, scale );
	
	//Write image to input buffer
	readPlane( in, 0, input );
	
	//TODO: Will require two-steps, as it is more low-level?
	auto doStep = [&](W2XConvFilterType type){
			w2xconv_apply_filter_y(conv, type,
				out.getData(), out.bytesPerLine(),
				in.getData(), in.bytesPerLine(),
				input.get_width(), input.get_height(),
				0
				);
		};
		
	//TODO: Other scaling factors
	if (scale > 1)
		doStep( W2XCONV_FILTER_SCALE2x );
	
	//Denoising TODO: Should this be before scaling?
	auto amount = [this](){
			switch(denoise){
				case  1: return W2XCONV_FILTER_DENOISE1;
				case  2: return W2XCONV_FILTER_DENOISE2;
				default: return W2XCONV_FILTER_DENOISE3;
			};
		}();
	if(denoise > 0)
		doStep( amount );
	
	return writePlane( out, 0 );
}

ImageEx Waifu::process( const ImageEx& input ) {
	ImageEx output;
	
	//Select between Gray/Rgb/Yuv depending on image
	auto color_space = input.getColorSpace();
	if( color_space.isRgb() )
		output = processRgb( input );
	else if( color_space.isYCbCr() )
		output = processYuv( input );
	else if( color_space.isGray() )
		output = ImageEx( process( input[0] ) );
	else
		return {};
	
	//Scale alpha plane as well
	if( input.alpha_plane() )
		output.alpha_plane() = process( input.alpha_plane() );
	
	return output;
}
