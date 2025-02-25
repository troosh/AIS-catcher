/*
Copyright(c) 2021 jvde.github@gmail.com

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include <vector>
#include <complex>

#include "Common.h"

namespace FFT
{
	static int log2(int x)
	{
		int y = 0;
		while (x >>= 1) y++;
		return y;
	}

	static int rev(int x,int logN)
	{
		int y = 0;

		for(int i = 0; i < logN; i++)
		{
			y <<= 1;
			y |= (x & 1);
			x >>= 1;
		}
		return y;
	}

	template <typename T>
	static void copyBitReverse(const std::vector<std::complex<T>> &in, std::vector<std::complex<T>> &out,int logN)
	{
		for (int i = 0; i < in.size(); i++)
			out[rev(i, logN)] = in[i];
	}

	template <typename T>
	static void calcOmega(std::vector<std::complex<T>> &Omega,int logN)
	{
		Omega.resize(logN);

		for (int s = 0, m = 2; s < logN; s++, m *= 2)
			Omega[s] = std::polar(T(1), T(-2.0 * PI / m));
	}


	// UNOPTIMIZED Cooley-Tukey textbook implementation
	template <typename T>
	void fft(std::vector<std::complex<T>> &x)
	{
		std::complex<T> o, u, t;
		static std::vector<std::complex<T>> Omega;

		int N = x.size();
		int logN = log2(N);
		int m = 2, m2 = 1;

		if(Omega.size()<logN) calcOmega(Omega,logN);

		for(int s = 0; s < logN; s++)
		{
			for(int k = 0; k < N; k+= m)
			{
				o = T(1.0);

				for(int j = 0; j < m2; j++)
				{
					t = o * x[k + j + m2];
					u = x[k + j];

					x[k + j] = u + t;
					x[k + j + m2] = u - t;

					o *= Omega[s];
				}
			}
			m <<= 1;
			m2 <<= 1;
		}
	}
}
