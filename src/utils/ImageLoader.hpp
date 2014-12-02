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

#ifndef DEBUG_HPP
#define DEBUG_HPP

#include "../planes/ImageEx.hpp"

#include <QtConcurrent>
#include <QString>

#include <vector>
#include <utility>
	
class ImageLoader{
	private:
		using Item = std::pair<QString,ImageEx&>;
		std::vector<Item> images;
		
	public:	
		ImageLoader( int reserve=0 ) { images.reserve( reserve ); }
		
		void add( QString filename, ImageEx& image ) { images.emplace_back( filename, image ); }
		const std::vector<Item>& loadAll(){
			QtConcurrent::blockingMap( images, []( Item& item ){
					item.second = ImageEx::fromFile( item.first );
				} );
			return images;
		}
};

#endif