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

double AnimationSeparator::find_threshold( const AContainer& container, Point<double> movement, AProcessWatcher* watcher ) const{
	ProgressWrapper progress( watcher );
	vector<color_type> errors;
	
	for( unsigned i=0; i<container.count()-1 && !progress.shouldCancel(); ++i ){
		errors.push_back( findError( container, i, i+1, movement ) );
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

double AnimationSeparator::findError( const AContainer& container, unsigned img1, unsigned img2, Point<double> movement ) const{
	if( skip_align ){
		//Use existing align information instead
		auto offset = (container.pos( img2 ) - container.pos( img1 )).round();
		offset = (offset * process.scale()).round();
		return container.image( img1 )[0].diff( container.image( img2 )[0], offset.x, offset.y );
	}
	else
		return AImageAligner::findOffset( container, movement, img1, img2 ).error;
}

void AnimationSeparator::align( AContainer& container, AProcessWatcher* watcher ) const{
	ProgressWrapper progress( watcher );
	progress.setTotal( container.count() * 2 );
	if( container.count() == 0 )
		return;
	
	ModifiedContainer modified( container, process );
	
	double threshold = find_threshold( modified, process.movement(), watcher ) * threshold_factor;
	
	
	//Init
	std::vector<int> backlog;
	for( unsigned i=0; i<container.count(); i++ )
		backlog.push_back( i );
	
	for( int iteration=0; !progress.shouldCancel(); iteration++ ){
		std::vector<unsigned> indexes;
		
		for( int& index : backlog )
			if( index >= 0 )
				if( indexes.size() == 0 || findError( modified, indexes.back(), index, process.movement() ) < threshold ){
					indexes.push_back( index );
					container.setFrame( index, iteration );
					index = -1;
					progress.add();
				}
		
		//Stop if no images
		if( indexes.size() == 0 || progress.shouldCancel() )
			break;
		
		qDebug() << "Frame" << iteration+1 << "contains" << indexes.size() << "images";
	}
}

