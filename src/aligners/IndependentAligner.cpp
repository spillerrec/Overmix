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
#include <QDebug> //For debug

using namespace Overmix;


void IndependentAligner::align( AContainer& container, AProcessWatcher* watcher ) const {
	Progress progress( "IndependentAligner", container.count() * range, watcher );
	for( unsigned i=0; i<container.count(); i++ )
		for( unsigned j=i; j<std::min(i+range, container.count()); j++ ){
			container.findOffset( i, j );
			progress.add();
		}
	
	std::vector<Point<double>> offsets( container.count(), {0.0, 0.0} );
	
	for( unsigned i=0; i<container.count(); i++ ){
		double error = 999999.9; //TODO:
		Point<double> best(0,0);
		for( unsigned j=0; j<i; j++ ){
			if( container.hasCachedOffset( i, j ) ){
				auto offset = container.getCachedOffset( i, j );
				qDebug() << "Error: " << i << "_" << j << " is " << offset.error << "\n";
				if( error > offset.error ){
					error = offset.error;
					best = offsets[j] - offset.distance;
					qDebug() << "Best: " << offset.distance.x << "_" << offset.distance.y << "  +  " << offsets[j].x << "_" << offsets[j].y  << "\n";
				}
			}
		}
		qDebug() << "Offset for " << i << ": (" << best.x << "_" << best.y << ")\n";
		offsets[i] = best;
	}
	
	for( unsigned i=0; i<container.count(); i++ )
		container.setPos( i, offsets[i] );
}

