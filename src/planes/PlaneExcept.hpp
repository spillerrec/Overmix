/*
	This file is part of AnimeRaster.

	AnimeRaster is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	AnimeRaster is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with AnimeRaster.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef PLANE_EXCEPT_HPP
#define PLANE_EXCEPT_HPP

#include "PlaneBase.hpp"



namespace Overmix{

namespace Throwers{

void sizeNotEqual( const char* const, Size<unsigned>, Size<unsigned> );

}

template<typename T>
void planeSizeEqual( const char* const where, const PlaneBase<T>& a, const PlaneBase<T>& b ){
	if( a.getSize() != b.getSize() )
		Throwers::sizeNotEqual( where, a.getSize(), b.getSize() );
}

}

#endif
