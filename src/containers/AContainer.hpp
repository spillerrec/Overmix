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

class ImageEx;
class Plane;
enum class ScalingFunction;

class AContainer{
	public:
		virtual unsigned count() const = 0;
		virtual const ImageEx& image( unsigned index ) const = 0;
		virtual ImageEx& imageRef( unsigned index ) = 0;
		virtual const Plane& alpha( unsigned index ) const = 0;
		virtual int imageMask( unsigned ) const{ return -1; }
		virtual const Plane& mask( unsigned index ) const;
		virtual unsigned maskCount() const{ return 0; }
		virtual Point<double> pos( unsigned index ) const = 0;
		virtual void setPos( unsigned index, Point<double> newVal ) = 0;
		virtual int frame( unsigned index ) const = 0;
		virtual void setFrame( unsigned index, int newVal ) = 0;
		virtual ~AContainer() { }
		
	public:
		void cropImage( unsigned index, unsigned left, unsigned top, unsigned right, unsigned bottom );
		void scaleImage( unsigned index, Point<double> scale, ScalingFunction scaling );
		
	public:
		Rectangle<double> size() const;
		Point<double> minPoint() const;
		Point<double> maxPoint() const;
		void resetPosition();
		void offsetAll( double dx, double dy );
		std::vector<int> getFrames() const;
};

#endif