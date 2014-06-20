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

#ifndef FOURIER_PLANE_HPP
#define FOURIER_PLANE_HPP

#include "Plane.hpp"

#include <QPoint>
#include <QList>
#include <cstdio>
#include <vector>
#include <utility>
#include <complex>

class FourierPlane : public PlaneBase<std::complex<double>>{
	public:
		FourierPlane( Size<unsigned> size) : PlaneBase( size ) { }
		FourierPlane( unsigned w, unsigned h ) : PlaneBase( w, h ) { }
		
		FourierPlane( const FourierPlane& p ) : PlaneBase( p ) { }
		FourierPlane( FourierPlane&& p ) : PlaneBase( p ) { }
		
		FourierPlane& operator=( const FourierPlane& p ){
			*(PlaneBase<std::complex<double>>*)this = p;
			return *this;
		}
		FourierPlane& operator=( FourierPlane&& p ){
			*(PlaneBase<std::complex<double>>*)this = p;
			return *this;
		}
		
		FourierPlane( const Plane& p );
		
		Plane asPlane() const;
		
};

#endif