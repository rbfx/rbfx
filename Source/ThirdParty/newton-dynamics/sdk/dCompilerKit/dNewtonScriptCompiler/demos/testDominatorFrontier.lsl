/* Copyright (c) <2003-2016> <Newton Game Dynamics>
* 
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
* 
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely
*/


public class testDominatorFrontier
{
	public static void testDominatorFrontier ()
	{
		int i = 1;
		int j = 1;
		int k = 1;
		int l = 1;
		do {
			if (k < 1000) {
				j = i;
				if (l == 3) {
					l = 2;
				} else {
					l = 3;
				}
				k = k + 1;
			} else {
				k = k + 2;
			}

			do {
				if (l < 100) {
					l = l + 4;
				}
			} while (l < 100);

			i = i + 6;
		} while (i < 200);
	}
}




