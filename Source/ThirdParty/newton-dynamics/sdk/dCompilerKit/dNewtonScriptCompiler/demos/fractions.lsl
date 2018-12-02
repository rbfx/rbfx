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

// a simple integer number fraction class 
class fraction
{
	fraction (int numerator, int denominator)
	{
		m_numerator = numerator;
		m_denominator = denominator;
		Reduce ();
	}

	//  Euclid's algorithm for calculating the greatest common divisor ofd tow integers
	private int gcd(int a, int b)
	{
		while(b) {
			int temp = a % b; 
			a = b; 
			b = temp;
		}
		return (a);
	}
	
	private void Reduce ()
	{
		int divisor = gcd(m_numerator, m_denominator);
		m_numerator = m_numerator / divisor;
		m_denominator = m_denominator / divisor;
	}

	// sample of operaotr overload +
	class fraction operator + (class fraction argument)
	{
		// find lowest common denominator
		int lcd = m_denominator / gcd(m_denominator, argument.m_denominator) * argument.m_denominator;
	
		int num1 = lcd / m_denominator * m_numerator;
		int num2 = lcd / argument.m_denominator * argument.m_numerator;
		return new class fraction (num1 + num2, lcd);
	}

	// sample of operator overload +
	class fraction operator - (class fraction argument)
	{
		// find lowest common denominator
		int lcd = m_denominator / gcd(m_denominator, argument.m_denominator) * argument.m_denominator;
	
		int num1 = lcd / m_denominator * m_numerator;
		int num2 = lcd / argument.m_denominator * argument.m_numerator;
		return new class fraction (num1 - num2, lcd);
	}
	
	int m_numerator;
	int m_denominator;
}

