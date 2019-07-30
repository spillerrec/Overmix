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


#include "IndependentAligner.hpp"
#include "../comparators/AComparator.hpp"
#include "../containers/AContainer.hpp"
#include "../utils/AProcessWatcher.hpp"

#include <algorithm>
#include <cassert>
#include <numeric>
#include <stdexcept>
#include <vector>
#include <iostream> //For debug

using namespace Overmix;


void IndependentAligner::align( AContainer& container, AProcessWatcher* watcher ) const {
	Progress progress( "IndependentAligner", container.count() * range, watcher );
	for( unsigned i=0; i<container.count(); i++ )
		for( unsigned j=i+1; j<=std::min(i+range, container.count()-1); j++ ){
			container.findOffset( i, j );
			progress.add();
		}
	
	std::vector<Point<double>> offsets( container.count(), {0.0, 0.0} );
	
	for( unsigned i=0; i<container.count(); i++ ){
		double error = std::numeric_limits<double>::max();
		Point<double> best(0,0);
		for( unsigned j=0; j<i; j++ ){
			if( container.hasCachedOffset( i, j ) ){
				auto offset = container.getCachedOffset( i, j );
				if( error > offset.error ){
					error = offset.error;
					best = offsets[j] - offset.distance;
				}
			}
		}
		offsets[i] = best;
	}
	
	//TODO: Use this base offset to determine how much other images overlaps
	
	/*
	for( int iterations = 0; iterations < 10; iterations++ ){
		for( unsigned i=0; i<container.count(); i++ ){
			Point<double> sum = {0.0, 0.0};
			int count = 0;
			for( unsigned j=0; j<container.count(); j++ ){
				if( container.hasCachedOffset( i, j ) && i != j ){
					auto offset = container.getCachedOffset( j, i );
					sum += offset.distance + offsets[j];
					count++;
				}
			}
			if (count > 0)
				offsets[i] = sum / count;
		}
	}*/
	
	for( unsigned i=0; i<container.count(); i++ )
		container.setPos( i, offsets[i].round() );
}

