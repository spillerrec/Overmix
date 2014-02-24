/*
	This file is part of imgviewer.

	imgviewer is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	imgviewer is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with imgviewer.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "imageCache.h"
#include "colorManager.h"

#include <QSize>
#include <QImage>
#include <QTransform>
#include <QImageReader>
#include <QPainter>
#include <QTime>

colorManager* imageCache::manager = NULL;

void imageCache::init(){
	if( !manager )
		manager = new colorManager();
	
	profile = NULL;
	frames_loaded = 0;
	memory_size = 0;
	current_status = EMPTY;
}

void imageCache::reset(){
	if( profile )
		cmsCloseProfile( this->profile );
	profile = nullptr;
	frames.clear();
	frame_delays.clear();
	error_msgs.clear();
	frames_loaded = 0;
	memory_size = 0;
	current_status = EMPTY;
	emit info_loaded();
}

void imageCache::set_profile( cmsHPROFILE profile ){
	if( profile )
		cmsCloseProfile( this->profile );
	this->profile = profile;
	emit info_loaded();
}

void imageCache::set_info( unsigned total_frames, bool is_animated, int loops ){
	current_status = INFO_READY;
	animate = is_animated;
	frame_amount = total_frames;
	loop_amount = loops;
	emit info_loaded();
}

void imageCache::add_frame( QImage frame, unsigned delay ){
	frames_loaded++;
	frames.push_back( frame );
	frame_delays.push_back( delay );
	current_status = FRAMES_READY;
	
	if( frame_amount < frames_loaded ){
		frame_amount = frames_loaded;
		emit info_loaded();
	}
	emit frame_loaded( frames_loaded-1 );
}

void imageCache::set_fully_loaded(){
	current_status = LOADED;
}


