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


#include "SuperResAligner.hpp"
#include "../renders/RobustSrRender.hpp"

using namespace Overmix;


void SuperResAlignerImpl::align( AProcessWatcher* watcher ){
	auto base = RobustSrRender( local_scale ).render( *this, watcher );
	for( unsigned i=0; i<count(); i++ ){
		auto img = image( i );
		img.scaleFactor( {local_scale, local_scale} );
		setPos( i, findOffset( base, img ).distance / local_scale );
	}
}

