/*******************************************************************************
Copyright (c) 2012, Jonathan Hiller (Cornell University)
If used in publication cite "J. Hiller and H. Lipson "Dynamic Simulation of Soft Heterogeneous Objects" In press. (2011)"

This file is part of Voxelyze.
Voxelyze is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Voxelyze is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/


#include "MarchCube.h"
#include <vector>

	static int edgeTable[256]={
	0x0  , 0x109, 0x203, 0x30a, 0x406, 0x50f, 0x605, 0x70c,
	0x80c, 0x905, 0xa0f, 0xb06, 0xc0a, 0xd03, 0xe09, 0xf00,
	0x190, 0x99 , 0x393, 0x29a, 0x596, 0x49f, 0x795, 0x69c,
	0x99c, 0x895, 0xb9f, 0xa96, 0xd9a, 0xc93, 0xf99, 0xe90,
	0x230, 0x339, 0x33 , 0x13a, 0x636, 0x73f, 0x435, 0x53c,
	0xa3c, 0xb35, 0x83f, 0x936, 0xe3a, 0xf33, 0xc39, 0xd30,
	0x3a0, 0x2a9, 0x1a3, 0xaa , 0x7a6, 0x6af, 0x5a5, 0x4ac,
	0xbac, 0xaa5, 0x9af, 0x8a6, 0xfaa, 0xea3, 0xda9, 0xca0,
	0x460, 0x569, 0x663, 0x76a, 0x66 , 0x16f, 0x265, 0x36c,
	0xc6c, 0xd65, 0xe6f, 0xf66, 0x86a, 0x963, 0xa69, 0xb60,
	0x5f0, 0x4f9, 0x7f3, 0x6fa, 0x1f6, 0xff , 0x3f5, 0x2fc,
	0xdfc, 0xcf5, 0xfff, 0xef6, 0x9fa, 0x8f3, 0xbf9, 0xaf0,
	0x650, 0x759, 0x453, 0x55a, 0x256, 0x35f, 0x55 , 0x15c,
	0xe5c, 0xf55, 0xc5f, 0xd56, 0xa5a, 0xb53, 0x859, 0x950,
	0x7c0, 0x6c9, 0x5c3, 0x4ca, 0x3c6, 0x2cf, 0x1c5, 0xcc ,
	0xfcc, 0xec5, 0xdcf, 0xcc6, 0xbca, 0xac3, 0x9c9, 0x8c0,
	0x8c0, 0x9c9, 0xac3, 0xbca, 0xcc6, 0xdcf, 0xec5, 0xfcc,
	0xcc , 0x1c5, 0x2cf, 0x3c6, 0x4ca, 0x5c3, 0x6c9, 0x7c0,
	0x950, 0x859, 0xb53, 0xa5a, 0xd56, 0xc5f, 0xf55, 0xe5c,
	0x15c, 0x55 , 0x35f, 0x256, 0x55a, 0x453, 0x759, 0x650,
	0xaf0, 0xbf9, 0x8f3, 0x9fa, 0xef6, 0xfff, 0xcf5, 0xdfc,
	0x2fc, 0x3f5, 0xff , 0x1f6, 0x6fa, 0x7f3, 0x4f9, 0x5f0,
	0xb60, 0xa69, 0x963, 0x86a, 0xf66, 0xe6f, 0xd65, 0xc6c,
	0x36c, 0x265, 0x16f, 0x66 , 0x76a, 0x663, 0x569, 0x460,
	0xca0, 0xda9, 0xea3, 0xfaa, 0x8a6, 0x9af, 0xaa5, 0xbac,
	0x4ac, 0x5a5, 0x6af, 0x7a6, 0xaa , 0x1a3, 0x2a9, 0x3a0,
	0xd30, 0xc39, 0xf33, 0xe3a, 0x936, 0x83f, 0xb35, 0xa3c,
	0x53c, 0x435, 0x73f, 0x636, 0x13a, 0x33 , 0x339, 0x230,
	0xe90, 0xf99, 0xc93, 0xd9a, 0xa96, 0xb9f, 0x895, 0x99c,
	0x69c, 0x795, 0x49f, 0x596, 0x29a, 0x393, 0x99 , 0x190,
	0xf00, 0xe09, 0xd03, 0xc0a, 0xb06, 0xa0f, 0x905, 0x80c,
	0x70c, 0x605, 0x50f, 0x406, 0x30a, 0x203, 0x109, 0x0   };

	static int triTable[256][16] =	{
		{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 1, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{1, 8, 3, 9, 8, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 8, 3, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{9, 2, 10, 0, 2, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{2, 8, 3, 2, 10, 8, 10, 9, 8, -1, -1, -1, -1, -1, -1, -1},
		{3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 11, 2, 8, 11, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{1, 9, 0, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{1, 11, 2, 1, 9, 11, 9, 8, 11, -1, -1, -1, -1, -1, -1, -1},
		{3, 10, 1, 11, 10, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 10, 1, 0, 8, 10, 8, 11, 10, -1, -1, -1, -1, -1, -1, -1},
		{3, 9, 0, 3, 11, 9, 11, 10, 9, -1, -1, -1, -1, -1, -1, -1},
		{9, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{4, 3, 0, 7, 3, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 1, 9, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{4, 1, 9, 4, 7, 1, 7, 3, 1, -1, -1, -1, -1, -1, -1, -1},
		{1, 2, 10, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{3, 4, 7, 3, 0, 4, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1},
		{9, 2, 10, 9, 0, 2, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1},
		{2, 10, 9, 2, 9, 7, 2, 7, 3, 7, 9, 4, -1, -1, -1, -1},
		{8, 4, 7, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{11, 4, 7, 11, 2, 4, 2, 0, 4, -1, -1, -1, -1, -1, -1, -1},
		{9, 0, 1, 8, 4, 7, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1},
		{4, 7, 11, 9, 4, 11, 9, 11, 2, 9, 2, 1, -1, -1, -1, -1},
		{3, 10, 1, 3, 11, 10, 7, 8, 4, -1, -1, -1, -1, -1, -1, -1},
		{1, 11, 10, 1, 4, 11, 1, 0, 4, 7, 11, 4, -1, -1, -1, -1},
		{4, 7, 8, 9, 0, 11, 9, 11, 10, 11, 0, 3, -1, -1, -1, -1},
		{4, 7, 11, 4, 11, 9, 9, 11, 10, -1, -1, -1, -1, -1, -1, -1},
		{9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{9, 5, 4, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 5, 4, 1, 5, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{8, 5, 4, 8, 3, 5, 3, 1, 5, -1, -1, -1, -1, -1, -1, -1},
		{1, 2, 10, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{3, 0, 8, 1, 2, 10, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1},
		{5, 2, 10, 5, 4, 2, 4, 0, 2, -1, -1, -1, -1, -1, -1, -1},
		{2, 10, 5, 3, 2, 5, 3, 5, 4, 3, 4, 8, -1, -1, -1, -1},
		{9, 5, 4, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 11, 2, 0, 8, 11, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1},
		{0, 5, 4, 0, 1, 5, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1},
		{2, 1, 5, 2, 5, 8, 2, 8, 11, 4, 8, 5, -1, -1, -1, -1},
		{10, 3, 11, 10, 1, 3, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1},
		{4, 9, 5, 0, 8, 1, 8, 10, 1, 8, 11, 10, -1, -1, -1, -1},
		{5, 4, 0, 5, 0, 11, 5, 11, 10, 11, 0, 3, -1, -1, -1, -1},
		{5, 4, 8, 5, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1},
		{9, 7, 8, 5, 7, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{9, 3, 0, 9, 5, 3, 5, 7, 3, -1, -1, -1, -1, -1, -1, -1},
		{0, 7, 8, 0, 1, 7, 1, 5, 7, -1, -1, -1, -1, -1, -1, -1},
		{1, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{9, 7, 8, 9, 5, 7, 10, 1, 2, -1, -1, -1, -1, -1, -1, -1},
		{10, 1, 2, 9, 5, 0, 5, 3, 0, 5, 7, 3, -1, -1, -1, -1},
		{8, 0, 2, 8, 2, 5, 8, 5, 7, 10, 5, 2, -1, -1, -1, -1},
		{2, 10, 5, 2, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1},
		{7, 9, 5, 7, 8, 9, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1},
		{9, 5, 7, 9, 7, 2, 9, 2, 0, 2, 7, 11, -1, -1, -1, -1},
		{2, 3, 11, 0, 1, 8, 1, 7, 8, 1, 5, 7, -1, -1, -1, -1},
		{11, 2, 1, 11, 1, 7, 7, 1, 5, -1, -1, -1, -1, -1, -1, -1},
		{9, 5, 8, 8, 5, 7, 10, 1, 3, 10, 3, 11, -1, -1, -1, -1},
		{5, 7, 0, 5, 0, 9, 7, 11, 0, 1, 0, 10, 11, 10, 0, -1},
		{11, 10, 0, 11, 0, 3, 10, 5, 0, 8, 0, 7, 5, 7, 0, -1},
		{11, 10, 5, 7, 11, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 8, 3, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{9, 0, 1, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{1, 8, 3, 1, 9, 8, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1},
		{1, 6, 5, 2, 6, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{1, 6, 5, 1, 2, 6, 3, 0, 8, -1, -1, -1, -1, -1, -1, -1},
		{9, 6, 5, 9, 0, 6, 0, 2, 6, -1, -1, -1, -1, -1, -1, -1},
		{5, 9, 8, 5, 8, 2, 5, 2, 6, 3, 2, 8, -1, -1, -1, -1},
		{2, 3, 11, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{11, 0, 8, 11, 2, 0, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1},
		{0, 1, 9, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1},
		{5, 10, 6, 1, 9, 2, 9, 11, 2, 9, 8, 11, -1, -1, -1, -1},
		{6, 3, 11, 6, 5, 3, 5, 1, 3, -1, -1, -1, -1, -1, -1, -1},
		{0, 8, 11, 0, 11, 5, 0, 5, 1, 5, 11, 6, -1, -1, -1, -1},
		{3, 11, 6, 0, 3, 6, 0, 6, 5, 0, 5, 9, -1, -1, -1, -1},
		{6, 5, 9, 6, 9, 11, 11, 9, 8, -1, -1, -1, -1, -1, -1, -1},
		{5, 10, 6, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{4, 3, 0, 4, 7, 3, 6, 5, 10, -1, -1, -1, -1, -1, -1, -1},
		{1, 9, 0, 5, 10, 6, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1},
		{10, 6, 5, 1, 9, 7, 1, 7, 3, 7, 9, 4, -1, -1, -1, -1},
		{6, 1, 2, 6, 5, 1, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1},
		{1, 2, 5, 5, 2, 6, 3, 0, 4, 3, 4, 7, -1, -1, -1, -1},
		{8, 4, 7, 9, 0, 5, 0, 6, 5, 0, 2, 6, -1, -1, -1, -1},
		{7, 3, 9, 7, 9, 4, 3, 2, 9, 5, 9, 6, 2, 6, 9, -1},
		{3, 11, 2, 7, 8, 4, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1},
		{5, 10, 6, 4, 7, 2, 4, 2, 0, 2, 7, 11, -1, -1, -1, -1},
		{0, 1, 9, 4, 7, 8, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1},
		{9, 2, 1, 9, 11, 2, 9, 4, 11, 7, 11, 4, 5, 10, 6, -1},
		{8, 4, 7, 3, 11, 5, 3, 5, 1, 5, 11, 6, -1, -1, -1, -1},
		{5, 1, 11, 5, 11, 6, 1, 0, 11, 7, 11, 4, 0, 4, 11, -1},
		{0, 5, 9, 0, 6, 5, 0, 3, 6, 11, 6, 3, 8, 4, 7, -1},
		{6, 5, 9, 6, 9, 11, 4, 7, 9, 7, 11, 9, -1, -1, -1, -1},
		{10, 4, 9, 6, 4, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{4, 10, 6, 4, 9, 10, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1},
		{10, 0, 1, 10, 6, 0, 6, 4, 0, -1, -1, -1, -1, -1, -1, -1},
		{8, 3, 1, 8, 1, 6, 8, 6, 4, 6, 1, 10, -1, -1, -1, -1},
		{1, 4, 9, 1, 2, 4, 2, 6, 4, -1, -1, -1, -1, -1, -1, -1},
		{3, 0, 8, 1, 2, 9, 2, 4, 9, 2, 6, 4, -1, -1, -1, -1},
		{0, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{8, 3, 2, 8, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1},
		{10, 4, 9, 10, 6, 4, 11, 2, 3, -1, -1, -1, -1, -1, -1, -1},
		{0, 8, 2, 2, 8, 11, 4, 9, 10, 4, 10, 6, -1, -1, -1, -1},
		{3, 11, 2, 0, 1, 6, 0, 6, 4, 6, 1, 10, -1, -1, -1, -1},
		{6, 4, 1, 6, 1, 10, 4, 8, 1, 2, 1, 11, 8, 11, 1, -1},
		{9, 6, 4, 9, 3, 6, 9, 1, 3, 11, 6, 3, -1, -1, -1, -1},
		{8, 11, 1, 8, 1, 0, 11, 6, 1, 9, 1, 4, 6, 4, 1, -1},
		{3, 11, 6, 3, 6, 0, 0, 6, 4, -1, -1, -1, -1, -1, -1, -1},
		{6, 4, 8, 11, 6, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{7, 10, 6, 7, 8, 10, 8, 9, 10, -1, -1, -1, -1, -1, -1, -1},
		{0, 7, 3, 0, 10, 7, 0, 9, 10, 6, 7, 10, -1, -1, -1, -1},
		{10, 6, 7, 1, 10, 7, 1, 7, 8, 1, 8, 0, -1, -1, -1, -1},
		{10, 6, 7, 10, 7, 1, 1, 7, 3, -1, -1, -1, -1, -1, -1, -1},
		{1, 2, 6, 1, 6, 8, 1, 8, 9, 8, 6, 7, -1, -1, -1, -1},
		{2, 6, 9, 2, 9, 1, 6, 7, 9, 0, 9, 3, 7, 3, 9, -1},
		{7, 8, 0, 7, 0, 6, 6, 0, 2, -1, -1, -1, -1, -1, -1, -1},
		{7, 3, 2, 6, 7, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{2, 3, 11, 10, 6, 8, 10, 8, 9, 8, 6, 7, -1, -1, -1, -1},
		{2, 0, 7, 2, 7, 11, 0, 9, 7, 6, 7, 10, 9, 10, 7, -1},
		{1, 8, 0, 1, 7, 8, 1, 10, 7, 6, 7, 10, 2, 3, 11, -1},
		{11, 2, 1, 11, 1, 7, 10, 6, 1, 6, 7, 1, -1, -1, -1, -1},
		{8, 9, 6, 8, 6, 7, 9, 1, 6, 11, 6, 3, 1, 3, 6, -1},
		{0, 9, 1, 11, 6, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{7, 8, 0, 7, 0, 6, 3, 11, 0, 11, 6, 0, -1, -1, -1, -1},
		{7, 11, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{3, 0, 8, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 1, 9, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{8, 1, 9, 8, 3, 1, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1},
		{10, 1, 2, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{1, 2, 10, 3, 0, 8, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1},
		{2, 9, 0, 2, 10, 9, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1},
		{6, 11, 7, 2, 10, 3, 10, 8, 3, 10, 9, 8, -1, -1, -1, -1},
		{7, 2, 3, 6, 2, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{7, 0, 8, 7, 6, 0, 6, 2, 0, -1, -1, -1, -1, -1, -1, -1},
		{2, 7, 6, 2, 3, 7, 0, 1, 9, -1, -1, -1, -1, -1, -1, -1},
		{1, 6, 2, 1, 8, 6, 1, 9, 8, 8, 7, 6, -1, -1, -1, -1},
		{10, 7, 6, 10, 1, 7, 1, 3, 7, -1, -1, -1, -1, -1, -1, -1},
		{10, 7, 6, 1, 7, 10, 1, 8, 7, 1, 0, 8, -1, -1, -1, -1},
		{0, 3, 7, 0, 7, 10, 0, 10, 9, 6, 10, 7, -1, -1, -1, -1},
		{7, 6, 10, 7, 10, 8, 8, 10, 9, -1, -1, -1, -1, -1, -1, -1},
		{6, 8, 4, 11, 8, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{3, 6, 11, 3, 0, 6, 0, 4, 6, -1, -1, -1, -1, -1, -1, -1},
		{8, 6, 11, 8, 4, 6, 9, 0, 1, -1, -1, -1, -1, -1, -1, -1},
		{9, 4, 6, 9, 6, 3, 9, 3, 1, 11, 3, 6, -1, -1, -1, -1},
		{6, 8, 4, 6, 11, 8, 2, 10, 1, -1, -1, -1, -1, -1, -1, -1},
		{1, 2, 10, 3, 0, 11, 0, 6, 11, 0, 4, 6, -1, -1, -1, -1},
		{4, 11, 8, 4, 6, 11, 0, 2, 9, 2, 10, 9, -1, -1, -1, -1},
		{10, 9, 3, 10, 3, 2, 9, 4, 3, 11, 3, 6, 4, 6, 3, -1},
		{8, 2, 3, 8, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1},
		{0, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{1, 9, 0, 2, 3, 4, 2, 4, 6, 4, 3, 8, -1, -1, -1, -1},
		{1, 9, 4, 1, 4, 2, 2, 4, 6, -1, -1, -1, -1, -1, -1, -1},
		{8, 1, 3, 8, 6, 1, 8, 4, 6, 6, 10, 1, -1, -1, -1, -1},
		{10, 1, 0, 10, 0, 6, 6, 0, 4, -1, -1, -1, -1, -1, -1, -1},
		{4, 6, 3, 4, 3, 8, 6, 10, 3, 0, 3, 9, 10, 9, 3, -1},
		{10, 9, 4, 6, 10, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{4, 9, 5, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 8, 3, 4, 9, 5, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1},
		{5, 0, 1, 5, 4, 0, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1},
		{11, 7, 6, 8, 3, 4, 3, 5, 4, 3, 1, 5, -1, -1, -1, -1},
		{9, 5, 4, 10, 1, 2, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1},
		{6, 11, 7, 1, 2, 10, 0, 8, 3, 4, 9, 5, -1, -1, -1, -1},
		{7, 6, 11, 5, 4, 10, 4, 2, 10, 4, 0, 2, -1, -1, -1, -1},
		{3, 4, 8, 3, 5, 4, 3, 2, 5, 10, 5, 2, 11, 7, 6, -1},
		{7, 2, 3, 7, 6, 2, 5, 4, 9, -1, -1, -1, -1, -1, -1, -1},
		{9, 5, 4, 0, 8, 6, 0, 6, 2, 6, 8, 7, -1, -1, -1, -1},
		{3, 6, 2, 3, 7, 6, 1, 5, 0, 5, 4, 0, -1, -1, -1, -1},
		{6, 2, 8, 6, 8, 7, 2, 1, 8, 4, 8, 5, 1, 5, 8, -1},
		{9, 5, 4, 10, 1, 6, 1, 7, 6, 1, 3, 7, -1, -1, -1, -1},
		{1, 6, 10, 1, 7, 6, 1, 0, 7, 8, 7, 0, 9, 5, 4, -1},
		{4, 0, 10, 4, 10, 5, 0, 3, 10, 6, 10, 7, 3, 7, 10, -1},
		{7, 6, 10, 7, 10, 8, 5, 4, 10, 4, 8, 10, -1, -1, -1, -1},
		{6, 9, 5, 6, 11, 9, 11, 8, 9, -1, -1, -1, -1, -1, -1, -1},
		{3, 6, 11, 0, 6, 3, 0, 5, 6, 0, 9, 5, -1, -1, -1, -1},
		{0, 11, 8, 0, 5, 11, 0, 1, 5, 5, 6, 11, -1, -1, -1, -1},
		{6, 11, 3, 6, 3, 5, 5, 3, 1, -1, -1, -1, -1, -1, -1, -1},
		{1, 2, 10, 9, 5, 11, 9, 11, 8, 11, 5, 6, -1, -1, -1, -1},
		{0, 11, 3, 0, 6, 11, 0, 9, 6, 5, 6, 9, 1, 2, 10, -1},
		{11, 8, 5, 11, 5, 6, 8, 0, 5, 10, 5, 2, 0, 2, 5, -1},
		{6, 11, 3, 6, 3, 5, 2, 10, 3, 10, 5, 3, -1, -1, -1, -1},
		{5, 8, 9, 5, 2, 8, 5, 6, 2, 3, 8, 2, -1, -1, -1, -1},
		{9, 5, 6, 9, 6, 0, 0, 6, 2, -1, -1, -1, -1, -1, -1, -1},
		{1, 5, 8, 1, 8, 0, 5, 6, 8, 3, 8, 2, 6, 2, 8, -1},
		{1, 5, 6, 2, 1, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{1, 3, 6, 1, 6, 10, 3, 8, 6, 5, 6, 9, 8, 9, 6, -1},
		{10, 1, 0, 10, 0, 6, 9, 5, 0, 5, 6, 0, -1, -1, -1, -1},
		{0, 3, 8, 5, 6, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{10, 5, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{11, 5, 10, 7, 5, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{11, 5, 10, 11, 7, 5, 8, 3, 0, -1, -1, -1, -1, -1, -1, -1},
		{5, 11, 7, 5, 10, 11, 1, 9, 0, -1, -1, -1, -1, -1, -1, -1},
		{10, 7, 5, 10, 11, 7, 9, 8, 1, 8, 3, 1, -1, -1, -1, -1},
		{11, 1, 2, 11, 7, 1, 7, 5, 1, -1, -1, -1, -1, -1, -1, -1},
		{0, 8, 3, 1, 2, 7, 1, 7, 5, 7, 2, 11, -1, -1, -1, -1},
		{9, 7, 5, 9, 2, 7, 9, 0, 2, 2, 11, 7, -1, -1, -1, -1},
		{7, 5, 2, 7, 2, 11, 5, 9, 2, 3, 2, 8, 9, 8, 2, -1},
		{2, 5, 10, 2, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1},
		{8, 2, 0, 8, 5, 2, 8, 7, 5, 10, 2, 5, -1, -1, -1, -1},
		{9, 0, 1, 5, 10, 3, 5, 3, 7, 3, 10, 2, -1, -1, -1, -1},
		{9, 8, 2, 9, 2, 1, 8, 7, 2, 10, 2, 5, 7, 5, 2, -1},
		{1, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 8, 7, 0, 7, 1, 1, 7, 5, -1, -1, -1, -1, -1, -1, -1},
		{9, 0, 3, 9, 3, 5, 5, 3, 7, -1, -1, -1, -1, -1, -1, -1},
		{9, 8, 7, 5, 9, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{5, 8, 4, 5, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1},
		{5, 0, 4, 5, 11, 0, 5, 10, 11, 11, 3, 0, -1, -1, -1, -1},
		{0, 1, 9, 8, 4, 10, 8, 10, 11, 10, 4, 5, -1, -1, -1, -1},
		{10, 11, 4, 10, 4, 5, 11, 3, 4, 9, 4, 1, 3, 1, 4, -1},
		{2, 5, 1, 2, 8, 5, 2, 11, 8, 4, 5, 8, -1, -1, -1, -1},
		{0, 4, 11, 0, 11, 3, 4, 5, 11, 2, 11, 1, 5, 1, 11, -1},
		{0, 2, 5, 0, 5, 9, 2, 11, 5, 4, 5, 8, 11, 8, 5, -1},
		{9, 4, 5, 2, 11, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{2, 5, 10, 3, 5, 2, 3, 4, 5, 3, 8, 4, -1, -1, -1, -1},
		{5, 10, 2, 5, 2, 4, 4, 2, 0, -1, -1, -1, -1, -1, -1, -1},
		{3, 10, 2, 3, 5, 10, 3, 8, 5, 4, 5, 8, 0, 1, 9, -1},
		{5, 10, 2, 5, 2, 4, 1, 9, 2, 9, 4, 2, -1, -1, -1, -1},
		{8, 4, 5, 8, 5, 3, 3, 5, 1, -1, -1, -1, -1, -1, -1, -1},
		{0, 4, 5, 1, 0, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{8, 4, 5, 8, 5, 3, 9, 0, 5, 0, 3, 5, -1, -1, -1, -1},
		{9, 4, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{4, 11, 7, 4, 9, 11, 9, 10, 11, -1, -1, -1, -1, -1, -1, -1},
		{0, 8, 3, 4, 9, 7, 9, 11, 7, 9, 10, 11, -1, -1, -1, -1},
		{1, 10, 11, 1, 11, 4, 1, 4, 0, 7, 4, 11, -1, -1, -1, -1},
		{3, 1, 4, 3, 4, 8, 1, 10, 4, 7, 4, 11, 10, 11, 4, -1},
		{4, 11, 7, 9, 11, 4, 9, 2, 11, 9, 1, 2, -1, -1, -1, -1},
		{9, 7, 4, 9, 11, 7, 9, 1, 11, 2, 11, 1, 0, 8, 3, -1},
		{11, 7, 4, 11, 4, 2, 2, 4, 0, -1, -1, -1, -1, -1, -1, -1},
		{11, 7, 4, 11, 4, 2, 8, 3, 4, 3, 2, 4, -1, -1, -1, -1},
		{2, 9, 10, 2, 7, 9, 2, 3, 7, 7, 4, 9, -1, -1, -1, -1},
		{9, 10, 7, 9, 7, 4, 10, 2, 7, 8, 7, 0, 2, 0, 7, -1},
		{3, 7, 10, 3, 10, 2, 7, 4, 10, 1, 10, 0, 4, 0, 10, -1},
		{1, 10, 2, 8, 7, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{4, 9, 1, 4, 1, 7, 7, 1, 3, -1, -1, -1, -1, -1, -1, -1},
		{4, 9, 1, 4, 1, 7, 0, 8, 1, 8, 7, 1, -1, -1, -1, -1},
		{4, 0, 3, 7, 4, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{4, 8, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{9, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{3, 0, 9, 3, 9, 11, 11, 9, 10, -1, -1, -1, -1, -1, -1, -1},
		{0, 1, 10, 0, 10, 8, 8, 10, 11, -1, -1, -1, -1, -1, -1, -1},
		{3, 1, 10, 11, 3, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{1, 2, 11, 1, 11, 9, 9, 11, 8, -1, -1, -1, -1, -1, -1, -1},
		{3, 0, 9, 3, 9, 11, 1, 2, 9, 2, 11, 9, -1, -1, -1, -1},
		{0, 2, 11, 8, 0, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{3, 2, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{2, 3, 8, 2, 8, 10, 10, 8, 9, -1, -1, -1, -1, -1, -1, -1},
		{9, 10, 2, 0, 9, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{2, 3, 8, 2, 8, 10, 0, 1, 8, 1, 10, 8, -1, -1, -1, -1},
		{1, 10, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{1, 3, 8, 9, 1, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 9, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 3, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}
};


CMarchCube::CMarchCube(void)
{
}

CMarchCube::~CMarchCube(void)
{
}

void CMarchCube::SingleMaterial(CMesh* pMeshOut, CArray3Df* pArray, float Thresh, float Scale)
{
	std::vector<CArray3Df> tmpVec; //put in an stl vector to call MultiMaterial()
	tmpVec.push_back(*pArray);
	CColor DefaultColor(0.5, 0.5, 0.5); //defaults to grey
	CMarchCube::MultiMaterial(pMeshOut, &tmpVec, true, &DefaultColor, Thresh, Scale);
}

void CMarchCube::SingleMaterialMultiColor(CMesh* pMeshOut, CArray3Df* pArray, CArray3Df* rColorArray, CArray3Df* gColorArray, CArray3Df* bColorArray, float Thresh, float Scale)
{	

	std::vector<CArray3Df> tmpVec; //put in an stl vector to call MultiMaterial()
	tmpVec.push_back(*pArray);

	
	//of course, assumes cubic structure!!!
	GRIDCELL GridCell;
	pMeshOut->Clear();
	
	//innefficient!
	CArray3Df Padded;
	CArray3Df aR;
	CArray3Df aG;
	CArray3Df aB;
	
	Padded.IniSpace(pArray->GetXSize()+2, pArray->GetYSize()+2, pArray->GetZSize()+2, -1e6);
	aR.IniSpace(Padded.GetXSize(), Padded.GetYSize(), Padded.GetZSize(), 0.0);
	aG.IniSpace(Padded.GetXSize(), Padded.GetYSize(), Padded.GetZSize(), 0.0);
	aB.IniSpace(Padded.GetXSize(), Padded.GetYSize(), Padded.GetZSize(), 0.0);
	
	for (int i=0; i<pArray->GetXSize(); i++)
		for (int j=0; j<pArray->GetYSize(); j++)
			for (int k=0; k<pArray->GetZSize(); k++){

				//existince array	
				Padded(i+1, j+1, k+1) = (*pArray)(i,j,k);

				//color arrays
				aR(i+1, j+1, k+1) = (*rColorArray)(i,j,k);
				aG(i+1, j+1, k+1) = (*gColorArray)(i,j,k);
				aB(i+1, j+1, k+1) = (*bColorArray)(i,j,k);
			}
	
	
	//form cubes
	float ai, aj, ak; //jmc: for speed, declare them only once
	
	for (int i=0; i<Padded.GetXSize()-1; i++){
		for (int j=0; j<Padded.GetYSize()-1; j++){
			for (int k=0; k<Padded.GetZSize()-1; k++){
				ai = i-0.5f;
				aj = j-0.5f;
				ak = k-0.5f;
				GridCell.p[0] = CVertex(Vec3D<>(Scale*ai, Scale*aj, Scale*ak), CColor(aR(i, j, k), aG(i, j, k), aB(i, j, k))); //put in ccppn-generated rgb here. 
				GridCell.val[0] = Padded(i, j, k);
				GridCell.p[1] = CVertex(Vec3D<>(Scale*(ai+1), Scale*aj, Scale*ak), CColor(aR(i+1, j, k), aG(i+1, j, k), aB(i+1, j, k)));
				GridCell.val[1] = Padded(i+1, j, k);
				GridCell.p[2] = CVertex(Vec3D<>(Scale*(ai+1), Scale*(aj+1), Scale*ak), CColor(aR(i+1, j+1, k), aG(i+1, j+1, k), aB(i+1, j+1, k)));
				GridCell.val[2] = Padded(i+1, j+1, k);
				GridCell.p[3] = CVertex(Vec3D<>(Scale*ai, Scale*(aj+1), Scale*ak), CColor(aR(i, j+1, k), aG(i, j+1, k), aB(i, j+1, k)));
				GridCell.val[3] = Padded(i, j+1, k);
				GridCell.p[4] = CVertex(Vec3D<>(Scale*ai, Scale*aj, Scale*(ak+1)), CColor(aR(i, j, k+1), aG(i, j, k+1), aB(i, j, k+1)));
				GridCell.val[4] = Padded(i, j, k+1);
				GridCell.p[5] = CVertex(Vec3D<>(Scale*(ai+1), Scale*aj, Scale*(ak+1)), CColor(aR(i+1, j, k+1), aG(i+1, j, k+1), aB(i+1, j, k+1)));
				GridCell.val[5] = Padded(i+1, j, k+1);
				GridCell.p[6] = CVertex(Vec3D<>(Scale*(ai+1), Scale*(aj+1), Scale*(ak+1)), CColor(aR(i+1, j+1, k+1), aG(i+1, j+1, k+1), aB(i+1, j+1, k+1)));
				GridCell.val[6] = Padded(i+1, j+1, k+1);
				GridCell.p[7] = CVertex(Vec3D<>(Scale*ai, Scale*(aj+1), Scale*(ak+1)), CColor(aR(i, j+1, k+1), aG(i, j+1, k+1), aB(i, j+1, k+1)));
				GridCell.val[7] = Padded(i, j+1, k+1);
				
				Polygonise(GridCell, Thresh, pMeshOut); //crunch this cube and add the vertices...
			}
		}
	}
	
	pMeshOut->WeldClose(Scale/2);
}


void CMarchCube::MultiMaterial(CMesh* pMeshOut, void* pArrays, bool SumMat, CColor* pColors, float Thresh, float Scale)
{
	std::vector<CArray3Df>* pDA = (std::vector<CArray3Df>*) pArrays;
	int NumMat = (*pDA).size();

	int Resolution=1;
	Scale=Scale/Resolution;

	//of course, assumes cubic structure!!!
	GRIDCELL GridCell;
	pMeshOut->Clear();

	//innefficient!
	CArray3Df Padded;
	CArray3Df aR;
	CArray3Df aG;
	CArray3Df aB;
	
	Padded.IniSpace(((*pDA)[0].GetXSize()*Resolution)+2, ((*pDA)[0].GetYSize()*Resolution)+2, ((*pDA)[0].GetZSize()*Resolution)+2, 0);
	aR.IniSpace(Padded.GetXSize()*Resolution, Padded.GetYSize()*Resolution, Padded.GetZSize()*Resolution, 0.0);
	aG.IniSpace(Padded.GetXSize()*Resolution, Padded.GetYSize()*Resolution, Padded.GetZSize()*Resolution, 0.0);
	aB.IniSpace(Padded.GetXSize()*Resolution, Padded.GetYSize()*Resolution, Padded.GetZSize()*Resolution, 0.0);

	for (int i=0; i<(*pDA)[0].GetXSize()*Resolution; i++)
		for (int j=0; j<(*pDA)[0].GetYSize()*Resolution; j++)
			for (int k=0; k<(*pDA)[0].GetZSize()*Resolution; k++){

				//find the max, for color (or for thresh, as well if !SumMat)
				float max = -9e9f;
//				float max = 0.0f;

				for (int  m=0; m<NumMat; m++){
					if ((*pDA)[m](i, j, k) > max){
						max = (*pDA)[m](i, j, k);
						if (pColors != NULL){ //grab the colors!
							aR(i+1, j+1, k+1) = pColors[m].r;
							aG(i+1, j+1, k+1) = pColors[m].g;
							aB(i+1, j+1, k+1) = pColors[m].b;
							//fill in outside of padded here if we wish...
							}
						}
					}

				if (SumMat){ //if we're summing the materials to create the mesh...
					for (int  m=0; m<NumMat; m++){
						float ToAdd = ((*pDA)[m])(i, j, k);
						if (m==0) Padded(i+1, j+1, k+1) = ToAdd;
						else Padded(i+1, j+1, k+1) += ToAdd;
					}
				}
				else { //if we're not summing the materials...
					Padded(i+1, j+1, k+1) = max;
				}
				//TRACE("%f\n", Padded(i+1, j+1, k+1));
				}


	//form cubes
	float ai, aj, ak; //jmc: for speed, declare them only once
	
	for (int i=0; i<Padded.GetXSize()*Resolution-1; i++){
		for (int j=0; j<Padded.GetYSize()*Resolution-1; j++){
			for (int k=0; k<Padded.GetZSize()*Resolution-1; k++){
				ai = i-0.5f;
				aj = j-0.5f;
				ak = k-0.5f;
				GridCell.p[0] = CVertex(Vec3D<>(Scale*ai, Scale*aj, Scale*ak), CColor(aR(i, j, k), aG(i, j, k), aB(i, j, k))); 
				GridCell.val[0] = Padded(i, j, k);
				GridCell.p[1] = CVertex(Vec3D<>(Scale*(ai+1), Scale*aj, Scale*ak), CColor(aR(i+1, j, k), aG(i+1, j, k), aB(i+1, j, k)));
				GridCell.val[1] = Padded(i+1, j, k);
				GridCell.p[2] = CVertex(Vec3D<>(Scale*(ai+1), Scale*(aj+1), Scale*ak), CColor(aR(i+1, j+1, k), aG(i+1, j+1, k), aB(i+1, j+1, k)));
				GridCell.val[2] = Padded(i+1, j+1, k);
				GridCell.p[3] = CVertex(Vec3D<>(Scale*ai, Scale*(aj+1), Scale*ak), CColor(aR(i, j+1, k), aG(i, j+1, k), aB(i, j+1, k)));
				GridCell.val[3] = Padded(i, j+1, k);
				GridCell.p[4] = CVertex(Vec3D<>(Scale*ai, Scale*aj, Scale*(ak+1)), CColor(aR(i, j, k+1), aG(i, j, k+1), aB(i, j, k+1)));
				GridCell.val[4] = Padded(i, j, k+1);
				GridCell.p[5] = CVertex(Vec3D<>(Scale*(ai+1), Scale*aj, Scale*(ak+1)), CColor(aR(i+1, j, k+1), aG(i+1, j, k+1), aB(i+1, j, k+1)));
				GridCell.val[5] = Padded(i+1, j, k+1);
				GridCell.p[6] = CVertex(Vec3D<>(Scale*(ai+1), Scale*(aj+1), Scale*(ak+1)), CColor(aR(i+1, j+1, k+1), aG(i+1, j+1, k+1), aB(i+1, j+1, k+1)));
				GridCell.val[6] = Padded(i+1, j+1, k+1);
				GridCell.p[7] = CVertex(Vec3D<>(Scale*ai, Scale*(aj+1), Scale*(ak+1)), CColor(aR(i, j+1, k+1), aG(i, j+1, k+1), aB(i, j+1, k+1)));
				GridCell.val[7] = Padded(i, j+1, k+1);

				Polygonise(GridCell, Thresh, pMeshOut); //crunch this cube and add the vertices...
			}
		}
	}

	pMeshOut->WeldClose(Scale/2);
	pMeshOut->CalcFaceNormals();
	pMeshOut->CalcVertNormals();
	//pMeshOut->WeldClose(Scale);
}


int CMarchCube::Polygonise(GRIDCELL grid, double iso, CMesh* pMeshOut)
{
	bool UseTets = false;
	if (UseTets){
		PolygoniseTet(grid, iso, pMeshOut, 0, 2, 3, 7);
		PolygoniseTet(grid, iso, pMeshOut, 0, 2, 6, 7);
		PolygoniseTet(grid, iso, pMeshOut, 0, 4, 6, 7);
		PolygoniseTet(grid, iso, pMeshOut, 0, 6, 1, 2);
		PolygoniseTet(grid, iso, pMeshOut, 0, 6, 1, 4);
		PolygoniseTet(grid, iso, pMeshOut, 5, 6, 1, 4);
	}
	else
		PolygoniseCube(grid, iso, pMeshOut);

	return 0;
}


int CMarchCube::PolygoniseCube(GRIDCELL grid, double iso, CMesh* pMeshOut)
{
	//(http://local.wasp.uwa.edu.au/~pbourke/geometry/polygonise/)
	int i,ntriang;
	int cubeindex;
	CVertex vertlist[12];

	//Determine the index into the edge table which tells us which vertices are inside of the surface
	cubeindex = 0;
	if (grid.val[0] < iso) cubeindex |= 1;
	if (grid.val[1] < iso) cubeindex |= 2;
	if (grid.val[2] < iso) cubeindex |= 4;
	if (grid.val[3] < iso) cubeindex |= 8;
	if (grid.val[4] < iso) cubeindex |= 16;
	if (grid.val[5] < iso) cubeindex |= 32;
	if (grid.val[6] < iso) cubeindex |= 64;
	if (grid.val[7] < iso) cubeindex |= 128;

	/* Cube is entirely in/out of the surface */
	if (edgeTable[cubeindex] == 0)
		return(0);

	/* Find the vertices where the surface intersects the cube */
	if (edgeTable[cubeindex] & 1)
		vertlist[0] = VertexInterp(iso,grid.p[0],grid.p[1],grid.val[0],grid.val[1]);
	if (edgeTable[cubeindex] & 2)
		vertlist[1] = VertexInterp(iso,grid.p[1],grid.p[2],grid.val[1],grid.val[2]);
	if (edgeTable[cubeindex] & 4)
		vertlist[2] = VertexInterp(iso,grid.p[2],grid.p[3],grid.val[2],grid.val[3]);
	if (edgeTable[cubeindex] & 8)
		vertlist[3] = VertexInterp(iso,grid.p[3],grid.p[0],grid.val[3],grid.val[0]);
	if (edgeTable[cubeindex] & 16)
		vertlist[4] = VertexInterp(iso,grid.p[4],grid.p[5],grid.val[4],grid.val[5]);
	if (edgeTable[cubeindex] & 32)
		vertlist[5] = VertexInterp(iso,grid.p[5],grid.p[6],grid.val[5],grid.val[6]);
	if (edgeTable[cubeindex] & 64)
		vertlist[6] = VertexInterp(iso,grid.p[6],grid.p[7],grid.val[6],grid.val[7]);
	if (edgeTable[cubeindex] & 128)
		vertlist[7] = VertexInterp(iso,grid.p[7],grid.p[4],grid.val[7],grid.val[4]);
	if (edgeTable[cubeindex] & 256)
		vertlist[8] = VertexInterp(iso,grid.p[0],grid.p[4],grid.val[0],grid.val[4]);
	if (edgeTable[cubeindex] & 512)
		vertlist[9] = VertexInterp(iso,grid.p[1],grid.p[5],grid.val[1],grid.val[5]);
	if (edgeTable[cubeindex] & 1024)
		vertlist[10] = VertexInterp(iso,grid.p[2],grid.p[6],grid.val[2],grid.val[6]);
	if (edgeTable[cubeindex] & 2048)
		vertlist[11] = VertexInterp(iso,grid.p[3],grid.p[7],grid.val[3],grid.val[7]);

	/* Create the triangle */
	ntriang = 0;
	for (i=0;triTable[cubeindex][i]!=-1;i+=3) {
		pMeshOut->AddFacet(vertlist[triTable[cubeindex][i]].v, vertlist[triTable[cubeindex][i+1]].v, vertlist[triTable[cubeindex][i+2]].v, vertlist[triTable[cubeindex][i]].VColor, vertlist[triTable[cubeindex][i+1]].VColor, vertlist[triTable[cubeindex][i+2]].VColor, false);
		ntriang++;
	}

	return(ntriang);
}

