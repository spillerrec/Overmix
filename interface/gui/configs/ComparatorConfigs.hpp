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

#ifndef COMPARATOR_CONFIGS_HPP
#define COMPARATOR_CONFIGS_HPP

#include "ConfigChooser.hpp"

namespace Overmix{

class AComparator;

class AComparatorConfig : public AConfig{
	Q_OBJECT
	
	public:
		AComparatorConfig( QWidget* parent ) : AConfig(parent) { }
		
		virtual std::unique_ptr<AComparator> getComparator() const = 0;
		
	signals:
		void changed();
};

class ComparatorConfigChooser : public ConfigChooser<AComparatorConfig>{
	Q_OBJECT
	
	public:
		ComparatorConfigChooser( QWidget* parent, bool expand=false );
		virtual void p_initialize() override;
		
		std::unique_ptr<AComparator> getComparator() const;
		
		QString name() const override{ return "Comparator selector"; }
		QString discription() const override{ return "Selects a comparator"; }
		
	signals:
		void changed();
};


}

#endif
