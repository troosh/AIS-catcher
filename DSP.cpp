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

#include <chrono>
#include <cassert>
#include <complex>

#include "FFT.h"
#include "DSP.h"

namespace DSP
{
	void SamplerPLL::Receive(const FLOAT32* data, int len)
	{
		for (int i = 0; i < len; i++)
		{
			BIT bit = (data[i] > 0);

			if (bit != prev)
			{
				PLL += (0.5f - PLL) * (FastPLL ? 0.6f : 0.05f);
			}

			PLL += 0.2f;

			if (PLL >= 1.0f)
			{
				sendOut(&data[i], 1);
				PLL -= (int) PLL;
			}
			prev = bit;
		}
	}

	void SamplerPLL::Message(const DecoderMessages& in)
	{
		switch (in)
		{
		case DecoderMessages::StartTraining: FastPLL = true; break;
		case DecoderMessages::StopTraining: FastPLL = false; break;
		default: break;
		}
	}

	void SamplerParallel::setBuckets(int n)
	{
		nBuckets = n;
		out.resize(nBuckets);
	}

	void SamplerParallel::Receive(const FLOAT32* data, int len)
	{
		for (int i = 0; i < len; i++)
		{
			out[lastSymbol].Send(&data[i], 1);
			lastSymbol = (lastSymbol + 1) % nBuckets;
		}
	}

	void SamplerParallelComplex::setBuckets(int n)
	{
		nBuckets = n;
		out.resize(nBuckets);
	}

	void SamplerParallelComplex::Receive(const CFLOAT32* data, int len)
	{
		for (int i = 0; i < len; i++)
		{
			out[lastSymbol].Send(&data[i], 1);
			lastSymbol = (lastSymbol + 1) % nBuckets;
		}
	}

// helper macros for moving averages
#define MA1(idx)		r##idx = z; z += h##idx;
#define MA2(idx)		h##idx = z; z += r##idx;

// CIC5 downsample

	void Downsample2CIC5::Receive(const CFLOAT32* data, int len)
	{
		assert(len % 2 == 0);

		if (output.size() < len / 2) output.resize(len / 2);

		CFLOAT32 z, r0, r1, r2, r3, r4;

		for (int i = 0, j = 0; i < len; i += 2, j++)
		{
			z = data[i];
			MA1(0); MA1(1); MA1(2); MA1(3); MA1(4);
			output[j] = z * (FLOAT32)0.03125f;
			z = data[i + 1];
			MA2(0); MA2(1); MA2(2); MA2(3); MA2(4);
		}

		sendOut(output.data(), len / 2);
	}

	int Downsample2CS32::Run(CS32* data, int len)
	{
		CS32 z, r0, r1, r2, r3, r4;

		for (int i = 0, j = 0; i < len; i += 2, j++)
		{
			z = data[i];
			MA1(0); MA1(1); MA1(2); MA1(3); MA1(4);
			data[j] = z;
			z = data[i + 1];
			MA2(0); MA2(1); MA2(2); MA2(3); MA2(4);
		}
		return len/2;
	}

	void Decimate2::Receive(const CFLOAT32* data, int len)
	{
		assert(len % 2 == 0);

		if (output.size() < len / 2) output.resize(len / 2);

		for (int i = 0, j = 0; i < len; i += 2, j++)
		{
			output[j] = data[i];
		}

		sendOut(output.data(), len / 2);
	}

	// FilterCIC5

	void FilterCIC5::Receive(const CFLOAT32* data, int len)
	{
		CFLOAT32 z, r0, r1, r2, r3, r4;

		assert(len % 2 == 0);

		if (output.size() < len) output.resize(len);

		for (int i = 0; i < len; i += 2)
		{
			z = data[i];
			MA1(0); MA1(1); MA1(2); MA1(3); MA1(4);
			output[i] = z * (FLOAT32)0.03125f;
			z = data[i + 1];
			MA2(0); MA2(1); MA2(2); MA2(3); MA2(4);
			output[i + 1] = z * (FLOAT32)0.03125f;
		}

		sendOut(output.data(), len);
	}

