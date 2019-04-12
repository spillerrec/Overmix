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


#include "SkipRenderPreview.hpp"

#include "../viewer/imageViewer.h"
#include "../viewer/imageCache.h"

#include "../Spinbox2D.hpp"
#include <containers/AContainer.hpp>
#include <renders/AverageRender.hpp>
#include <color.hpp>

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSpinBox>

using namespace Overmix;


SkipRenderPreview::SkipRenderPreview( QSettings& settings, const AContainer& images, QWidget* parent )
	:	QDialog( parent ), images(images)
{
	setWindowTitle( tr("Demosaic calculator") );
	
	id = new QSpinBox( this );
	skip   = new DoubleSpinbox2D( this );
	offset = new DoubleSpinbox2D( this );
	viewer = new imageViewer( settings, this );
	auto buttons = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel );
	
	id->setRange( 0, images.count()-1 );
	skip->setValue( { 5, 5 } );
	skip  ->setSingleStep( 0.1 );
	offset->setSingleStep( 0.1 );
	
	buttons->setSizePolicy( QSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed ) );
	viewer->setSizePolicy( QSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding ) );
	viewer->setMinimumSize( {480, 360} );
	
	auto layout          = new QHBoxLayout( this );
	auto layout_settings = new QVBoxLayout( this );
	
 	layout_settings->addWidget( id );
 	layout_settings->addWidget( skip );
	layout_settings->addWidget( offset );
	layout_settings->addStretch();
	layout_settings->addWidget( buttons );
	layout_settings->setSizeConstraint( QLayout::SetMinimumSize );
	
	layout->addLayout( layout_settings );
	layout->addWidget( viewer );
	setLayout( layout );
	
	update_preview();
	
 	connect( id, SIGNAL(valueChanged(int)), this, SLOT( update_preview() ) );
 	skip  ->connectToChanges( this, SLOT( update_preview() ) );
 	offset->connectToChanges( this, SLOT( update_preview() ) );
	connect( buttons, SIGNAL(accepted()), this, SLOT(accept()) );
	connect( buttons, SIGNAL(rejected()), this, SLOT(reject()) );
}


void SkipRenderPreview::update_preview(){
	auto img_id = id->value();
	if( unsigned(img_id) >= images.count() || img_id < 0 ){
		viewer->change_image( nullptr, true );
		return;
	}
	auto img = images.image( img_id );
	
	SumPlane sum( img.getSize() );
	sum.spacing = skip->getValue() + Point<double>(1,1);
	sum.offset = offset->getValue();
	
	Plane temp( img.getSize() );
	temp.fill( color::WHITE );
	sum.addPlane( temp, {0,0} );
	
	auto result = sum.alpha();
	
	auto out = img.toRgb();
	for( unsigned i=0; i<out.size(); i++ )
		out[i] = out[i].maxPlane( result );
	
	viewer->change_image( new imageCache( out.to_qimage() ), true );
}
Point<double> SkipRenderPreview::getSkip() const
	{ return skip->getValue(); }

Point<double> SkipRenderPreview::getOffset() const
	{ return offset->getValue(); }
	
void SkipRenderPreview::setConfig( Point<double> newSkip, Point<double> newOffset ){
	skip  ->setValue( newSkip   );
	offset->setValue( newOffset );
}


