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

#ifndef ESTIMATOR_RENDER_HPP
#define ESTIMATOR_RENDER_HPP


#include "ARender.hpp"
class Plane;

class EstimatorRender : public ARender{
	private:
		int upscale_factor;
		int iterations{ 150 };
		
		Plane degrade( const Plane& original, const AContainer &group, int channel ) const;

	public:
		EstimatorRender( int upscale_factor ) : upscale_factor(upscale_factor) { }
		virtual ImageEx render( const AContainer& group, AProcessWatcher* watcher=nullptr ) const override;
};


#endif
