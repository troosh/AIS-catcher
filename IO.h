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
#include <fstream>
#include <iostream>

#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netdb.h>
#endif

#include "Stream.h"

namespace IO
{
	template<typename T>
	class SampleCounter : public StreamIn<T>
	{
		uint64_t count = 0;
		uint64_t lastcount = 0;
		float rate = 0.0f;

		high_resolution_clock::time_point time_lastupdate;

	public:

		SampleCounter() : StreamIn<T>()
		{
			resetStatistic();
		}

		void Receive(const T* data, int len)
		{
			count += len;
		}

		uint64_t getCount() { return count; }

		float getRate()
		{
			auto timeNow = high_resolution_clock::now();
			float seconds = 1e-6f * duration_cast<microseconds>(timeNow - time_lastupdate).count();

			rate += 0.9f * ((count - lastcount) / seconds - rate);

			time_lastupdate = timeNow;
			lastcount = count;

			return rate;
		}
		void resetStatistic()
		{
			count = 0;
			time_lastupdate = high_resolution_clock::now();
		}
	};

	template <typename T>
	class DumpFile : public StreamIn<T>
	{
		std::ofstream file;
		std::string filename;

	public:

		~DumpFile()
		{
			if (file.is_open())
				file.close();
		}
		void openFile(std::string fn)
		{
			filename = fn;
			file.open(filename, std::ios::out | std::ios::binary);
		}

		void Receive(const T* data, int len)
		{
			file.write((char*)data, len * sizeof(T));
		}
	};

	class DumpScreen : public StreamIn<NMEA>
	{
	public:

		void Receive(const NMEA* data, int len)
		{
			for (int i = 0; i < len; i++)
				for (auto s : data[i].sentence)
					std::cout << s << " ( MSG: " << data[i].msg << ", REPEAT: " << data[i].repeat << ", MMSI: " << data[i].mmsi << ")" << std::endl;
		}
	};

	class UDP : public StreamIn<NMEA>
	{
		int sock;
		struct addrinfo* address;
#ifdef WIN32
		WSADATA wsaData;
#endif
		void startWSA();
		void closeWSA();

	public:

		void Receive(const NMEA* data, int len);
		void openConnection(std::string host, std::string portname);

		~UDP() { closeWSA(); }
	};
}
