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
#include "AverageAligner.hpp"
#include "SimpleRender.hpp"
#include "AnimationSaver.hpp"

#include <QInputDialog>

#include <fstream>

#include "debug.hpp"

using namespace std;

class AnimFrame{
	public:
		AverageAligner& aligner;
		std::vector<unsigned> indexes;
		
	public:
		AnimFrame( AverageAligner& aligner ) : aligner(aligner){ }
		void add_index( unsigned index ){ indexes.push_back( index ); }
		unsigned size() const{ return indexes.size(); }
		
		double find_error( unsigned index ){
			unsigned comp_index = indexes[ indexes.size()-1 ];
			aligner.set_raw( true );
			auto offset = aligner.find_offset( *aligner.plane( comp_index, 0 ), *aligner.plane( index, 0 ) );
			qDebug( "Offset: %f", offset.distance_y );
			aligner.set_raw( false );
			return offset.error;
		}
		
		void save( int frame, const ImageEx& background, AnimationSaver& anim ){
			//Initialize aligner
			AverageAligner render( aligner.get_method(), aligner.get_scale() );
			render.set_movement( aligner.get_movement() );
			for( int index : indexes )
				render.add_image( (ImageEx*)aligner.image( index ) ); //TODO: fix cast
			render.align(); //TODO: avoid having to realign. Add stuff to FakeAligner?
			
			//Render this frame
			ImageEx* img = SimpleRender().render( render );
			QImage raw = img->to_qimage( ImageEx::SYSTEM_REC709, ImageEx::SETTING_DITHER | ImageEx::SETTING_GAMMA );
			auto offset = aligner.find_offset( *background[0], *(*img)[0] );
			delete img;
			
			//Add it to the file
			int img_index = anim.addImage( offset.distance_x, offset.distance_y, raw );
			for( int index : indexes )
				anim.addFrame( index, img_index );
		}
};

double AnimatedAligner::find_threshold( const std::vector<int>& imgs ){
	vector<double> errors;
	set_raw( true ); //TODO: remove the need of this
	for( unsigned i=0; i<images.size()-1; ++i )
		errors.push_back( find_offset( *plane( i, 0 ), *plane( i+1, 0 ) ).error );
	set_raw( false );
	sort( errors.begin(), errors.end() );
	
	//avoid breaking at the end
	//TODO: pretty random... improve
	for( int i=0; i<errors.size()*0.1; i++ )
		errors.pop_back();
	
	
	double longest = 0;
	double threshold = 0; //TODO: high value?
	for( unsigned i=0; i<errors.size()-1; i++ ){
		double diff = errors[i+1] - errors[i];
		if( diff > longest ){
			longest = diff;
			threshold = diff / 2 + errors[i];
		}
	}
	
	debug::CsvFile error_csv( "AnimatedAligner-Raw/errors.csv" );
	error_csv.add( "error" ).add( "threshold" ).stop();
	for( auto error : errors )
		error_csv.add( error ).add( threshold ).stop();
	
	return threshold;
}

#include "color.hpp"
void AnimatedAligner::align( AProcessWatcher* watcher ){
	if( count() == 0 )
		return;
	
	//Align and render the image
	AverageAligner average( method, scale );
	average.set_movement( get_movement() );
	average.set_edges( false );
	for( unsigned i=0; i<count(); i++ )
		average.add_image( (ImageEx*)image( i ) );
	average.align();
	ImageEx* average_render = SimpleRender().render( average );
	
	//Init
	std::vector<int> backlog;
	for( unsigned i=0; i<images.size(); i++ )
		backlog.push_back( i );
	
	/* Debug differences
	{
		debug::CsvFile cvs( "AnimatedAligner-Raw/animated.csv" );
		cvs.add( "Index" ).add( "diff1" ).stop();
		for( unsigned i=0; i<images.size(); i++ ){
			auto diff1 = average.find_offset( *average.plane( i, 0 ), *average.plane( (i+1)%images.size(), 0 ) );
			
			cvs.add( i )
				.	add( diff1.error )
				.	stop()
				;
		}
	}
	//*/
	
	/* Debug graph
	{
		debug::CsvFile graph( "AnimatedAligner-Raw/graph.csv" );
		graph.add("");
		for( unsigned i=0; i<images.size(); i++ )
			graph.add( to_string(i) );
		graph.stop();
		
		for( unsigned i=0; i<images.size(); i++ ){
			graph.add( to_string(i) );
			for( unsigned j=0; j<images.size(); j++ ){
				if( i == j )
					graph.add( 0.0 );
				else
					graph.add( find_offset( *plane( i,0 ), *plane( j,0 ) ).error );
			}
			graph.stop();
		}
	}
	//*/
	
	//double threshold = QInputDialog::getDouble( nullptr, "Specify threshold", "Threshold", 0.5, 0.0, 9999.0, 6 ) * color::WHITE;
	
	double threshold = find_threshold( backlog );
	
	AnimationSaver anim( "AnimatedAligner-Raw/package" );
	
	int iteration = 0;
		debug::CsvFile frame_error_csv( "AnimatedAligner-Raw/frames.csv" );//s.c_str() );
		frame_error_csv.add( "error" ).stop();
	while( true ){
		AnimFrame frame( average );
		//string s = QString("AnimatedAligner-Raw/frame %1.csv").arg( iteration ).toLocal8Bit().constData();
		
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
			frame.save( iteration++, *average_render, anim );
	}
	
	anim.write();
	
	delete average_render;
}

