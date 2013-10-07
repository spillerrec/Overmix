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
#include "SimpleRender.hpp"

void AverageAligner::align(){
	if( count() == 0 )
		return;
	
	SimpleRender render;
	render.set_filter( SimpleRender::FOR_MERGING );
	
 	images[0].pos = QPointF( 0,0 );
	for( unsigned i=1; i<count(); i++ ){
		ImageEx* img = render.render( *this, i );
		if( !img )
			qFatal( "NoOOO" );
		
		ImageOffset offset = find_offset( *((*img)[0]), *(images[i].image) );
		images[i].pos = QPointF( offset.distance_x, offset.distance_y );
		
		delete img;
	}
}

