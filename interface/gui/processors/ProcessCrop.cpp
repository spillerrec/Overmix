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


#include "ProcessCrop.hpp"

#include "planes/ImageEx.hpp"

#include <QSpinBox>

using namespace Overmix;


ProcessCrop::ProcessCrop( QWidget* parent ) : AProcessor( parent ){
	left   = newItem<QSpinBox>( "Left" );
	top    = newItem<QSpinBox>( "Top" );
	right  = newItem<QSpinBox>( "Right" );
	bottom = newItem<QSpinBox>( "Bottom" );
}

QString ProcessCrop::name() const{ return "Crop"; }

Point<double> ProcessCrop::modifyOffset( Point<double> ) const
	{ return { double(left->value()), double(top->value()) }; }

ImageEx ProcessCrop::process( const ImageEx& input ) const{
	ImageEx temp( input );
	temp.crop(
			left  ->value()
		,	top   ->value()
		,	right ->value()
		,	bottom->value()
		);
	return temp;
}