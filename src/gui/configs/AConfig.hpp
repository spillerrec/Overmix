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

#ifndef A_CONFIG_HPP
#define A_CONFIG_HPP

#include <QWidget>
#include <QString>

class AConfig : public QWidget{
	Q_OBJECT
	
	public:
		AConfig( QWidget* parent ) : QWidget(parent) { }
		
		virtual QString name() const = 0;
		virtual QString discription() const = 0;
		//TODO: load and save settings?
	
		
	//For lazy initialization
	private:
		bool is_initialized{ false };
	protected:
		virtual void p_initialize(){ };
	public:
		void initialize(){
			if( !is_initialized )
				p_initialize();
		}
};


#endif

