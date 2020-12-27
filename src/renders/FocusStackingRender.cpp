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


#include "FocusStackingRender.hpp"
#include "../debug.hpp"

#include "../containers/AContainer.hpp"
#include "../planes/ImageEx.hpp"
#include "../utils/AProcessWatcher.hpp"
#include "../utils/PlaneUtils.hpp"

#include <QTime>
#include <QImage>
#include <vector>
using namespace std;
using namespace Overmix;

ImageEx diff( const ImageEx& img1, const ImageEx& img2 )
{
	ImageEx out(img1);
	for (int i=0; i<(int)out.size(); i++)
		out[i].substract( img2[i] );
	return out;
}


ImageEx FocusStackingRender::render( const AContainer& aligner, AProcessWatcher* watcher ) const{
	Timer t( "FocusStackingRender::render()" );
	
	auto makeError = [](const char* msg){
		qWarning( "%s", msg );
		return ImageEx();
	};
	
	//Abort if no images
	if( aligner.count() == 0 )
		return makeError( "No images to render!" );
	
	Progress progress( "FocusStackingRender", aligner.count() * 1, watcher );
	
	//Determine if we need to care about alpha per plane
// 	for( unsigned i=0; i<aligner.count(); ++i )
// 		if( aligner.alpha( i ) || aligner.imageMask( i ) >= 0 )
// 			return makeError( "Alpha not supported in FocusStacking" );
		
	//Check for movement in both direction
	auto movement = aligner.hasMovement();
	if( movement.first && movement.second )
		return makeError( "No movement support in FocusStacking yet" );
	
	
	std::vector<Plane> planes;
	for( unsigned i=0; i<aligner.count(); i++){
		if( progress.shouldCancel() )
			return {};
		
		auto inputImg = aligner.image(i);
		inputImg.to_grayscale();
		
		planes.push_back( inputImg[0].edge_sobel() );
		
		progress.add();
	}
	
	
	auto out = ImageEx(aligner.image(0));
	auto size = planes[0].getSize();
	Plane selected( size );
	Plane weight( size );
	
	for( int y=0; y<(int)size.height(); y++ )
		for( int x=0; x<(int)size.width(); x++ ){
			int best = 0;
			int id = 0;
			for( int i=0; i<(int)aligner.count(); i++ ){
				auto val = planes[i][y][x];
				if( val > best ){
					best = val;
					id = i;
				}
			}
			
			selected[y][x] = id;// * 256;
			weight[y][x] = best;
		}
		
	Plane selected_filter( selected );
	int k = kernel_size;
	for(int y=k; y<(int)size.height()-k; y++)
		for (int x=k; x<(int)size.width()-k; x++)
		{
			int best = 0;
			int id = 0;
			for( int i=-k; i<=k; i++ )
				for( int j=-k; j<=k; j++ ){
					auto val = weight[y+i][x+j];
					if( val > best ){
						best = val;
						id = selected[y+i][x+j];
					}
				}
			selected_filter[y][x] = id;
		}
	
	for( int y=0; y<(int)size.height(); y++ )
		for ( int x=0; x<(int)size.width(); x++ ){
			auto id = selected_filter[y][x];// / 256;
			auto& img = aligner.image(id);
			for( int c=0; c<(int)img.size(); c++ )
				out[c][y][x] = img[c][y][x];
		}
		
// 	ImageEx(Plane(selected)).to_qimage().save("selected.png");
// 	ImageEx(Plane(selected_filter)).to_qimage().save("selected_filter.png");
// 	ImageEx(Plane(weight)).to_qimage().save("weight.png");
	return out;
}



