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

#include <QDir>

#include <fstream>

using namespace std;


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

int AnimationSaver::addImage( int x, int y, QImage img ){
	//Create thumbnail for first frame
	if( current_id == 1 )
		img.scaled( 256, 256, Qt::KeepAspectRatio )
			.save( folder + "/Thumbnails/thumbnail.jpg", nullptr, 95 );
	
	QString filename = "data/" + numberZeroFill( current_id++, 4 ) + ".png"; //TODO: allow file format to be changed?
	img.save( folder + "/" + filename );
	
	images.push_back( {x, y, filename} );
	return images.size() - 1;
}

void AnimationSaver::write(){
	//Make sure the frames are in the correct order
	sort( frames.begin(), frames.end(), [](pair<int,int> first, pair<int,int> second){
			return first.first < second.first;
		} );
	
	//TODO: Remove unneeded frames
	
	//Write the file
	ofstream file( (folder + "/stack.xml").toLocal8Bit().constData() );
	//TODO: get the actual size
	//TODO: add complete XML file
	
	//Write all the frames
	for( auto frame : frames ){
		file << "<stack><layer src=\"" << images[frame.second].name.toUtf8().constData() << "\" ";
		file << "x=\"" << images[frame.second].x << "\" ";
		file << "y=\"" << images[frame.second].y << "\"/></stack>\n";
		//TODO: add option to have average image as background
	}
	
	file.close();
}