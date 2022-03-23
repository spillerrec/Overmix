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

#ifndef PARALLAX_RENDER_HPP
#define PARALLAX_RENDER_HPP

#include "ARender.hpp"

#include "../Geometry.hpp"

namespace Overmix{

class Plane;

class ParallaxRender : public ARender{
	private:
		Plane iteration( const AContainer& aligner, const AContainer& real, Size<unsigned> size ) const;
		
		int iteration_count{ 2 };
		double threshold{ 0.5 }; //For binarization
		unsigned dilate_size{ 10 };
		
	public:
		ParallaxRender( int iteration_count, double threshold, unsigned dilate_size )
			: iteration_count(iteration_count), threshold(threshold), dilate_size(dilate_size) { }
		
		virtual ImageEx render( const AContainer& group, AProcessWatcher* watcher=nullptr ) const override;
};

}

#endif