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
#include "FloatRender.hpp"
#include "AnimationSaver.hpp"

#include <QInputDialog>
#include <QDir>

#include <fstream>

#include "debug.hpp"

using namespace std;

Plane AnimatedAligner::prepare_plane( const Plane& p ){
	Plane prepared = AImageAligner::prepare_plane( p );
	if( use_edges ){
		if( prepared )
			return prepared.edge_sobel();
		else
			return p.edge_sobel();
	}
	else
		return prepared;
}


class AnimFrame{
	public:
		RecursiveAligner& aligner;
		std::vector<unsigned> indexes;
		
	public:
		AnimFrame( RecursiveAligner& aligner ) : aligner(aligner){ }
		void add_index( unsigned index ){ indexes.push_back( index ); }
		unsigned size() const{ return indexes.size(); }
		
		double find_error( unsigned index ){
			unsigned comp_index = indexes[ indexes.size()-1 ];
			aligner.set_raw( true );
			auto offset = aligner.find_offset( aligner.plane( comp_index, 0 ), aligner.plane( index, 0 ) );
			qDebug( "Offset: %f", offset.distance_y );
			aligner.set_raw( false );
			return offset.error;
		}
		
		void save( int frame, const ImageEx& background, AnimationSaver& anim ){
			//Initialize aligner
			RecursiveAligner render( aligner.get_method(), aligner.get_scale() );
			render.set_movement( aligner.get_movement() );
			for( int index : indexes )
				render.add_image( aligner.image( index ) );
			render.align(); //TODO: avoid having to realign. Add stuff to FakeAligner?
			
			//Find offset
			//TODO: optimize if scale == 1.0
			ImageEx img2 = FloatRender(aligner.x_scale(), aligner.y_scale()).render( render );
			auto offset = aligner.find_offset( background[0], img2[0] );
			
			//Render this frame
			ImageEx img = FloatRender().render( render );
			QImage raw = img.to_qimage( ImageEx::SYSTEM_REC709, ImageEx::SETTING_DITHER | ImageEx::SETTING_GAMMA );
			
			//Add it to the file
			int img_index = anim.addImage( raw );
			for( int index : indexes )
				anim.addFrame( offset.distance_x/aligner.get_scale(), offset.distance_y/aligner.get_scale(), index, img_index );
		}
};

double AnimatedAligner::find_threshold( const std::vector<int>& imgs ){
	vector<double> errors;
	set_raw( true ); //TODO: remove the need of this
	for( unsigned i=0; i<count()-1; ++i )
		errors.push_back( find_offset( plane( i ), plane( i+1 ) ).error );
	set_raw( false );
	
	auto errors2 = errors;
	sort( errors.begin(), errors.end() );
	
	//avoid breaking at the end
	//TODO: pretty random... improve
//	for( int i=0; i<errors.size()*0.1; i++ )
//		errors.pop_back();
	
	
	double longest = 0;
	double threshold = 0; //TODO: high value?
	
	/*
	for( unsigned i=0; i<errors.size()-1; i++ ){
		double diff = errors[i+1] / errors[i];
		if( diff > longest ){
			longest = diff;
			threshold = (errors[i+1] - errors[i]) / 2 + errors[i];
		}
	}
	/*/
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
	//*/
	
	debug::CsvFile error_csv( "AnimatedAligner/package/errors.csv" );
	error_csv.add( "errors2" ).add( "errors" ).add( "threshold" ).add( "old" ).add( "new" ).stop();
	for( unsigned i=0; i<errors.size(); i++ ){
		error_csv.add( errors2[i] );
		error_csv.add( errors[i] );
		error_csv.add( threshold );
		error_csv.add( i>0 ? errors[i] - errors[i-1] : 0.0 );
		error_csv.add( i>0 && errors[i-1] != 0.0 ? errors[i]/(double)errors[i-1] : 0.0 );
		error_csv.stop();
	}
	
	return threshold;
}

#include "color.hpp"
void AnimatedAligner::align( AProcessWatcher* watcher ){
	if( count() == 0 )
		return;
	set_edges( false );
	
	//Align and render the image
	RecursiveAligner average( method, scale );
	average.set_movement( get_movement() );
	average.set_edges( false );
	for( unsigned i=0; i<count(); i++ )
		average.add_image( image( i ) );
	average.align();
	ImageEx average_render = FloatRender( average.x_scale(), average.y_scale() ).render( average );
	average_render.to_qimage( ImageEx::SYSTEM_REC709, ImageEx::SETTING_DITHER | ImageEx::SETTING_GAMMA ).save("AnimatedAligner/background.png" );
	
	//Init
	std::vector<int> backlog;
	for( unsigned i=0; i<count(); i++ )
		backlog.push_back( i );
	
	/* Debug differences
	{
		debug::CsvFile cvs( "AnimatedAligner-Raw/animated.csv" );
		cvs.add( "Index" ).add( "diff1" ).stop();
		for( unsigned i=0; i<count(); i++ ){
			auto diff1 = average.find_offset( average.plane( i, 0 ), average.plane( (i+1)%count(), 0 ) );
			
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
		for( unsigned i=0; i<count(); i++ )
			graph.add( to_string(i) );
		graph.stop();
		
		for( unsigned i=0; i<count(); i++ ){
			graph.add( to_string(i) );
			for( unsigned j=0; j<count(); j++ ){
				if( i == j )
					graph.add( 0.0 );
				else
					graph.add( find_offset( plane( i,0 ), plane( j,0 ) ).error );
			}
			graph.stop();
		}
	}
	//*/
	
	double factor = 1.0;//QInputDialog::getDouble( nullptr, "Specify threshold", "Threshold", 1.0, 0.01, 9.99, 2 );
	
	QDir( "." ).mkdir( "AnimatedAligner" );
	AnimationSaver anim( "AnimatedAligner/package" );
	
	double threshold = find_threshold( backlog ) * factor;
	
	int iteration = 0;
		debug::CsvFile frame_error_csv( "AnimatedAligner/package/frames.csv" );//s.c_str() );
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
			frame.save( iteration++, average_render, anim );
	}
	
	anim.write();
}