	void Downsample3Complex::Receive(const CFLOAT32* data, int len)
	{
		assert(len % 3 == 0);

		int ptr, i, j;

		if (output.size() < len/3) output.resize(len/3);

		for (j = i = 0, ptr = 21 - 1; i < 21 - 1; i += 3, j++)
		{
			buffer[ptr++] = data[i];
			buffer[ptr++] = data[i + 1];
			buffer[ptr++] = data[i + 2];

			CFLOAT32 x = 0.33292088503f * buffer[i + 10];
			x -= 0.00101073661f * (buffer[i + 0] + buffer[i + 20]);
			x += 0.00616649466f * (buffer[i + 2] + buffer[i + 18]);
			x += 0.01130778123f * (buffer[i + 3] + buffer[i + 17]);
			x -= 0.03044260089f * (buffer[i + 5] + buffer[i + 15]);
			x -= 0.04750748661f * (buffer[i + 6] + buffer[i + 14]);
			x += 0.12579695977f * (buffer[i + 8] + buffer[i + 12]);
			x += 0.26922914593f * (buffer[i + 9] + buffer[i + 11]);

			output[j] = x;
		}

		for (i = 1; i < len - 21 + 1; i += 3, j++)
		{
			CFLOAT32 x = 0.33292088503f * data[i + 10];
			x -= 0.00101073661f * (data[i + 0] + data[i + 20]);
			x += 0.00616649466f * (data[i + 2] + data[i + 18]);
			x += 0.01130778123f * (data[i + 3] + data[i + 17]);
			x -= 0.03044260089f * (data[i + 5] + data[i + 15]);
			x -= 0.04750748661f * (data[i + 6] + data[i + 14]);
			x += 0.12579695977f * (data[i + 8] + data[i + 12]);
			x += 0.26922914593f * (data[i + 9] + data[i + 11]);

			output[j] = x;
		}
		for (ptr = 0; i < len; i++, ptr++)
		{
			buffer[ptr] = data[i];
		}

		sendOut(output.data(), len / 3);
	}

	void Downsample5Complex::Receive(const CFLOAT32* data, int len)
	{
		assert(len % 5 == 0);

		int ptr, i, j;

		if (output.size() < len/5) output.resize(len/5);

		for (j = i = 0, ptr = 19 - 1; i < 19 - 1; i += 5, j++)
		{
			buffer[ptr++] = data[i];
			buffer[ptr++] = data[i + 1];
			buffer[ptr++] = data[i + 2];
			buffer[ptr++] = data[i + 3];
			buffer[ptr++] = data[i + 4];

			CFLOAT32 x = 0.31070225733f * buffer[i + 9];
			x -= 0.02029180052f * (buffer[i + 0] + buffer[i + 18]);
			x -= 0.03693692581f * (buffer[i + 1] + buffer[i + 17]);
			x -= 0.04221362949f * (buffer[i + 2] + buffer[i + 16]);
			x -= 0.03043770079f * (buffer[i + 3] + buffer[i + 15]);
			x += 0.04565655118f * (buffer[i + 5] + buffer[i + 13]);
			x += 0.09849846882f * (buffer[i + 6] + buffer[i + 12]);
			x += 0.14774770323f * (buffer[i + 7] + buffer[i + 11]);
			x += 0.18262620471f * (buffer[i + 8] + buffer[i + 10]);

			output[j] = x;
		}

		for (i = 1; i < len - 19 + 1; i += 5, j++)
		{
			CFLOAT32 x = 0.31070225733f * data[i + 9];
			x -= 0.02029180052f * (data[i + 0] + data[i + 18]);
			x -= 0.03693692581f * (data[i + 1] + data[i + 17]);
			x -= 0.04221362949f * (data[i + 2] + data[i + 16]);
			x -= 0.03043770079f * (data[i + 3] + data[i + 15]);
			x += 0.04565655118f * (data[i + 5] + data[i + 13]);
			x += 0.09849846882f * (data[i + 6] + data[i + 12]);
			x += 0.14774770323f * (data[i + 7] + data[i + 11]);
			x += 0.18262620471f * (data[i + 8] + data[i + 10]);

			output[j] = x;
		}
		for (ptr = 0; i < len; i++, ptr++)
		{
			buffer[ptr] = data[i];
		}

		sendOut(output.data(), len / 5);
	}
	// Filter Generic

