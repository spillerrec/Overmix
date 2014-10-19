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


#include "AnimatedAligner.hpp"
#include "RecursiveAligner.hpp"
#include "../renders/FloatRender.hpp"
#include "../containers/FrameContainer.hpp"
#include "AnimationSaver.hpp"

#include <QInputDialog>
#include <QDir>

#include <fstream>

#include "../debug.hpp"

using namespace std;


class AnimFrame{
	public:
		RecursiveAligner& aligner;
		std::vector<unsigned> indexes;
		int frame;
		
	public:
		AnimFrame( RecursiveAligner& aligner, int frame ) : aligner(aligner), frame(frame) { }
		void add_index( unsigned index ){
			indexes.push_back( index );
			aligner.setFrame( index, frame );
		}
		unsigned size() const{ return indexes.size(); }
		
		double find_error( unsigned index ){
			unsigned comp_index = indexes[ indexes.size()-1 ];
			aligner.set_raw( true );
			auto offset = aligner.find_offset( aligner.image( comp_index )[0], aligner.image( index )[0] );
			qDebug( "Offset: %f", offset.distance_y );
			aligner.set_raw( false );
			return offset.error;
		}
		
		void save( const ImageEx& background, AnimationSaver& anim, debug::CsvFile& csv ){
			//Initialize container
			FrameContainer container( aligner, frame );
			
			//Find offset
			//TODO: optimize if scale == 1.0
			ImageEx img2 = FloatRender(aligner.x_scale(), aligner.y_scale()).render( container );
			auto offset = aligner.find_offset( background[0], img2[0] );
			
			//Render this frame
			ImageEx img = FloatRender().render( container );
			
			//Add it to the file
			int img_index = anim.addImage( img );
			for( unsigned i=0; i<container.count(); i++ )
				anim.addFrame(
						offset.distance_x/aligner.get_scale(), offset.distance_y/aligner.get_scale()
					,	container.realIndex(i), img_index
					);
			
			//Add movement stuff to csv file
			for( unsigned i=0; i<container.count(); i++ )
				csv.add( (color_type)container.realIndex(i) ).add( container.pos(i).x ).add( container.pos(i).y ).stop();
			csv.stop();
		}
};

double AnimatedAligner::find_threshold(){
	vector<color_type> errors;
	set_raw( true ); //TODO: remove the need of this
	for( unsigned i=0; i<count()-1; ++i )
		errors.push_back( find_offset( image( i )[0], image( i+1 )[0] ).error );
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
	
	return threshold;
}

void AnimatedAligner::align( AProcessWatcher* watcher ){
	if( count() == 0 )
		return;
	
	//Create directories
	QDir( "." ).mkdir( "AnimatedAligner" );
	AnimationSaver anim( "AnimatedAligner/package" );
	
	double factor = 1.0;//QInputDialog::getDouble( nullptr, "Specify threshold", "Threshold", 1.0, 0.01, 9.99, 2 );
	double threshold = find_threshold() * factor;
	
	//Align and render the image
	//TODO: remove the need of this
	RecursiveAligner average( container, method, scale );
	average.set_movement( get_movement() );
	average.set_edges( use_edges );
	average.addImages();
	average.align();
	ImageEx average_render = FloatRender( average.x_scale(), average.y_scale() ).render( average );
	average_render.to_qimage( ImageEx::SYSTEM_REC709, ImageEx::SETTING_DITHER | ImageEx::SETTING_GAMMA ).save("AnimatedAligner/background.png" );
	
	//Init
	std::vector<int> backlog;
	for( unsigned i=0; i<count(); i++ )
		backlog.push_back( i );
	
	debug::CsvFile frame_error_csv( "AnimatedAligner/package/frames.csv" );
	frame_error_csv.add( "error" ).stop();
	debug::CsvFile movement_csv( "AnimatedAligner/package/movement.csv" );
	movement_csv.add( "index" ).add( "x" ).add( "y" ).stop();
	
	int iteration = 0;
	while( true ){
		AnimFrame frame( average, iteration++ );
		
		for( int& index : backlog ){
			if( index < 0 ) //Already used, ignore it
				continue;
			
			if( frame.size() == 0 ){
				frame.add_index( index );
				index = -1;
			}
			else{
				double error = frame.find_error( index );
				qDebug( "Error: %f", error );
				frame_error_csv.add( error );
				if( error < threshold ){
					frame.add_index( index );
					index = -1;
					qDebug( "Added" );
				}
			}
		}
		frame_error_csv.stop();
		
		//Stop if no images
		if( frame.size() == 0 )
			break;
		else
			frame.save( average_render, anim, movement_csv );
	}
	
	anim.write();
}

