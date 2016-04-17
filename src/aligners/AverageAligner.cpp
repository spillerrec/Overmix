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


#include "AverageAligner.hpp"
#include "../renders/AverageRender.hpp"
#include "../utils/AProcessWatcher.hpp"

using namespace Overmix;

void AverageAligner::align( AContainer& container, AProcessWatcher* watcher ) const {
	container.resetPosition();
	if( container.count() <= 1 ) //If there is nothing to align
		return;
	
	ProgressWrapper progress( watcher );
	progress.setTotal( container.count() );
	
	SumPlane render;
	render.addAlphaPlane( process( container.plane( 0 ) )(), process( container.alpha( 0 ) )(), {0,0} );
	
	for( unsigned i=1; i<container.count() && !progress.shouldCancel(); i++ ){
		progress.setCurrent( i );
		auto img   = process( container.plane( i ) );
		auto alpha = process.scalePlane( container.alpha( i ) );
		
		//TODO: fix findOffset
		auto offset = AImageAligner::findOffset( process.movement(), render.average(), img(), render.alpha(), alpha() ).distance;
		container.setPos( i, container.minPoint() + offset/process.scale() );
		render.addAlphaPlane( img(), alpha(), offset );
	}
}

