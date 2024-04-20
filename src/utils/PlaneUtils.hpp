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

#ifndef PLANE_UTILS_HPP
#define PLANE_UTILS_HPP

#include "../color.hpp"
#include "../planes/Plane.hpp"

namespace Overmix{

/** Contains either a const reference or an instance to a plane. This makes
  * it easy to only send a reference instead of a copy if an image operation
  * could be avoided. */
template<class T>
class Modified{
	private:
		T modified;
		const T* original{ nullptr };
		
	public:
		Modified( T&& p ) : modified(std::move(p)) { }
		Modified( const T& p ) : original(&p) { }
		const T& operator()() const{ return original ? *original : modified; }
		
		//TODO: "modifier" must not modify the original plane??
		template<typename Func> void modify( Func modifier )
		{
			modified = modifier((*this)());
			original = nullptr;
		}
};

using ModifiedPlane = Modified<Plane>;

inline ModifiedPlane getScaled( const Plane& p, Size<unsigned> size ){
	if( p && p.getSize() != size )
		return { p.scale_cubic( Plane(), size ) }; //TODO: alpha?
	else
		return { p };
}

}

#endif
