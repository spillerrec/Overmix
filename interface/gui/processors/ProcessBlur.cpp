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


#include "ProcessBlur.hpp"

#include <QComboBox>
#include "../Spinbox2D.hpp"
#include "planes/ImageEx.hpp"

using namespace Overmix;

ProcessBlur::ProcessBlur( QWidget* parent ) : AProcessor( parent ){
	method = newItem<QComboBox>( "Method" );
	amount = newItem<Spinbox2D>( "Size"  );
	
	//Configure
	method->addItem( "Box" );
	method->addItem( "Gaussian" );
}

QString ProcessBlur::name() const{ return "Blur"; }

bool ProcessBlur::modifiesImage() const{
	return amount->getValue() != Point<double>( 0.0, 0.0 );
}

ImageEx ProcessBlur::process( const ImageEx& input ) const{	
	auto size = amount->getValue();
	switch( method->currentIndex() ){
		case 0: return input.copyApplyAll( true, &Plane::blur_box,      (unsigned)size.width(), (unsigned)size.height() ); break;
		case 1: return input.copyApplyAll( true, &Plane::blur_gaussian, (double)  size.width(), (double)  size.height() ); break;
		default: return input; //TODO: Error
	}
}