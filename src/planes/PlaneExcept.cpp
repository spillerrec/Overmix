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

#include "PlaneExcept.hpp"

#include <stdexcept>
#include <string>

using namespace Overmix;

template<typename T>
std::string sizeToString( Size<T> a ){
	return "("
		+	std::to_string( a.width() )
		+	" x "
		+	std::to_string( a.height() )
		+ 	")"
		;
}

void Throwers::sizeNotEqual( const char* const where, Size<unsigned> a, Size<unsigned> b ){
	auto err = std::string(where)
		+	" => planes where not of equal size\n\t"
		+	sizeToString( a )
		+	" vs. "
		+	sizeToString( b )
		+	"\n"
		;
	throw std::runtime_error( err ); 
}

