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

#ifndef VIDEO_FRAME_HPP
#define VIDEO_FRAME_HPP

struct AVFrame;
struct AVCodecContext;
class QImage;

namespace Overmix{

class ImageEx;

class VideoFrame{
	private:
		AVFrame *frame;
		AVCodecContext& context;
		
	public:
		VideoFrame( AVCodecContext &context );
		VideoFrame( const VideoFrame& ) = delete;
		VideoFrame( VideoFrame&& );
		~VideoFrame();
		
		ImageEx toImageEx();
		QImage toPreview( int max_size );
		
		operator AVFrame*(){ return frame; }
		
		bool is_keyframe() const;
		
};

}

#endif
