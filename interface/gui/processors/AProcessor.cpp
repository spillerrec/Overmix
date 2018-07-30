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


#include "AProcessor.hpp"

#include <QPushButton>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QDebug>

using namespace Overmix;

AProcessor::AProcessor( QWidget* parent ) : QGroupBox(parent) {
	form = new QFormLayout;
	setLayout( form );
	buttons = new QHBoxLayout( this );
	form   ->setContentsMargins( 3,3,3,3 );
	buttons->setContentsMargins( 0,0,0,0 );
	
	//Add a button to the layout at the top
	auto addButton = [&]( QString text, auto signal ){
		auto btn = new QPushButton( text, this );
		connect( btn, &QPushButton::clicked, signal );
		buttons->addWidget( btn );
		
		btn->setStyleSheet( "padding: 1px" ); //Avoid way too large padding
		auto size_hint = btn->minimumSizeHint();
		//TODO: Size hint is 0?
		btn->resize( size_hint.height(), size_hint.height() );
	};
	
	addButton( "▲", [=]( bool a ){ emit moveUp(  this); } );
	addButton( "▼", [=]( bool a ){ emit moveDown(this); } );
	addButton( "X", [=]( bool a ){ emit closed(  this); } );
}

void AProcessor::showEvent( QShowEvent* ){
	setTitle( name() );
}

void AProcessor::resizeEvent( QResizeEvent* ){
	//Move to the top-right corner
	auto new_size = buttons->sizeHint();
	auto new_pos = QPoint( geometry().topRight().x() - new_size.width()-2, buttons->geometry().y() );
	//TODO: It gets clipped without "-2", why?
	buttons->setGeometry( QRect( new_pos, new_size ) );
}

void AProcessor::addItem( QString name, QWidget* item ){
	form->addRow( name, item );
}
