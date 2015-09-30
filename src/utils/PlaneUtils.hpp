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

class ScaledPlane{
	private:
		Plane scaled;
		const Plane* original;
		
	public:
		ScaledPlane( const Plane& p, Size<unsigned> size ) : original( &p ){
			if( p.getSize() != size )
				scaled = p.scale_cubic( size );
		}
		
		ScaledPlane( const Plane& p, const Plane& wanted_size )
			: ScaledPlane( p, wanted_size.getSize() ) { }
		
		const Plane& operator()() const{ return scaled.valid() ? scaled : *original; }
};


class ColorRow{
	private:
		color_type* r, * g, * b;
		
	public:
		ColorRow( color_type* r, color_type* g, color_type* b )
			: r(r), g(g), b(b) { }
		ColorRow( ImageEx& img, int ix )
			:	r(img[0].scan_line(ix).begin())
			,	g(img[1].scan_line(ix).begin())
			,	b(img[2].scan_line(ix).begin())
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
