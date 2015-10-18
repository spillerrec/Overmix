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


#include "RenderConfigs.hpp"

#include "renders/AnimRender.hpp" //TODO: how to handle?
#include "renders/AverageRender.hpp"
#include "renders/DiffRender.hpp"
#include "renders/FloatRender.hpp"
#include "renders/StatisticsRender.hpp"
#include "renders/PixelatorRender.hpp"
#include "renders/RobustSrRender.hpp"
#include "renders/EstimatorRender.hpp"
#include "renders/JpegRender.hpp"

#include <QComboBox>
#include <QVBoxLayout>
#include <QSpinBox>
#include <QLineEdit>

using namespace Overmix;


RenderConfigChooser::RenderConfigChooser( QWidget* parent, bool expand )
	: ConfigChooser<ARenderConfig>( parent, expand )
	{ connectChange( [&](int){ emit changed(); } ); }

void RenderConfigChooser::p_initialize(){
	auto set = [&]( auto config ){
		connect( config, SIGNAL(changed()), this, SIGNAL(changed()) );
	};
	
	set( &addConfig<AverageRenderConfig>() );
	set( &addConfig<DiffRenderConfig>() );
	set( &addConfig<FloatRenderConfig>() );
	set( &addConfig<StatisticsRenderConfig>() );
	set( &addConfig<EstimatorRenderConfig>() );
	set( &addConfig<PixelatorRenderConfig>() );
	set( &addConfig<JpegRenderConfig>() );
}

std::unique_ptr<ARender> RenderConfigChooser::getRender() const
	{ return getSelected().getRender(); }


std::unique_ptr<ARender> AverageRenderConfig::getRender() const{
	//TODO: chroma upscale, add more as well
	return std::make_unique<AverageRender>( false );
}

std::unique_ptr<ARender> DiffRenderConfig::getRender() const
	{ return std::make_unique<DiffRender>(); }

std::unique_ptr<ARender> FloatRenderConfig::getRender() const
//TODO: doublespinbox for scale
	{ return std::make_unique<FloatRender>( 1.0 ); }

StatisticsRenderConfig::StatisticsRenderConfig( QWidget* parent )
	: ARenderConfig( parent ) {
		setLayout( new QVBoxLayout( this ) );
		function = addWidget<QComboBox>( "Function" );
		
		function->addItem( "Difference" );
		function->addItem( "Minimum" );
		function->addItem( "Maximum" );
		function->addItem( "Median" );
		function->addItem( "Average" );
		connect( function, SIGNAL(currentIndexChanged(int)), this, SIGNAL(changed()) );
	}

static Statistics getStats( int id ){
	switch( id ){
		case 0: return Statistics::DIFFERENCE;
		case 1: return Statistics::MIN;
		case 2: return Statistics::MAX;
		case 3: return Statistics::MEDIAN;
		case 4: return Statistics::AVG;
		default: return Statistics::AVG;
	}
}

std::unique_ptr<ARender> StatisticsRenderConfig::getRender() const{
	return std::make_unique<StatisticsRender>( getStats( function->currentIndex() ) );
}

std::unique_ptr<ARender> EstimatorRenderConfig::getRender() const
//TODO: Variouss controls for pretty much everything
	{ return std::make_unique<EstimatorRender>( 1.0 ); }

std::unique_ptr<ARender> PixelatorRenderConfig::getRender() const
	{ return std::make_unique<PixelatorRender>(); }

JpegRenderConfig::JpegRenderConfig( QWidget* parent ) : ARenderConfig( parent ) {
	setLayout( new QVBoxLayout( this ) );
	path = addWidget<QLineEdit>( "Jpeg sample" );
	path->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Minimum );
	iterations = addWidget<QSpinBox>( "Iterations" );
	iterations->setRange( 1, 1000 );
	iterations->setValue( 300 );
	
	connect( iterations, SIGNAL(valueChanged(int)), this, SIGNAL(changed()) );
	connect( path, SIGNAL(textChanged(QString)), this, SIGNAL(changed()) );
}

std::unique_ptr<ARender> JpegRenderConfig::getRender() const
	{ return std::make_unique<JpegRender>( path->text(), iterations->value() ); }

