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

#include "../planes/PlaneBase.hpp"

#include <vector>

namespace Overmix{

class Plane;
class ImageEx;
class DctPlane;

class QuantTable{
	private:
		PlaneBase<double> table; //TODO: type?
		
		double scale( int ix, int iy ) const
			{ return 2 * 2 * 4 * (ix==0?sqrt(2):1) * (iy==0?sqrt(2):1); }
			//NOTE: 4 is defined by JPEG, 2 is from FFTW, last 2???
		
		int quantize( double coeff, double quant ) const
			{ return std::round( coeff * quant ); }
		
		double degradeCoeff( double coeff, double quant ) const{
			return std::round( coeff * quant ) / quant;
		}
		
	public:
		QuantTable();
		QuantTable( uint16_t* input );
		
		unsigned degrade8x8Comp( DctPlane& f1, DctPlane& f2, const Plane& p1, const Plane& p2, Point<unsigned> pos ) const;
		
		Plane degradeComp( const Plane& mask, const Plane& p1, const Plane& p2, unsigned& change ) const;
};

class JpegPlane{
	private:
		QuantTable quant;
		Size<double> sampling;
		
	public:
		JpegPlane( QuantTable quant, double sub_h, double sub_v )
			:	quant(quant), sampling(sub_h, sub_v) { }
		
		Plane degradeComp( const Plane& mask, const Plane& p1, const Plane& p2, unsigned& change ) const;
};

class JpegDegrader{
	public:
		std::vector<JpegPlane> planes;
		
	public:
		void addPlane( JpegPlane plane ){ planes.emplace_back( plane ); }
};

}

#endif