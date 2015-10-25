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

#include "planes/Plane.hpp"
#include "color.hpp"
using namespace Overmix;


#include <benchmark/benchmark.h>
#include <random>

Plane makeRandomPlane( unsigned width, unsigned height ){
	//Set-up random number generation
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dis( color::BLACK, color::WHITE );
	
	//Fill plane with random numbers
	Plane p( width, height );
	for( auto row : p )
		for( auto& val : row )
			val = dis( gen );
	
	return p;
}

static void BM_PlaneCreation( benchmark::State& state ) {
	while (state.KeepRunning()){
		Plane p( state.range_x(), state.range_x() );
	}
}

static void BM_PlaneFill( benchmark::State& state ) {
	while (state.KeepRunning()){
		Plane p( state.range_x(), state.range_x() );
		p.fill(color::WHITE);
	}
}

static void BM_PlaneRandom( benchmark::State& state ) {
	while (state.KeepRunning()){
		makeRandomPlane( state.range_x(), state.range_x() );
	}
}

static void BM_PlaneBlur( benchmark::State& state ) {
	while (state.KeepRunning()){
		makeRandomPlane( state.range_x(), state.range_x() ).blur_gaussian( state.range_y(), state.range_y() );
	}
}
static void BM_PlaneBlurBox( benchmark::State& state ) {
	while (state.KeepRunning()){
		makeRandomPlane( state.range_x(), state.range_x() ).blur_box( state.range_y(), state.range_y() );
	}
}
static void BM_PlaneEdgeLarge( benchmark::State& state ) {
	while (state.KeepRunning()){
		makeRandomPlane( state.range_x(), state.range_x() ).edge_laplacian_large();
	}
}
BENCHMARK(BM_PlaneCreation )->Range( 8, 1024 );
BENCHMARK(BM_PlaneFill     )->Range( 8, 1024 );
BENCHMARK(BM_PlaneRandom   )->Range( 8, 1024 );
BENCHMARK(BM_PlaneEdgeLarge)->Range( 8, 1024 );
BENCHMARK(BM_PlaneBlur     )->ArgPair(256,3)->ArgPair(256,5)->ArgPair(256,9)->ArgPair(256,15)->ArgPair(256,21);
BENCHMARK(BM_PlaneBlurBox  )->ArgPair(256,3)->ArgPair(256,5)->ArgPair(256,9)->ArgPair(256,15)->ArgPair(256,21);

BENCHMARK_MAIN();
