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


#include "ProcessorList.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QPushButton>
#include "planes/ImageEx.hpp"

using namespace Overmix;

ProcessorList::ProcessorList( QWidget* parent ) : QWidget( parent ){
	auto main_layout = new QVBoxLayout;
	setLayout( main_layout );
	
	add_processor = new QPushButton( "Add", this );
	processor_selector = new QComboBox( this );
	connect( add_processor, SIGNAL(clicked()), this, SLOT(addProcessor()) );
	
	//Add all the types of processors to the combobox
	for( int i=0; i<factory.amount(); i++ )
		processor_selector->addItem( factory.name( i ) );
	
	//Add widgets to layout
	auto adder_layout = new QHBoxLayout( this );
	adder_layout->addWidget( processor_selector );
	adder_layout->addWidget( add_processor );
	
	main_layout->addLayout( adder_layout );
}

ImageEx ProcessorList::process( const ImageEx& input ) const{
	ImageEx output( input );
	
	//Run trough all the widgets
	for( auto processor : processors )
		if( processor->modifiesImage() )
			output = processor->process( output );
	//TODO: Support caching
	
	return output;
}

void ProcessorList::addProcessor(){
	auto widget = factory.create( processor_selector->currentIndex(), this );
	processors.push_back( widget.release() );
	layout()->addWidget( processors.back() );
}

void ProcessorList::deleteProcessor( AProcessor* ){
	
}
