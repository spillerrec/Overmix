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

#include <vector>
#include <utility>

class AnimationSaver{
	private:
		struct FrameInfo{
			int x, y;
			QString name;
		};
		std::vector<FrameInfo> images;
		std::vector<std::pair<int,int>> frames;
		
	public:
		int addImage( int x, int y, QString filename ){
			images.push_back( {x, y, filename} );
			return images.size() - 1;
		}
		
		void addFrame( int id, int image ){
			frames.emplace_back( id, image );
		}
		
		void write( QString output_name );
};

#endif