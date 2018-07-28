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


#include "ProcessScale.hpp"

#include <QComboBox>
#include <QFormLayout>
#include "../Spinbox2D.hpp"
#include "planes/Plane.hpp"
#include "planes/ImageEx.hpp"

using namespace Overmix;

struct ScalingFunc{
	ScalingFunction func;
	const char* const name;
};

static const ScalingFunc scale_mapping[] = {
		{ ScalingFunction::SCALE_NEAREST  , "Nearest"                    }
	,	{ ScalingFunction::SCALE_LINEAR   , "Linear"                     }
	,	{ ScalingFunction::SCALE_CATROM   , "Catmull-Rom (cubic)"        }
	,	{ ScalingFunction::SCALE_MITCHELL , "Mitchell-Netravali (cubic)" }
	,	{ ScalingFunction::SCALE_SPLINE   , "Spline (cubic)"             }
	,	{ ScalingFunction::SCALE_LANCZOS_3, "Lanczos 3-lopes (sinc)"     }
	,	{ ScalingFunction::SCALE_LANCZOS_5, "Lanczos 5-lopes (sinc)"     }
	,	{ ScalingFunction::SCALE_LANCZOS_7, "Lanczos 7-lopes (sinc)"     }
};

ProcessScale::ProcessScale( QWidget* parent ) : AProcessor( parent ){
	method = newItem<QComboBox      >( "Method" );
	scale  = newItem<DoubleSpinbox2D>( "Scale"  );
	
	for( auto func : scale_mapping )
		method->addItem( func.name );
	
	//Configure
	method->setCurrentIndex( 3 );
	scale->setValue( {1.0, 1.0} );
	scale->setSingleStep( 0.001 );
	scale->call( &QDoubleSpinBox::setDecimals, 3 );
	scale->call( &QDoubleSpinBox::setRange, 0.001,  32.0 );
}

QString ProcessScale::name() const{ return "Scale"; }

bool ProcessScale::modifiesImage() const{
	return scale->getValue() != Point<double>( 1.0, 1.0 );
}

ImageEx ProcessScale::process( const ImageEx& input ) const{
	//Get settings
	//TODO: assert max value of currentIndex
	auto func = scale_mapping[ method->currentIndex() ].func;
	auto size = scale->getValue();
	
	//Scale image
	ImageEx output( input );
	output.scale( (output.getSize() * size ).round(), func );
	//TODO: Only scale luma option?
	return output;
}