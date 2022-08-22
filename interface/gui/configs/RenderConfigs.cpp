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
#include "RenderConfigsInternal.hpp"

#include "renders/AnimRender.hpp" //TODO: how to handle?
#include "renders/AverageRender.hpp"
#include "renders/DiffRender.hpp"
#include "renders/FastRender.hpp"
#include "renders/FocusStackingRender.hpp"
#include "renders/FloatRender.hpp"
#include "renders/StatisticsRender.hpp"
#include "renders/RobustSrRender.hpp"
#include "renders/EstimatorRender.hpp"
#include "renders/JpegRender.hpp"
#include "renders/JpegConstrainerRender.hpp"
#include "renders/DistanceMatrixRender.hpp"
#include "renders/ParallaxRender.hpp"

#include "../Spinbox2D.hpp"

#include <QComboBox>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QSpinBox>
#include <QDoubleSpinBox>
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
	skipRender = static_cast<SkipRenderConfig*>(&addConfig<SkipRenderConfig>());
	set( skipRender );
	set( &addConfig<DiffRenderConfig>() );
	set( &addConfig<FloatRenderConfig>() );
	set( &addConfig<StatisticsRenderConfig>() );
	set( &addConfig<EstimatorRenderConfig>() );
	set( &addConfig<JpegRenderConfig>() );
	set( &addConfig<JpegConstrainerRenderConfig>() );
	set( &addConfig<DistanceMatrixRenderConfig>() );
	set( &addConfig<FastRenderConfig>() );
	set( &addConfig<FocusStackingRenderConfig>() );
	set( &addConfig<ParallaxRenderConfig>() );
}

std::unique_ptr<ARender> RenderConfigChooser::getRender() const
	{ return getSelected().getRender(); }

void RenderConfigChooser::setSkipRenderConfig( Point<double> skip, Point<double> offset ){
	skipRender->skip  ->setValue( skip   );
	skipRender->offset->setValue( offset );
}
Point<double> RenderConfigChooser::getSkipRenderSkip() const
	{ return skipRender->skip->getValue(); }
Point<double> RenderConfigChooser::getSkipRenderOffset() const
	{ return skipRender->offset->getValue(); }


std::unique_ptr<ARender> AverageRenderConfig::getRender() const{
	return std::make_unique<AverageRender>( upscale_chroma->isChecked() );
}

std::unique_ptr<ARender> FastRenderConfig::getRender() const {
	return std::make_unique<FastRender>();
}

std::unique_ptr<ARender> SkipRenderConfig::getRender() const{
	auto render = std::make_unique<AverageRender>( true ); //always scale chroma
	render->setSpacing( skip  ->getValue() + Point<double>( 1, 1 ) );
	render->setOffset(  offset->getValue() );
	skip->setSingleStep( 0.1 );
	offset->setSingleStep( 0.1 );
	return render;
}

DiffRenderConfig::DiffRenderConfig( QWidget* parent )
	: ARenderConfig( parent ) {
		setLayout( new QVBoxLayout( this ) );
		iterations  = addWidget<QSpinBox>( "Iterations" );
		threshold   = addWidget<QDoubleSpinBox>( "Threshold" );
		dilate_size = addWidget<QSpinBox>( "Dilate amount" );
		
		iterations ->setValue( 2 );
		threshold  ->setValue( 0.5 );
		dilate_size->setValue( 10 );
		
		iterations->setRange( 1, 99 );
		threshold->setRange( 0.0, 1.0 );
		
		threshold->setSingleStep( 0.05 );
		
		connect( iterations,  SIGNAL(valueChanged(int)   ), this, SIGNAL(changed()) );
		connect( threshold,   SIGNAL(valueChanged(double)), this, SIGNAL(changed()) );
		connect( dilate_size, SIGNAL(valueChanged(int)   ), this, SIGNAL(changed()) );
	}

std::unique_ptr<ARender> DiffRenderConfig::getRender() const {
	return std::make_unique<DiffRender>(
			iterations->value(), threshold->value(), dilate_size->value()
		);
}

AverageRenderConfig::AverageRenderConfig( QWidget* parent )
	: ARenderConfig( parent ) {
		setLayout( new QVBoxLayout( this ) );
		upscale_chroma = addWidget<QCheckBox>( "Scale chroma" );
		connect( upscale_chroma, SIGNAL(toggled(bool)), this, SIGNAL(changed()) );
	}

