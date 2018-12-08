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

//package demosPackage;


// naive method to calculate a ConstantFolding number
public class ConstantFolding
{

	public static int ConstantFolding (int a, int b, int c)
	{
		if (8 > 9) return 8; else if (5 >= 4) return 5; else return 9;
	}


	public static int ConstantFolding (int[] a, int b, int c)
	{
		int x = 1;
		int k;
		for (int i = 0; i < c; i++) {
//			x = 3;
			if (i < 10) {
				x = 2;
				i ++;
			}
			k = i;	
			a[i] = x;
		}
		return x + k;
	}
}

