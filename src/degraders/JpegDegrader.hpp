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

#ifndef JPEG_DEGRADER_HPP
#define JPEG_DEGRADER_HPP

#include <vector>

#include "../planes/Plane.hpp"

class ImageEx;
class DctPlane;

class QuantTable{
	private:
		Plane table; //TODO: type?
		
		double scale( int ix, int iy ) const
			{ return 2 * 2 * 4 * (ix==0?sqrt(2):1) * (iy==0?sqrt(2):1); }
			//NOTE: 4 is defined by JPEG, 2 is from FFTW, last 2???
		
		int quantize( int ix, int iy, double coeff, double quant ) const
			{ return std::round( coeff / (quant * scale(ix,iy)) ); }
		
		double degradeCoeff( int ix, int iy, double coeff, double quant ) const{
			auto factor = quant * scale(ix,iy);
			return std::round( coeff / factor ) * factor;
		}
		
	public:
		QuantTable();
		QuantTable( uint16_t* input );
		
		Plane degrade8x8( DctPlane& f, const Plane& p, Point<unsigned> pos ) const;
		Plane degrade8x8Comp( DctPlane& f1, DctPlane& f2, const Plane& p1, const Plane& p2, Point<unsigned> pos ) const;
		
		Plane degrade( const Plane& p ) const;
		Plane degradeComp( const Plane& p1, const Plane& p2 ) const;
};

class JpegPlane{
	private:
		QuantTable quant;
		Size<double> sampling;
		
	public:
		JpegPlane( QuantTable quant, double sub_h, double sub_v )
			:	quant(quant), sampling(sub_h, sub_v) { }
		
		Plane degrade( const Plane& p ) const;
		Plane degradeComp( const Plane& p1, const Plane& p2 ) const;
};

class JpegDegrader{
	public:
		std::vector<JpegPlane> planes;
		
	public:
		void addPlane( JpegPlane plane ){ planes.emplace_back( plane ); }
		
		ImageEx degrade( const ImageEx& img ) const;
};

#endif