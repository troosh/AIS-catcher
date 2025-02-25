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

#include "Common.h"

template<typename T>
class StreamIn
{
public:

	virtual void Receive(const T* data, int len) {}
	virtual void Receive(T* data, int len)
	{
		Receive( (const T *) data, len);
	}
};

template <typename S>
class Connection
{
	std::vector<StreamIn<S>*> connections;

public:

	void Send(const S* data, int len)
	{
		for (auto c : connections) c->Receive(data, len);
	}

	void Send(S* data, int len)
	{
		if(connections.size() == 0) return;

		int sz1 = connections.size()-1;

		for(int i = 0; i < sz1; i++)
			connections[i]->Receive((const S*)data, len);

		connections[sz1]->Receive(data, len);

	}

	void Connect(StreamIn<S>* s)
	{
		connections.push_back(s);
	}
};

template <typename S>
class StreamOut
{
public:

	Connection<S> out;

	void Send(const S* data, int len)
	{
		out.Send(data, len);
	}

	void Send(S* data, int len)
	{
		out.Send(data, len);
	}
};

template <typename T, typename S>
class SimpleStreamInOut : public StreamOut<S>, public StreamIn<T>
{
public:

	void sendOut(const S* data, int len)
	{
		StreamOut<S>::Send(data, len);
	}

	void sendOut(S* data, int len)
	{
		StreamOut<S>::Send(data, len);
	}
};



template <typename S>
inline StreamIn<S>& operator>>(Connection<S>& a, StreamIn<S>& b) { a.Connect(&b); return b; }
template <typename T, typename S>
inline SimpleStreamInOut<S,T>& operator>>(Connection<S>& a, SimpleStreamInOut<S,T>& b) { a.Connect(&b); return b; }

template <typename S>
inline StreamIn<S>& operator>>(StreamOut<S>& a, StreamIn<S>& b) { a.out.Connect(&b); return b; }
template <typename S, typename U>
inline SimpleStreamInOut<S, U>& operator>>(StreamOut<S>& a, SimpleStreamInOut<S, U>& b) { a.out.Connect(&b); return b; }

template <typename T, typename S>
inline StreamIn<S>& operator>>(SimpleStreamInOut<T,S>& a, StreamIn<S>& b) { a.out.Connect(&b); return b; }
template <typename T, typename S, typename U>
inline SimpleStreamInOut<S,U>& operator>>(SimpleStreamInOut<T,S>& a, SimpleStreamInOut<S,U>& b) { a.out.Connect(&b); return b; }
