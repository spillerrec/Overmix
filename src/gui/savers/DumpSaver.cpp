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


#include "DumpSaver.hpp"

#include "../../planes/ImageEx.hpp"

using namespace Overmix;

DumpSaver::DumpSaver( const ImageEx& image, QString filename ) : ASaver( image, filename ){
	setupUi(this);
	
	save_alpha->setChecked( image.alpha_plane() );
	save_alpha->setEnabled( false );//save_alpha->isChecked() );
	compression->setEnabled( false );
}


void DumpSaver::save( const ImageEx& image, QString filename ){
	image.saveDump( filename, color_depth->value() );
}


