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

#include <imageCache.h>
#include <imageViewer.h>

#include <QKeyEvent>
#include <QMessageBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QImage>


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
	
	connect( view.selectionModel(), &QItemSelectionModel::currentChanged, [&]( auto index ){
		auto info = s.images[index.row()];
		viewer->change_image( new imageCache( QImage( info.filename ) ), true );
	} );
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

void MainWindow::keyPressEvent( QKeyEvent* event ) {
	switch( event->key() ){
		case Qt::Key_I: toogleInterlaze(); break;
		case Qt::Key_O: loadSlide(); break;
		case Qt::Key_S: saveSlide(); break;
		case Qt::Key_E: evaluateInterlaze(); break;
	}
}

void MainWindow::loadSlide() {
	auto filename = QFileDialog::getOpenFileName( this, "Open Slide", QString(), "Slide information (*.xml)" );
	if( !filename.isNull() ){
		s.loadXml( filename );
		view.reset();
	}
}

void MainWindow::saveSlide() {
	auto filename = QFileDialog::getSaveFileName( this, "Save Slide", QString(), "Slide information (*.xml)" );
	if( !filename.isNull() )
		s.saveXml( filename );
}

void MainWindow::toogleInterlaze() {
	auto index = view.selectionModel()->currentIndex();
	auto row = index.row();
	if( row >= 0 && (unsigned)row < s.images.size() ){
		s.images[row].interlazed = true;
		view.reset();
		view.selectionModel()->setCurrentIndex( index, QItemSelectionModel::Select );
	}
}

void MainWindow::evaluateInterlaze() {
	int correct = 0;
	
	for( unsigned i=0; i<s.images.size(); i++ ){
		auto prev_file = (i > 0) ? s.images[i-1].filename : QString();
		if( s.images[i].interlazeTest( prev_file ) )
			correct++;
	}
		
	int wrong = s.images.size() - correct;
	view.reset();
	
	QMessageBox::information( this, "Test", QString::number(correct) + " - " + QString::number(wrong) );
}
