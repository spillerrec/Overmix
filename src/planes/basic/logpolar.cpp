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

#include "logpolar.hpp"

#include "../Plane.hpp"
#include "interpolation.hpp"

#include <iostream>

using namespace Overmix;

constexpr double PI = 3.14159;

Plane Transformations::logPolar( const Plane& p, Size<unsigned> endSize, double epScale ){
	Plane lp(endSize);

	std::vector<double> epPrecal;
	for (int ix=0; ix<lp.get_width(); ix++)
		epPrecal.push_back( std::max(0.0, std::exp(((double)ix) / (lp.get_width()/epScale))) );

	#pragma omp parallel for
	for (int iy=0; iy<lp.get_height(); iy++){
		auto angle = (iy) / (double)lp.get_height() * 2 * PI * PI;
		auto half_size = p.getSize() / 2.0;
		Point<double> wantedPre = { std::cos(angle), std::sin(angle) };

		for (int ix=0; ix<lp.get_width(); ix++){
			auto ep = epPrecal[ix];
			Point<double> pos = wantedPre * ep + half_size;
			lp[iy][ix] = bilinear(p, pos);
		}
	}
	
	return lp;
}