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
#include "../comparators/AComparator.hpp"

#include <exception>
#include <stdexcept>

namespace Overmix{

class ConstDelegatedContainer : public AContainer{
	protected:
		const AContainer& container;
		auto const_exception() const
			{ return std::invalid_argument("Const container does not allow calling mutation methods!"); }
		
	public: //AContainer implementation
		virtual unsigned count() const override{ return container.count(); }
		virtual const ImageEx& image( unsigned index ) const override{ return container.image( index ); }
		virtual int imageMask( unsigned index ) const override{ return container.imageMask( index ); }
		virtual const Plane& alpha( unsigned index ) const override{ return container.alpha( index ); }
		virtual const Plane& mask( unsigned index ) const override{ return container.mask( index ); }
		virtual unsigned maskCount() const override{ return container.maskCount(); }
		virtual Point<double> pos( unsigned index ) const override{ return container.pos( index ); }
		virtual int frame( unsigned index ) const override{ return container.frame( index ); }
		
		virtual ImageEx& imageRef( unsigned ) override{ throw const_exception(); }
		virtual void setMask( unsigned, int ) override { throw const_exception(); }
		virtual void setFrame( unsigned, int ) override{ throw const_exception(); }
		virtual void setPos( unsigned, Point<double> ) override{ throw const_exception(); }
		
		virtual const AComparator* getComparator() const override{ return container.getComparator(); }
		virtual bool        hasCachedOffset( unsigned i, unsigned j ) const override
			{ return container.hasCachedOffset( i, j ); }
		virtual ImageOffset getCachedOffset( unsigned i, unsigned j ) const override
			{ return container.getCachedOffset( i, j ); }
		virtual void setCachedOffset( unsigned, unsigned, ImageOffset ) override{ throw const_exception(); }
		
	public:
		explicit ConstDelegatedContainer( const AContainer& container ) : container(container) { }
};

class DelegatedContainer : public ConstDelegatedContainer{
	private:
		AContainer& mutable_container;
		
	public: //AContainer implementation
		virtual ImageEx& imageRef( unsigned index ) override{ return mutable_container.imageRef( index ); }
		virtual void setPos( unsigned index, Point<double> pos ) override{ mutable_container.setPos( index, pos ); }
		virtual void setFrame( unsigned index, int frame ) override{ mutable_container.setFrame( index, frame ); }
		virtual void setMask( unsigned index, int id ) override { mutable_container.setMask( index, id ); }
		virtual void setCachedOffset( unsigned i, unsigned j, ImageOffset offset ) override { mutable_container.setCachedOffset( i, j, offset ); }
		
	public:
		explicit DelegatedContainer( AContainer& container ) : ConstDelegatedContainer(container), mutable_container(container) { }
};

}

#endif
