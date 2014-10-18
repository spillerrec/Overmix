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

#include <QRect>
#include <QPointF>

class ImageEx;
class Plane;

class AContainer{
	public:
		virtual unsigned count() const = 0;
		virtual const ImageEx& image( unsigned index ) const = 0;
		virtual const Plane& alpha( unsigned index ) const = 0;
		virtual QPointF pos( unsigned index ) const = 0;
		virtual void setPos( unsigned index, QPointF newVal ) = 0;
		virtual int frame( unsigned index ) const = 0;
		virtual void setFrame( unsigned index, int newVal ) = 0;
		virtual ~AContainer() { }
		
	public:
		QRect size() const;
		QPointF minPoint() const;
		void resetPosition();
		void offsetAll( double dx, double dy );
};

#endif