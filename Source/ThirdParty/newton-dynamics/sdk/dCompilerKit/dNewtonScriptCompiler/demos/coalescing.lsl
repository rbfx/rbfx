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

// a very naive qsort for integers
public class coalescing
{
	public static int coalescing (int a, int b)
	{
		int d = 0;
		int e = a;
		do {
			d = d + b;
			e = e - 1;
		} while (e>0);
		return d;
	}
}




