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
#include "../debug.hpp"
#include "AverageRender.hpp"
#include "../planes/ImageEx.hpp"
#include "../containers/AContainer.hpp"
#include "../containers/ImageContainer.hpp"
#include <QImage>


using namespace std;

static Plane save( Plane p, QString name ){
	ImageEx( p ).to_qimage().save( name + ".png" );
	return p;
}

static Point<double> channelScale( const AContainer& container, unsigned index, unsigned channel ){
	return container.image(index)[channel].getSize().to<double>() / container.image(index).getSize().to<double>();
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
	out.crop( pos.round(), para.container.image(para.index)[para.channel].getSize() );
	
	return out;
}

JpegRender::JpegRender( QString path, int iterations ) : iterations(iterations)
	{ jpeg = ImageEx::getJpegDegrader( path ); }

ImageEx JpegRender::render(const AContainer &group, AProcessWatcher *watcher) const {
	auto planes_amount = 1u;//group.image(0).size();
	ProgressWrapper( watcher ).setTotal( planes_amount * iterations * group.count() );
	
	//Work on a copy of the images
	ImageContainer imgs;
	imgs.prepareAdds( group.count() );
	for( unsigned j=0; j<group.count(); j++ )
		imgs.addImage( ImageEx{ group.image( j ) } );
	imgs.rebuildIndexes();
	for( unsigned j=0; j<group.count(); j++ )
		imgs.setPos( j, group.pos( j ) );
	
	//* Waifu test code
	Plane mask( 853, 480 );
	mask.fill( 1 );
	for( unsigned j=0; j<group.count(); j++ ){
		auto waifu = ImageEx::fromFile( "out-25/waifu/0" + QString::number(j) + ".png" );
		auto deg = deVlcImage( waifu )[0];
		auto lr = imgs.image(j)[0];
		save( lr, "input" + QString::number(j) + "lr" );
		unsigned change = 0;
		imgs.imageRef(j)[0] = jpeg.planes[0].degradeComp( mask, deg, lr, change );
		save( imgs.imageRef(j)[0], "input" + QString::number(j) + "waifu" );
	}//*/
	
	//auto truth = ImageEx::fromFile( "truth.dump" );
	//auto waifu = ImageEx::fromFile( "out-25/_waifu.png" );
	//waifu = deVlcImage( waifu );
	//save( waifu[0], "waifu" );
	auto est = AverageRender().render( imgs ); //Starting estimate
	//save( est[0], "waifu-overmix" );
	//est[0] = est[0].deconvolve_rl( 0.6, 5 );
	auto diff = est;
	for( unsigned i=0; i<diff.size(); i++ )
		diff[i].fill( 1 );
	//est = waifu;
	
	for( int i=0; i<iterations; i++ ){
		//save( est[0], "Est" + QString::number(i) );
		for( unsigned c=0; c<planes_amount; ++c ){
			//Improve Jpeg image quality
				unsigned change = 0;
			for( unsigned j=0; j<imgs.count(); j++, ProgressWrapper( watcher ).add() ){
				auto deg  = degrade(  est[c], {imgs, j, c} );
				auto mask = degrade( diff[c], {imgs, j, c} );
				auto lr = imgs.image(j)[c];
				imgs.imageRef(j)[c] = jpeg.planes[c].degradeComp( mask, deg, lr, change );
			}
			qCDebug(LogDelta) << "Change: " << change / imgs.count();
			if( change == 0 )
				break;
		}
		
		auto new_est = AverageRender().render( imgs );
		for( unsigned c=0; c<diff.size(); c++ ){
			diff[c] = new_est[c];
			diff[c].difference( est[c] );
			diff[c].binarize_threshold( 0 );
			
			if( iterations % 50 == 0 )
				diff[c].fill( 1 );
		}
	//	auto copy = diff[0];
	//	copy.binarize_threshold( 0 );
	//	save( copy, "diff-it" + QString::number(i) );
		est = new_est;
	}

	return est;
}