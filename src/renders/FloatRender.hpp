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

#ifndef FLOAT_RENDER_HPP
#define FLOAT_RENDER_HPP

#include "ARender.hpp"

class FloatRender : public ARender{
	double scale_x, scale_y;
	public:
		FloatRender( double scale_x, double scale_y ) : scale_x( scale_x ), scale_y( scale_y ) { }
		FloatRender( double scale=1.0 ) : FloatRender( scale, scale ) { }
		virtual ImageEx render( const AContainer& aligner, unsigned max_count=-1, AProcessWatcher* watcher=nullptr ) const override;
};

#endif