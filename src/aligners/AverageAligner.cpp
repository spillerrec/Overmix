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

using namespace Overmix;

void AverageAligner::align( AContainer& container, AProcessWatcher* watcher ){
	container.resetPosition();
	if( container.count() <= 1 ) //If there is nothing to align
		return;
	
	ProgressWrapper( watcher ).setTotal( container.count() );
	
	SumPlane render;
	render.addAlphaPlane( process( container.image( 0 )[0] )(), process( container.alpha( 0 ) )(), {0,0} );
	
	for( unsigned i=1; i<container.count(); i++ ){
		ProgressWrapper( watcher ).setCurrent( i );
		auto img   = process( container.image( i )[0] );
		auto alpha = process.scalePlane( container.alpha( i ) );
		
		//TODO: fix findOffset and movement constant
		auto offset = AImageAligner::findOffset( process.filter({0.5,0.5}), render.average(), img(), render.alpha(), alpha() ).distance;
		container.setPos( i, container.minPoint() + offset/process.scale() );
		render.addAlphaPlane( img(), alpha(), offset );
	}
}

