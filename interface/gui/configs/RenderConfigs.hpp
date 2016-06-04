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

#ifndef RENDER_CONFIGS_HPP
#define RENDER_CONFIGS_HPP

#include "ConfigChooser.hpp"

class QSpinBox;
class QDoubleSpinBox;
class QLineEdit;
class QComboBox;
class QCheckBox;

namespace Overmix{

class ARender;
struct Spinbox2D;

class ARenderConfig : public AConfig{
	Q_OBJECT
	
	public:
		ARenderConfig( QWidget* parent ) : AConfig(parent) { }
		
		virtual std::unique_ptr<ARender> getRender() const = 0;
		
	signals:
		void changed();
};

class RenderConfigChooser : public ConfigChooser<ARenderConfig>{
	Q_OBJECT
	
	public:
		RenderConfigChooser( QWidget* parent, bool expand=false );
		virtual void p_initialize() override;
		
		std::unique_ptr<ARender> getRender() const;
		
		QString name() const override{ return "Render selector"; }
		QString discription() const override{ return "Selects a render"; }
		
	signals:
		void changed();
};

class AverageRenderConfig : public ARenderConfig{
	private:
		Spinbox2D* skip;
		Spinbox2D* offset;
		QCheckBox* upscale_chroma;
	
	public:
		AverageRenderConfig( QWidget* parent );
		std::unique_ptr<ARender> getRender() const override;
		
		QString name() const override { return "Average"; }
		QString discription() const override{ return "Each pixel is the average of its location in all frames"; }
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

class DistanceMatrixRenderConfig : public ARenderConfig{
	public:
		DistanceMatrixRenderConfig( QWidget* parent ) : ARenderConfig( parent ) { }
		std::unique_ptr<ARender> getRender() const override;
		
		QString name() const override { return "Alignment matrix"; }
		QString discription() const override{ return "Shows which images have been compared, and their distance"; }
};

}

#endif
