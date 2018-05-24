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


#include "AverageRender.hpp"
#include "../debug.hpp"

#include "../containers/AContainer.hpp"
#include "../planes/ImageEx.hpp"
#include "../utils/AProcessWatcher.hpp"
#include "../utils/PlaneUtils.hpp"

#include <QTime>
#include <vector>
using namespace std;
using namespace Overmix;

template<typename T>
PlaneBase<T> resizePlaneToFit( const PlaneBase<T>& input, Size<> size, Point<> pos ){
	PlaneBase<T> output( size );
	output.fill( 0 );
	output.copy( input, {0u,0u}, input.getSize(), pos );
	return output;
}

void SumPlane::resizeToFit( Point<double>& pos, Size<> size ){
	auto current_size = sum.getSize();
	auto offset = pos.min( Point<>( 0,0 ) ); //This will never be positive
	auto new_size = current_size.max( current_size - offset );
	new_size = new_size.max( pos + size );
	
	//Adjust to new position
	pos -= offset;
	
	//Don't do anything if size hasn't changed
	if( new_size == sum.getSize() )
		return;
	
	sum    = resizePlaneToFit( sum,    new_size, Point<>(0,0)-offset );
	amount = resizePlaneToFit( amount, new_size, Point<>(0,0)-offset );
}

void SumPlane::addPlane( const Plane& p, Point<double> pos ){
	resizeToFit( pos, p.getSize() );
	//TODO: make multi-threaded //NOTE: haven't been working out too well...
	for( double iy=offset.y; iy<p.get_height(); iy += spacing.y ){
		//Add to sum
		auto in  = p     .scan_line(  iy         );
		auto out = sum   .scan_line(  iy + pos.y );
		auto a   = amount.scan_line( iy + pos.y );
		for( double ix=offset.x; ix<p.get_width(); ix += spacing.x ){
			out[ix+pos.x] += in[ix];
			  a[ix+pos.x] += color::WHITE;
		}
	}
}

void SumPlane::addAlphaPlane( const Plane& p, const Plane& alpha, Point<double> pos ){
	//Fallback
	if( !alpha.valid() ){
		addPlane( p, pos );
		return;
	}
	
	//Scale alpha if needed
	auto alpha_scaled = getScaled( alpha, p.getSize() );
	
	resizeToFit( pos, p.getSize() );
	for( double iy=offset.y; iy<p.get_height(); iy += spacing.y ){
		//Add to sum
		auto in    = p             .scan_line( iy         );
		auto a_in  = alpha_scaled().scan_line( iy         );
		auto out   = sum           .scan_line( iy + pos.y );
		auto a_out = amount        .scan_line( iy + pos.y );
		
		for( double ix=offset.x; ix<p.get_width(); ix += spacing.x ){
			auto a_val = a_in[ix];
			  out[ix+pos.x] += in[ix] * color::asDouble( a_val );
			a_out[ix+pos.x] += a_val;
		}
	}
}

Plane SumPlane::average() const{
	Plane avg( sum.getSize() );
	
	for( unsigned iy=0; iy<avg.get_height(); iy++ ){
		auto sums    = sum   .scan_line( iy );
		auto amounts = amount.scan_line( iy );
		auto out     = avg   .scan_line( iy );
		
		//Calculate the average for each point using sum and amount
		for( unsigned ix=0; ix<avg.get_width(); ix++ )
			if( amounts[ix] != 0 )
				out[ix] = sums[ix] / ((double)amounts[ix] / color::WHITE);
			else
				out[ix] = color::BLACK;
	}
	
	return avg;
}

Plane SumPlane::alpha() const{
	Plane alpha( amount.getSize() );
	
	//Set to transparent if nothing have been added here
	for( unsigned iy=0; iy<alpha.get_height(); iy++ )
		for( auto val : makeZipRowIt( alpha.scan_line( iy ), amount.scan_line( iy ) ) )
			val.first = val.second != 0 ? color::WHITE : color::BLACK;
	
	return alpha;
}

class AlphaScales{
	private:
		vector<vector<ModifiedPlane>> items;
		
	public:
		void addScale( const AContainer& aligner, Point<double> scale ){
			vector<ModifiedPlane> scales;
			scales.reserve( aligner.maskCount() );
			for( unsigned i=0; i<aligner.maskCount(); i++ ){
				auto& mask = aligner.mask( i );
				scales.emplace_back( getScaled( mask, (mask.getSize() * scale).round() ) );
			}
			items.emplace_back( scales );
		}
		
		const Plane& getAlpha( int channel, int mask, const Plane& fallback )
			{ return ( mask < 0 ) ? fallback : items[channel][mask](); }
};

ImageEx AverageRender::render( const AContainer& aligner, AProcessWatcher* watcher ) const{
	Timer t( "AverageRender::render()" );
	
	//Abort if no images
	if( aligner.count() == 0 ){
		qWarning( "No images to render!" );
		return ImageEx();
	}
	unsigned planes_amount = for_merging ? 1 : aligner.image(0).size();
	ProgressWrapper( watcher ).setTotal( aligner.count() * planes_amount );
	
	//Determine if we need to care about alpha per plane
	bool use_plane_alpha = true;//false;
	for( unsigned i=0; i<aligner.count(); ++i )
		if( aligner.alpha( i ) || aligner.imageMask( i ) >= 0 ){
			use_plane_alpha = true;
			break;
		}
	//Check for movement in both direction
	auto movement = aligner.hasMovement();
	if( movement.first && movement.second )
		use_plane_alpha = true;
	
	auto color_space = aligner.image(0).getColorSpace();
	ImageEx img( for_merging ? color_space.changed( Transform::GRAY ) : color_space );
	
	AlphaScales masks;
	for( unsigned c=0; c<planes_amount; c++ ){
		auto scale = upscale_chroma ? Point<double>(1,1)
            : aligner.image( 0 )[c].getSize().to<double>() / aligner.image( 0 )[0].getSize().to<double>();
		masks.addScale( aligner, scale );
	}
	
	auto min_point = aligner.minPoint();
	auto full = aligner.size().size;
	for( unsigned c=0; c<planes_amount; c++ ){
		//Determine local size
		auto scale = upscale_chroma ? Point<double>(1,1)
			: aligner.image( 0 )[c].getSize().to<double>() / aligner.image( 0 )[0].getSize().to<double>();
		
		
		//TODO: something is wrong with the rounding, chroma-channels are slightly off
		SumPlane sum( (scale * full).ceil() );
		sum.spacing = spacing;
		sum.offset  = offset;
		
		for( auto align : aligner ){
			auto pos = scale * (align.pos() - min_point);
			auto plane = getScaled( align.image()[c], (scale * align.image()[0].getSize()).round() );
			
			const Plane& alpha_plane = masks.getAlpha( c, align.imageMask(), align.alpha() );
			if( use_plane_alpha && alpha_plane.valid() )
				sum.addAlphaPlane( plane(), alpha_plane, pos );
			else
				sum.addPlane( plane(), pos );
			
			ProgressWrapper( watcher ).add();
		}
		
		img.addPlane( sum.average() );
		
		if( c == 0 && use_plane_alpha )
			img.alpha_plane() = sum.alpha();
			//TODO: what to do about the rest? We should try to fill in the gaps?
	}
	
	return img;
}



