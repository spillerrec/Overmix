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


#include "ProcessBinarize.hpp"

#include "planes/ImageEx.hpp"
#include "color.hpp"

#include <QSpinBox>

using namespace Overmix;


ProcessBinarize::ProcessBinarize( QWidget* parent ) : AProcessor( parent ){
	threshold = newItem<QSpinBox>( "Threshold" );
	threshold->setRange( 0, 255 );
	threshold->setValue( 127 );
}

QString ProcessBinarize::name() const{ return "Binarize"; }

ImageEx ProcessBinarize::process( const ImageEx& input ) const{
	auto threshold_8bit = color::from8bit( threshold->value() );
	ImageEx temp( input );
	for( unsigned c=0; c<temp.size(); c++ )
		temp[c].binarize_threshold( threshold_8bit );
	return temp;
}