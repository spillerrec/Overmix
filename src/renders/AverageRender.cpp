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
#include "../color.hpp"
#include "../planes/PlaneBase.hpp"


#include "../aligners/AImageAligner.hpp"
#include "../planes/ImageEx.hpp"

#include <QTime>
#include <vector>
using namespace std;


class ScaledPlane{
	private:
		Plane scaled;
		const Plane& original;
		
	public:
		ScaledPlane( const Plane& p, Size<unsigned> size ) : original( p ){
			if( p.getSize() != size )
				scaled = p.scale_cubic( size );
		}
		
		ScaledPlane( const Plane& p, const Plane& wanted_size )
			: ScaledPlane( p, wanted_size.getSize() ) { }
		
		const Plane& operator()() const{ return scaled.valid() ? scaled : original; }
};

class SumPlane {
	//NOTE: we could split this into two classes, specialized in handling precision alpha or not
	private:
		PlaneBase<precision_color_type> sum;
		PlaneBase<precision_color_type> amount;
	
	public:
		SumPlane( Size<> size ) : sum( size ), amount( size ){
			sum.fill( 0 );
			amount.fill( 0 );
		}
		
		void addPlane( const Plane& p, Point<> pos ){
			//TODO: make multi-threaded //NOTE: haven't been working out too well...
			for( unsigned iy=0; iy<p.get_height(); iy++ ){
				//Add to sum
				auto in = p.scan_line( iy );
				auto out =  sum.scan_line( iy + pos.y ) + pos.x;
				auto a = amount.scan_line( iy + pos.y ) + pos.x;
				for( unsigned ix=0; ix<p.get_width(); ix++ ){
					out[ix] += in[ix];
					a[ix] += color::WHITE;
				}
			}
		}
		
		void addAlphaPlane( const Plane& p, const Plane& alpha, Point<> pos ){
			//Scale alpha if needed
			ScaledPlane alpha_scaled( alpha, p );
			
			for( unsigned iy=0; iy<p.get_height(); iy++ ){
				//Add to sum
				auto in = p.const_scan_line( iy );
				auto out = sum.scan_line( iy + pos.y ) + pos.x;
				auto a_in = alpha_scaled().const_scan_line( iy );
				auto a_out = amount.scan_line( iy + pos.y ) + pos.x;
				
				for( unsigned ix=0; ix<p.get_width(); ix++ ){
					auto a_val = a_in[ix];
					out[ix] += in[ix] * color::asDouble( a_val );
					a_out[ix] += a_val;
				}
			}
		}
		
		Plane average() const{
			Plane avg( sum.getSize() );
			
			for( unsigned iy=0; iy<avg.get_height(); iy++ ){
				auto sums = sum.const_scan_line( iy );
				auto amounts = amount.const_scan_line( iy );
				auto out = avg.scan_line( iy );
				
				//Calculate the average for each point using sum and amount
				for( unsigned ix=0; ix<avg.get_width(); ix++ )
					if( amounts[ix] != 0 )
						out[ix] = sums[ix] / ((double)amounts[ix] / color::WHITE);
					else
						out[ix] = color::BLACK;
			}
			
			return avg;
		}
		
		Plane alpha() const{
			Plane alpha( amount.getSize() );
			
			for( unsigned iy=0; iy<alpha.get_height(); iy++ ){
				auto amounts = amount.const_scan_line( iy );
				auto out = alpha.scan_line( iy );
				
				//Set to transparent if nothing have been added here
				for( unsigned ix=0; ix<alpha.get_width(); ix++ )
					out[ix] = amounts[ix] != 0 ? color::WHITE : color::BLACK;
			}
			
			return alpha;
		}
};

class AlphaScales{
	private:
		vector<vector<ScaledPlane>> items;
		
	public:
		void addScale( const AContainer& aligner, Point<double> scale ){
			vector<ScaledPlane> scales;
			for( unsigned i=0; i<aligner.maskCount(); i++ ){
				auto& mask = aligner.mask( i );
				scales.emplace_back( mask, (mask.getSize() * scale).round() );
			}
			items.emplace_back( scales );
		}
		
		const Plane& getAlpha( int channel, int mask, const Plane& fallback )
			{ return ( mask < 0 ) ? fallback : items[channel][mask](); }
};

ImageEx AverageRender::render( const AContainer& aligner, AProcessWatcher* watcher ) const{
	//Abort if no images
	if( aligner.count() == 0 ){
		qWarning( "No images to render!" );
		return ImageEx();
	}
	unsigned planes_amount = (for_merging || aligner.image(0).get_system() == ImageEx::GRAY) ? 1 : 3;
	
	//Determine if we need to care about alpha per plane
	bool use_plane_alpha = false;
	for( unsigned i=0; i<aligner.count(); ++i )
		if( aligner.alpha( i ) || aligner.imageMask( i ) >= 0 ){
			use_plane_alpha = true;
			break;
		}
	
	ImageEx img( (planes_amount==1) ? ImageEx::GRAY : aligner.image(0).get_system() );
	
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
		
		for( unsigned j=0; j<aligner.count(); j++ ){
			auto& image = aligner.image( j );
			auto pos = (scale * (aligner.pos(j) - min_point)).round();
			ScaledPlane plane( image[c], (scale * image[0].getSize()).round() );
			
			const Plane& alpha_plane = masks.getAlpha( c, aligner.imageMask( j ), aligner.alpha( j ) );
			if( use_plane_alpha && alpha_plane.valid() )
				sum.addAlphaPlane( plane(), alpha_plane, pos );
			else
				sum.addPlane( plane(), pos );
		}
		
		img.addPlane( sum.average() );
		
		if( c == 0 && use_plane_alpha )
			img.alpha_plane() = sum.alpha();
			//TODO: what to do about the rest? We should try to fill in the gaps?
	}
	
	return img;
}



