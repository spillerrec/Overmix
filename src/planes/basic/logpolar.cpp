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

			auto clamped = pos.max({0,0}).min(p.getSize()-1);
			auto base0 = clamped.floor();
			auto delta = clamped - base0;
			auto base1 = (base0 + 1).min(p.getSize()-1);
			
			auto v00 = p[base0.y][base0.x];
			auto v01 = p[base0.y][base1.x];
			auto v10 = p[base1.y][base0.x];
			auto v11 = p[base1.y][base1.x];
			
			auto mix = [](auto a, auto b, double x){ return a * (1.0-x) + b * x; };
			lp[iy][ix] = mix(
				mix(v00, v01, delta.x),
				mix(v10, v11, delta.x),
				delta.y);
		}
	}
	
	return lp;
}