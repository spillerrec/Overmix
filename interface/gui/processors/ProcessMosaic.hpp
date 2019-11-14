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

#ifndef PROCESS_Mosaic_HPP
#define PROCESS_Mosaic_HPP

#include "AProcessor.hpp"

namespace Overmix{

class ProcessMosaic : public AProcessor{
	Q_OBJECT
	
	public:
		ProcessMosaic( QWidget* parent );
		
		QString name() const override;
		ImageEx process( const ImageEx& img ) const override;
};

}

#endif

