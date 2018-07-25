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

#include "VideoFrame.hpp"

#include "../planes/Plane.hpp"
#include "../planes/ImageEx.hpp"
#include "../color.hpp"


extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/mathematics.h>
}


using namespace Overmix;


static unsigned read8( uint8_t *&data ){
	return color::from8bit( *(data++) );
}
static unsigned read16( uint8_t *&data ){
	unsigned p1 = *(data++);
	unsigned p2 = *(data++);
	return ( p1 + (p2 << 8) );
}


VideoFrame::~VideoFrame(){ av_free( frame ); }

VideoFrame::VideoFrame( AVCodecContext &context ) {
	frame = av_frame_alloc();
	
	//Set subsampling resolution and color depth
	unsigned h_sub = 2;
	unsigned v_sub = 2;
	
	switch( context.pix_fmt ){
		// Half-width packed
		case AV_PIX_FMT_YUYV422:
				planar = false;
				v_sub = 1;
			break;
			
		// Quarter chroma planar
		case AV_PIX_FMT_YUV420P10LE:
				depth = 10;
		case AV_PIX_FMT_YUV420P: break;
		case AV_PIX_FMT_YUVJ420P: break;
		
		// Full chroma planar
		case AV_PIX_FMT_YUV444P10LE:
				depth = 10;
		case AV_PIX_FMT_YUV444P:
				h_sub = v_sub = 1;
			break;
			
		case AV_PIX_FMT_GBRP:
				h_sub = v_sub = 1;
				rgb = true;
			break;
		
		default:
			throw std::runtime_error( "Unknown format: " + std::to_string( context.pix_fmt ) );
	}
	
	//Initialize planes
	planes.emplace_back( context.width,       context.height       );
	planes.emplace_back( context.width/h_sub, context.height/v_sub );
	planes.emplace_back( context.width/h_sub, context.height/v_sub );
	
}


bool VideoFrame::is_keyframe() const{ return frame->key_frame; }


void VideoFrame::prepare_planes(){
	for( auto& p : planes )
		p.fill( 0 );
	
	if( planar ){
		auto readPlane = [=]( int index, Plane& p, unsigned (*f)( uint8_t*& ) ){
				//Read an entire plane into 'p', using reader 'f'
				for( unsigned iy=0; iy<p.get_height(); iy++ ){
					uint8_t *data = frame->data[index] + iy*frame->linesize[index];
					auto row = p[iy];
					
					for( unsigned ix=0; ix < p.get_width(); ix++)
						row[ix] = (*f)( data );
				}
			};
		
		for( int i=0; i<3; i++ )
			readPlane( i, planes[i], (depth <= 8) ? &read8 : &read16 );
	}
	else{	
		
		for( unsigned iy=0; iy<planes[0].get_height(); iy++ ){
			auto luma = planes[0][iy];
			auto row1 = planes[1][iy];
			auto row2 = planes[2][iy];
			uint8_t *data = frame->data[0] + iy*frame->linesize[0];
			//TODO: support other packing formats, and higher bit depths
			
			for( unsigned ix=0; ix < planes[0].get_width()/2; ix++){
				luma[ix] = read8( data );
				row1[ix] = read8( data );
				luma[ix] = read8( data );
				row2[ix] = read8( data );
			}
		}
	}
}

static Transform getColorSpace( AVFrame& frame ){
	switch( frame.colorspace ){
		case AVCOL_SPC_BT709    : return Transform::YCbCr_709;
		case AVCOL_SPC_SMPTE170M:
		case AVCOL_SPC_SMPTE240M: return Transform::YCbCr_601;
		case AVCOL_SPC_RGB      : return Transform::RGB;
		default: {
			//TODO: warning
			return Transform::UNKNOWN;
		}
	}
}

static Transfer getColorTransfer( AVFrame& frame ){
	switch( frame.color_range ){
		case AVCOL_TRC_SMPTE170M   : return Transfer::REC709; //TODO: Correct?
		case AVCOL_TRC_BT709       : return Transfer::REC709;
		case AVCOL_TRC_LINEAR      : return Transfer::LINEAR;
		case AVCOL_TRC_IEC61966_2_1: return Transfer::SRGB;
		default: {
			//TODO: warning
			return Transfer::UNKNOWN;
		}
	}
}

ImageEx VideoFrame::toImageEx(){
	//TODO: Detect actual color space
	ImageEx out( { getColorSpace( *frame ), getColorTransfer( *frame ) } );
	for( auto& p : planes )
		out.addPlane( std::move( p ) );
	return out;
}

