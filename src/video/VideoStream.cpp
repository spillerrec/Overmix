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

#include "VideoStream.hpp"
#include "VideoFrame.hpp"

#include "../planes/Plane.hpp"

#include <QString>
#include <QDebug>
#include <QFileInfo>

#include <iostream>
#include <stdexcept>
#include <memory>
#include <string>
#include <vector>

#include <boost/lexical_cast.hpp>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/mathematics.h>
}

using namespace Overmix;

VideoStream::VideoStream( QString filepath ){
//	av_register_all();
	if( avformat_open_input( &format_context
		,	filepath.toLocal8Bit().constData(), nullptr, nullptr ) < 0 ){
			throw std::runtime_error( "Couldn't open video file, either missing, unsupported or corrupted\n" );
		}
	
	if( avformat_find_stream_info( format_context, nullptr ) < 0 )
		throw std::runtime_error( "Couldn't find stream\n" );
	
	//Find the first video stream (and be happy)
	AVCodec *codec = nullptr;
	stream_index = av_find_best_stream( format_context, AVMEDIA_TYPE_VIDEO, -1, -1, &codec, 0 );
	if( stream_index < 0 )
		std::runtime_error( "Could not find video stream" );
	
	auto st = format_context->streams[stream_index];
	codec = avcodec_find_decoder(st->codecpar->codec_id);
	if( !codec )
		throw std::runtime_error( "Does not support this video codec :\\\n" );
	
	codec_context = avcodec_alloc_context3( codec );
	if( !codec_context )
		throw std::runtime_error( "Could not allocate context!\n" );
	//TODO: free it again
	
	avcodec_parameters_to_context( codec_context, format_context->streams[stream_index]->codecpar );
	
	if( avcodec_open2( codec_context, codec, nullptr ) < 0 )
		throw std::runtime_error( "Couldn't open codec\n" );
}

bool VideoStream::isVideoFile( QString path )
{
	auto ext = QFileInfo( path ).suffix().toLower();
	//TODO: multiple extesions
	return ext == "mkv" || ext == "mp4" || ext == "webm";
}

bool VideoStream::seek( double seconds ){
	int64_t wanted_time = seconds * AV_TIME_BASE;
	int64_t target = av_rescale_q( wanted_time, AV_TIME_BASE_Q, format_context->streams[stream_index]->time_base );
	if( av_seek_frame( format_context, stream_index, target,  0 ) < 0 ){
		std::cout << "Couldn't seek\n";
		return false;
	}
	avcodec_flush_buffers( codec_context );
	return true;
}

struct StreamPacket{
	AVPacket packet;
	
	StreamPacket()
		{ av_init_packet( &packet ); }
	~StreamPacket()
		{ av_packet_unref( &packet ); }
		
	operator AVPacket*(){ return &packet; }
};

VideoFrame VideoStream::getFrame(){
	VideoFrame frame( *codec_context );
	
	while( !eof ){
		StreamPacket packet;
		if( av_read_frame( format_context, packet ) < 0 )
			eof = true;
		
		if( eof ){
			//TODO: This looks suspect, but is in the example
			packet.packet.data = nullptr;
			packet.packet.size = 0;
			//NOTE: Is this to feed avcodec_decode_video2 an empty packet on EOF? Seems weird...
		}
		
		if( packet.packet.stream_index == stream_index || eof ){
		avcodec_send_packet( codec_context, packet );
		if( avcodec_receive_frame( codec_context, frame ) >= 0)
		//	int frame_done;
		//	if( avcodec_decode_video2( codec_context, frame, &frame_done, packet ) < 0 )
		//		throw std::runtime_error( "Error while decoding frame" );
		//	if( frame_done )
				return frame;
		}
	}
	
	throw std::runtime_error( "No more frames" );
	//TODO: handle...
}

