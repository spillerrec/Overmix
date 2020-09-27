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

#ifndef SPINBOX_2D_HPP
#define SPINBOX_2D_HPP

#include <QWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QHBoxLayout>

#include <Geometry.hpp>

namespace Overmix{

template<class SpinBox, typename T>
class AbstractSpinbox2D : public QWidget {
	protected:
		SpinBox spin_x;
		SpinBox spin_y;
		QPushButton locker;
		double scale = 1.0;
		
		void setValueNoScale( Point<T> value ){
			spin_x.setValue( value.x );
			spin_y.setValue( value.y );
		}
		
		void updatedX( T new_x ){
			spin_y.blockSignals( true );
			if( isLocked() )
				spin_y.setValue( (scale > 0.0) ? new_x / scale : 0.0 );
			spin_y.blockSignals( false );
		}
		
		void updatedY( T new_y ){
			spin_x.blockSignals( true );
			if( isLocked() )
				spin_x.setValue( new_y * scale );
			spin_x.blockSignals( false );
		}
		
		double calculateScale( Point<T> value ) const {
			if( value.x == value.y ) //Handle the 0x0 case
				return 1.0;
			//Return 0.0 if one of the terms is 0.0
			return (value.y != 0.0 ) ? (double)value.x / value.y : 0.0;
		}		
		
	public:
		explicit AbstractSpinbox2D( QWidget* parent ) : QWidget( parent ), locker( "X" ) {
			//Position widgets
			setLayout( new QHBoxLayout( this ) );
			layout()->addWidget( &spin_x );
			layout()->addWidget( &locker );
			layout()->addWidget( &spin_y );
			
			//Set properties
			layout()->setContentsMargins( 0,0,0,0 );
			locker.setCheckable( true );
			locker.setChecked( true );
			locker.setMaximumSize( 16, 16 );
			
			//Update
			auto valueChanged = static_cast<void (SpinBox::*)(T)>(&SpinBox::valueChanged);
			connect( &spin_x, valueChanged, [&](T val){ updatedX(val); } );
			connect( &spin_y, valueChanged, [&](T val){ updatedY(val); } );
			connect( &locker, &QPushButton::toggled, [&](){ setValue( getValue() ); } );
		}
		
		bool isLocked() const { return locker.isChecked(); }
		void setScale( double new_scale ) { scale = new_scale; }
		
		Point<T> getValue() const{ return {spin_x.value(), spin_y.value()}; }
		void setValue( Point<T> value ){
			scale = calculateScale( value );
			if( scale == 0.0 )
				locker.setChecked( false );
			setValueNoScale( value ); //We already set the scale, so updates will not break it
		}
		
		template<typename Return, typename... Args>
		void call( Return (SpinBox::*func)(Args...), Args... args ){
			(spin_x.*func)( args... );
			(spin_y.*func)( args... );
		}
		
		void setSingleStep( T step ){ call( &SpinBox::setSingleStep, step ); }
};

struct Spinbox2D : public AbstractSpinbox2D<QSpinBox,int>{
	explicit Spinbox2D( QWidget* parent ) : AbstractSpinbox2D( parent ) { }
	
	template<typename Func>
	void connectToChanges( QObject* receiver, Func func ){
		connect( &spin_x, SIGNAL(valueChanged(int)), receiver, func );
		connect( &spin_y, SIGNAL(valueChanged(int)), receiver, func );
	}
};

struct DoubleSpinbox2D : public AbstractSpinbox2D<QDoubleSpinBox,double>{
	explicit DoubleSpinbox2D( QWidget* parent ) : AbstractSpinbox2D( parent ) { }
	
	template<typename Func>
	void connectToChanges( QObject* receiver, Func func ){
		connect( &spin_x, SIGNAL(valueChanged(double)), receiver, func );
		connect( &spin_y, SIGNAL(valueChanged(double)), receiver, func );
	}
};

}

#endif

