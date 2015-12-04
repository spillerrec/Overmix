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
#include "aligners/AImageAligner.hpp"


#include "ui_imagealigner.h"


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
		AAlignerConfig( QWidget* parent, int edits );
		
		virtual std::unique_ptr<AAligner> getAligner() const = 0;
		
		AlignMethod getMethod() const;
		double getScale() const;
	
	private slots:
		void toggled_hor();
		void toggled_ver();
};

class AlignerConfigChooser : public ConfigChooser<AAlignerConfig>{
	Q_OBJECT
	
	public:
		AlignerConfigChooser( QWidget* parent, bool expand=false ); //Add all the configs
		virtual void p_initialize();
		
		std::unique_ptr<AAligner> getAligner() const { return getSelected().getAligner(); }
		
		QString name() const override{ return "Aligner selector"; }
		QString discription() const override{ return "Selects a method for aligning images"; }
};

class AverageAlignerConfig : public AAlignerConfig{
	public:
		AverageAlignerConfig( QWidget* parent ) : AAlignerConfig( parent, ENABLE_ALL ) { }
		std::unique_ptr<AAligner> getAligner() const override;
		
		QString name() const override { return "Ordered"; }
		QString discription() const override{ return "Aligns images by aligning one image against the previously aligned images"; }
};

class RecursiveAlignerConfig : public AAlignerConfig{
	public:
		RecursiveAlignerConfig( QWidget* parent ) : AAlignerConfig( parent, ENABLE_ALL ) { }
		std::unique_ptr<AAligner> getAligner() const override;
		
		QString name() const override { return "Recursive"; }
		QString discription() const override{ return "Splits the set of images into two halves, and applies the algorithm recursively"; }
};

class FakeAlignerConfig : public AAlignerConfig{
	public:
		FakeAlignerConfig( QWidget* parent ) : AAlignerConfig( parent, DISABLE_ALL ) { }
		std::unique_ptr<AAligner> getAligner() const override;
		
		QString name() const override { return "None (Stills)"; }
		QString discription() const override{ return "Sets all images to 0x0"; }
};

class LinearAlignerConfig : public AAlignerConfig{
	public:
		LinearAlignerConfig( QWidget* parent ) : AAlignerConfig( parent, DISABLE_RES | DISABLE_EXTRA | DISABLE_MOVE ) { }
		std::unique_ptr<AAligner> getAligner() const override;
		
		QString name() const override { return "Fit to Linear Curve"; }
		QString discription() const override{ return "Tries to fit all images onto a linear curve which fits the current data the best."; }
};

class SeperateAlignerConfig : public AAlignerConfig{
	public:
		SeperateAlignerConfig( QWidget* parent ) : AAlignerConfig( parent, ENABLE_ALL ) { }
		std::unique_ptr<AAligner> getAligner() const override;
		
		QString name() const override { return "Seperate Frames"; }
		QString discription() const override{ return "Detects cyclic animation by assuming each animation-frame is repeated at least twice"; }
};

class AlignFrameAlignerConfig : public AAlignerConfig{
	public:
		AlignFrameAlignerConfig( QWidget* parent ) : AAlignerConfig( parent, ENABLE_ALL ) { }
		std::unique_ptr<AAligner> getAligner() const override;
		
		QString name() const override { return "Align Frames"; }
		QString discription() const override{ return "Aligns animation frames"; }
};

class SuperResAlignerConfig : public AAlignerConfig{
	public:
		SuperResAlignerConfig( QWidget* parent ) : AAlignerConfig( parent, ENABLE_ALL ) { }
		std::unique_ptr<AAligner> getAligner() const override;
		
		QString name() const override { return "SuperRes"; }
		QString discription() const override{ return "Aligns against the super-resolution image"; }
};

}

#endif
