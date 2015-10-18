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

#include "aligners/AnimationSeparator.hpp"
#include "aligners/AverageAligner.hpp"
#include "aligners/FakeAligner.hpp"
#include "aligners/FrameAligner.hpp"
#include "aligners/RecursiveAligner.hpp"
#include "aligners/LinearAligner.hpp"
#include "aligners/SuperResAligner.hpp"

#include <QGridLayout>

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
	addConfig<SuperResAlignerConfig>();
}


AAlignerConfig::AAlignerConfig( QWidget* parent, int edits ) : AConfig( parent ) {
	setupUi(this);
	layout()->setContentsMargins( 0,0,0,0 );
	
	if( edits & DISABLE_DIR ){
		cbx_merge_h->hide();
		cbx_merge_v->hide();
	}
	else{
		connect( cbx_merge_v,   &QCheckBox::toggled, this, &AAlignerConfig::toggled_ver );
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

void AAlignerConfig::configure( AImageAligner& aligner ) const{
	//TODO: See if AImageAligner can have method and scale as settings
	aligner.set_edges( cbx_edges->isChecked() );
	aligner.set_movement( merge_movement->value() / 100.0 );
}


AImageAligner::AlignMethod AAlignerConfig::getMethod() const{
	bool h = cbx_merge_h->isChecked(), v = cbx_merge_v->isChecked();
	if( h && v )
		return AImageAligner::ALIGN_BOTH;
	if( h )
		return AImageAligner::ALIGN_VER;
	if( v )
		return AImageAligner::ALIGN_HOR;
	return AImageAligner::ALIGN_VER;
}

double AAlignerConfig::getScale() const{
	return merge_scale->value();
}


std::unique_ptr<AImageAligner> AverageAlignerConfig::getAligner( AContainer& container ) const{
	auto aligner = std::make_unique<AverageAligner>( container, getMethod(), getScale() );
	configure( *aligner );
	return std::move( aligner );
}


std::unique_ptr<AImageAligner> RecursiveAlignerConfig::getAligner( AContainer& container ) const{
	auto aligner = std::make_unique<RecursiveAligner>( container, getMethod(), getScale() );
	configure( *aligner );
	return std::move( aligner );
}


std::unique_ptr<AImageAligner> FakeAlignerConfig::getAligner( AContainer& container ) const{
	auto aligner = std::make_unique<FakeAligner>( container );
	configure( *aligner );
	return std::move( aligner );
}


std::unique_ptr<AImageAligner> LinearAlignerConfig::getAligner( AContainer& container ) const{
	auto aligner = std::make_unique<LinearAligner>( container, getMethod() );
	configure( *aligner );
	return std::move( aligner );
}

std::unique_ptr<AImageAligner> SeperateAlignerConfig::getAligner( AContainer& container ) const{
	auto aligner = std::make_unique<AnimationSeparator>( container, getMethod(), getScale() );
	configure( *aligner );
	return std::move( aligner );
}

std::unique_ptr<AImageAligner> AlignFrameAlignerConfig::getAligner( AContainer& container ) const{
	auto aligner = std::make_unique<FrameAligner>( container, getMethod(), getScale() );
	configure( *aligner );
	return std::move( aligner );
}

std::unique_ptr<AImageAligner> SuperResAlignerConfig::getAligner( AContainer& container ) const{
	auto aligner = std::make_unique<SuperResAligner>( container, getMethod(), getScale() );
	configure( *aligner );
	return std::move( aligner );
}

