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
#include "ui_VideoImporter.h"

#include "utils/AProcessWatcher.hpp"
#include "video/VideoStream.hpp"
#include "video/VideoFrame.hpp"
#include "containers/ImageContainer.hpp"
#include <QFileInfo>

using namespace Overmix;

VideoImporter::VideoImporter( QString filepath, QWidget* parent )
	:	QDialog( parent ), ui( new Ui_Dialog ), filepath( filepath )
	{
	ui->setupUi( this );
}


bool VideoImporter::supportedFile( QString filename ){
	auto ext = QFileInfo( filename ).suffix().toLower();
	//TODO: multiple extesions
	return ext == "mkv" || ext == "mp4" || ext == "webm";
}

void VideoImporter::import( ImageContainer &files, AProcessWatcher* watcher ){
	VideoStream video( filepath );
	video.seek( ui->offset_min->value()*60 + ui->offset_sec->value() );
	
	auto amount = ui->frames_amount->value();
	Progress progress( "VideoImporter", amount, watcher, [&](int){
			files.addImage( video.getFrame().toImageEx() );
		});
}

