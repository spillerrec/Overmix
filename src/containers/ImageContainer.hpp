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

#ifndef IMAGE_CONTAINER_HPP
#define IMAGE_CONTAINER_HPP

#include "ImageGroup.hpp"

class ImageContainer{
	private:
		std::vector<ImageGroup> groups;
		std::vector<Plane> masks;
		
	public:
		unsigned groupAmount() const{ return groups.size(); }
		const ImageGroup& getConstGroup( unsigned index ) const{ return groups[index]; }
		ImageGroup& getGroup( unsigned index ){ return groups[index]; }
		unsigned amount() const{
			unsigned sum=0;
			for( auto group : groups )
				sum += group.items.size();
			return sum;
			//TODO: accumulate
		}
		void clear(){
			groups.clear();
			masks.clear();
		}
		
	public:
		void addImage( ImageEx&& img, int group=-1 );
		void addFile( QString filepath );
		
		void addGroup( QString name ){ groups.push_back( { name } ); }
		
		void moveImage( unsigned from_group, unsigned from_img
			,	unsigned to_group, unsigned to_img );
		void moveGroup( unsigned from, unsigned to );
		
		void removeImage( unsigned group, unsigned img );
		
		//TODO: Get group thing
		
		//TODO: Get render thing
};

#endif