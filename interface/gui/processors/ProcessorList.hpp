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

#ifndef PROCESSOR_LIST_HPP
#define PROCESSOR_LIST_HPP

#include "AProcessor.hpp"
#include "AProcessorFactory.hpp"

class QComboBox;
class QPushButton;
class QVBoxLayout;

namespace Overmix{

class ProcessorList : public QWidget{
	Q_OBJECT
	
	private:
		AProcessorFactory factory;
		QComboBox* processor_selector;
		QPushButton* add_processor;
		std::vector<AProcessor*> processors;
		QVBoxLayout* main_layout;
		
		int indexOf( AProcessor* ) const;
	
	public:
		explicit ProcessorList( QWidget* parent );
		
		ImageEx process( const ImageEx& img ) const;
		Point<double> modifyOffset( Point<double> ) const;
		
	private slots:
		void addProcessor();
		void moveProcessorUp( AProcessor* );
		void moveProcessorDown( AProcessor* );
		void deleteProcessor( AProcessor* );
};

}

#endif

