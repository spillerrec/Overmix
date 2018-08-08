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


#include "ProcessWaifu.hpp"

#include <w2xconv.h>
#include <QSpinBox>
#include "planes/ImageEx.hpp"
#include "color.hpp"

using namespace Overmix;

ProcessWaifu::ProcessWaifu( QWidget* parent ) : AProcessor( parent ){
	scale_amount  = newItem<QSpinBox>( "Scale"   );
	denoise_level = newItem<QSpinBox>( "Denoise" );
	
	scale_amount ->setRange( 1, 4 );
	denoise_level->setRange( 0, 2 );
}

QString ProcessWaifu::name() const{ return "Waifu2x"; }

ImageEx ProcessWaifu::process( const ImageEx& input ) const{
	auto scale   = scale_amount ->value();
	auto denoise = denoise_level->value();
	
	auto pixel_count = input.get_width() * input.get_height() * 100;
	auto line_bytes = input.get_width() * 3 * 4; //3 colors, 4 byte floating point
	auto in_buf  = std::make_unique<float[]>( pixel_count * 3 );
	auto out_buf = std::make_unique<float[]>( pixel_count * scale * scale * 3 );
	
	//Write image to input buffer
	ImageEx rgb = input.toRgb();
	int pos = 0;
	for( unsigned iy=0; iy<rgb.get_height(); iy++ ){
		auto row_r = rgb[0][iy];
		auto row_g = rgb[1][iy];
		auto row_b = rgb[2][iy];
		for( unsigned ix=0; ix<rgb.get_width(); ix++ ){
			in_buf[pos++] = color::asDouble( row_r[ix] );
			in_buf[pos++] = color::asDouble( row_g[ix] );
			in_buf[pos++] = color::asDouble( row_b[ix] );
		}
	}
	
	//Convert image
	auto conv = w2xconv_init( W2XCONV_GPU_AUTO, 0, false );
	w2xconv_load_models( conv, "/usr/share/waifu2x-converter-cpp/" );
	
	w2xconv_convert_rgb_f32( conv, (unsigned char*)out_buf.get(), line_bytes*scale, (unsigned char*)in_buf.get(), line_bytes, rgb.get_width(), rgb.get_height(), denoise, scale, 0 );
	
	w2xconv_fini( conv );
	
	//Read image from output buffer
	ImageEx output( { Transform::RGB, Transfer::SRGB } );
	for( int i=0; i<3; i++ ){
		Plane p( input.getSize() * scale );
		
		for( unsigned iy=0; iy<p.get_height(); iy++ ){
			auto row = p[iy];
			for( unsigned ix=0; ix<p.get_width(); ix++ )
				row[ix] = color::fromDouble( out_buf[ 3*p.get_width()*iy + ix*3 + i ] );
		}
		
		output.addPlane( std::move( p ) );
	}
	
	return output;
}
