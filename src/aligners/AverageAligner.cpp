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
#include "../renders/SimpleRender.hpp"


void AverageAligner::align( AProcessWatcher* watcher ){
	if( count() == 0 )
		return;
	
	raw = true;
	
	if( watcher )
		watcher->setTotal( count() );
	
	resetPosition();
	for( unsigned i=1; i<count(); i++ ){
		if( watcher )
			watcher->setCurrent( i );
		
		ImageEx img = SimpleRender( SimpleRender::FOR_MERGING ).render( *this, i );
		setPos( i, minPoint() + find_offset( img[0], image( i )[0], img.alpha_plane(), alpha( i ) ).distance );
	}
	
	raw = false;
}

