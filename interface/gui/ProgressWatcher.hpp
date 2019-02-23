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

#ifndef PROGRESS_WATCHER_HPP
#define PROGRESS_WATCHER_HPP

#include <QProgressDialog>

#include "utils/AProcessWatcher.hpp"


namespace Overmix{

class ProgressWatcher : public AProcessWatcher{
	private:
		QProgressDialog dialog;
	public:
		ProgressWatcher( QWidget* parent, QString label ) : dialog( parent ) {
			dialog.setLabelText( label );
			dialog.setWindowModality( Qt::WindowModal );
			dialog.setMinimum( 0 );
			dialog.setValue( 0 );
		}
		
		void setTitle( std::string /*title*/ ) override {
			//TODO:
		}
		void setTotal( int total ) override{
			dialog.setMaximum( total );
		}
		void setCurrent( int current ) override{
			dialog.setValue( current );
		}
		int getCurrent() const override{ return dialog.value(); }
		
		bool shouldCancel() const override{ return dialog.wasCanceled(); }
		
		AProcessWatcher* makeSubtask() override
			{ return this; } //TODO: Do something sensible
};
	
}

#endif
