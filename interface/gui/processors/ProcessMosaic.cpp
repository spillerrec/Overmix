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


#include "ProcessMosaic.hpp"

#include "planes/ImageEx.hpp"
#include "planes/manipulators/Mosaic.hpp"

using namespace Overmix;

ProcessMosaic::ProcessMosaic( QWidget* parent ) : AProcessor( parent ){ }

QString ProcessMosaic::name() const{ return "Mosaic detection"; }

ImageEx ProcessMosaic::process( const ImageEx& input ) const{
	return Mosaic::detect( input.toRgb() );
}
