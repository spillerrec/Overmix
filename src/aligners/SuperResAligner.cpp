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


void SuperResAligner::align( class AContainer& container, class AProcessWatcher* watcher ){
	auto base = RobustSrRender( scale ).render( container, watcher );
	for( unsigned i=0; i<container.count(); i++ ){
		auto img = container.image( i );
		img.scaleFactor( {scale, scale} );
		//TODO: movement...
		container.setPos( i, AImageAligner::findOffset( {0.25,0.25}, base[0], img[0], base.alpha_plane(), img.alpha_plane() ).distance / scale );
	}
}

