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

#include "../../aligners/AverageAligner.hpp"
#include "../../aligners/RecursiveAligner.hpp"

#include <QGridLayout>

AlignerConfigChooser::AlignerConfigChooser( QWidget* parent, bool expand )
	: ConfigChooser<AAlignerConfig>( parent, expand ){
	addConfig<AverageAlignerConfig>();
	addConfig<RecursiveAlignerConfig>();
}


AAlignerConfig::AAlignerConfig( QWidget* parent ) : AConfig( parent ) {
	setupUi(this);
	
	connect( cbx_merge_v,   &QCheckBox::toggled, this, &AAlignerConfig::toggled_ver );
	connect( cbx_merge_h, &QCheckBox::toggled, this, &AAlignerConfig::toggled_hor );
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
	return AImageAligner::ALIGN_VER; //TODO:
}

double AAlignerConfig::getScale() const{
	return merge_scale->value();
}


std::unique_ptr<AImageAligner> AverageAlignerConfig::getAligner( AContainer& container ) const{
	auto aligner = std::make_unique<AverageAligner>( container, getMethod(), getScale() );
	configure( *aligner );
	return aligner;
}


std::unique_ptr<AImageAligner> RecursiveAlignerConfig::getAligner( AContainer& container ) const{
	auto aligner = std::make_unique<RecursiveAligner>( container, getMethod(), getScale() );
	configure( *aligner );
	return aligner;
}

