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


#include "MovementGraph.hpp"

#include "../../containers/FrameContainer.hpp"
#include "../../containers/ImageContainer.hpp"

#include <kplotwidget.h>
#include <kplotobject.h>
#include <kplotaxis.h>

#include <QDebug>
#include <QHBoxLayout>

static Point<bool> imagesMoves( const AContainer& container ){
	if( container.count() == 0 )
		return { false, false };
	
	Point<bool> moves{ false, false };
	auto base = container.pos( 0 );
	for( unsigned i=1; i<container.count(); ++i ){
		auto current = container.pos( i );
		moves.x = moves.x || (base.x != current.x);
		moves.y = moves.y || (base.y != current.y);
	}
	
	return moves;
}

static void addPoint( KPlotObject* line, FrameContainer& container, int index, Point<bool> moves ){
	auto pos = container.pos( index );
	auto real_index = container.realIndex( index );
	
	line->addPoint(
			moves.x ? pos.x : real_index
		,	moves.y ? pos.y : real_index
		);
}

static void setLimit( KPlotWidget* plot, const AContainer& images, Point<bool> moves ){
	auto min_point = images.minPoint();
	auto max_point = images.maxPoint();
	plot->setLimits(
			moves.x ? min_point.x : 0, moves.x ? max_point.x : images.count()
		,	moves.y ? min_point.y : 0, moves.y ? max_point.y : images.count()
		);
	
	plot->axis( KPlotWidget::LeftAxis   )->setLabel( moves.y ? QObject::tr("Y") : QObject::tr("Id") );
	plot->axis( KPlotWidget::BottomAxis )->setLabel( moves.x ? QObject::tr("X") : QObject::tr("Id") );
}

QColor getColor( int frame ){
	if( frame < 0 )
		return Qt::black;
	
	switch( frame ){
		case 0: return Qt::red;
		case 1: return Qt::green;
		case 2: return Qt::blue;
		case 3: return Qt::cyan;
		case 4: return Qt::magenta;
		case 5: return Qt::yellow;
		default: return getColor( frame - 6 );
	}
}

MovementGraph::MovementGraph( ImageContainer& images ) : QWidget(nullptr), images(images) {
	auto plot = new KPlotWidget( this );
	setLayout( new QHBoxLayout( this ) );
	layout()->addWidget( plot );
	layout()->setContentsMargins( 0, 0, 0, 0 );
	
	plot->setTopPadding(    4 );
	plot->setRightPadding(  4 );
	plot->setBackgroundColor( Qt::white );
	plot->setForegroundColor( Qt::black );
	plot->setAntialiasing( true );
	
	auto moves = imagesMoves( images );
	setLimit( plot, images, moves );
	
	for( auto frame : images.getFrames() ){
		auto line = new KPlotObject( getColor( frame ), KPlotObject::Lines );
		
		FrameContainer container( images, frame );
		for( unsigned i=0; i<container.count(); ++i )
			addPoint( line, container, i, moves );
		
		plot->addPlotObject( line );
	}
	
	resize( 640, 480 );
	show();
}