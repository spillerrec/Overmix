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

#include "LogPolarComparator.hpp"

#include <stdexcept>

using namespace Overmix;

ImageOffset LogPolarComparator::findOffset( const Plane& img1, const Plane& img2, const Plane& a1, const Plane& a2, Point<double> hint ) const {
	throw std::logic_error("Not implemented");
}

double LogPolarComparator::findError( const Plane& img1, const Plane& img2, const Plane& a1, const Plane& a2, double x, double y ) const {
	throw std::logic_error("Not implemented");
}
