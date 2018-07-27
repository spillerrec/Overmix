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
	QString name;
};

static const ScalingFunc scale_mapping[] = {
		{ ScalingFunction::SCALE_NEAREST,   QString("Nearest") }
	,	{ ScalingFunction::SCALE_LINEAR,    QString("Linear") }
	,	{ ScalingFunction::SCALE_CATROM,    QString("Catmull-Rom (cubic)") }
	,	{ ScalingFunction::SCALE_MITCHELL,  QString("Mitchell-Netravali (cubic)") }
	,	{ ScalingFunction::SCALE_SPLINE,    QString("Spline (cubic)") }
	,	{ ScalingFunction::SCALE_LANCZOS_3, QString("Lanczos 3-lopes (sinc)") }
	,	{ ScalingFunction::SCALE_LANCZOS_5, QString("Lanczos 5-lopes (sinc)") }
	,	{ ScalingFunction::SCALE_LANCZOS_7, QString("Lanczos 7-lopes (sinc)") }
};

ProcessScale::ProcessScale( QWidget* parent ) : AProcessor( parent ){
	method = newItem<QComboBox      >( "Method" );
	scale  = newItem<DoubleSpinbox2D>( "Scale"  );
	
	for( auto func : scale_mapping )
		method->addItem( func.name );
	
	//TODO: Configure
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
	return output;
}