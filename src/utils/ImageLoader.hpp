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

#ifndef IMAGE_LOADER_HPP
#define IMAGE_LOADER_HPP

#include <QString>
#include <QStringList>

#include <vector>
#include <utility>
	
class Deteleciner;
class ImageEx;
class ImageContainer;

class ImageLoader{
	private:
		using Item = std::pair<QString,ImageEx&>;
		std::vector<Item> images;
		
	public:	
		ImageLoader( int reserve ) { images.reserve( reserve ); }
		
		void add( QString filename, ImageEx& image ) { images.emplace_back( filename, image ); }
		const std::vector<Item>& loadAll();
		
		static std::vector<ImageEx> loadImages( QStringList list );
		static void loadImages( QStringList list, ImageContainer& container, Deteleciner& detele, int alpha_mask=-1 );
};

#endif