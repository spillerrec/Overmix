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

#ifndef APROCESSOR_HPP
#define APROCESSOR_HPP

#include <QGroupBox>
class QFormLayout;
class QPushButton;

namespace Overmix{

class ImageEx;

class AProcessor : public QGroupBox{
	Q_OBJECT
	
	private:
		QFormLayout* form;
		QPushButton* exit_btn;
	
	public:
		AProcessor( QWidget* parent );
		virtual ~AProcessor() = default;
		
		virtual QString name() const = 0;
		virtual bool modifiesImage() const { return true; }
		virtual ImageEx process( const ImageEx& img ) const = 0;
		
		void addItem( QString name, QWidget* item );
		template<class widget>
		widget* newItem( QString name ){
			auto out = new widget( this );
			addItem( name, out );
			return out;
		}
		
	signals:
		void changed();
		void closed(AProcessor*);
		void moveUp(AProcessor*);
		void moveDown(AProcessor*);
	
	protected:
		void showEvent( QShowEvent* ) override;
		void resizeEvent( QResizeEvent* ) override;
};

}

#endif
