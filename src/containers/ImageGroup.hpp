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
#include "AContainer.hpp"

#include <QString>
#include <vector>

class ImageItem{
	private:
		ImageEx img;
		Plane mask;
		int mask_id{ -1 };
		
	public:
		QString filename;
		Point<double> offset;
		int frame{ -1 };
		
		ImageItem( QString filename, ImageEx&& img )
			:	img(img), filename(filename) { }
		
		const ImageEx& image() const{ return img; }
		int maskId() const{ return mask_id; }
		const Plane& alpha( const std::vector<Plane>& masks ) const{
			return ( mask_id >= 0 ) ? masks[mask_id] : (mask ? mask : img.alpha_plane());
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

class ImageGroup : public AContainer{
	public:
		QString name;
		std::vector<ImageItem> items;
		const std::vector<Plane>& masks;
		
		ImageGroup( QString name, const std::vector<Plane>& masks ) : name(name), masks(masks) { }
		
	public: //AContainer implementation
		virtual unsigned count() const{ return items.size(); }
		virtual const ImageEx& image( unsigned index ) const{ return items[index].image(); }
		virtual int imageMask( unsigned index ) const{ return items[index].maskId(); }
		virtual const Plane& alpha( unsigned index ) const override{ return items[index].alpha( masks ); }
		virtual const Plane& mask( unsigned index ) const override{ return masks[index]; }
		virtual unsigned maskCount() const override{ return masks.size(); }
		virtual Point<double> pos( unsigned index ) const{ return items[index].offset; }
		virtual void setPos( unsigned index, Point<double> newVal ){ items[index].offset = newVal; }
		virtual int frame( unsigned index ) const{ return items[index].frame; }
		virtual void setFrame( unsigned index, int newVal ){ items[index].frame = newVal; }
};

#endif