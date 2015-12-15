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

#include "MainWindow.hpp"

#include <imageViewer.h>

#include <QHBoxLayout>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QMimeData>


using namespace Overmix;

MainWindow::MainWindow() : QWidget(), s_model(&s), view(this)
#ifdef PORTABLE //Portable settings
	,	settings( QCoreApplication::applicationDirPath() + "/settings.ini", QSettings::IniFormat )
#else
	,	settings( "spillerrec", "overmix" )
#endif
{
	setLayout( new QHBoxLayout( this ) );
	
	view.setModel( &s_model );
	view.setSizePolicy( QSizePolicy::Maximum, QSizePolicy::Preferred );
	layout()->addWidget( &view );
	
	setAcceptDrops( true );
	
	viewer = new imageViewer( settings, this );
	layout()->addWidget( viewer );
}


void MainWindow::dragEnterEvent( QDragEnterEvent *event ){
	if( event->mimeData()->hasUrls() )
		event->acceptProposedAction();
}
void MainWindow::dropEvent( QDropEvent *event ){
	if( event->mimeData()->hasUrls() ){
		event->setDropAction( Qt::CopyAction );
		event->accept();
		
		for( auto url : event->mimeData()->urls() )
			s.add( url.toLocalFile(), false );
		view.reset();
	}
}
