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

using namespace std;
using namespace Overmix;

class ModifiedContainer : public ConstDelegatedContainer {
	private:
		vector<Modified<ImageEx>> images;
		
	public:
		ModifiedContainer( AContainer& container, AlignerProcessor f ) : ConstDelegatedContainer( container ) {
			images.reserve( container.count() );
			for( unsigned i=0; i<container.count(); i++ )
				images.push_back( f.image( container.image( i ) ) );
		}
		
		
		virtual const ImageEx& image( unsigned index ) const override{ return images[index](); }
		virtual const Plane& alpha(   unsigned index ) const override{ return image( index ).alpha_plane(); }
};


class AnimFrame{
	public:
		AContainer& aligner;
		std::vector<unsigned> indexes;
		
	public:
		AnimFrame( AContainer& aligner ) : aligner(aligner) { }
		void add_index( unsigned index, int frame ){ indexes.push_back( index ); }
		unsigned size() const{ return indexes.size(); }
		
		double find_error( unsigned index )
			{ return 0.0;/*aligner.findOffset( indexes.back(), index ).error;*/ } //TODO:
};

double AnimationSeparator::find_threshold( const AContainer& container, AProcessWatcher* watcher ){
	ProgressWrapper progress( watcher );
	vector<color_type> errors;
	
	for( unsigned i=0; i<container.count()-1; ++i ){
		//errors.push_back( findOffset( i, i+1 ).error ); //TODO:
		progress.add();
	}
	
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

void AnimationSeparator::align( AContainer& container, AProcessWatcher* watcher ) const{
	ProgressWrapper progress( watcher );
	progress.setTotal( container.count() * 2 );
	if( container.count() == 0 )
		return;
	
	ModifiedContainer modified( container, process );
	
	double factor = 1.0;//QInputDialog::getDouble( nullptr, "Specify threshold", "Threshold", 1.0, 0.01, 9.99, 2 );
	double threshold = find_threshold( modified, watcher ) * factor;
	
	
	//Init
	std::vector<int> backlog;
	for( unsigned i=0; i<container.count(); i++ )
		backlog.push_back( i );
	
	for( int iteration=0; true; iteration++ ){
		AnimFrame frame( modified );
		
		for( int& index : backlog )
			if( index >= 0 )
				if( frame.size() == 0 || frame.find_error( index ) < threshold ){
					frame.add_index( index, iteration );
					container.setFrame( index, iteration );
					index = -1;
					progress.add();
				}
		
		//Stop if no images
		if( frame.size() == 0 )
			break;
	}
}

