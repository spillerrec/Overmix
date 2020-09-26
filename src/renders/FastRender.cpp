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

#include <iostream>
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
		
		const auto& current = aligner.plane(i);
		auto offset = aligner.pos(i) - min_point;
		
		auto writeArea = [&](int x, int y, int w, int h){
			for( int iy=y; iy<h; iy++ )
				for( int ix=x; ix<w; ix++ )
				{
					Point<int> dPos( ix, iy );
					out.setPixel( offset + dPos, current.pixel( dPos ) );
				}
		};
		
		Rectangle<int> img1(offset, current.getSize());
		Rectangle<int> img2(lastPos, lastSize);
		
		if( !img1.intersects(img2) ) //Write entire image
			writeArea(0,0, current.get_width(), current.get_height());
		else if( !img2.contains(img1) ){
			auto p1 = img1.pos     .max(img2.pos     ) - img1.pos;
			auto p2 = img1.endPos().min(img2.endPos()) - img1.pos;
			int w = current.get_width();
			int h = current.get_height();
			
			writeArea(   0,    0,   p1.x, p1.y);
			writeArea(p1.x,    0,   p2.x, p1.y);
			writeArea(p2.x,    0,      w, p1.y);
			
			writeArea(   0, p1.y,   p1.x, p2.y);
			writeArea(p2.x, p1.y,      w, p2.y);
			
			writeArea(   0, p2.y,   p1.x,    h);
			writeArea(p1.x, p2.y,   p2.x,    h);
			writeArea(p2.x, p2.y,   w,       h);
		}
		
		
		lastSize = current.getSize();
		lastPos = offset;
		progress.add();
	}
	
	return ImageEx( std::move(out) );
}



