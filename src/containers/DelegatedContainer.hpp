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

#ifndef DELEGATED_CONTAINER_HPP
#define DELEGATED_CONTAINER_HPP

#include "AContainer.hpp"

namespace Overmix{

class ConstDelegatedContainer : public AContainer{
	protected:
		ImageEx* temp; //TODO: avoid? used in imageRef
		const AContainer& container;
		
	public: //AContainer implementation
		virtual unsigned count() const override{ return container.count(); }
		virtual const ImageEx& image( unsigned index ) const override{ return container.image( index ); }
		virtual ImageEx& imageRef( unsigned ) override{ return *temp; } //TODO: throw exception
		virtual void setMask( unsigned, int ) override { } //TODO: throw exception
		virtual int imageMask( unsigned index ) const override{ return container.imageMask( index ); }
		virtual const Plane& alpha( unsigned index ) const override{ return container.alpha( index ); }
		virtual const Plane& mask( unsigned index ) const override{ return container.mask( index ); }
		virtual unsigned maskCount() const override{ return container.maskCount(); }
		virtual Point<double> pos( unsigned index ) const override{ return container.pos( index ); }
		virtual void setPos( unsigned, Point<double> ) override{  }
		virtual int frame( unsigned index ) const override{ return container.frame( index ); }
		virtual void setFrame( unsigned, int ) override{  }
		
	public:
		ConstDelegatedContainer( const AContainer& container ) : container(container) { }
};

class DelegatedContainer : public ConstDelegatedContainer{
	private:
		AContainer& container;
		
	public: //AContainer implementation
		virtual ImageEx& imageRef( unsigned index ) override{ return container.imageRef( index ); }
		virtual void setPos( unsigned index, Point<double> pos ) override{ container.setPos( index, pos ); }
		virtual void setFrame( unsigned index, int frame ) override{ container.setFrame( index, frame ); }
		virtual void setMask( unsigned index, int id ) override { container.setMask( index, id ); }
		
	public:
		DelegatedContainer( AContainer& container ) : ConstDelegatedContainer(container), container(container) { }
};

}

#endif