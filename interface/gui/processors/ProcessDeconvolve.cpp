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


#include "ProcessDeconvolve.hpp"

#include <QSpinBox>
#include "../Spinbox2D.hpp"
#include "planes/ImageEx.hpp"

using namespace Overmix;

ProcessDeconvolve::ProcessDeconvolve( QWidget* parent ) : AProcessor( parent ){
	deviation  = newItem<DoubleSpinbox2D>( "Deviation"  );
	iterations = newItem<QSpinBox       >( "Iterations" );
	
	//Configure
	deviation->setValue( {1.0, 1.0} );
	deviation->setSingleStep( 0.001 );
	deviation->call( &QDoubleSpinBox::setDecimals, 3 );
	deviation->call( &QDoubleSpinBox::setRange, 0.000, 128.0 );
	iterations->setRange( 0, 1000 );
	iterations->setSingleStep( 5 );
}

QString ProcessDeconvolve::name() const{ return "Deconvolve"; }

bool ProcessDeconvolve::modifiesImage() const{
	return deviation->getValue() != Point<double>( 0.0, 0.0 ) && iterations->value() > 0;
}

ImageEx ProcessDeconvolve::process( const ImageEx& input ) const{
	return input.deconvolve_rl( deviation->getValue(), iterations->value() );
}