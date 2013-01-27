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


#include "ui_mainwindow.h"
#include "mainwindow.h"

#include <vector>

#include <QFileInfo>
#include <QFile>
#include <QMimeData>
#include <QUrl>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QImage>
#include <QPainter>
#include <QFileDialog>

main_widget::main_widget(): QMainWindow(), ui(new Ui_main_widget), viewer((QWidget*)this), image(&viewer){
	ui->setupUi(this);
	
	//Buttons
	connect( ui->btn_clear, SIGNAL( clicked() ), this, SLOT( clear_image() ) );
	connect( ui->btn_refresh, SIGNAL( clicked() ), this, SLOT( refresh_image() ) );
	connect( ui->btn_save, SIGNAL( clicked() ), this, SLOT( save_image() ) );
	//TODO: undo
	
	//Checkboxes
	change_dither();
	change_diff();
	change_use_average();
	connect( ui->cbx_diff, SIGNAL( toggled(bool) ), this, SLOT( change_diff() ) );
	connect( ui->cbx_dither, SIGNAL( toggled(bool) ), this, SLOT( change_dither() ) );
	connect( ui->cbx_average, SIGNAL( toggled(bool) ), this, SLOT( change_use_average() ) );
	
	//Sliders
	change_threshould();
	change_movement();
	connect( ui->sld_threshould, SIGNAL( valueChanged(int) ), this, SLOT( change_threshould() ) );
	connect( ui->sld_movement, SIGNAL( valueChanged(int) ), this, SLOT( change_movement() ) );
	
	//Merge method
	change_merge_method();
	connect( ui->merge_both, SIGNAL( clicked() ), this, SLOT( change_merge_method() ) );
	connect( ui->merge_h, SIGNAL( clicked() ), this, SLOT( change_merge_method() ) );
	connect( ui->merge_v, SIGNAL( clicked() ), this, SLOT( change_merge_method() ) );

	//Refresh info labels
	refresh_text();
	
	setAcceptDrops( true );
	ui->main_layout->addWidget( &viewer );
	viewer.setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding ) );
}


void main_widget::dragEnterEvent( QDragEnterEvent *event ){
	if( event->mimeData()->hasUrls() )
		event->acceptProposedAction();
}
void main_widget::dropEvent( QDropEvent *event ){
	if( event->mimeData()->hasUrls() ){
		event->setDropAction( Qt::CopyAction );
		
		foreach( QUrl url, event->mimeData()->urls() ){
			image.add_image( url.toLocalFile() );
		}
		refresh_text();
		update();
		
		event->accept();
	}
}


void main_widget::refresh_text(){
	QRect s = image.get_size();
	ui->lbl_height->setText( tr( "Size: " ) + QString::number(s.width()) + "x" + QString::number(s.height()) );
	ui->lbl_amount->setText( tr( "Images: " ) + QString::number( image.get_count() ) );
}

void main_widget::refresh_image(){
	image.draw();
	refresh_text();
}

void main_widget::save_image(){
	QString filename = QFileDialog::getSaveFileName( this, tr("Save image"), "", tr("PNG files (*.png)") );
	if( !filename.isEmpty() )
		image.save( filename );
}

void main_widget::change_dither(){
	image.set_dither( ui->cbx_dither->isChecked() );
}

void main_widget::change_diff(){
	image.set_diff( ui->cbx_diff->isChecked() );
}

void main_widget::change_use_average(){
	image.set_use_average( ui->cbx_average->isChecked() );
}

void main_widget::change_movement(){
	image.set_movement( (double)ui->sld_movement->value()/(double)ui->sld_movement->maximum() );
}

void main_widget::change_threshould(){
	image.set_threshould( ui->sld_threshould->value() );
}

void main_widget::change_merge_method(){
	int selected = 0;
	if( ui->merge_both->isChecked() )
		selected = 0;
	else if( ui->merge_h->isChecked() )
		selected = 1;
	else if( ui->merge_v->isChecked() )
		selected = 2;
	image.set_merge_method( selected );
}

void main_widget::clear_image(){
	image.clear();
	refresh_text();
}
		