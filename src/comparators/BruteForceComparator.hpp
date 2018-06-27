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

#ifndef BRUTE_FORCE_COMPARATOR_HPP
#define BRUTE_FORCE_COMPARATOR_HPP

#include "AComparator.hpp"
#include "../aligners/AAligner.hpp" //For AlignMethod
#include "../planes/basic/difference.hpp" //For SimpleSettings

namespace Overmix{

class BruteForceComparator : public AComparator{
	public:
		AlignMethod method{ AlignMethod::VER };
		double movement{ 0.75 };
		Difference::SimpleSettings settings;
		
	public:
		ImageOffset findOffset( const Plane& img1, const Plane& img2, const Plane& a1, const Plane& a2 ) const override;
};

}

#endif
