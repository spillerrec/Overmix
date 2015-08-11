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

#include "JpegRender.hpp"
#include "../planes/Plane.hpp"
#include "../color.hpp"
#include "AverageRender.hpp"
#include "../planes/ImageEx.hpp"
#include "../containers/AContainer.hpp"
#include "../containers/ImageContainer.hpp"
#include <QDebug>
#include <QImage>


using namespace std;

static Plane save( Plane p, QString name ){
	ImageEx( p ).to_qimage( ImageEx::SYSTEM_KEEP ).save( name + ".png" );
	return p;
}

static Point<double> channelScale( const AContainer& container, unsigned index, unsigned channel ){
	return container.image(index)[channel].getSize() / container.image(index).getSize().to<double>();
}

struct Parameters{
	const AContainer& container;
	unsigned index;
	unsigned channel;
	Point<double> min_point;
	
	Parameters( const AContainer& container, unsigned index, unsigned channel )
		: container(container), index(index), channel(channel)
			{ min_point = container.minPoint(); }
};

Plane JpegRender::degrade( const Plane& original, const Parameters& para ) const{
	Plane out( original );
	
	//Crop to the area overlapping with our current image
	auto pos = (para.container.pos(para.index)-para.min_point)*channelScale(para.container, para.index, para.channel);
	out.crop( pos, para.container.image(para.index)[para.channel].getSize() );
	
	return out;
}



ImageEx JpegRender::render(const AContainer &group, AProcessWatcher *watcher) const {
	auto planes_amount = 1;//group.image(0).size();
	ProgressWrapper( watcher ).setTotal( planes_amount * iterations * group.count() );
	
	//Work on a copy of the images
	ImageContainer imgs;
	imgs.prepareAdds( group.count() );
	for( unsigned j=0; j<group.count(); j++ )
		imgs.addImage( ImageEx{ group.image( j ) } );
	imgs.rebuildIndexes();
	for( unsigned j=0; j<group.count(); j++ )
		imgs.setPos( j, group.pos( j ) );
	
	
	for( int i=0; i<iterations; i++ ){
		auto est = AverageRender().render( imgs ); //Starting estimate
		for( unsigned c=0; c<planes_amount; ++c ){
			//Improve Jpeg image quality
			for( unsigned j=0; j<imgs.count(); j++, ProgressWrapper( watcher ).add() ){
				auto deg = degrade( est[c], {imgs, j, c} );
				auto lr = imgs.image(j)[c];
				imgs.imageRef(j)[c] = jpeg.planes[c].degradeComp( deg, lr );
			}
		}
	}

	return AverageRender().render( imgs );
}