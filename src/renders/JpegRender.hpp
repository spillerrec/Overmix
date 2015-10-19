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

#ifndef JPEG_RENDER_HPP
#define JPEG_RENDER_HPP


#include "ARender.hpp"
#include "../degraders/JpegDegrader.hpp"
#include <QString>

namespace Overmix{

struct Parameters;
class Plane;
class ImageEx;

class JpegRender : public ARender{
	private:
		//Estimation parameters
		int iterations;
		
		//Model parameters
		JpegDegrader jpeg;
		
		Plane degrade( const Plane& original, const Parameters& para ) const;

	public:
		JpegRender( QString path, int iterations=300 );
		virtual ImageEx render( const AContainer& group, AProcessWatcher* watcher=nullptr ) const override;
};

}

#endif
