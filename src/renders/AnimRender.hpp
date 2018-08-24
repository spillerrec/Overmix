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

#ifndef ANIM_RENDER_HPP
#define ANIM_RENDER_HPP

#include "ARender.hpp"
#include "../containers/ImageContainer.hpp"
#include <vector>

namespace Overmix{

class AnimRender{
	protected:
		ImageContainer frames;
		std::vector<Rectangle<>> old;
		
	public:
		AnimRender( const AContainer& aligner, ARender& render, AProcessWatcher* watcher=nullptr ); //TODO: watcher?
		
		int count() const{ return frames.count(); }
		
		ImageEx render( int frame, AProcessWatcher* watcher=nullptr );
};

}

#endif