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
#include "planes/ImageEx.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QPushButton>

#include <algorithm>

using namespace Overmix;

ProcessorList::ProcessorList( QWidget* parent ) : QWidget( parent ){
	main_layout = new QVBoxLayout;
	main_layout->setContentsMargins( 0,0,0,0 );
	setLayout( main_layout );
	
	add_processor = new QPushButton( "Add", this );
	processor_selector = new QComboBox( this );
	connect( add_processor, SIGNAL(clicked()), this, SLOT(addProcessor()) );
	
	//Add all the types of processors to the combobox
	for( int i=0; i<factory.amount(); i++ )
		processor_selector->addItem( factory.name( i ) );
	
	//Add widgets to layout
	auto adder_layout = new QHBoxLayout( this );
	adder_layout->setContentsMargins( 0,0,0,0 );
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

Point<double> ProcessorList::modifyOffset( Point<double> offset ) const{
	for( auto processor : processors )
		offset = processor->modifyOffset( offset );
	return offset;
}

int ProcessorList::indexOf( AProcessor* p ) const{
	auto it = std::find( processors.begin(), processors.end(), p );
	if( it == processors.end() )
		throw std::runtime_error( "AProcessor not in ProcessList" );
	return it - processors.begin();
}

void ProcessorList::addProcessor(){
	auto widget = factory.create( processor_selector->currentIndex(), this ).release();
	processors.push_back( widget );
	layout()->addWidget( widget );
	connect( widget, SIGNAL(closed(  AProcessor*)), this, SLOT(deleteProcessor(  AProcessor*)) );
	connect( widget, SIGNAL(moveUp(  AProcessor*)), this, SLOT(moveProcessorUp(  AProcessor*)) );
	connect( widget, SIGNAL(moveDown(AProcessor*)), this, SLOT(moveProcessorDown(AProcessor*)) );
}

void ProcessorList::moveProcessorUp( AProcessor* p ){
	int index = indexOf( p );
	if( index > 0 ){
		std::swap( processors.at(index-1), processors.at(index) );
		//NOTE: Layout is one off due to the button-layout is the first index
		main_layout->insertItem( index+1, main_layout->takeAt( index-1+1 ) );
	}
}
void ProcessorList::moveProcessorDown( AProcessor* p ){
	int index = indexOf( p );
	if( unsigned(index) < processors.size()-1 ){
		std::swap( processors.at(index), processors.at(index+1) );
		//NOTE: Layout is one off due to the button-layout is the first index
		main_layout->insertItem( index+1, main_layout->takeAt( index+1+1 ) );
	}
}

void ProcessorList::deleteProcessor( AProcessor* processor ){
	//Remove from our pipeline
	auto new_end = std::remove( processors.begin(), processors.end(), processor );
	processors.erase( new_end, processors.end() );
	
	//Remove the widget from the UI
	delete processor;
}
