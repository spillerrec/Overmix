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

#ifndef VIDEO_STREAM_HPP
#define VIDEO_STREAM_HPP

#include <memory>
#include <vector>

class QString;
struct AVFormatContext;
struct AVCodecContext;

namespace Overmix{

class VideoFrame;

class VideoStream{
	private:
		AVFormatContext* format_context{ nullptr };
		AVCodecContext*   codec_context{ nullptr };
		int stream_index{ -1 };
		bool eof{ false };
		
		
	public:
		explicit VideoStream( QString path );
		
		bool seek( double seconds );
		VideoFrame getFrame();
		void skipFrame();
		
		static bool isVideoFile( QString path );
};

}

#endif
