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

#include <QObject>
#include <QString>

class AConfig : public QObject{
	Q_OBJECT
	
	public:
		AConfig( QObject* parent ) : QObject(parent) { }
		
		virtual QString name() const = 0;
		virtual QString discription() const = 0;
		//TODO: load and save settings?
};


#endif

