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

#include "JpegConstrainerRender.hpp"
#include "../planes/Plane.hpp"
#include "../color.hpp"
#include "../debug.hpp"
#include "AverageRender.hpp"
#include "../planes/ImageEx.hpp"
#include "../containers/AContainer.hpp"
#include "../utils/AProcessWatcher.hpp"

#include <QFile>


using namespace Overmix;


JpegConstrainerRender::JpegConstrainerRender( QString path )
	:	degrader(ImageEx::getJpegDegrader( path ))
{
	QFile f( path );
	if( !f.open( QIODevice::ReadOnly ) )
		throw std::runtime_error( "Could not open file" );
	jpeg = from_jpeg( f );
}

ImageEx JpegConstrainerRender::render(const AContainer &group, AProcessWatcher *watcher) const {
	//Get the one image we want to constrain
	if( group.count() != 1 )
		throw std::runtime_error( "Can only be used on one image" );
	auto img = group.image(0);
	
	//TODO: Check color types
	
	
	Progress progress( "JpegConstrainerRender", img.size(), watcher );
	progress.loopAll( [&]( int c ){
			//Constrain it to the Coeff
			unsigned change=0;
			img[c] = degrader.planes[c].degradeFromJpegPlane( img[c], jpeg.planes[c], change );
		} );
	
	return img;
}
