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

#include <QDebug>
#include <vector>


using namespace Overmix;


VideoFrame::VideoFrame( AVCodecContext &context )
	:	frame( av_frame_alloc() ), context( context ) { }
VideoFrame::~VideoFrame(){ av_frame_free( &frame ); }
VideoFrame::VideoFrame( VideoFrame&& other ) : frame( other.frame ), context( other.context ){
	other.frame = nullptr;
}

bool VideoFrame::is_keyframe() const{ return frame->key_frame; }



struct VideoInfo{
	unsigned h_sub  { 2 };
	unsigned v_sub  { 2 };
	unsigned depth  { 8 };
	bool     planar { true };
	bool     rgb    { false };
};

static VideoInfo getVideoInfo( AVPixelFormat pix_fmt ){
	VideoInfo out; 
	
	//NOTE: See VideoInfo for defaults
	switch( pix_fmt ){
		// Half-width packed
		case AV_PIX_FMT_YUYV422:
				out.planar = false;
				out.v_sub = 1;
			break;
			
		// Quarter chroma planar
		case AV_PIX_FMT_YUV420P10LE:
				out.depth = 10;
		case AV_PIX_FMT_YUV420P: break;
		case AV_PIX_FMT_YUVJ420P: break;
		
		// Full chroma planar
		case AV_PIX_FMT_YUV444P10LE:
				out.depth = 10;
		case AV_PIX_FMT_YUV444P:
				out.h_sub = out.v_sub = 1;
			break;
			
		case AV_PIX_FMT_GBRP:
				out.h_sub = out.v_sub = 1;
				out.rgb = true;
			break;
		
		default:
			throw std::runtime_error( "Unknown format: " + std::to_string( pix_fmt ) );
	}
	
	return out;
};


static Transform getColorSpace( AVColorSpace colorspace ){
	switch( colorspace ){
		case AVCOL_SPC_UNSPECIFIED:
		case AVCOL_SPC_BT709      : return Transform::YCbCr_709;
		case AVCOL_SPC_SMPTE170M  :
		case AVCOL_SPC_BT470BG    : return Transform::YCbCr_601;
		case AVCOL_SPC_RGB        : return Transform::RGB;
		default: {
			qWarning() << "Unknown AVColorSpace: " << colorspace;
			return Transform::UNKNOWN;
		}
	}
}

static Transfer getColorTransfer( AVColorTransferCharacteristic color_trc ){
	switch( color_trc ){
		case AVCOL_TRC_UNSPECIFIED : return Transfer::REC709;
		case AVCOL_TRC_SMPTE170M   : return Transfer::REC709; //TODO: Correct?
		case AVCOL_TRC_BT709       : return Transfer::REC709;
		case AVCOL_TRC_LINEAR      : return Transfer::LINEAR;
		case AVCOL_TRC_IEC61966_2_1: return Transfer::SRGB;
		default: {
			qWarning() << "Unknown AVColorTransferCharacteristic: " << color_trc;
			return Transfer::UNKNOWN;
		}
	}
}


static unsigned read8( uint8_t *&data ){
	return color::from8bit( *(data++) );
}
static unsigned read16( uint8_t *&data ){
	unsigned p1 = *(data++);
	unsigned p2 = *(data++);
	return ( p1 + (p2 << 8) ) << 4; //TODO: Adjust properly afterwards
}

ImageEx VideoFrame::toImageEx(){
	if( frame->width != context.width || frame->height != context.height )
		throw std::runtime_error( "VideoFrame::toImageEx - AVFrame invalid!" );
	
	auto info = getVideoInfo( context.pix_fmt );
	std::vector<Plane> planes;
	
	//Initialize planes
	planes.emplace_back( context.width,            context.height            );
	planes.emplace_back( context.width/info.h_sub, context.height/info.v_sub );
	planes.emplace_back( context.width/info.h_sub, context.height/info.v_sub );
	for( auto& p : planes ) //TODO: needed?
		p.fill( 0 );
	
	if( info.planar ){
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
			readPlane( i, planes[i], (info.depth <= 8) ? &read8 : &read16 );
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
	
	//TODO: We do not get the YUV signal range with this. H264 does support full data range!
	ImageEx out( { getColorSpace( context.colorspace ), getColorTransfer( context.color_trc ) } );
	for( auto& p : planes )
		out.addPlane( std::move( p ) );
	
	return out;
}

#include <QImage>
QImage VideoFrame::toPreview( int to_size ){
	//Extract the image data
	auto out = toImageEx();
	
	//Keep aspect ration
	auto new_size = out.getSize();
	auto max_size = std::max( new_size.x, new_size.y );
	new_size *= to_size / (double)max_size;
	
	//Convert without interpolation and dithering
	out.scale(new_size, ScalingFunction::SCALE_NEAREST);
	return out.to_qimage(false);
}

