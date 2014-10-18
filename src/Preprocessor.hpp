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

#ifndef PREPROCESSOR_HPP
#define PREPROCESSOR_HPP

#include "Geometry.hpp"
#include "ImageEx.hpp"

#include <QString>
#include <vector>

class Preprocessor{
	private:
		//Filters
		//TODO: detelecine
		
	public:
		//Crop
		unsigned crop_left{ 0 }, crop_top{ 0 }, crop_bottom{ 0 }, crop_right{ 0 };
		
		//Deconvolve
		double deviation{ 0.0 };
		unsigned dev_iterations{ 0 };
		
		//Scaling
		//TODO: method
		double scale_x{ 1.0 };
		double scale_y{ 1.0 };
		bool scale_chroma{ false };
		
	public:
		void processFile( ImageEx& img );
		
};

#endif