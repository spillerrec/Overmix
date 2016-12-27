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

#include "PorterDuff.hpp"

#include "../color.hpp"
#include "../utils/utils.hpp"

#include <cassert>

using namespace Overmix;

PorterDuff::PorterDuff( const Plane& source_alpha, const Plane& destination_alpha )
	:	source_alpha(source_alpha), destination_alpha(destination_alpha)
	,	a_src( source_alpha.getSize() ), a_dest( source_alpha.getSize() ), a_both( source_alpha.getSize() )
	,	zero( source_alpha.getSize() ), ones( source_alpha.getSize() )
{
	assert( source_alpha.getSize() == destination_alpha.getSize() );
	zero.fill( 0.0 );
	ones.fill( 1.0 );
	
	for( unsigned iy=0; iy<zero.get_height(); iy++ ){
		for( unsigned ix=0; ix<zero.get_width(); ix++ ){
			auto in_src  = color::asDouble( source_alpha     [iy][ix] );
			auto in_dest = color::asDouble( destination_alpha[iy][ix] );
			a_src [iy][ix] = in_src  * (1.0 - in_dest);
			a_dest[iy][ix] = in_dest * (1.0 - in_src );
			a_both[iy][ix] = in_src * in_dest;
		}
	}
}

Plane PorterDuff::values( const PlaneBase<double>& s, const PlaneBase<double>& d, const PlaneBase<double>& b ) const{
	Plane output( s.getSize() );
	
	for( unsigned iy=0; iy<zero.get_height(); iy++ ){
		for( unsigned ix=0; ix<zero.get_width(); ix++ ){
			output[iy][ix] = color::fromDouble(
					a_src [iy][ix] * s[iy][ix]
				+	a_dest[iy][ix] * d[iy][ix]
				+	a_both[iy][ix] * b[iy][ix]
				);
		}
	}
	
	return output;
}

Plane PorterDuff::alpha( bool s, bool d, bool b ) const{
	auto value = [&]( bool use ){ return use ? ones : zero; };
	return values( value(s), value(d), value(b) );
}

Plane PorterDuff::over( const Plane& src, const Plane dest ) const{
	auto s_src = src.toDouble();
	return values( s_src, dest.toDouble(), s_src );
}

Plane PorterDuff::overAlpha() const{
	return alpha( true, true, true );
}
