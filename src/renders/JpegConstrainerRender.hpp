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

#ifndef JPEG_CONTARINER_RENDER_HPP
#define JPEG_CONTARINER_RENDER_HPP


#include "ARender.hpp"
#include "../degraders/JpegDegrader.hpp"
#include "../planes/JpegImage.hpp"
#include <QString>

namespace Overmix{

class ImageEx;

class JpegConstrainerRender : public ARender{
	private:
		//Model parameters
        JpegImage jpeg;
		JpegDegrader degrader;
        
	public:
		explicit JpegConstrainerRender( QString path );
		virtual ImageEx render( const AContainer& group, AProcessWatcher* watcher=nullptr ) const override;
};

}

#endif
