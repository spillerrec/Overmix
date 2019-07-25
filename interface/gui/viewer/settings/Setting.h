/*
	This file is part of imgviewer.

	imgviewer is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	imgviewer is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with imgviewer.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SETTINGS_SETTING_H
#define SETTINGS_SETTING_H

#include <QSettings>
#include <QVariant>


template<typename T> inline T QVariantTo( QVariant value );
template<> inline bool QVariantTo( QVariant value ){ return value.toBool(); }

template<typename T>
class Setting{
	protected:
		QSettings& settings;
		const char* const id;
		T default_value;
		
	public:
		Setting( QSettings& settings, const char* const id, T default_value )
			: settings(settings), id(id), default_value(default_value) {}
		
		void set( T value )
			{ settings.setValue( id, value ); }
			
		bool get() const{ return QVariantTo<bool>( settings.value( id, default_value ) ); }
		operator bool() const{ return get(); }
};

#endif
