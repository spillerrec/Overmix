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

#ifndef APROCESSOR_FACTORY_HPP
#define APROCESSOR_FACTORY_HPP

#include <vector>
#include <memory>
#include <functional>

class QWidget;

namespace Overmix{

class AProcessor;

class AProcessorFactory{
	private:
		struct FactoryItem{
			std::function<std::unique_ptr<AProcessor> (QWidget*)> create;
			const char* const name;
		};
		std::vector<FactoryItem> factory;
		
		template<class Processor>
		void addProcessor( const char* const name ){
			auto func = [](QWidget* parent){ return std::make_unique<Processor>( parent ); };
			factory.push_back( FactoryItem{ func, name } );
		}
	
	public:
		AProcessorFactory();
		
		const char* const name( int index ) const;
		std::unique_ptr<AProcessor> create( int index, QWidget* parent ) const;
		int amount() const{ return factory.size(); }
};

}

#endif

