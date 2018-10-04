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

#ifndef GRADIENT_COMPARATOR_HPP
#define GRADIENT_COMPARATOR_HPP

#include "SimpleComparatorBase.hpp"
#include "../aligners/AAligner.hpp" //For AlignMethod
#include "../planes/basic/difference.hpp" //For SimpleSettings
#include "../color.hpp"

namespace Overmix{

class GradientComparator : public SimpleComparatorBase{
	public:
		AlignMethod method{ AlignMethod::VER };
		double movement{ 0.75 };
		int start_level{ 1 };
		int max_level{ 6 };
		color_type max_difference = 0.10*color::WHITE; //Difference must not be above this to match
		
	public:
		GradientComparator() { };
		GradientComparator( Difference::SimpleSettings settings
			,	AlignMethod method, double movement
			,	int start_level, int max_level, color_type max_difference
			)
			:	SimpleComparatorBase(settings), method(method), movement(movement)
			,	start_level(start_level), max_level(max_level), max_difference(max_difference)
			{ }
		
		ImageOffset findOffset( const Plane& img1, const Plane& img2, const Plane& a1, const Plane& a2 ) const override;
};

}

#endif
