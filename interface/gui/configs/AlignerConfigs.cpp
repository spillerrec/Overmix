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


#include "AlignerConfigs.hpp"
#include "AlignerConfigsInternal.hpp"

#include "aligners/AnimationSeparator.hpp"
#include "aligners/AverageAligner.hpp"
#include "aligners/ClusterAligner.hpp"
#include "aligners/FakeAligner.hpp"
#include "aligners/FrameAligner.hpp"
#include "aligners/NearestFrameAligner.hpp"
#include "aligners/FrameCalculatorAligner.hpp"
#include "aligners/RecursiveAligner.hpp"
#include "aligners/LinearAligner.hpp"
#include "aligners/SuperResAligner.hpp"

#include <QGridLayout>

#include <QCheckBox>
#include <QDoubleSpinBox>

using namespace Overmix;

AlignerConfigChooser::AlignerConfigChooser( QWidget* parent, bool expand )
	: ConfigChooser<AAlignerConfig>( parent, expand ){ }

void AlignerConfigChooser::p_initialize(){
	addConfig<AverageAlignerConfig>();
	addConfig<RecursiveAlignerConfig>();
	addConfig<FakeAlignerConfig>();
	addConfig<LinearAlignerConfig>();
	addConfig<SeperateAlignerConfig>();
	addConfig<AlignFrameAlignerConfig>();
	addConfig<FrameCalculatorAlignerConfig>();
	addConfig<SuperResAlignerConfig>();
	addConfig<ClusterAlignerConfig>();
	addConfig<NearestFrameAlignerConfig>();
}


AAlignerConfig::AAlignerConfig( QWidget* parent, int edits ) : AConfig( parent ) {
	setupUi(this);
	layout()->setContentsMargins( 0,0,0,0 );
	
	if( edits & DISABLE_DIR ){
		cbx_merge_h->hide();
		cbx_merge_v->hide();
	}
	else{
		connect( cbx_merge_v, &QCheckBox::toggled, this, &AAlignerConfig::toggled_ver );
		connect( cbx_merge_h, &QCheckBox::toggled, this, &AAlignerConfig::toggled_hor );
	}
	
	if( edits & DISABLE_MOVE ){
		label->hide();
		label_2->hide();
		label_3->hide();
		merge_movement->hide();
	}
	
	if( edits & DISABLE_RES ){
		label_18->hide();
		merge_scale->hide();
	}
	
	if( edits & DISABLE_EXTRA )
		cbx_edges->hide();
}


void AAlignerConfig::toggled_hor(){
	//Always have one checked
	if( !(cbx_merge_h->isChecked()) )
		cbx_merge_v->setChecked( true );
}
void AAlignerConfig::toggled_ver(){
	//Always have one checked
	if( !(cbx_merge_v->isChecked()) )
		cbx_merge_h->setChecked( true );
}


AlignMethod AAlignerConfig::getMethod() const{
	bool h = cbx_merge_h->isChecked(), v = cbx_merge_v->isChecked();
	if( h && v )
		return AlignMethod::BOTH;
	if( h )
		return AlignMethod::VER;
	if( v )
		return AlignMethod::HOR;
	return AlignMethod::VER;
}

double AAlignerConfig::getScale() const{
	return merge_scale->value();
}

FrameCalculatorAlignerConfig::FrameCalculatorAlignerConfig( QWidget* parent )
	:	AAlignerConfig( parent, DISABLE_ALL ) {
	offset  = addWidget<QSpinBox>( "Offset" );
	amount  = addWidget<QSpinBox>( "Frame amount" );
	repeats = addWidget<QSpinBox>( "Frame repetitions" );
	
	amount ->setMinimum( 1 );
	repeats->setMinimum( 1 );
}

SeperateAlignerConfig::SeperateAlignerConfig( QWidget* parent )
	: AAlignerConfig( parent, DISABLE_ALL ) {
	threshold    = addWidget<QDoubleSpinBox>( "Reduce" );
	
	threshold->setValue( 1.0 );
	threshold->setRange( 0.01, 9.99 );
	threshold->setSingleStep( 0.01 );
}

ClusterAlignerConfig::ClusterAlignerConfig( QWidget* parent )
	:	AAlignerConfig( parent, DISABLE_ALL ) {
	min_groups = addWidget<QSpinBox>( "Minimum frames" );
	max_groups = addWidget<QSpinBox>( "Maximum frames" );
	
	min_groups->setRange( 1, 99 );
	max_groups->setRange( 1, 99 );
	min_groups->setValue(  1 );
	max_groups->setValue( 20 );
}


std::unique_ptr<AAligner> AverageAlignerConfig::getAligner() const
	{ return std::make_unique<AverageAligner>(); }


std::unique_ptr<AAligner> RecursiveAlignerConfig::getAligner() const
	{ return std::make_unique<RecursiveAligner>(); }


std::unique_ptr<AAligner> FakeAlignerConfig::getAligner() const
	{ return std::make_unique<FakeAligner>(); }


std::unique_ptr<AAligner> LinearAlignerConfig::getAligner() const
	{ return std::make_unique<LinearAligner>( getMethod() ); }

std::unique_ptr<AAligner> SeperateAlignerConfig::getAligner() const {
	auto aligner = std::make_unique<AnimationSeparator>();
	aligner->setThresholdFactor( threshold->value() );
	return std::move( aligner );
}

std::unique_ptr<AAligner> AlignFrameAlignerConfig::getAligner() const
	{ return std::make_unique<FrameAligner>(); }

std::unique_ptr<AAligner> FrameCalculatorAlignerConfig::getAligner() const{
	return std::make_unique<FrameCalculatorAligner>(
			offset ->value()
		,	amount ->value()
		,	repeats->value()
		);
}

std::unique_ptr<AAligner> SuperResAlignerConfig::getAligner() const
	{ return std::make_unique<SuperResAligner>( getMethod(), getScale() ); }

std::unique_ptr<AAligner> ClusterAlignerConfig::getAligner() const
	{ return std::make_unique<ClusterAligner>( min_groups->value(), max_groups->value() ); }

std::unique_ptr<AAligner> NearestFrameAlignerConfig::getAligner() const
	{ return std::make_unique<NearestFrameAligner>(); }

