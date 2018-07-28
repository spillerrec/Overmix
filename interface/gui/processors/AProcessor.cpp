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
#include <QDebug>

using namespace Overmix;

AProcessor::AProcessor( QWidget* parent ) : QGroupBox(parent) {
	form = new QFormLayout;
	setLayout( form );
	
	exit_btn = new QPushButton( "X", this );
	connect( exit_btn, SIGNAL(clicked(bool)), this, SIGNAL(closed()) );
	
	//Resize button
	exit_btn->setStyleSheet( "padding: 1px" ); //Avoid way too large padding
	auto size_hint = exit_btn->minimumSizeHint();
	exit_btn->resize( size_hint.height(), size_hint.height() );
}

void AProcessor::showEvent( QShowEvent* ){
	setTitle( name() );
}

void AProcessor::resizeEvent( QResizeEvent* ){
	//Move to the top-right corner
	exit_btn->move( geometry().topRight().x() - exit_btn->width()-2, exit_btn->y() );
	//TODO: It gets clipped without "-2", why?
}

void AProcessor::addItem( QString name, QWidget* item ){
	form->addRow( name, item );
}