//---------------------------------------------------------------------------
int CMarchCube::PolygoniseTet(GRIDCELL g, double iso, CMesh* pMeshOut, int v0,int v1,int v2,int v3)
//---------------------------------------------------------------------------
{// CDataList* pShortlist, GRIDCELL grid,double isolevel, CGeometry* pGeom

	int ntri = 0;
//	int triindex = 0;
	Vec3D<> t0,t1,t2;
/*
	// Determine which of the 16 cases we have given which vertices
	// are above or below the isosurface

	if (g.val[v0] < iso) triindex |= 1;
	if (g.val[v1] < iso) triindex |= 2;
	if (g.val[v2] < iso) triindex |= 4;
	if (g.val[v3] < iso) triindex |= 8;

	//CTriangle tri;

	// Form the vertices of the triangles for each case

	switch (triindex) {
   case 0x00: 
   case 0x0F:
	   break;
   case 0x0E:
   case 0x01:
	   t0=VertexInterp(iso,g.p[v0],g.p[v1],g.val[v0],g.val[v1]);
	   t1=VertexInterp(iso,g.p[v0],g.p[v2],g.val[v0],g.val[v2]);
	   t2=VertexInterp(iso,g.p[v0],g.p[v3],g.val[v0],g.val[v3]);
	   pMeshOut->AddFacet(t0, t1, t2);
	   ntri++;
	   break;
   case 0x0D:
   case 0x02:
	   t0=VertexInterp(iso,g.p[v1],g.p[v0],g.val[v1],g.val[v0]);
	   t1=VertexInterp(iso,g.p[v1],g.p[v3],g.val[v1],g.val[v3]);
	   t2=VertexInterp(iso,g.p[v1],g.p[v2],g.val[v1],g.val[v2]);
	   pMeshOut->AddFacet(t0, t1, t2);
	   ntri++;
	   break;
   case 0x0C:
   case 0x03:
	   t0=VertexInterp(iso,g.p[v0],g.p[v3],g.val[v0],g.val[v3]);
	   t1=VertexInterp(iso,g.p[v0],g.p[v2],g.val[v0],g.val[v2]);
	   t2=VertexInterp(iso,g.p[v1],g.p[v3],g.val[v1],g.val[v3]);
	   pMeshOut->AddFacet(t0, t1, t2);
	   ntri++;
	   t0=VertexInterp(iso,g.p[v1],g.p[v3],g.val[v1],g.val[v3]);
	   t1=VertexInterp(iso,g.p[v1],g.p[v2],g.val[v1],g.val[v2]);
	   t2=VertexInterp(iso,g.p[v0],g.p[v2],g.val[v0],g.val[v2]);;
	   pMeshOut->AddFacet(t0, t1, t2);
	   ntri++;
	   break;
   case 0x0B:
   case 0x04:
	   t0=VertexInterp(iso,g.p[v2],g.p[v0],g.val[v2],g.val[v0]);
	   t1=VertexInterp(iso,g.p[v2],g.p[v1],g.val[v2],g.val[v1]);
	   t2=VertexInterp(iso,g.p[v2],g.p[v3],g.val[v2],g.val[v3]);
	   pMeshOut->AddFacet(t0, t1, t2);
	   ntri++;
	   break;
   case 0x0A:
   case 0x05:
	   t0=VertexInterp(iso,g.p[v0],g.p[v1],g.val[v0],g.val[v1]);
	   t1=VertexInterp(iso,g.p[v2],g.p[v3],g.val[v2],g.val[v3]);
	   t2=VertexInterp(iso,g.p[v0],g.p[v3],g.val[v0],g.val[v3]);
	   pMeshOut->AddFacet(t0, t1, t2); //not ok
	   ntri++;
	   t0=VertexInterp(iso,g.p[v0],g.p[v1],g.val[v0],g.val[v1]);
	   t1=VertexInterp(iso,g.p[v1],g.p[v2],g.val[v1],g.val[v2]);
	   t2=VertexInterp(iso,g.p[v2],g.p[v3],g.val[v2],g.val[v3]);
	   pMeshOut->AddFacet(t0, t1, t2); // ok
	   ntri++;
	   break;
   case 0x09:
   case 0x06:
	   t0=VertexInterp(iso,g.p[v0],g.p[v1],g.val[v0],g.val[v1]);
	   t1=VertexInterp(iso,g.p[v1],g.p[v3],g.val[v1],g.val[v3]);
	   t2=VertexInterp(iso,g.p[v2],g.p[v3],g.val[v2],g.val[v3]);
	   pMeshOut->AddFacet(t0, t1, t2); // ok
	   ntri++;
	   t0=VertexInterp(iso,g.p[v0],g.p[v1],g.val[v0],g.val[v1]);;
	   t1=VertexInterp(iso,g.p[v0],g.p[v2],g.val[v0],g.val[v2]);
	   t2=VertexInterp(iso,g.p[v2],g.p[v3],g.val[v2],g.val[v3]);
	   pMeshOut->AddFacet(t0, t1, t2); // ok
	   ntri++;
	   break;
   case 0x07:
   case 0x08:
	   t0=VertexInterp(iso,g.p[v3],g.p[v0],g.val[v3],g.val[v0]);
	   t1=VertexInterp(iso,g.p[v3],g.p[v2],g.val[v3],g.val[v2]);
	   t2=VertexInterp(iso,g.p[v3],g.p[v1],g.val[v3],g.val[v1]);
	   pMeshOut->AddFacet(t0, t1, t2); // ok
	   ntri++;
	   break;
	}
*/
	return(ntri);
	
}

//Linearly interpolate the position where an isosurface cuts an edge between two vertices, each with their own scalar value
CVertex CMarchCube::VertexInterp(double iso, CVertex p1, CVertex p2, double valp1, double valp2)
{
	if (fabs(iso-valp1) < 0.00001)
	return(p1);
	if (fabs(iso-valp2) < 0.00001)
	return(p2);
	if (fabs(valp1-valp2) < 0.00001)
	return(p1);
	
	double mu = (iso - valp1) / (valp2 - valp1);
	CVertex p;

	p.v.x = p1.v.x + mu * (p2.v.x - p1.v.x);
	p.v.y = p1.v.y + mu * (p2.v.y - p1.v.y);
	p.v.z = p1.v.z + mu * (p2.v.z - p1.v.z);
	p.VColor.r = p1.VColor.r + mu * (p2.VColor.r - p1.VColor.r);
	p.VColor.g = p1.VColor.g + mu * (p2.VColor.g - p1.VColor.g);
	p.VColor.b = p1.VColor.b + mu * (p2.VColor.b - p1.VColor.b);

	return(p);
}




