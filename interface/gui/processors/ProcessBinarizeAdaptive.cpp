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


#include "ProcessBinarizeAdaptive.hpp"

#include "planes/ImageEx.hpp"
#include "color.hpp"

#include <QSpinBox>

using namespace Overmix;


ProcessBinarizeAdaptive::ProcessBinarizeAdaptive( QWidget* parent ) : AProcessor( parent ){
	threshold = newItem<QSpinBox>( "Threshold" );
	size      = newItem<QSpinBox>( "Box size" );
	threshold->setRange( -128, 127 );
	size     ->setRange(    0, 255 );
	threshold->setValue( 0 );
	size     ->setValue( 5 );
}

QString ProcessBinarizeAdaptive::name() const{ return "Binarize (adaptive)"; }

ImageEx ProcessBinarizeAdaptive::process( const ImageEx& input ) const{
	auto threshold_8bit = color::from8bit( threshold->value() );
	auto amount = size->value();
	ImageEx temp( input );
	for( unsigned c=0; c<temp.size(); c++ )
		temp[c].binarize_adaptive( amount, threshold_8bit >> 6 ); //TODO: Why the bit shifting?
	return temp;
}