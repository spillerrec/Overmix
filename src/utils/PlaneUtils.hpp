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

#include "../planes/Plane.hpp"
#include "../color.hpp"

namespace Overmix{

/** Contains either a const reference or an instance to a plane. This makes
  * it easy to only send a reference instead of a copy if an image operation
  * could be avoided. */
class ModifiedPlane{
	private:
		Plane modified;
		const Plane* original{ nullptr };
		
	public:
		ModifiedPlane( Plane&& p ) : modified(std::move(p)) { }
		ModifiedPlane( const Plane& p ) : original(&p) { }
		const Plane& operator()() const{ return original ? *original : modified; }
		
		//TODO: "modifier" must not modify the original plane??
		template<typename Func> void modify( Func modifier )
			{ modified = modifier(modified); }
};

inline ModifiedPlane getScaled( const Plane& p, Size<unsigned> size ){
	if( p && p.getSize() != size )
		return { p.scale_cubic( size ) };
	else
		return { p };
}


class ColorRow{
	private:
		RowIt<color_type> r, g, b;
		
	public:
		ColorRow( ImageEx& img, int iy )
			:	r(img[0].scan_line(iy))
			,	g(img[1].scan_line(iy))
			,	b(img[2].scan_line(iy))
			{ }
		
		color operator[]( int i ) const
			{ return { r[i], g[i], b[i] }; }
		
		void set( int ix, color rgb ){
			r[ix] = rgb.r;
			g[ix] = rgb.g;
			b[ix] = rgb.b;
		}
};

}

#endif
