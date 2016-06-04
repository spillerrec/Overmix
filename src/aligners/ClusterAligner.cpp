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


#include "ClusterAligner.hpp"
#include "../utils/AProcessWatcher.hpp"

using namespace Overmix;

void ClusterAligner::align( AContainer& container, AProcessWatcher* watcher ) const {
	for( unsigned i=0; i<container.count(); i++ )
		for( unsigned j=0; j<container.count(); j++ )
			container.findOffset( i, j );
}

