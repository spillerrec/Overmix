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

#ifndef PORTER_DUFF_HPP
#define PORTER_DUFF_HPP

#include "Plane.hpp"

namespace Overmix{

class PorterDuff{
	private:
		const Plane& source_alpha;
		const Plane& destination_alpha;
		PlaneBase<double> a_src;
		PlaneBase<double> a_dest;
		PlaneBase<double> a_both;
		
		PlaneBase<double> zero;
		PlaneBase<double> ones;
		
	public:
		PorterDuff( const Plane& source_alpha, const Plane& destination_alpha );
		
		Plane values( const PlaneBase<double>& s, const PlaneBase<double>& d, const PlaneBase<double>& b ) const;
		Plane alpha( bool s, bool d, bool b ) const;
		
		Plane over( const Plane& src, const Plane dest ) const;
		Plane overAlpha() const;
};

}

#endif
