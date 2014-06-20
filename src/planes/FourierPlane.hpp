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
	private:
		unsigned real_width;
		Plane toPlaneInvalidate() const;
		
	public:
		FourierPlane( Size<unsigned> size ) : PlaneBase( size ), real_width( size.width*2 ) { }
		FourierPlane( unsigned w, unsigned h ) : PlaneBase( w, h ), real_width( w * 2 ) { }
		
		FourierPlane( const FourierPlane& p ) : PlaneBase( p ), real_width( p.real_width ) { }
		FourierPlane( FourierPlane&& p ) : PlaneBase( p ), real_width( p.real_width ) { }
		
		FourierPlane& operator=( const FourierPlane& p ){
			*(PlaneBase<std::complex<double>>*)this = p;
			real_width = p.real_width;
			return *this;
		}
		FourierPlane& operator=( FourierPlane&& p ){
			*(PlaneBase<std::complex<double>>*)this = p;
			real_width = p.real_width;
			return *this;
		}
		
		FourierPlane( const Plane& p );
		
		Plane asPlane() const;
		Plane toPlane() const{
			return FourierPlane( *this ).toPlaneInvalidate();
		}
		
};

#endif