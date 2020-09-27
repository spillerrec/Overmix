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

#ifndef COMPARATOR_CONFIGS_INTERNAL_HPP
#define COMPARATOR_CONFIGS_INTERNAL_HPP

#include "ComparatorConfigs.hpp"

class QSpinBox;
class QDoubleSpinBox;
class QLineEdit;
class QComboBox;
class QCheckBox;

namespace Overmix{

struct Spinbox2D;
class AlignMethodSelector;

class GradientComparatorConfig : public AComparatorConfig{
	private:
		AlignMethodSelector* method;
		QDoubleSpinBox* movement;
		QSpinBox*       start_level;
		QSpinBox*       max_level;
		QCheckBox*      use_l2;
		QSpinBox*       epsilon;
		QSpinBox*       max_difference;
	
	public:
		explicit GradientComparatorConfig( QWidget* parent );
        std::unique_ptr<AComparator> getComparator() const override;
		
		QString name() const override { return "Gradient"; }
		QString discription() const override{ return "Uses gradient decent"; }
};

class BruteForceComparatorConfig : public AComparatorConfig{
	private:
		AlignMethodSelector* method;
		QDoubleSpinBox* movement;
		QCheckBox*      use_l2;
		QSpinBox*       epsilon;
	
	public:
		explicit BruteForceComparatorConfig( QWidget* parent );
        std::unique_ptr<AComparator> getComparator() const override;
		
		QString name() const override { return "Brute force"; }
		QString discription() const override{ return "Tries EVERYTHING!"; }
};

class MultiScaleComparatorConfig : public AComparatorConfig{
	public:
		explicit MultiScaleComparatorConfig( QWidget* parent );
        std::unique_ptr<AComparator> getComparator() const override;
		
		QString name() const override { return "Multi-scale"; }
		QString discription() const override{ return "Halves resolution recursively"; }
};


}

#endif
