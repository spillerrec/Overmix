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

#ifndef ALIGNER_CONFIGS_INTERNAL_HPP
#define ALIGNER_CONFIGS_INTERNAL_HPP

#include "AlignerConfigs.hpp"

class QCheckBox;
class QSpinBox;
class QDoubleSpinBox;

namespace Overmix{

class AverageAlignerConfig : public AAlignerConfig{
	public:
		AverageAlignerConfig( QWidget* parent ) : AAlignerConfig( parent, DISABLE_ALL ) { }
		std::unique_ptr<AAligner> getAligner() const override;
		
		QString name() const override { return "Ordered"; }
		QString discription() const override{ return "Aligns images by aligning one image against the previously aligned images"; }
};

class RecursiveAlignerConfig : public AAlignerConfig{
	public:
		RecursiveAlignerConfig( QWidget* parent ) : AAlignerConfig( parent, DISABLE_ALL ) { }
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
	private:
		QCheckBox* skip_align;
		QDoubleSpinBox* threshold;
	
	public:
		SeperateAlignerConfig( QWidget* parent );
		std::unique_ptr<AAligner> getAligner() const override;
		
		QString name() const override { return "Separate Frames"; }
		QString discription() const override{ return "Detects cyclic animation by assuming each animation-frame is repeated at least twice"; }
};

class AlignFrameAlignerConfig : public AAlignerConfig{
	public:
		AlignFrameAlignerConfig( QWidget* parent ) : AAlignerConfig( parent, DISABLE_ALL ) { }
		std::unique_ptr<AAligner> getAligner() const override;
		
		QString name() const override { return "Align Frames"; }
		QString discription() const override{ return "Aligns animation frames"; }
};

class FrameCalculatorAlignerConfig : public AAlignerConfig{
	private:
		QSpinBox* offset;
		QSpinBox* amount;
		QSpinBox* repeats;
		
	public:
		FrameCalculatorAlignerConfig( QWidget* parent );
		std::unique_ptr<AAligner> getAligner() const override;
		
		QString name() const override { return "Frame calculator"; }
		QString discription() const override{ return "Calculates the frame number based on its index"; }
};

class SuperResAlignerConfig : public AAlignerConfig{
	public:
		SuperResAlignerConfig( QWidget* parent ) : AAlignerConfig( parent, ENABLE_ALL ) { }
		std::unique_ptr<AAligner> getAligner() const override;
		
		QString name() const override { return "SuperRes"; }
		QString discription() const override{ return "Aligns against the super-resolution image"; }
};

class ClusterAlignerConfig : public AAlignerConfig{
	private:
		QSpinBox* min_groups;
		QSpinBox* max_groups;
		
	public:
		ClusterAlignerConfig( QWidget* parent );
		std::unique_ptr<AAligner> getAligner() const override;
		
		QString name() const override { return "Cluster animation"; }
		QString discription() const override{ return "Tries to seperate frames through clustering"; }
};

class NearestFrameAlignerConfig : public AAlignerConfig{
	public:
		NearestFrameAlignerConfig( QWidget* parent ) : AAlignerConfig( parent, DISABLE_ALL ) { }
		std::unique_ptr<AAligner> getAligner() const override;
		
		QString name() const override { return "Set unassigned frames"; }
		QString discription() const override{ return "Finds the best match for unassigned frames"; }
};

}

#endif
