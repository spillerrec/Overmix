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


#include "FastRender.hpp"
#include "../debug.hpp"

#include "../containers/AContainer.hpp"
#include "../planes/ImageEx.hpp"
#include "../utils/AProcessWatcher.hpp"
#include "../utils/PlaneUtils.hpp"

#include <vector>
using namespace std;
using namespace Overmix;


ImageEx FastRender::render( const AContainer& aligner, AProcessWatcher* watcher ) const{
	Timer t( "FastRender::render()" );
	
	//Abort if no images
	if( aligner.count() == 0 ){
		qWarning( "No images to render!" );
		return ImageEx();
	}
	
	Progress progress( "FastRender", aligner.count(), watcher );
	
	Size<unsigned> lastSize;
	Point<int> lastPos;
	Plane out( aligner.size().size );
	auto min_point = aligner.minPoint();
	for( unsigned i=0; i<aligner.count(); i++ ){
		if( progress.shouldCancel() )
			return {};
		
		auto current = aligner.plane(i);
		auto offset = aligner.pos(i) - min_point;
		
		auto writeArea = [&](int x, int y, int w, int h){
			for( int iy=y; iy<h; iy++ )
				for( int ix=x; ix<w; ix++ )
				{
					Point<int> dPos( ix, iy );
					out.setPixel( offset + dPos, current.pixel( dPos ) );
				}
		};
		if( current.getSize() == lastSize )
		{
			//Write only new parts, this could be
			int w = current.get_width();
			int h = current.get_height();
			auto posDiff = (lastPos - offset).to<int>();
			
			auto startPos = posDiff.max( {0,0} );
			auto endPos = (posDiff + lastSize).min( {w,h} );
			writeArea(0,0, w, startPos.y);
			writeArea(0,startPos.y, startPos.x, endPos.y);
			writeArea(endPos.x,startPos.y, w, endPos.y);
			writeArea(0, endPos.y, w, h);
		}
		else //Write entire image
			writeArea(0,0, current.get_width(), current.get_height());
		
		lastSize = current.getSize();
		lastPos = offset;
		progress.add();
	}
	
	return ImageEx( out );
}



