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

#include "ImageLoader.hpp"

#include "../Deteleciner.hpp"
#include "../planes/ImageEx.hpp"
#include "../renders/ARender.hpp"
#include "../containers/ImageContainer.hpp"
#include "../containers/ImageContainerSaver.hpp"

#include <QStringList>
#include <QtConcurrent>
#include <QFutureWatcher>
#include <QCoreApplication>

using namespace std;
using namespace Overmix;

const vector<ImageLoader::Item>& ImageLoader::loadAll( AProcessWatcher* watcher ){
	// Set-up watcher
	ProgressWrapper(watcher).setTotal( images.size() );
	QFutureWatcher<void> future_watcher;
	QObject::connect( &future_watcher, &QFutureWatcher<void>::progressValueChanged
		,	[&](int val){ ProgressWrapper(watcher).setCurrent( val ); } );
	
	// Start loading images
	auto future = QtConcurrent::map( images, []( Item& item ){
			item.second = ImageEx::fromFile( item.first );
		} );
	future_watcher.setFuture( future );
	
	//Wait for completion and handle all events
	while( !future.isFinished() )
		QCoreApplication::processEvents( QEventLoop::AllEvents, 100 );
	return images;
}

vector<ImageEx> ImageLoader::loadImages( QStringList list, AProcessWatcher* watcher ){
	std::vector<ImageEx> cache( list.count() );
	ImageLoader loader( list.count() );
	
	for( unsigned i=0; i<cache.size(); i++ )
		loader.add( list[i], cache[i] );
	loader.loadAll(watcher);
	
	return cache;
}

void ImageLoader::loadImages( QStringList files, ImageContainer& container, Deteleciner& detelecine, int alpha_mask, AProcessWatcher* watcher ){
	auto cache = loadImages( files, watcher );
	container.prepareAdds( files.count() );
	
	for( unsigned i=0; i<cache.size(); i++ ){ //TODO: Show process
		auto file = files[i];
		
		if( QFileInfo( file ).completeSuffix() == "xml.overmix" )
			ImageContainerSaver::load( container, file );
		else{
			auto& img = cache[i];
			
			//De-telecine
			if( detelecine.isActive() ){
				img = detelecine.process( img );
				file = ""; //The result might be a combination of several files
			}
			if( !img.is_valid() )
				continue;
			
			container.addImage( std::move( img ), alpha_mask, -1, file );
		}
	}
}

