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

static QString numberZeroFill( int number, int digits ){
	QString str = QString::number( number );
	while( str.count() < digits )
		str = "0" + str;
	return str;
}

class AnimFrame{
	public:
		const AverageAligner& aligner;
		std::vector<unsigned> indexes;
		
	public:
		AnimFrame( const AverageAligner& aligner ) : aligner(aligner){ }
		void add_index( unsigned index ){ indexes.push_back( index ); }
		unsigned size() const{ return indexes.size(); }
		
		double find_error( unsigned index ){
			unsigned comp_index = indexes[ indexes.size()-1 ];
			auto offset = aligner.find_offset( *aligner.plane( comp_index, 0 ), *aligner.plane( index, 0 ) );
			return offset.error;
		}
		
		void save( int frame, const ImageEx& background ){
			//Initialize aligner
			AverageAligner render( aligner.get_method(), aligner.get_scale() );
			for( int index : indexes )
				render.add_image( (ImageEx*)aligner.image( index ) ); //TODO: fix cast
			render.align(); //TODO: avoid having to realign. Add stuff to FakeAligner?
			
			//Render this frame
			ImageEx* img = SimpleRender().render( render );
			ImageEx merged( background );
			auto offset = aligner.find_offset( *merged[0], *(*img)[0] );
			//QPoint pos = ( aligner.pos( indexes[indexes.size()-1] ) - aligner.pos(aligner.count()-1) ).toPoint();
			merged[0]->copy( offset.distance_x, offset.distance_y, *(*img)[0] );
			merged[1]->copy( offset.distance_x, offset.distance_y, *(*img)[1] );
			merged[2]->copy( offset.distance_x, offset.distance_y, *(*img)[2] );
			QImage converted = merged.to_qimage( ImageEx::SYSTEM_REC709, ImageEx::SETTING_DITHER | ImageEx::SETTING_GAMMA );
			QImage raw = img->to_qimage( ImageEx::SYSTEM_REC709, ImageEx::SETTING_DITHER | ImageEx::SETTING_GAMMA );
			delete img;
			
			//Save all the frames
			raw.save( "AnimatedAligner-Raw/Frame (" + numberZeroFill( frame+1, 4 ) + ").png" );
			for( int index : indexes )
				converted.save( "AnimatedAligner/Frame " + numberZeroFill( index+1, 4 ) + " (" + numberZeroFill( frame+1, 2 ) + ").png" );
		}
};

void AnimatedAligner::align( AProcessWatcher* watcher ){
	if( count() == 0 )
		return;
	
	//Align and render the image
	AverageAligner average( method, scale );
	for( unsigned i=0; i<count(); i++ )
		average.add_image( (ImageEx*)image( i ) );
	average.align();
	ImageEx* average_render = SimpleRender().render( average );
	
	//Init
	std::vector<int> backlog;
	for( unsigned i=0; i<images.size(); i++ )
		backlog.push_back( i );
	
	int iteration = 0;
	while( true ){
		AverageAligner aligner( method, scale );
		AnimFrame frame( average );
		
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
				if( error < 225 ){
					frame.add_index( index );
					index = -1;
				}
			}
		}
		
		//Stop if no images
		if( frame.size() == 0 )
			break;
		else
			frame.save( iteration++, *average_render );
	}
	
	delete average_render;
}

