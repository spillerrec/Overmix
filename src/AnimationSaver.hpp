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

class AnimationSaver{
	private:
		struct FrameInfo{
			int x, y;
			QString name;
			//TODO: add delay
		};
		std::vector<FrameInfo> images;
		std::vector<std::pair<int,int>> frames;
		
		QString folder;
		int current_id{ 1 };
		
	public:
		AnimationSaver( QString folder );
		
		int addImage( int x, int y, QImage img );
		
		void addFrame( int id, int image ){
			frames.emplace_back( id, image );
		}
		
		void write();
};

#endif