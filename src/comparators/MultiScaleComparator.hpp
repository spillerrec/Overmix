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

#ifndef MULTI_SCALE_COMPARATOR_HPP
#define MULTI_SCALE_COMPARATOR_HPP

#include "SimpleComparatorBase.hpp"
#include "../aligners/AAligner.hpp" //For AlignMethod

namespace Overmix{

class MultiScaleComparator : public SimpleComparatorBase{
	public:
		AlignMethod method{ AlignMethod::VER };
		
	public:
		MultiScaleComparator() { };
		explicit MultiScaleComparator( AlignMethod method )
			:	method(method) { }
		
		ImageOffset findOffset( const Plane& img1, const Plane& img2, const Plane& a1, const Plane& a2, Point<double> hint ) const override;
};

}

#endif
