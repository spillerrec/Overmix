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
BENCHMARK(BM_PlaneCreation)->Arg(64)->Arg(128)->Arg(256)->Arg(512)->Arg(1024)->Arg(2048)->Arg(4096);
BENCHMARK(BM_PlaneFill    )->Arg(64)->Arg(128)->Arg(256)->Arg(512)->Arg(1024)->Arg(2048)->Arg(4096);

BENCHMARK_MAIN();
