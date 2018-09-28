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


#include "FrameAligner.hpp"
#include "RecursiveAligner.hpp"
#include "../renders/FloatRender.hpp"
#include "../containers/ImageContainer.hpp"
#include "../containers/FrameContainer.hpp"
#include "../utils/AProcessWatcher.hpp"

using namespace Overmix;

void FrameAligner::align( class AContainer& container, class AProcessWatcher* watcher ) const{
	auto frames = container.getFrames();
	auto base_point = container.minPoint();
	
	MultiProgress progress( "FrameAligner", 3, watcher );
	auto progress_render = progress.makeProgress( "Render frames", frames.size() );
	auto progress_align = progress.makeWatcher();
	auto progress_offset = progress.makeProgress( "Offset frames", frames.size() );
	
	ImageContainer images;
	for( auto& frame : frames ){
		FrameContainer current( container, frame );
		images.addImage( FloatRender( 1.0, 1.0 ).render( current ) );
		progress_render.add();
	}
	
	RecursiveAligner().align( images, progress_align ); //TODO: make configurable
	
	progress_offset.loopAll( [&](int i){
			FrameContainer current( container, frames[i] );
			auto aligned_offset = base_point - current.minPoint();
			current.offsetAll( aligned_offset + (images.pos(i) - images.minPoint()) );
		} );
}

