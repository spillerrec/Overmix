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

using namespace std;


class AnimFrame{
	public:
		AnimationSeparator& aligner;
		std::vector<unsigned> indexes;
		
	public:
		AnimFrame( AnimationSeparator& aligner ) : aligner(aligner) { }
		void add_index( unsigned index, int frame ){
			indexes.push_back( index );
			aligner.setFrame( index, frame );
		}
		unsigned size() const{ return indexes.size(); }
		
		double find_error( unsigned index )
			{ return aligner.findOffset( indexes.back(), index ).error; }
};

double AnimationSeparator::find_threshold( AProcessWatcher* watcher ){
	ProgressWrapper progress( watcher );
	vector<color_type> errors;
	set_raw( true ); //TODO: remove the need of this
	for( unsigned i=0; i<count()-1; ++i ){
		errors.push_back( findOffset( i, i+1 ).error );
		progress.add();
	}
	set_raw( false );
	
	auto errors2 = errors;
	sort( errors.begin(), errors.end() );
	
	double longest = 0;
	double threshold = 0; //TODO: high value?
	
	//Try each threshold and pick the one which crosses the most
	for( unsigned i=1; i<errors.size(); i++ ){
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

void AnimationSeparator::align( AProcessWatcher* watcher ){
	ProgressWrapper progress( watcher );
	progress.setTotal( count() * 2 );
	if( count() == 0 )
		return;
	
	double factor = 1.0;//QInputDialog::getDouble( nullptr, "Specify threshold", "Threshold", 1.0, 0.01, 9.99, 2 );
	double threshold = find_threshold( watcher ) * factor;
	
	//Init
	std::vector<int> backlog;
	for( unsigned i=0; i<count(); i++ )
		backlog.push_back( i );
	
	for( int iteration=0; true; iteration++ ){
		AnimFrame frame( *this );
		
		for( int& index : backlog )
			if( index >= 0 )
				if( frame.size() == 0 || frame.find_error( index ) < threshold ){
					frame.add_index( index, iteration );
					index = -1;
					progress.add();
				}
		
		//Stop if no images
		if( frame.size() == 0 )
			break;
	}
}

