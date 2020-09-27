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


#include "ProcessLevels.hpp"

#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QHBoxLayout>

#include "planes/ImageEx.hpp"
#include "color.hpp"

using namespace Overmix;

class Overmix::ColorRangeSpinbox : public QWidget{
	
	private:
		QSpinBox start; //TODO: make a color spinbox
		QSpinBox end;
		
	public:
		explicit ColorRangeSpinbox( QWidget* parent )
			:	QWidget( parent ), start(this), end(this)
		{
			setLayout( new QHBoxLayout );
			layout()->setContentsMargins( 0,0,0,0 );
			layout()->addWidget( &start );
			layout()->addWidget( &end   );
			
			start.setRange( 0, 255 );
			end  .setRange( 0, 255 );
			end.setValue( 255 );
		}
		
		bool defaults() const
			{ return start.value() == 0 && end.value() == 255; }
		
		color_type valueStart() const
			{ return color::from8bit( start.value() ); }
		
		color_type valueEnd() const
			{ return color::from8bit( end.value() ); }
};

ProcessLevels::ProcessLevels( QWidget* parent ) : AProcessor( parent ){
	in_levels  = newItem<ColorRangeSpinbox>( "Input"  );
	gamma      = newItem<QDoubleSpinBox   >( "Gamma"  );
	out_levels = newItem<ColorRangeSpinbox>( "Output" );
	
	//Configure
	gamma->setValue( 1 );
	gamma->setRange( 0.05, 10 );
	gamma->setSingleStep( 0.05 );
}

QString ProcessLevels::name() const{ return "Level adjustment"; }

bool ProcessLevels::modifiesImage() const{
	return gamma->value() != 1.0 || !in_levels->defaults() || !out_levels->defaults();
}

ImageEx ProcessLevels::process( const ImageEx& input ) const{
	return input.copyApply(
			&Plane::level
		,	in_levels->valueStart()
		,	in_levels->valueEnd()
		,	out_levels->valueStart()
		,	out_levels->valueEnd()
		,	gamma->value()
		);
}