SkipRenderConfig::SkipRenderConfig( QWidget* parent )
	: ARenderConfig( parent ) {
		setLayout( new QVBoxLayout( this ) );
		skip   = addWidget<DoubleSpinbox2D>( "Skip" );
		offset = addWidget<DoubleSpinbox2D>( "Offset" );
		skip->setValue( {10.0, 10.0} );
		
		skip->connectToChanges( this, SIGNAL(changed()) );
		offset->connectToChanges( this, SIGNAL(changed()) );
	}

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


EstimatorRenderConfig::EstimatorRenderConfig( QWidget* parent )
: ARenderConfig( parent ) {
	setLayout( new QVBoxLayout( this ) );
	scale      = addWidget<QDoubleSpinBox>("Upscale factor");
	iterations = addWidget<      QSpinBox>("Iterations");
	beta       = addWidget<QDoubleSpinBox>("Beta");
	lambda     = addWidget<QDoubleSpinBox>("Lambda");
	alpha      = addWidget<QDoubleSpinBox>("Alpha");
	reg_size   = addWidget<      QSpinBox>("Regularisation");
	
	iterations->setRange(0, 999);
	beta->setDecimals(3);
	
	EstimatorPara defaults;
	scale     ->setValue( 2.0 );
	iterations->setValue( defaults.iterations );
	beta      ->setValue( defaults.beta );
	lambda    ->setValue( defaults.lambda );
	alpha     ->setValue( defaults.alpha );
	reg_size  ->setValue( defaults.reg_size );
}

std::unique_ptr<ARender> EstimatorRenderConfig::getRender() const {
	EstimatorPara para;
	para.iterations = iterations->value();
	para.beta       = beta      ->value();
	para.lambda     = lambda    ->value();
	para.alpha      = alpha     ->value();
	para.reg_size   = reg_size  ->value();
	return std::make_unique<EstimatorRender>( scale->value(), para );
}

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

JpegConstrainerRenderConfig::JpegConstrainerRenderConfig( QWidget* parent ) : ARenderConfig( parent ) {
	setLayout( new QVBoxLayout( this ) );
	path = addWidget<QLineEdit>( "Jpeg sample" );
	path->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Minimum );
	
	connect( path, SIGNAL(textChanged(QString)), this, SIGNAL(changed()) );
}

std::unique_ptr<ARender> JpegConstrainerRenderConfig::getRender() const
	{ return std::make_unique<JpegConstrainerRender>( path->text() ); }


std::unique_ptr<ARender> DistanceMatrixRenderConfig::getRender() const
	{ return std::make_unique<DistanceMatrixRender>(); }
	

FocusStackingRenderConfig::FocusStackingRenderConfig( QWidget* parent )
: ARenderConfig( parent ) {
	setLayout( new QVBoxLayout( this ) );
	blur_amount = addWidget<QDoubleSpinBox>("Blurring");
	kernel_size = addWidget<QSpinBox>("Size");
	
	blur_amount->setRange(0, 999);
	blur_amount->setDecimals(3);
	blur_amount->setValue( 0.0 );
	
	kernel_size->setRange(0, 999);
	kernel_size->setValue( 15 );
}
std::unique_ptr<ARender> FocusStackingRenderConfig::getRender() const {
	auto amount = blur_amount->value();
	auto size = kernel_size->value();
	return std::make_unique<FocusStackingRender>( amount, size );
}

ParallaxRenderConfig::ParallaxRenderConfig( QWidget* parent )
: ARenderConfig( parent ) {
		setLayout( new QVBoxLayout( this ) );
		iterations  = addWidget<QSpinBox>( "Iterations" );
		threshold   = addWidget<QDoubleSpinBox>( "Threshold" );
		dilate_size = addWidget<QSpinBox>( "Dilate amount" );
		
		iterations ->setValue( 2 );
		threshold  ->setValue( 0.5 );
		dilate_size->setValue( 10 );
		
		iterations->setRange( 1, 99 );
		threshold->setRange( 0.0, 1.0 );
		
		threshold->setSingleStep( 0.05 );
		
		connect( iterations,  SIGNAL(valueChanged(int)   ), this, SIGNAL(changed()) );
		connect( threshold,   SIGNAL(valueChanged(double)), this, SIGNAL(changed()) );
		connect( dilate_size, SIGNAL(valueChanged(int)   ), this, SIGNAL(changed()) );
}
std::unique_ptr<ARender> ParallaxRenderConfig::getRender() const {
	return std::make_unique<ParallaxRender>(
			iterations->value(), threshold->value(), dilate_size->value()
		);
}

