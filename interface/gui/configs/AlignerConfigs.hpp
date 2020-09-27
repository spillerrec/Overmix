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

#ifndef ALIGNER_CONFIGS_HPP
#define ALIGNER_CONFIGS_HPP

#include "ConfigChooser.hpp"
#include "aligners/AAligner.hpp"


#include "ui_imagealigner.h"

class QCheckBox;
class QSpinBox;
class QDoubleSpinBox;

namespace Overmix{

class AAlignerConfig : public AConfig, private Ui::ImageAligner{
	Q_OBJECT
	
	protected:
		enum Enabled{
			ENABLE_ALL    = 0x0,
			DISABLE_DIR   = 0x1,
			DISABLE_MOVE  = 0x2,
			DISABLE_RES   = 0x4,
			DISABLE_EXTRA = 0x8,
			DISABLE_ALL   = 0xF
		};
	
	public:
		explicit AAlignerConfig( QWidget* parent, int edits );
		
		virtual std::unique_ptr<AAligner> getAligner() const = 0;
		
		AlignMethod   getMethod()   const;
		double getScale() const;
	
	private slots:
		void toggled_hor();
		void toggled_ver();
};

class AlignerConfigChooser : public ConfigChooser<AAlignerConfig>{
	Q_OBJECT
	
	public:
		explicit AlignerConfigChooser( QWidget* parent, bool expand=false ); //Add all the configs
		virtual void p_initialize() override;
		
		std::unique_ptr<AAligner> getAligner() const { return getSelected().getAligner(); }
		
		QString name() const override{ return "Aligner selector"; }
		QString discription() const override{ return "Selects a method for aligning images"; }
};

}

#endif
