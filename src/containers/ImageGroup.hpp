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

#ifndef IMAGE_GROUP_HPP
#define IMAGE_GROUP_HPP

#include "../ImageEx.hpp"

#include <QString>
#include <QPointF>
#include <vector>

class ImageItem{
	private:
		QString filename;
		ImageEx img;
		Plane mask;
		int mask_id{ -1 };
		
	public:
		QPointF offset;
		
		ImageItem( QString filename, ImageEx&& img )
			:	filename(filename), img(img) { }
		
		const ImageEx& image() const{ return img; }
		const Plane& alpha( const std::vector<Plane> masks ) const{
			return ( mask_id >= 0 ) ? masks[mask_id] : mask;
		}
		
		void setMask( Plane&& mask ){
			this->mask = mask;
			mask_id = -1;
		}
	//	void setMask( Plane mask ){ setMask( mask ); }
		void setSharedMask( int id ){
			mask_id = id;
			mask = Plane();
		}
};

struct ImageGroup{
	QString name;
	std::vector<ImageItem> items;
	
	ImageGroup( QString name ) : name(name) { }
};

#endif