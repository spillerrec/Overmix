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

#ifndef ANIMATION_SAVER_HPP
#define ANIMATION_SAVER_HPP

#include <QString>
#include <QImage>

#include <vector>
#include <utility>

namespace Overmix{

class ImageEx;

class AnimationSaver{
	private:
		struct ImageInfo{
			QString name;
			QSize size;
		};
		struct FrameInfo{
			int x, y;
			int image_id;
			int delay;
		};
		std::vector<ImageInfo> images;
		std::vector<std::pair<int,FrameInfo>> frames;
		
		QString folder;
		int current_id{ 1 };
		
		int frames_per_second{ 25 };
		
		bool removeUnneededFrames();
		QSize normalize();
		
	public:
		explicit AnimationSaver( QString folder );
		
		int addImage( const ImageEx& img );
		
		void addFrame( int x, int y, int id, int image ){
			frames.emplace_back( id, FrameInfo( {x, y, image, 1000/frames_per_second} ) );
		}
		
		void write();
};

}

#endif