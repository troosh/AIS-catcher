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

#include "Demod.h"
#include "DSP.h"

namespace DSP
{

	void FMDemodulation::Receive(const CFLOAT32* data, int len)
	{
		if (output.size() < len) output.resize(len);

		for (int i = 0; i < len; i++)
		{
			auto p = data[i] * std::conj(prev);
			output[i] = (atan2f(p.imag(), p.real()) + DC_shift) / PI;
			prev = data[i];
		}

		sendOut(output.data(), len);
	}

	void CoherentDemodulation::setPhases()
	{
		int np2 = nPhases / 2;
		phase.resize(np2);

		for (int i = 0; i < np2; i++)
		{
			float alpha = PI / 2.0 / np2 * i + PI / 2.0 / (2.0 * np2);
			phase[i] = std::polar(1.0f, alpha);
		}
	}

	void CoherentDemodulation::Receive(const CFLOAT32* data, int len)
	{
		if (phase.size() == 0) setPhases();

		for (int i = 0; i < len; i++)
		{
			FLOAT32 re, im;

			//  multiply samples with (1j) ** i, to get all points on the same line 
			switch (rot)
			{
			case 0: re = data[i].real(); im = data[i].imag(); break;
			case 1: im = data[i].real(); re = -data[i].imag(); break;
			case 2: re = -data[i].real(); im = -data[i].imag(); break;
			case 3: im = -data[i].real(); re = data[i].imag(); break;
			}

			rot = (rot + 1) & 3;

			// Determining the phase is approached as a linear classification problem. 
			for (int j = 0; j < nPhases / 2; j++)
			{
				FLOAT32 a = re * phase[j].real();
				FLOAT32 b = im * phase[j].imag();
				FLOAT32 t;

				bits[j] <<= 1;
				bits[nPhases - 1 - j] <<= 1;

				t = a + b;
				bits[j] |= ((t) > 0) & 1;
				memory[j][last] = std::abs(t);

				t = a - b;
				bits[nPhases - 1 - j] |= ((t) > 0) & 1;
				memory[nPhases - 1 - j][last] = std::abs(t);
			}
			last = (last + 1) % nHistory;


			update = (update + 1) % nUpdate;
			if (update == 0)
			{
				FLOAT32 max_val = 0;
				int prev_max = max_idx;

				// local minmax search
				for (int p = nPhases - nSearch; p <= nPhases + nSearch; p++)
				{
					int j = (p + prev_max) % nPhases;
					FLOAT32 min_abs = memory[j][0];

					for (int l = 1; l < nHistory; l++)
						min_abs = memory[j][l] < min_abs ? memory[j][l] : min_abs;

					if (min_abs > max_val)
					{
						max_val = min_abs;
						max_idx = j;
					}
				}
			}

			// determine the bit
			bool b2 = (bits[max_idx] & 2) >> 1;
			bool b1 = bits[max_idx] & 1;

			FLOAT32 b = b1 ^ b2 ? 1.0f : -1.0f;

			sendOut(&b, 1);
		}
	}

	void ChallengerDemodulation::setPhases()
	{
		int np2 = nPhases / 2;
		phase.resize(np2);

		for (int i = 0; i < np2; i++)
		{
			float alpha = PI / 2.0 / np2 * i + PI / 2.0 / (2.0 * np2);
			phase[i] = std::polar(1.0f, alpha);
		}
	}

	void ChallengerDemodulation::Message(const DecoderMessages& in)
	{
	}

	void ChallengerDemodulation::Receive(const CFLOAT32* data, int len)
	{
		if (phase.size() == 0) setPhases();

		for (int i = 0; i < len; i++)
		{
			FLOAT32 re, im;

			//  multiply samples with (1j) ** i, to get all points on the same line 
			switch (rot)
			{
			case 0: re = data[i].real(); im = data[i].imag(); break;
			case 1: im = data[i].real(); re = -data[i].imag(); break;
			case 2: re = -data[i].real(); im = -data[i].imag(); break;
			case 3: im = -data[i].real(); re = data[i].imag(); break;
			}

			rot = (rot + 1) & 3;

			// Determining the phase is approached as a linear classification problem. 
			for (int j = 0; j < nPhases / 2; j++)
			{
				FLOAT32 a = re * phase[j].real();
				FLOAT32 b = -im * phase[j].imag();
				FLOAT32 t;

				bits[j] <<= 1;
				bits[nPhases - 1 - j] <<= 1;

				t = a + b;
				bits[j] |= ((t) > 0) & 1;
				memory[j][last] = std::abs(t);

				t = a - b;
				bits[nPhases - 1 - j] |= ((t) > 0) & 1;
				memory[nPhases - 1 - j][last] = std::abs(t);
			}
			last = (last + 1) % nHistory;

			update = (update + 1) % nUpdate;
			if (update == 0)
			{
				FLOAT32 max_val = 0;
				int prev_max = max_idx;

				// local minmax search
				for (int p = nPhases - nSearch; p <= nPhases + nSearch; p++)
				{
					int j = (p + prev_max) % nPhases;
					FLOAT32 min_abs = memory[j][0];

					for (int l = 1; l < nHistory; l++)
						min_abs = memory[j][l] < min_abs ? memory[j][l] : min_abs;

					if (min_abs > max_val)
					{
						max_val = min_abs;
						max_idx = j;
					}
				}
			}

			// determine the bit
			bool b2 = (bits[max_idx] & 2) >> 1;
			bool b1 = bits[max_idx] & 1;

			FLOAT32 b = b1 ^ b2 ? 1.0f : -1.0f;

			sendOut(&b, 1);
		}
	}
}
