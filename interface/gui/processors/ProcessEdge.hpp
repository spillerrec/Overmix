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

#ifndef PROCESS_EDGE_HPP
#define PROCESS_EDGE_HPP

#include "AProcessor.hpp"

class QComboBox;
class QSpinBox;
class QDoubleSpinBox;

namespace Overmix{

class ProcessEdge : public AProcessor{
	Q_OBJECT
	
	private:
		QComboBox* method;
	
	public:
		explicit ProcessEdge( QWidget* parent );
		
		QString name() const override;
		ImageEx process( const ImageEx& img ) const override;
};

class ProcessEdge2 : public AProcessor{
	Q_OBJECT
	
	private:
		QDoubleSpinBox* sigma;
		QDoubleSpinBox* amount;
		QSpinBox* size;
	
	public:
		explicit ProcessEdge2( QWidget* parent );
		
		QString name() const override;
		ImageEx process( const ImageEx& img ) const override;
};

class ProcessEdge3 : public AProcessor{
	Q_OBJECT
	
	private:
		QDoubleSpinBox* sigma1;
		QDoubleSpinBox* sigma2;
		QDoubleSpinBox* amount;
		QSpinBox* size;
	
	public:
		explicit ProcessEdge3( QWidget* parent );
		
		QString name() const override;
		ImageEx process( const ImageEx& img ) const override;
};

}

#endif

