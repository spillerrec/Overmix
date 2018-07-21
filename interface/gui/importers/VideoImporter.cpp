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


#include "VideoImporter.hpp"

#include "video/VideoStream.hpp"
#include "video/VideoFrame.hpp"
#include "containers/ImageContainer.hpp"
#include <QFileInfo>

using namespace Overmix;


bool VideoImporter::supportedFile( QString filename ){
	auto extension = QFileInfo( filename ).suffix();
	//TODO: multiple extesions
	return extension.toLower() == "mkv";
}

void VideoImporter::loadFile( QString filepath, ImageContainer &files ){
	VideoStream video( filepath );
	//TODO:
	
	for( int i=0; i<10; i++ ){
		auto frame = video.getFrame();
		files.addImage( frame.toImageEx() );
	}
}

