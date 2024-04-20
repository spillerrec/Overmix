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

#include "../planes/ImageEx.hpp"
#include "AContainer.hpp"

#include <QString>
#include <QFileInfo>
#include <vector>

namespace Overmix{

class ImageItem{
	private:
		ImageEx img;
		Plane mask;
		int mask_id{ -1 };
		
	public:
		QString filename;
		Point<double> offset;
		int frame{ -1 };
		double rotation { 0.0 };
		Point<double> zoom { 1.0, 1.0 };
		
		ImageItem() { }
		ImageItem( QString filename, ImageEx&& img )
			:	img(std::move(img)), filename(filename) { }
		
		const ImageEx& image() const{ return img; }
		ImageEx& imageRef(){ return img; }
		int maskId() const{ return mask_id; }
		const Plane& alpha( const std::vector<Plane>& masks ) const{
			return ( mask_id >= 0 ) ? masks.at(mask_id) : (mask ? mask : img.alpha_plane());
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
		
		void imageModified() {
			//Remove absolute path info from filename
			filename = QFileInfo(filename).fileName();
		}
};

class ImageGroup : public AContainer{
	public:
		const AComparator* comparator{ nullptr };
		QString name;
		std::vector<ImageItem> items;
		const std::vector<Plane>* masks;
		
		ImageGroup() : masks{nullptr} { } //NOTE: don't use this!
		ImageGroup( AComparator* comparator, QString name, const std::vector<Plane>& masks )
			:	comparator(comparator), name(name), masks(&masks) { }
		
		std::vector<ImageItem>::iterator        begin()       { return items.begin(); }
		std::vector<ImageItem>::const_iterator  begin()  const{ return items.begin(); }
		std::vector<ImageItem>::iterator        end()         { return items.end();   }
		std::vector<ImageItem>::const_iterator  end()    const{ return items.end();   }
		
	public: //AContainer implementation
		virtual unsigned       count()                     const override{ return items.size(); }
		virtual const ImageEx& image(     unsigned index ) const override{ return items.at(index).image(); }
		virtual       ImageEx& imageRef(  unsigned index )       override{
			items.at(index).imageModified();
			return items.at(index).imageRef();
		}
		virtual int            imageMask( unsigned index ) const override{ return items.at(index).maskId(); }
		virtual const Plane&   alpha(     unsigned index ) const override{ return items.at(index).alpha( *masks ); }
		virtual const Plane&   mask(      unsigned index ) const override{ return masks->at(index); }
		virtual void           setMask(   unsigned index, int id ) override{      items.at(index).setSharedMask( id ); }
		virtual int            frame(     unsigned index ) const override{ return items.at(index).frame; }
		virtual unsigned       maskCount()                 const override{ return masks->size(); }
		virtual Point<double>  rawPos(    unsigned index ) const override{ return items.at(index).offset; }
		virtual Point<double>  zoom(      unsigned index ) const override{ return items.at(index).zoom; }
		virtual       double   rotation(  unsigned index ) const override{ return items.at(index).rotation; }
		
		virtual void setRawPos(   unsigned index, Point<double> newVal ) override{ items.at(index).offset   = newVal; }
		virtual void setFrame(    unsigned index, int           newVal ) override{ items.at(index).frame    = newVal; }
		virtual void setZoom(     unsigned index, Point<double> newVal ) override{ items.at(index).zoom     = newVal; }
		virtual void setRotation( unsigned index, double        newVal ) override{ items.at(index).rotation = newVal; }
		
	public:
		const AComparator* getComparator() const override{ return comparator; };
};

}

#endif
