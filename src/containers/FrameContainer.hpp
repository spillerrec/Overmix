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

#ifndef FRAME_CONTAINER_HPP
#define FRAME_CONTAINER_HPP

#include "AContainer.hpp"

class FrameContainer : public AContainer{
	private:
		AContainer& container;
		std::vector<unsigned> indexes;
		
	public: //AContainer implementation
		virtual unsigned count() const override{ return indexes.size(); }
		virtual const ImageEx& image( unsigned index ) const override{ return container.image( indexes[index] ); }
		virtual const Plane& alpha( unsigned index ) const override{ return container.alpha( indexes[index] ); }
		virtual const Plane& mask( unsigned index ) const override{ return container.mask( indexes[index] ); }
		virtual unsigned maskCount() const override{ return container.maskCount(); }
		virtual QPointF pos( unsigned index ) const override{ return container.pos( indexes[index] ); }
		virtual void setPos( unsigned index, QPointF newVal ) override{ return container.setPos( indexes[index], newVal ); }
		virtual int frame( unsigned index ) const override{ return container.frame( index ); }
		virtual void setFrame( unsigned index, int newVal ) override{ return container.setFrame( index, newVal ); }
		
	public:
		FrameContainer( AContainer& container, int frame ) : container(container) {
			for( unsigned i=0; i<container.count(); i++ )
				if( container.frame( i ) == frame )
					indexes.push_back( i );
		}
		
		unsigned realIndex( unsigned index ) const{ return indexes[index]; }
};

#endif