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


#include "ProcessWaifu.hpp"

#include <QSpinBox>
#include "planes/ImageEx.hpp"
#include "planes/manipulators/Waifu.hpp"
#include "color.hpp"

using namespace Overmix;

ProcessWaifu::ProcessWaifu( QWidget* parent ) : AProcessor( parent ){
	scale_amount  = newItem<QSpinBox>( "Scale"   );
	denoise_level = newItem<QSpinBox>( "Denoise" );
	
	scale_amount ->setRange( 1, 4 );
	denoise_level->setRange( 0, 2 );
}

QString ProcessWaifu::name() const{ return "Waifu2x"; }

ImageEx ProcessWaifu::process( const ImageEx& input ) const{
	auto scale   = scale_amount ->value();
	auto denoise = denoise_level->value();
	
	return Waifu( scale, denoise, nullptr ).process( input );
}
