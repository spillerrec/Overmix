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

#ifndef AVERAGE_RENDER_HPP
#define AVERAGE_RENDER_HPP

#include "ARender.hpp"

#include "../color.hpp"
#include "../planes/PlaneBase.hpp"

namespace Overmix{

class Plane;

class SumPlane {
	//NOTE: we could split this into two classes, specialized in handling precision alpha or not
	private:
		PlaneBase<precision_color_type> sum;
		PlaneBase<precision_color_type> amount;
		void resizeToFit( Point<>& pos, Size<> size );
	
	public:
		SumPlane() { }
		SumPlane( Size<> size ) : sum( size ), amount( size ){
			sum.fill( 0 );
			amount.fill( 0 );
		}
		
		void addPlane( const Plane& p, Point<> pos );
		void addAlphaPlane( const Plane& p, const Plane& alpha, Point<> pos );
		
		Plane average() const;
		Plane alpha() const;
};

class AverageRender : public ARender{
	protected:
		bool upscale_chroma;
		bool for_merging;
		
	public:
		AverageRender( bool upscale_chroma=false, bool for_merging=false )
			:	upscale_chroma(upscale_chroma), for_merging(for_merging) { }
		
		virtual ImageEx render( const AContainer& group, AProcessWatcher* watcher=nullptr ) const override;
};

}

#endif