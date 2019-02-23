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


#include "AnimationSeparator.hpp"

#include <QDir>

#include <fstream>
#include <QDebug>

#include "../renders/ARender.hpp"
#include "../debug.hpp"
#include "../containers/DelegatedContainer.hpp"
#include "../utils/AProcessWatcher.hpp"

using namespace std;
using namespace Overmix;


double AnimationSeparator::findError( AContainer& container, int index1, int index2 ) const{
	if( skip_align ) //Assume container is already aligned
		return container.findError( index1, index2 );
	else
		return container.findOffset( index1, index2+1 ).error;
}

double AnimationSeparator::findThreshold( AContainer& container, AProcessWatcher* watcher ) const{
	Progress progress( "Find threshold", container.count(), watcher );
	vector<color_type> errors;
	
	for( unsigned i=0; i<container.count()-1 && !progress.shouldCancel(); ++i ){
		errors.push_back( findError( container, i, i+1 ) );
		progress.add();
	}
	
	auto errors2 = errors;
	sort( errors.begin(), errors.end() );
	
	double longest = 0;
	double threshold = 0; //TODO: high value?
	
	//Try each threshold and pick the one which crosses the most
	for( unsigned i=1; i<errors.size() && !progress.shouldCancel(); i++ ){
		double error = (errors[i] - errors[i-1]) / 2 + errors[i-1];
		unsigned amount = 0;
		bool below = false;
		for( unsigned j=0; j<errors2.size(); j++ ){
			bool current = errors2[j] > error;
			if( current != below )
				amount++;
			below = current;
		}
		if( amount >= longest ){
			longest = amount;
			threshold = error;
		}
	}
	
	debug::CsvFile error_csv( "AnimatedAligner/package/errors.csv" );
	error_csv.add( "errors2" ).add( "errors" ).add( "threshold" ).stop();
	for( unsigned i=0; i<errors.size(); i++ ){
		error_csv.add( errors2[i] );
		error_csv.add( errors[i] );
		error_csv.add( threshold );
		error_csv.stop();
	}
	progress.add();
	
	return threshold;
}

void AnimationSeparator::align( AContainer& container, AProcessWatcher* watcher ) const{
	if( container.count() == 0 )
		return;
	
	//TODO: Handle above return in AProcessWatcher?
	Progress progress( "AnimationSeparator", 2, watcher );
	auto threshold_progress = progress.makeSubtask();
	auto distribute_progress = progress.makeProgress( "Distribute frames", container.count() );
	
	double threshold = findThreshold( container, threshold_progress ) * threshold_factor;
	
	
	//Init
	std::vector<int> backlog;
	for( unsigned i=0; i<container.count(); i++ )
		backlog.push_back( i );
	
	for( int iteration=0; !distribute_progress.shouldCancel(); iteration++ ){
		std::vector<unsigned> indexes;
		
		for( int& index : backlog )
			if( index >= 0 )
				if( indexes.size() == 0 || findError( container, indexes.back(), index ) < threshold ){
					indexes.push_back( index );
					container.setFrame( index, iteration );
					index = -1;
					distribute_progress.add();
				}
		
		//Stop if no images
		if( indexes.size() == 0 )
			break;
		
		qDebug() << "Frame" << iteration+1 << "contains" << indexes.size() << "images";
	}
}

