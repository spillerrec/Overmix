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

#ifndef A_CONTAINER_HPP
#define A_CONTAINER_HPP

#include "../Geometry.hpp"
#include <vector>
#include <utility>

namespace Overmix{

class ImageEx;
class Plane;
class AComparator;
struct ImageOffset;
enum class ScalingFunction;

template<class PointerType>
class ContainerImageRef{
	protected:
		PointerType* parent;
		unsigned index;

	public:
		ContainerImageRef( PointerType& parent, unsigned index )
			: parent( &parent ), index(index) {}

		auto& image()     const{ return parent->image(     index ); }
		auto& imageRef()       { return parent->imageRef(  index ); }
		auto& alpha()     const{ return parent->alpha(     index ); }
		auto  imageMask() const{ return parent->imageMask( index ); }
		auto  pos()       const{ return parent->pos(       index ); }
		auto  frame()     const{ return parent->frame(     index ); }
		auto& plane()     const{ return parent->plane(     index ); }
		
		void  setPos( Point<double> newVal ) { parent->setPos(   index, newVal ); }
		void  setFrame(int newVal )          { parent->setFrame( index, newVal ); }
};


class AContainer{
	public:
		virtual unsigned count() const = 0;
		virtual const ImageEx& image(     unsigned index ) const = 0;
		virtual       ImageEx& imageRef(  unsigned index )       = 0;
		virtual const Plane&   alpha(     unsigned index ) const = 0;
		virtual       int      imageMask( unsigned index ) const{ return -1; }
		virtual Point<double>  pos(       unsigned index ) const = 0;
		virtual       void     setPos(    unsigned index, Point<double> newVal ) = 0;
		virtual       int      frame(     unsigned index ) const = 0;
		virtual       void     setFrame(  unsigned index, int newVal ) = 0;
		
		
		virtual const Plane&   mask(      unsigned index ) const;
		virtual       void     setMask(   unsigned index, int id ) = 0;
		virtual unsigned maskCount() const{ return 0; }
        
		virtual ~AContainer() { }
		
		virtual const Plane& plane( unsigned index ) const;
		
	public:
		virtual const AComparator* getComparator() const = 0;
		virtual bool        hasCachedOffset( unsigned, unsigned ) const{ return false; }
		virtual ImageOffset getCachedOffset( unsigned, unsigned ) const;
		virtual void        setCachedOffset( unsigned, unsigned, ImageOffset );
		ImageOffset findOffset( unsigned, unsigned );
		
	public:
		Rectangle<double> size() const;
		Point<double> minPoint() const;
		Point<double> maxPoint() const;
		std::pair<bool,bool> hasMovement() const;
		void resetPosition()
			{ for( unsigned i=0; i<count(); i++ ) setPos( i, { 0.0, 0.0 } ); }
		void offsetAll( Point<double> offset )
			{ for( unsigned i=0; i<count(); i++ ) setPos( i, pos( i ) + offset ); }
		std::vector<int> getFrames() const;
		
		//Indexed access, including typedefs
		using ConstRef = ContainerImageRef<const AContainer>;
		using      Ref = ContainerImageRef<      AContainer>;
		auto operator[]( unsigned index ) const{ return ConstRef( *this, index );}
		auto operator[]( unsigned index )      { return      Ref( *this, index );}
		
};

/* We add the iterator members here, so we can't accidently use them */
template<class PointerType>
class ContainerImageIterator : public ContainerImageRef<PointerType>{
    public:
        ContainerImageIterator( PointerType& container, unsigned index )
            :   ContainerImageRef<PointerType>( container, index ) { }
        
        auto& operator++(){
			this->index++;
			return *this;
		}
        ContainerImageRef<PointerType> operator*(){ return { *(this->parent), this->index }; }
        bool operator!=( ContainerImageIterator b ){ return this->parent != b.parent || this->index != b.index; }
};

// All the container iterators
inline ContainerImageIterator<      AContainer> begin(       AContainer& container ){ return { container, 0                 }; }
inline ContainerImageIterator<const AContainer> begin( const AContainer& container ){ return { container, 0                 }; }
inline ContainerImageIterator<      AContainer> end(         AContainer& container ){ return { container, container.count() }; }
inline ContainerImageIterator<const AContainer> end(   const AContainer& container ){ return { container, container.count() }; }

}

#endif
