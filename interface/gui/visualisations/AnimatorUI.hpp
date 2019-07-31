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

#ifndef ANIMATOR_UI_HPP
#define ANIMATOR_UI_HPP


#include <planes/ImageEx.hpp>

#include <QDialog>
#include <vector>

class QSettings;
class imageViewer;
class QSpinBox;

namespace Ui {
	class AnimatorUI;
}

namespace Overmix{

struct DoubleSpinbox2D;
struct Spinbox2D;
class ImageContainer;

class AnimatorUI : public QDialog{
	Q_OBJECT
	
	private:
		Ui::AnimatorUI* ui;
		imageViewer* viewer;
		ImageEx img;
		DoubleSpinbox2D* movement;
		Spinbox2D* size;
		
		std::vector<Rectangle<double>> getCrops();
		
	public:
		AnimatorUI(QSettings& settings, ImageEx img, QWidget* parent );
		~AnimatorUI();
		void render(ImageContainer& container);
		
	public slots:
		void update_preview();
};

}

#endif