	void FilterComplex::Receive(const CFLOAT32* data, int len)
	{
		int ptr, i, j;

		if (output.size() < len) output.resize(len);

		for (j = i = 0, ptr = taps.size() - 1; i < taps.size() - 1; i++, ptr++, j++)
		{
			buffer[ptr] = data[i];
			output[j] = filter(&buffer[i]);
		}

		for (i = 0; i < len - taps.size() + 1; i++, j++)
		{
			output[j] = filter(&data[i]);
		}

		for (ptr = 0; i < len; i++, ptr++)
		{
			buffer[ptr] = data[i];
		}

		sendOut(output.data(), len);
	}

	void Filter::Receive(const FLOAT32* data, int len)
	{
		int ptr, i, j;

		if (output.size() < len) output.resize(len);

		for (j = i = 0, ptr = taps.size() - 1; i < taps.size() - 1; i++, ptr++, j++)
		{
			buffer[ptr] = data[i];
			output[j] = filter(&buffer[i]);
		}

		for (i = 0; i < len - taps.size() + 1; i++, j++)
		{
			output[j] = filter(&data[i]);
		}

		for (ptr = 0; i < len; i++, ptr++)
		{
			buffer[ptr] = data[i];
		}

		sendOut(output.data(), len);
	}

	void Rotate::Receive(const CFLOAT32* data, int len)
	{
		if (output_up.size() < len) output_up.resize(len);
		if (output_down.size() < len) output_down.resize(len);

		for (int i = 0; i < len; i++)
		{
			output_up[i] = rot_up * data[i];
			rot_up *= mult_up;

			output_down[i] = rot_down * data[i];
			rot_down *= mult_down;
		}

		up.Send(output_up.data(), len);
		down.Send(output_down.data(), len);

		rot_up /= std::abs(rot_up);
		rot_down /= std::abs(rot_down);
	}

	// square the signal, find the mid-point between two peaks
	void SquareFreqOffsetCorrection::correctFrequency()
	{
		FLOAT32 max_val = 0.0, fz = -1;
		int delta = (int)9600.0 / 48000.0 * N;

		FFT::fft(fft_data);

		for(int i = window; i<N-window-delta; i++)
		{
			FLOAT32 h = std::abs(fft_data[(i + N / 2) % N]) + std::abs(fft_data[(i + delta + N / 2) % N]);

			if(h > max_val)
			{
				max_val = h;
				fz = (N / 2 - (i + delta / 2.0));
			}
		}

		CFLOAT32 rot_step = std::polar(1.0f, (float)(fz / 2.0 / N * 2 * PI));

		for(int i = 0; i<N; i++)
		{
			rot *= rot_step;
			output[i] *= rot;
		}

		rot /= std::abs(rot);
	}

	void SquareFreqOffsetCorrection::setN(int n,int w)
	{
		N = n;
		logN = FFT::log2(N);
		window = w;
	}

	void SquareFreqOffsetCorrection::Receive(const CFLOAT32* data, int len)
	{
		if(fft_data.size() < N) fft_data.resize(N);
		if(output.size() < N) output.resize(N);

		for(int i = 0; i< len; i++)
		{
			fft_data[FFT::rev(count, logN)] = data[i] * data[i];
			output[count] = data[i];

			if(++count == N)
			{
				correctFrequency();
				sendOut(output.data(), N);
				count = 0;
			}
		}
	}
}
