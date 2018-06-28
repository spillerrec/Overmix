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


#include "ComparatorConfigs.hpp"

#include "comparators/GradientComparator.hpp"
#include "comparators/BruteForceComparator.hpp"

#include "../Spinbox2D.hpp"
#include "../AlignMethodSelector.hpp"

#include <QComboBox>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLineEdit>

using namespace Overmix;


ComparatorConfigChooser::ComparatorConfigChooser( QWidget* parent, bool expand )
	: ConfigChooser<AComparatorConfig>( parent, expand )
	{ connectChange( [&](int){ emit changed(); } ); }

void ComparatorConfigChooser::p_initialize(){
	auto set = [&]( auto config ){
		connect( config, SIGNAL(changed()), this, SIGNAL(changed()) );
	};
	
	set( &addConfig<GradientComparatorConfig>() );
	set( &addConfig<BruteForceComparatorConfig>() );
}

std::unique_ptr<AComparator> ComparatorConfigChooser::getComparator() const
	{ return getSelected().getComparator(); }


GradientComparatorConfig::GradientComparatorConfig( QWidget* parent ) : AComparatorConfig( parent ) {
	setLayout( new QVBoxLayout( this ) );
	
	method         = addWidget<AlignMethodSelector>( "Movement directions" );
	movement       = addWidget<QDoubleSpinBox>( "Allowed movement" );
	start_level    = addWidget<QSpinBox>(       "Start level" );
	max_level      = addWidget<QSpinBox>(       "Maximum level" );
	fast_diffing   = addWidget<QCheckBox>(      "Use fast diffing" );
	epsilon        = addWidget<QSpinBox>(       "Ignore threshold" );
	max_difference = addWidget<QSpinBox>(       "Maximum difference" );
	
	//Propergate change
	auto set = [&]( auto config ){
		connect( config, SIGNAL(valueChanged(int)), this, SIGNAL(changed()) );
	};
	set( method );
	set( start_level );
	set( max_level );
	set( epsilon );
	set( max_difference );
	connect( movement, SIGNAL(valueChanged(double)), this, SIGNAL(changed()) );
	connect( fast_diffing, SIGNAL(toggled(bool)), this, SIGNAL(changed()) );
	
	//Limits on spinboxes
	movement      ->setRange( 0.0, 1.0          );
	start_level   ->setRange( 1,   100          );
	max_level     ->setRange( 1,   100          );
	epsilon       ->setRange( 0,   color::WHITE );
	max_difference->setRange( 0,   color::WHITE );
	
	movement->setSingleStep( 0.05 );
	
	//Set default values
	GradientComparator defaults;
	movement      ->setValue(   defaults.movement       );
	start_level   ->setValue(   defaults.start_level    );
	max_level     ->setValue(   defaults.max_level      );
	fast_diffing  ->setChecked( defaults.settings.fast  );
	epsilon       ->setValue(   color::WHITE * 0.1      ); //NOTE: We don't want the default '0' value
	max_difference->setValue(   defaults.max_difference );
}

std::unique_ptr<AComparator> GradientComparatorConfig::getComparator() const{
	auto comperator = std::make_unique<GradientComparator>();
	
	comperator->method           = method         ->getValue();
	comperator->movement         = movement       ->value();
	comperator->start_level      = start_level    ->value();
	comperator->max_level        = max_level      ->value();
	comperator->settings.fast    = fast_diffing   ->isChecked();
	comperator->settings.epsilon = epsilon        ->value();
	comperator->max_difference   = max_difference ->value();
	
	return std::move( comperator );
}

BruteForceComparatorConfig::BruteForceComparatorConfig( QWidget* parent ) : AComparatorConfig( parent ) {
	setLayout( new QVBoxLayout( this ) );
	
	method         = addWidget<AlignMethodSelector>( "Movement directions" );
	movement       = addWidget<QDoubleSpinBox>( "Allowed movement" );
	fast_diffing   = addWidget<QCheckBox>(      "Use fast diffing" );
	epsilon        = addWidget<QSpinBox>(       "Ignore threshold" );
	
	//Propergate change
	auto set = [&]( auto config ){
		connect( config, SIGNAL(valueChanged(int)), this, SIGNAL(changed()) );
	};
	set( method );
	set( epsilon );
	connect( movement, SIGNAL(valueChanged(double)), this, SIGNAL(changed()) );
	connect( fast_diffing, SIGNAL(toggled(bool)), this, SIGNAL(changed()) );
	
	//Limits on spinboxes
	movement      ->setRange( 0.0, 1.0          );
	epsilon       ->setRange( 0,   color::WHITE );
	
	movement->setSingleStep( 0.05 );
	
	//Set default values
	movement      ->setValue( 0.75 );
	Difference::SimpleSettings defaults;
	fast_diffing  ->setChecked( defaults.fast      );
	epsilon       ->setValue(   color::WHITE * 0.1 );
}

std::unique_ptr<AComparator> BruteForceComparatorConfig::getComparator() const{
	auto comperator = std::make_unique<BruteForceComparator>();
	
	comperator->method           = method         ->getValue();
	comperator->movement         = movement       ->value();
	comperator->settings.fast    = fast_diffing   ->isChecked();
	comperator->settings.epsilon = epsilon        ->value();
	
	return std::move( comperator );
}

