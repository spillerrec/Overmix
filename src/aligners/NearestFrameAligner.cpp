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


#include "NearestFrameAligner.hpp"
#include "../containers/FrameContainer.hpp"
#include "../comparators/AComparator.hpp"
#include "../renders/AverageRender.hpp"
#include "../planes/ImageEx.hpp"
#include "../utils/AProcessWatcher.hpp"

using namespace Overmix;

void NearestFrameAligner::align( class AContainer& container, class AProcessWatcher* watcher ) const{
	//Get all used frame IDs
	auto frames = container.getFrames();
	if( frames.size() == 0 )
		return;
	//TODO: Verify that all frame IDs are actually valid?
	
	//Fill all containers with the already marked frames
	std::vector<FrameContainer> containers;
	for( auto& frame : frames )
		containers.emplace_back( container, frame, false );
	
	
	std::vector<SumPlane> renders( containers.size() );
	for( unsigned j=0; j<containers.size(); j++ ){
		auto base = AverageRender( false, true ).render( containers[j] );
		//TODO: for_merging? and progress info
		renders[j].addAlphaPlane( base[0], base.alpha_plane(), {0,0} );
	}
	//TODO: If we make the comparator work with empty images, we wouldn't need this special case
	
	ProgressWrapper progress( watcher );
	progress.setTotal( container.count() );
	for( unsigned i=0; i<container.count() && !progress.shouldCancel(); i++ ){
		progress.setCurrent( i );
		
		//Skip already know frames
		if( container.frame( i ) >= 0 )
			continue;
		
		auto comparator = container.getComparator();
		auto img   = comparator->process( container.plane( i ) );
		auto alpha = comparator->processAlpha( container.alpha( i ) );
		
		//Find best offset from all frames
		ImageOffset best_offset( {0,0}, 99999, 0.0 );
		unsigned best_id = 0;
		for( unsigned j=0; j<containers.size(); j++ ){
			auto offset = comparator->findOffset( renders[j].average(), img(), renders[j].alpha(), alpha() );
			if( offset.error < best_offset.error ){
				best_offset = offset;
				best_id = j;
			}
		}
		
		//Add to best frame and update frame id
		renders[best_id].addAlphaPlane( img(), alpha(), best_offset.distance );
		container.setFrame( i, frames[best_id] );
	}
}

