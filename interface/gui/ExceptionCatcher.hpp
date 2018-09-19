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

#ifndef EXCEPTIONCATCHER_HPP
#define EXCEPTIONCATCHER_HPP

#include <QDialog>

#include <stdexcept>

namespace Ui {
	class ExceptionCatcher;
}

class ExceptionCatcher : public QDialog {
    Q_OBJECT

	private:
		Ui::ExceptionCatcher *ui;
		
	public:
		ExceptionCatcher( QWidget* parent = nullptr );
		ExceptionCatcher( QString what, QWidget* parent = nullptr );
		~ExceptionCatcher();
		
		template<typename Function>
		static auto Guard( QWidget* parent, Function func ) -> decltype( func() ){
			try{
				return func();
			}
			catch( const std::exception& e ){
				ExceptionCatcher catcher( e.what(), parent );
				catcher.exec();
			}
			catch( ... ){
				ExceptionCatcher catcher( "Not a std::exception, no error message :\\", parent );
				catcher.exec();
			}
			return decltype( func() )();
		}

	public slots:
		void close();
		void copy();
		void report();
};

#endif // EXCEPTIONCATCHER_HPP
