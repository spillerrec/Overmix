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


#include "AnimationSaver.hpp"

#include "../ImageEx.hpp"
#include "../containers/FrameContainer.hpp"

#include <QDir>

#include <cmath>
#include <fstream>

using namespace std;

//TODO: not needed, can be done with QString directly
static QString numberZeroFill( int number, int digits ){
	QString str = QString::number( number );
	while( str.count() < digits )
		str = "0" + str;
	return str;
}

AnimationSaver::AnimationSaver( QString folder ) : folder(folder){
	//Create the folder
	QDir dir( folder );
	QString name = dir.dirName();
	dir.cdUp();
	dir.mkdir( name );
	dir.cd( name );
	
	//Create sub-folders for data
	dir.mkdir( "data" );
	dir.mkdir( "Thumbnails" );
	
	//Create mimetype file
	ofstream mime( (folder + "/mimetype" ).toLocal8Bit().constData() );
	mime << "image/openraster";
	mime.close();
}

bool AnimationSaver::removeUnneededFrames(){
	for( unsigned i=1; i<frames.size(); i++ ){
		auto& first = frames[i-1].second;
		auto& second = frames[i].second;
		if( first.image_id == second.image_id && first.x == second.x && first.y == second.y ){
			first.delay += second.delay;
			frames.erase( frames.begin() + i );
			return true;
		}
	}
	
	return false;
}

QSize AnimationSaver::normalize(){
	QPoint pos;
	for( auto frame : frames ){
		pos.setX( min( pos.x(), frame.second.x ) );
		pos.setY( min( pos.y(), frame.second.y ) );
	}
	
	for( auto& frame : frames ){
		frame.second.x -= pos.x();
		frame.second.y -= pos.y();
	}
	
	QSize size;
	for( auto& frame : frames ){
		auto image = images[frame.second.image_id];
		
		size.setWidth( max( size.width(), frame.second.x + image.size.width() ) );
		size.setHeight( max( size.height(), frame.second.y + image.size.height() ) );
	}
	return size;
}

int AnimationSaver::addImage( const ImageEx& img ){
	//Create thumbnail for first frame
	if( current_id == 1 ){
		ImageEx temp( img ); //TODO: fix const on to_qimage!
		QImage raw = temp.to_qimage( ImageEx::SYSTEM_REC709, ImageEx::SETTING_DITHER | ImageEx::SETTING_GAMMA );
		raw.scaled( 256, 256, Qt::KeepAspectRatio )
			.save( folder + "/Thumbnails/thumbnail.jpg", nullptr, 95 );
	}
	
	QString filename = "data/" + numberZeroFill( current_id++, 4 ) + ".dump"; //TODO: allow file format to be changed?
	img.saveDump( (folder + "/" + filename).toLocal8Bit().constData() );
	
	images.push_back( {filename, QSize( img.get_width(), img.get_height() )} );
	return images.size() - 1;
}

void AnimationSaver::write(){
	//Make sure the frames are in the correct order
	sort( frames.begin(), frames.end(), [](pair<int,FrameInfo> first, pair<int,FrameInfo> second){
			return first.first < second.first;
		} );
	
	//Remove unneeded frames
	while( removeUnneededFrames() );
	
	//Write the file
	ofstream file( (folder + "/stack.xml").toLocal8Bit().constData() );
	file << "<?xml version='1.0' encoding='UTF-8'?>\n";
	
	//Get the actual size, and avoid negative positions
	auto size = normalize();
	file << "<image w=\"" << size.width() << "\" h=\"" << size.height() << "\" loops=\"-1\">\n";
	
	//Write all the frames
	for( auto frame : frames ){
		auto image = frame.second;
		file << "<stack delay=\"" << image.delay << "\">";
		file << "<layer src=\"" << images[image.image_id].name.toUtf8().constData() << "\" ";
		file << "x=\"" << image.x << "\" ";
		file << "y=\"" << image.y << "\"/></stack>\n";
		//TODO: add option to have average image as background
	}
	
	file << "</image>";
	file.close();
}