/*******************************************************************************
Copyright (c) 2010, Jonathan Hiller (Cornell University)
If used in publication cite "J. Hiller and H. Lipson "Dynamic Simulation of Soft Heterogeneous Objects" In press. (2011)"

This file is part of Voxelyze.
Voxelyze is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Voxelyze is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#ifndef VX_BENCHMARK_H
#define VX_BENCHMARK_H

class CVX_Benchmark
{
public:
	CVX_Benchmark(void); //!< Constructor
	~CVX_Benchmark(void); //!< Destructor
	CVX_Benchmark& operator=(const CVX_Benchmark& rBenchmark); //!< Overload "=" 

	bool AxialSimpleTest();

};

#endif //VX_BENCHMARK_H