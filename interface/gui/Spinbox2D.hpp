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
		
	public:
		AbstractSpinbox2D( QWidget* parent ) : QWidget( parent ), locker( "X" ) {
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
			connect( &spin_x, valueChanged, [&](T val){ setX(val); } );
			connect( &spin_y, valueChanged, [&](T val){ setY(val); } );
			connect( &locker, &QPushButton::toggled, [&](){ setValue( getValue() ); } );
		}
		
		bool isLocked() const { return locker.isChecked(); }
		void setScale( double new_scale ) { scale = new_scale; }
		
		Point<T> getValue() const{ return {spin_x.value(), spin_y.value()}; }
		void setValue( Point<T> value ){
			setValueNoScale( value );
			scale = (value.y > 0.0 ) ? (double)value.x / value.y : 0.0;
		}
		
		void setX( T new_x ){
			if( isLocked() )
				setValueNoScale( { new_x, T((scale > 0.0) ? new_x / scale : 0.0) } );
			else
				setValueNoScale( { new_x, getValue().y } );
		}
		
		void setY( T new_y ){
			if( isLocked() )
				setValueNoScale( { T(new_y * scale), new_y } );
			else
				setValueNoScale( { getValue().x, new_y } );
		}
		
		template<typename Return, typename... Args>
		void call( Return (SpinBox::*func)(Args...), Args... args ){
			(spin_x.*func)( args... );
			(spin_y.*func)( args... );
		}
		
		void setSingleStep( T step ){ call( &SpinBox::setSingleStep, step ); }
};

struct Spinbox2D : public AbstractSpinbox2D<QSpinBox,int>{
	Spinbox2D( QWidget* parent ) : AbstractSpinbox2D( parent ) { }
	
	void connectToChanges( QObject* receiver, auto&& func ){
		connect( &spin_x, SIGNAL(valueChanged(int)), receiver, func );
		connect( &spin_y, SIGNAL(valueChanged(int)), receiver, func );
	}
};

struct DoubleSpinbox2D : public AbstractSpinbox2D<QDoubleSpinBox,double>{
	DoubleSpinbox2D( QWidget* parent ) : AbstractSpinbox2D( parent ) { }
	
	void connectToChanges( QObject* receiver, auto&& func ){
		connect( &spin_x, SIGNAL(valueChanged(double)), receiver, func );
		connect( &spin_y, SIGNAL(valueChanged(double)), receiver, func );
	}
};

}

#endif

