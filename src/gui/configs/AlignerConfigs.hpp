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
#include "../../aligners/AImageAligner.hpp"

#include <QCheckBox>


#include "ui_imagealigner.h"


class AAlignerConfig : public AConfig, private Ui::ImageAligner{
	Q_OBJECT
	
	protected:
		virtual void configure( AImageAligner& container ) const;
	
	public:
		AAlignerConfig( QWidget* parent );
		
		virtual std::unique_ptr<AImageAligner> getAligner(AContainer&) const = 0;
		
		AImageAligner::AlignMethod getMethod() const;
		double getScale() const;
	
	private slots:
		void toggled_hor();
		void toggled_ver();
};

class AlignerConfigChooser : public ConfigChooser<AAlignerConfig>{
	Q_OBJECT
	
	public:
		AlignerConfigChooser( QWidget* parent, bool expand=false ); //Add all the configs
		
		std::unique_ptr<AImageAligner> getAligner( AContainer& container ) const
			{ return getSelected().getAligner( container ); }
		
		QString name() const override{ return "Aligner selector"; }
		QString discription() const override{ return "Selects a method for aligning images"; }
};

class AverageAlignerConfig : public AAlignerConfig{
	public:
		AverageAlignerConfig( QWidget* parent ) : AAlignerConfig( parent ) { }
		std::unique_ptr<AImageAligner> getAligner(AContainer&) const override;
		
		QString name() const override { return "Ordered"; }
		QString discription() const override{ return "Aligns images by aligning one image against the previously aligned images"; }
};

class RecursiveAlignerConfig : public AAlignerConfig{
	public:
		RecursiveAlignerConfig( QWidget* parent ) : AAlignerConfig( parent ) { }
		std::unique_ptr<AImageAligner> getAligner(AContainer&) const override;
		
		QString name() const override { return "Recursive"; }
		QString discription() const override{ return "Splits the set of images into two halves, and applies the algorithm recursively"; }
};


#endif
