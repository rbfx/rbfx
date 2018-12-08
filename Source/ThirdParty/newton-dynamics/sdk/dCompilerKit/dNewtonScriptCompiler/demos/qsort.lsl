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
public class qsort
{

	public static void qsort (int[] a, int left, int right)
	{
      int i = left, j = right;
      int pivot = a[(left + right) / 2];

      while (i <= j) {
		i--;
		do {} while (a[++i] < pivot);

		j++;
		do {} while (a[--j] < pivot);
        if (i <= j) {
             int tmp = a[i];
             a[i] = a[j];
             a[j] = tmp;
             i++;
             j--;
        }
      }

      if (left < j)
		qsort (a, left, j);
	  if (i < right)
		qsort (a, i, right);
	}

}




