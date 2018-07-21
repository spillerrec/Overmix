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

using namespace std; //TODO: remove
using namespace Overmix;

class VideoFile{
	private:
		QString filepath;
		AVFormatContext* format_context;
		AVCodecContext* codec_context;
		
		int stream_index;
		
		
	public:
		VideoFile( QString filepath )
			:	filepath( filepath )
			,	format_context( nullptr )
			,	codec_context( nullptr )
			{ }
		
		bool open();
		bool seek( unsigned min, unsigned sec );
		bool seek( int64_t byte );
		void run( QString dir );
		
		void only_keyframes(){
			codec_context->skip_loop_filter = AVDISCARD_NONKEY;
			codec_context->skip_idct = AVDISCARD_NONKEY;
			codec_context->skip_frame = AVDISCARD_NONKEY;
		//	codec_context->lowres = 2;
		}
		
		void debug_containter();
		void debug_video();
};

VideoStream::VideoStream( QString filepath ){
	av_register_all();
	if( avformat_open_input( &format_context
		,	filepath.toLocal8Bit().constData(), nullptr, nullptr ) < 0 ){
			throw std::runtime_error( "Couldn't open video file, either missing, unsupported or corrupted\n" );
		}
	
	if( avformat_find_stream_info( format_context, nullptr ) < 0 )
		throw std::runtime_error( "Couldn't find stream\n" );
	
	//Find the first video stream (and be happy)
	for( unsigned i=0; i<format_context->nb_streams; i++ ){
		if( format_context->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO ){
			stream_index = i;
			codec_context = format_context->streams[i]->codec;
			break;
		}
	}
	
	if( !codec_context )
		throw std::runtime_error( "Couldn't find a video stream!\n" );
	
	AVCodec *codec = avcodec_find_decoder( codec_context->codec_id );
	if( !codec )
		throw std::runtime_error( "Does not support this video codec :\\\n" );
	
	if( avcodec_open2( codec_context, codec, nullptr ) < 0 )
		throw std::runtime_error( "Couldn't open codec\n" );
}

bool VideoStream::seek( double seconds ){
	int64_t wanted_time = seconds * AV_TIME_BASE;
	int64_t target = av_rescale_q( wanted_time, AV_TIME_BASE_Q, format_context->streams[stream_index]->time_base );
	if( av_seek_frame( format_context, stream_index, target,  0 ) < 0 ){
		cout << "Couldn't seek\n";
		return false;
	}
	avcodec_flush_buffers( codec_context );
	cout << "target: " << target << "\n";
	return true;
}

struct StreamPacket{
	AVPacket packet;
	
	StreamPacket()
		{ av_init_packet( &packet ); }
	~StreamPacket()
		{ av_free_packet( &packet ); }
		
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
			int frame_done;
			if( avcodec_decode_video2( codec_context, frame, &frame_done, packet ) < 0 )
				throw std::runtime_error( "Error while decoding frame" );
			if( frame_done ){
				frame.prepare_planes();
				return std::move( frame );
			}
		}
	}
	
	throw std::runtime_error( "No more frames" );
	//TODO: handle...
}

