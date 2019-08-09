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

#ifndef RENDER_CONFIGS_INTERNAL_HPP
#define RENDER_CONFIGS_INTERNAL_HPP

#include "RenderConfigs.hpp"

class QSpinBox;
class QDoubleSpinBox;
class QLineEdit;
class QComboBox;
class QCheckBox;

namespace Overmix{

struct Spinbox2D;
struct DoubleSpinbox2D;

class AverageRenderConfig : public ARenderConfig{
	private:
		QCheckBox* upscale_chroma;
	
	public:
		AverageRenderConfig( QWidget* parent );
		std::unique_ptr<ARender> getRender() const override;
		
		QString name() const override { return "Average"; }
		QString discription() const override{ return "Each pixel is the average of its location in all frames"; }
};

class FastRenderConfig : public ARenderConfig{
	public:
		FastRenderConfig( QWidget* parent ) : ARenderConfig( parent ) { }
		std::unique_ptr<ARender> getRender() const override;
		
		QString name() const override { return "Fast preview"; }
		QString discription() const override{ return "A very simple render to get an indication if something is off"; }
};

class SkipRenderConfig : public ARenderConfig{
	public:
		DoubleSpinbox2D* skip;
		DoubleSpinbox2D* offset;
	
	public:
		SkipRenderConfig( QWidget* parent );
		std::unique_ptr<ARender> getRender() const override;
		
		QString name() const override { return "Demosaic"; }
		QString discription() const override{ return "Remove blurring from mosaic censor"; }
};

class DiffRenderConfig : public ARenderConfig{
	private:
		QSpinBox* iterations;
		QDoubleSpinBox* threshold;
		QSpinBox* dilate_size;
		
	public:
		DiffRenderConfig( QWidget* parent );
		std::unique_ptr<ARender> getRender() const override;
		
		QString name() const override { return "Static Difference"; }
		QString discription() const override{ return "Detects static content (logo, credits, etc.)"; }
};

class FloatRenderConfig : public ARenderConfig{
	public:
		FloatRenderConfig( QWidget* parent ) : ARenderConfig( parent ) { }
		std::unique_ptr<ARender> getRender() const override;
		
		QString name() const override { return "Subpixel"; }
		QString discription() const override{ return "Subpixel interpolation rendering"; }
};

class StatisticsRenderConfig : public ARenderConfig{
	private:
		QComboBox* function;
	public:
		StatisticsRenderConfig( QWidget* parent );
		std::unique_ptr<ARender> getRender() const override;
		
		QString name() const override { return "Statistics"; }
		QString discription() const override{ return "Various simply ways of extracting a single value from a set. (e.g. min, max, median)"; }
};

class EstimatorRenderConfig : public ARenderConfig{
	public:
		EstimatorRenderConfig( QWidget* parent ) : ARenderConfig( parent ) { }
		std::unique_ptr<ARender> getRender() const override;
		
		QString name() const override { return "Estimator"; }
		QString discription() const override{ return "Super-Resolution image estimator"; }
};

class PixelatorRenderConfig : public ARenderConfig{
	public:
		PixelatorRenderConfig( QWidget* parent ) : ARenderConfig( parent ) { }
		std::unique_ptr<ARender> getRender() const override;
		
		QString name() const override { return "Pixelator"; }
		QString discription() const override{ return "Pixelates the image"; }
};

class JpegRenderConfig : public ARenderConfig{
	private:
		QSpinBox* iterations;
		QLineEdit* path;
	public:
		JpegRenderConfig( QWidget* parent );
		std::unique_ptr<ARender> getRender() const override;
		
		QString name() const override { return "Jpeg estimator"; }
		QString discription() const override{ return "Estimates a JPEG degraded image"; }
};

class JpegConstrainerRenderConfig : public ARenderConfig{
	private:
		QLineEdit* path;
	public:
		JpegConstrainerRenderConfig( QWidget* parent );
		std::unique_ptr<ARender> getRender() const override;
		
		QString name() const override { return "Jpeg constrainer"; }
		QString discription() const override{ return "Constrains an image to a set of JPEG coeffs"; }
};

class DistanceMatrixRenderConfig : public ARenderConfig{
	public:
		DistanceMatrixRenderConfig( QWidget* parent ) : ARenderConfig( parent ) { }
		std::unique_ptr<ARender> getRender() const override;
		
		QString name() const override { return "Alignment matrix"; }
		QString discription() const override{ return "Shows which images have been compared, and their distance"; }
};

}

#endif
