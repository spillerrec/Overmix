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

void AverageAligner::align( AProcessWatcher* watcher ){
	if( count() == 0 )
		return;
	
	ProgressWrapper( watcher ).setTotal( count() );
	
	raw = true;
	resetPosition();
	
	SumPlane render;
	render.addAlphaPlane( image( 0 )[0], alpha( 0 ), {0,0} );
	
	for( unsigned i=1; i<count(); i++ ){
		ProgressWrapper( watcher ).setCurrent( i );
		
		auto offset = find_offset( render.average(), image( i )[0], render.alpha(), alpha( i ) ).distance;
		setPos( i, minPoint() + offset );
		render.addAlphaPlane( image( i )[0], alpha( i ), offset );
	}
	
	raw = false;
}

