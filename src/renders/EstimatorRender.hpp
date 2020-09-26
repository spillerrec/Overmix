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
#include "../planes/Plane.hpp"

namespace Overmix{

struct Parameters;

struct EstimatorPara{
	int iterations{ 150 };
	double beta{ 1.3/255 };
	double lambda{ 0.1 };
	double alpha{ 0.7 };
	int reg_size{ 7 };
};

class EstimatorRender : public ARender{
	private:
		//Estimation parameters
		int iterations{ 150 };
		double beta{ 1.3/255 };
		double lambda{ 0.1 };
		double alpha{ 0.7 };
		int reg_size{ 7 };
		
		//Model parameters
		ScalingFunction scale_method{ ScalingFunction::SCALE_CATROM };
		Point<double> upscale_factor;
		double bluring{ 0.0 };
		
		Plane degrade( const Plane& original, const Parameters& para ) const;

	public:
		explicit EstimatorRender( double upscale_factor, const EstimatorPara& para={} )
			:	iterations(para.iterations)
			,	beta      (para.beta)
			,	lambda    (para.lambda)
			,	alpha     (para.alpha)
			,	reg_size  (para.reg_size)
			,	upscale_factor(upscale_factor,upscale_factor)
			{ }
		virtual ImageEx render( const AContainer& group, AProcessWatcher* watcher=nullptr ) const override;
};

}

#endif
