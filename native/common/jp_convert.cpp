/*****************************************************************************
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

		http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   See NOTICE file for details.
 *****************************************************************************/
#include "jpype.h"
#include "pyjp.h"
#include "jp_primitive_accessor.h"
#include <math.h>
#include <bitset>
#include <cctype>

namespace
{

template <jvalue func(void *c) >
class Half
{
public:
	static jvalue convert(void* c)
    {
        uint16_t i = *(uint16_t*) c;
		uint32_t sign = (i&0x8000)>>15;
		uint32_t exp  = (i&0x7C00)>>10;
		uint32_t frac = (i&0x03ff);
		uint32_t k = sign<<31;
		uint32_t count = (i&0x03ff);

		if (exp == 0)
		{
			// subnormal numbers
			if (frac != 0)
			{
				count = count | (count >> 1);
				count = count | (count >> 2);
				count = count | (count >> 4);
				count = count | (count >> 8);
				int zeros = std::bitset<32>(~count).count();
				exp = 127-zeros+7;
				exp <<= 23;
				frac <<= zeros-8;
				frac &= 0x7fffff;
				k |= exp | frac;
			}
		}
		else if (exp < 31)
		{
			// normal numbers
			exp = exp-15+127;
			exp <<= 23;
			frac <<= 13;
			k |= exp | frac;
		}
		else
		{
			// to infinity and beyond!
			if (frac == 0)
				k |= 0x7f800000;
			else 
				k |= 0x7f800001 | ((frac&0x200)<<12);
		}
		return func(&k);
	}
};

template <class T>
class Convert
{
public:

	static jvalue toZ(void* c)
	{
		jvalue v;
		v.z = (*(T*) c) != 0;
		return v;
	}

	static jvalue toB(void* c)
	{
		jvalue v;
		v.b = (jbyte) (*(T*) c);
		return v;
	}

	static jvalue toC(void* c)
	{
		jvalue v;
		v.c = (jchar) (*(T*) c);
		return v;
	}

	static jvalue toS(void* c)
	{
		jvalue v;
		v.s = (jshort) (*(T*) c);
		return v;
	}

	static jvalue toI(void* c)
	{
		jvalue v;
		v.i = (jint) (*(T*) c);
		return v;
	}

	static jvalue toJ(void* c)
	{
		jvalue v;
		v.j = (jlong) (*(T*) c);
		return v;
	}

	static jvalue toF(void* c)
	{
		jvalue v;
		v.f = (jfloat) (*(T*) c);
		return v;
	}

	static jvalue toD(void* c)
	{
		jvalue v;
		v.d = (jdouble) (*(T*) c);
		return v;
	}

} ;

template <jvalue func(void *c) >
class Reverse
{
public:

	static jvalue call2(void* c)
	{
		char r[2];
		char* c2 = (char*)c;
		r[0]=c2[1];
		r[1]=c2[0];
		return func(r);
	}

	static jvalue call4(void* c)
	{
		char r[4];
		char* c2 = (char*)c;
		r[0]=c2[3];
		r[1]=c2[2];
		r[2]=c2[1];
		r[3]=c2[0];
		return func(r);
	}

	static jvalue call8(void* c)
	{
		char r[8];
		char* c2 = (char*)c;
		r[0]=c2[7];
		r[1]=c2[6];
		r[2]=c2[5];
		r[3]=c2[4];
		r[4]=c2[3];
		r[5]=c2[2];
		r[6]=c2[1];
		r[7]=c2[0];
		return func(r);
	}
} ;


} // namespace

jconverter getConverter(const char* from, int itemsize, const char* to)
{
	// If not specified then the type is bytes
	if (from == nullptr)
		from = "B";

	// Skip specifiers
	bool reverse = false;
	unsigned int x = 1;
	bool little = *((char*)&x)==1;
	switch (from[0])
	{
		case '!':
		case '>':
			if (little)
				reverse = true;
			from++;
			break;
		case '<':
			if (!little)
				reverse = true;
			from++;
			break;
		case '@':
		case '=':
			from++;
		default:
			break;
	}

	// Standard size for 'l' is 4 in docs, but numpy uses format 'l' for long long
	if (itemsize == 8 && from[0] == 'l')
		from = "q";
	if (itemsize == 8 && from[0] == 'L')
		from = "Q";

	switch (from[0])
	{
		case '?':
		case 'c':
		case 'b':
			switch (to[0])
			{
				case 'z': return &Convert<int8_t>::toZ;
				case 'b': return &Convert<int8_t>::toB;
				case 'c': return &Convert<int8_t>::toC;
				case 's': return &Convert<int8_t>::toS;
				case 'i': return &Convert<int8_t>::toI;
				case 'j': return &Convert<int8_t>::toJ;
				case 'f': return &Convert<int8_t>::toF;
				case 'd': return &Convert<int8_t>::toD;
			}
			break;
		case 'B':
			switch (to[0])
			{
				case 'z': return &Convert<uint8_t>::toZ;
				case 'b': return &Convert<uint8_t>::toB;
				case 'c': return &Convert<uint8_t>::toC;
				case 's': return &Convert<uint8_t>::toS;
				case 'i': return &Convert<uint8_t>::toI;
				case 'j': return &Convert<uint8_t>::toJ;
				case 'f': return &Convert<uint8_t>::toF;
				case 'd': return &Convert<uint8_t>::toD;
			}
			break;
		case 'h':
			if (reverse) switch (to[0])
			{
				case 'z': return &Reverse<Convert<int16_t>::toZ>::call2;
				case 'b': return &Reverse<Convert<int16_t>::toB>::call2;
				case 'c': return &Reverse<Convert<int16_t>::toC>::call2;
				case 's': return &Reverse<Convert<int16_t>::toS>::call2;
				case 'i': return &Reverse<Convert<int16_t>::toI>::call2;
				case 'j': return &Reverse<Convert<int16_t>::toJ>::call2;
				case 'f': return &Reverse<Convert<int16_t>::toF>::call2;
				case 'd': return &Reverse<Convert<int16_t>::toD>::call2;
			}
			else switch (to[0])
			{
				case 'z': return &Convert<int16_t>::toZ;
				case 'b': return &Convert<int16_t>::toB;
				case 'c': return &Convert<int16_t>::toC;
				case 's': return &Convert<int16_t>::toS;
				case 'i': return &Convert<int16_t>::toI;
				case 'j': return &Convert<int16_t>::toJ;
				case 'f': return &Convert<int16_t>::toF;
				case 'd': return &Convert<int16_t>::toD;
			}
			break;
		case 'H':
			if (reverse) switch (to[0])
			{
				case 'z': return &Reverse<Convert<uint16_t>::toZ>::call2;
				case 'b': return &Reverse<Convert<uint16_t>::toB>::call2;
				case 'c': return &Reverse<Convert<uint16_t>::toC>::call2;
				case 's': return &Reverse<Convert<uint16_t>::toS>::call2;
				case 'i': return &Reverse<Convert<uint16_t>::toI>::call2;
				case 'j': return &Reverse<Convert<uint16_t>::toJ>::call2;
				case 'f': return &Reverse<Convert<uint16_t>::toF>::call2;
				case 'd': return &Reverse<Convert<uint16_t>::toD>::call2;
			}
			else switch (to[0])
			{
				case 'z': return &Convert<uint16_t>::toZ;
				case 'b': return &Convert<uint16_t>::toB;
				case 'c': return &Convert<uint16_t>::toC;
				case 's': return &Convert<uint16_t>::toS;
				case 'i': return &Convert<uint16_t>::toI;
				case 'j': return &Convert<uint16_t>::toJ;
				case 'f': return &Convert<uint16_t>::toF;
				case 'd': return &Convert<uint16_t>::toD;
			}
			break;
		case 'i':
		case 'l':
			if (reverse) switch (to[0])
			{
				case 'z': return &Reverse<Convert<int32_t>::toZ>::call4;
				case 'b': return &Reverse<Convert<int32_t>::toB>::call4;
				case 'c': return &Reverse<Convert<int32_t>::toC>::call4;
				case 's': return &Reverse<Convert<int32_t>::toS>::call4;
				case 'i': return &Reverse<Convert<int32_t>::toI>::call4;
				case 'j': return &Reverse<Convert<int32_t>::toJ>::call4;
				case 'f': return &Reverse<Convert<int32_t>::toF>::call4;
				case 'd': return &Reverse<Convert<int32_t>::toD>::call4;
			}
			else switch (to[0])
			{
				case 'z': return &Convert<int32_t>::toZ;
				case 'b': return &Convert<int32_t>::toB;
				case 'c': return &Convert<int32_t>::toC;
				case 's': return &Convert<int32_t>::toS;
				case 'i': return &Convert<int32_t>::toI;
				case 'j': return &Convert<int32_t>::toJ;
				case 'f': return &Convert<int32_t>::toF;
				case 'd': return &Convert<int32_t>::toD;
			}
			break;
		case 'I':
		case 'L':
			if (reverse) switch (to[0])
			{
				case 'z': return &Reverse<Convert<uint32_t>::toZ>::call4;
				case 'b': return &Reverse<Convert<uint32_t>::toB>::call4;
				case 'c': return &Reverse<Convert<uint32_t>::toC>::call4;
				case 's': return &Reverse<Convert<uint32_t>::toS>::call4;
				case 'i': return &Reverse<Convert<uint32_t>::toI>::call4;
				case 'j': return &Reverse<Convert<uint32_t>::toJ>::call4;
				case 'f': return &Reverse<Convert<uint32_t>::toF>::call4;
				case 'd': return &Reverse<Convert<uint32_t>::toD>::call4;
			}
			else switch (to[0])
			{
				case 'z': return &Convert<uint32_t>::toZ;
				case 'b': return &Convert<uint32_t>::toB;
				case 'c': return &Convert<uint32_t>::toC;
				case 's': return &Convert<uint32_t>::toS;
				case 'i': return &Convert<uint32_t>::toI;
				case 'j': return &Convert<uint32_t>::toJ;
				case 'f': return &Convert<uint32_t>::toF;
				case 'd': return &Convert<uint32_t>::toD;
			}
			break;
		case 'q':
			if (reverse) switch (to[0])
			{
				case 'z': return &Reverse<Convert<int64_t>::toZ>::call8;
				case 'b': return &Reverse<Convert<int64_t>::toB>::call8;
				case 'c': return &Reverse<Convert<int64_t>::toC>::call8;
				case 's': return &Reverse<Convert<int64_t>::toS>::call8;
				case 'i': return &Reverse<Convert<int64_t>::toI>::call8;
				case 'j': return &Reverse<Convert<int64_t>::toJ>::call8;
				case 'f': return &Reverse<Convert<int64_t>::toF>::call8;
				case 'd': return &Reverse<Convert<int64_t>::toD>::call8;
			}
			else switch (to[0])
			{
				case 'z': return &Convert<int64_t>::toZ;
				case 'b': return &Convert<int64_t>::toB;
				case 'c': return &Convert<int64_t>::toC;
				case 's': return &Convert<int64_t>::toS;
				case 'i': return &Convert<int64_t>::toI;
				case 'j': return &Convert<int64_t>::toJ;
				case 'f': return &Convert<int64_t>::toF;
				case 'd': return &Convert<int64_t>::toD;
			}
			break;
		case 'Q':
			if (reverse) switch (to[0])
			{
				case 'z': return &Reverse<Convert<uint64_t>::toZ>::call8;
				case 'b': return &Reverse<Convert<uint64_t>::toB>::call8;
				case 'c': return &Reverse<Convert<uint64_t>::toC>::call8;
				case 's': return &Reverse<Convert<uint64_t>::toS>::call8;
				case 'i': return &Reverse<Convert<uint64_t>::toI>::call8;
				case 'j': return &Reverse<Convert<uint64_t>::toJ>::call8;
				case 'f': return &Reverse<Convert<uint64_t>::toF>::call8;
				case 'd': return &Reverse<Convert<uint64_t>::toD>::call8;
			}
			else switch (to[0])
			{
				case 'z': return &Convert<uint64_t>::toZ;
				case 'b': return &Convert<uint64_t>::toB;
				case 'c': return &Convert<uint64_t>::toC;
				case 's': return &Convert<uint64_t>::toS;
				case 'i': return &Convert<uint64_t>::toI;
				case 'j': return &Convert<uint64_t>::toJ;
				case 'f': return &Convert<uint64_t>::toF;
				case 'd': return &Convert<uint64_t>::toD;
			}
			break;
		case 'f':
			if (reverse) switch (to[0])
			{
				case 'z': return &Reverse<Convert<float>::toZ>::call4;
				case 'b': return &Reverse<Convert<float>::toB>::call4;
				case 'c': return &Reverse<Convert<float>::toC>::call4;
				case 's': return &Reverse<Convert<float>::toS>::call4;
				case 'i': return &Reverse<Convert<float>::toI>::call4;
				case 'j': return &Reverse<Convert<float>::toJ>::call4;
				case 'f': return &Reverse<Convert<float>::toF>::call4;
				case 'd': return &Reverse<Convert<float>::toD>::call4;
			}
			else switch (to[0])
			{
				case 'z': return &Convert<float>::toZ;
				case 'b': return &Convert<float>::toB;
				case 'c': return &Convert<float>::toC;
				case 's': return &Convert<float>::toS;
				case 'i': return &Convert<float>::toI;
				case 'j': return &Convert<float>::toJ;
				case 'f': return &Convert<float>::toF;
				case 'd': return &Convert<float>::toD;
			}
			break;
		case 'd':
			if (reverse) switch (to[0])
			{
				case 'z': return &Reverse<Convert<double>::toZ>::call8;
				case 'b': return &Reverse<Convert<double>::toB>::call8;
				case 'c': return &Reverse<Convert<double>::toC>::call8;
				case 's': return &Reverse<Convert<double>::toS>::call8;
				case 'i': return &Reverse<Convert<double>::toI>::call8;
				case 'j': return &Reverse<Convert<double>::toJ>::call8;
				case 'f': return &Reverse<Convert<double>::toF>::call8;
				case 'd': return &Reverse<Convert<double>::toD>::call8;
			}
			else switch (to[0])
			{
				case 'z': return &Convert<double>::toZ;
				case 'b': return &Convert<double>::toB;
				case 'c': return &Convert<double>::toC;
				case 's': return &Convert<double>::toS;
				case 'i': return &Convert<double>::toI;
				case 'j': return &Convert<double>::toJ;
				case 'f': return &Convert<double>::toF;
				case 'd': return &Convert<double>::toD;
			}
			break;
		case 'e':
			// call2, not call4: a float16 element is 2 bytes on the wire
			// even though Half::convert widens it to a 4-byte float
			// internally -- swapping 4 bytes here would read past the
			// element (into the next one, or out of bounds at the end of
			// the buffer) and reverse the wrong byte pair.
			if (reverse) switch (to[0])
			{
				case 'z': return &Reverse<Half<Convert<float>::toZ>::convert>::call2;
				case 'b': return &Reverse<Half<Convert<float>::toB>::convert>::call2;
				case 'c': return &Reverse<Half<Convert<float>::toC>::convert>::call2;
				case 's': return &Reverse<Half<Convert<float>::toS>::convert>::call2;
				case 'i': return &Reverse<Half<Convert<float>::toI>::convert>::call2;
				case 'j': return &Reverse<Half<Convert<float>::toJ>::convert>::call2;
				case 'f': return &Reverse<Half<Convert<float>::toF>::convert>::call2;
				case 'd': return &Reverse<Half<Convert<float>::toD>::convert>::call2;
			}
			else switch (to[0])
			{
				case 'z': return &Half<Convert<float>::toZ>::convert;
				case 'b': return &Half<Convert<float>::toB>::convert;
				case 'c': return &Half<Convert<float>::toC>::convert;
				case 's': return &Half<Convert<float>::toS>::convert;
				case 'i': return &Half<Convert<float>::toI>::convert;
				case 'j': return &Half<Convert<float>::toJ>::convert;
				case 'f': return &Half<Convert<float>::toF>::convert;
				case 'd': return &Half<Convert<float>::toD>::convert;
			}
			break;

		case 'n':
			// GCOVR_EXCL_START -- 'n'/'N' (Py_ssize_t/size_t) are
			// native-only in the struct module's own format-string rules;
			// neither numpy (whose intp buffer format is 'l'/'q', not 'n')
			// nor ctypes nor memoryview.cast() can produce a byte-order-
			// prefixed 'n' buffer, so `reverse` can't legitimately be true
			// here. Kept in case a non-standard buffer exporter ever lies
			// about its own format string.
			if (reverse) switch (to[0])
			{
				case 'z': return &Reverse<Convert<Py_ssize_t>::toZ>::call8;
				case 'b': return &Reverse<Convert<Py_ssize_t>::toB>::call8;
				case 'c': return &Reverse<Convert<Py_ssize_t>::toC>::call8;
				case 's': return &Reverse<Convert<Py_ssize_t>::toS>::call8;
				case 'i': return &Reverse<Convert<Py_ssize_t>::toI>::call8;
				case 'j': return &Reverse<Convert<Py_ssize_t>::toJ>::call8;
				case 'f': return &Reverse<Convert<Py_ssize_t>::toF>::call8;
				case 'd': return &Reverse<Convert<Py_ssize_t>::toD>::call8;
			}
			// GCOVR_EXCL_STOP
			else switch (to[0])
			{
				case 'z': return &Convert<Py_ssize_t>::toZ;
				case 'b': return &Convert<Py_ssize_t>::toB;
				case 'c': return &Convert<Py_ssize_t>::toC;
				case 's': return &Convert<Py_ssize_t>::toS;
				case 'i': return &Convert<Py_ssize_t>::toI;
				case 'j': return &Convert<Py_ssize_t>::toJ;
				case 'f': return &Convert<Py_ssize_t>::toF;
				case 'd': return &Convert<Py_ssize_t>::toD;
			}
			break;
		case 'N':
			// GCOVR_EXCL_START -- same reasoning as case 'n' above: 'N' is
			// native-only per the struct module's own rules, so no
			// standard buffer exporter can produce a byte-order-prefixed
			// 'N' buffer for `reverse` to legitimately be true here.
			if (reverse) switch (to[0])
			{
				case 'z': return &Reverse<Convert<size_t>::toZ>::call8;
				case 'b': return &Reverse<Convert<size_t>::toB>::call8;
				case 'c': return &Reverse<Convert<size_t>::toC>::call8;
				case 's': return &Reverse<Convert<size_t>::toS>::call8;
				case 'i': return &Reverse<Convert<size_t>::toI>::call8;
				case 'j': return &Reverse<Convert<size_t>::toJ>::call8;
				case 'f': return &Reverse<Convert<size_t>::toF>::call8;
				case 'd': return &Reverse<Convert<size_t>::toD>::call8;
			}
			// GCOVR_EXCL_STOP
			else switch (to[0])
			{
				case 'z': return &Convert<size_t>::toZ;
				case 'b': return &Convert<size_t>::toB;
				case 'c': return &Convert<size_t>::toC;
				case 's': return &Convert<size_t>::toS;
				case 'i': return &Convert<size_t>::toI;
				case 'j': return &Convert<size_t>::toJ;
				case 'f': return &Convert<size_t>::toF;
				case 'd': return &Convert<size_t>::toD;
			}
			break;
		default: break;
	}
	PyErr_Format(PyExc_ValueError, "Unable to handle buffer type '%s'", from);
	JP_RAISE_PYTHON();
}

JPRawTransferMode classifyRawTransfer(jconverter converter, JPPrimitiveType* pcls,
		const char* format, int itemsize, const char* code)
{
	if (format == nullptr)
		format = "B";

	// Strip an explicit byte-order prefix the same way getConverter does,
	// so re-resolving with the bare remainder always means "as if native
	// order" regardless of what the original prefix said.
	const char* stripped = format;
	switch (stripped[0])
	{
		case '!':
		case '>':
		case '<':
		case '@':
		case '=':
			stripped++;
			break;
		default:
			break;
	}

	// Match getConverter's own itemsize==8 'l'/'L' -> 'q'/'Q' aliasing so
	// the "as-if-native" re-resolution below picks the same base type
	// getConverter itself would have picked for this format/itemsize.
	char adj[2] = {stripped[0], 0};
	if (itemsize == 8 && adj[0] == 'l')
		adj[0] = 'q';
	if (itemsize == 8 && adj[0] == 'L')
		adj[0] = 'Q';

	if (adj[0] == 'e')
	{
		// Half precision is never a raw reinterpret (no native 16-bit
		// float type), but is still cheap, fixed-cost, bulk-friendly work
		// -- distinguish only whether the source order matches native.
		if (itemsize != 2)
			return RAW_NONE;
		jconverter nativeHalf = getConverter(adj, itemsize, code);
		return (converter == nativeHalf) ? RAW_HALF_NATIVE : RAW_HALF_SWAPPED;
	}

	jconverter identity = getConverter(pcls->getBufferFormat(), (int) pcls->getItemSize(), code);
	if (converter == identity)
		return RAW_NATIVE;

	jconverter asNative = getConverter(adj, itemsize, code);
	if (asNative == identity)
		return RAW_SWAPPED;

	return RAW_NONE;
}

bool classifyBufferSource(const char* format, int itemsize, JPBufferSource& out)
{
	if (format == nullptr)
		format = "B";

	// Same byte-order-prefix stripping as getConverter above.
	bool reverse = false;
	unsigned int x = 1;
	bool little = *((char*) &x) == 1;
	switch (format[0])
	{
		case '!':
		case '>':
			if (little)
				reverse = true;
			format++;
			break;
		case '<':
			if (!little)
				reverse = true;
			format++;
			break;
		case '@':
		case '=':
			format++;
		default:
			break;
	}

	// Same itemsize==8 'l'/'L' -> 'q'/'Q' aliasing as getConverter above.
	char base = format[0];
	if (itemsize == 8 && base == 'l')
		base = 'q';
	if (itemsize == 8 && base == 'L')
		base = 'Q';

	out.swapped = reverse;
	switch (base)
	{
		case '?':
		case 'c':
		case 'b':
			out.kind = 'i';
			out.size = 1;
			return true;
		case 'B':
			out.kind = 'u';
			out.size = 1;
			return true;
		case 'h':
			out.kind = 'i';
			out.size = 2;
			return true;
		case 'H':
			out.kind = 'u';
			out.size = 2;
			return true;
		case 'i':
		case 'l':
			out.kind = 'i';
			out.size = 4;
			return true;
		case 'I':
		case 'L':
			out.kind = 'u';
			out.size = 4;
			return true;
		case 'q':
		case 'n':
			out.kind = 'i';
			out.size = 8;
			return true;
		case 'Q':
		case 'N':
			out.kind = 'u';
			out.size = 8;
			return true;
		case 'f':
			out.kind = 'f';
			out.size = 4;
			return true;
		case 'd':
			out.kind = 'f';
			out.size = 8;
			return true;
		case 'e':
			out.kind = 'f';
			out.size = 2;
			return true;
		default:
			return false;
	}
}

bool tryFastBufferPush(JPJavaFrame &frame, JPPrimitiveType *pcls, jarray dest,
		jsize start, jsize step, jsize length, PyObject *sequence)
{
	if (length <= 0 || !PyObject_CheckBuffer(sequence))
		return false;

	JPPyBuffer buffer(sequence, PyBUF_STRIDES | PyBUF_FORMAT);
	if (!buffer.valid())
	{
		PyErr_Clear();
		return false;
	}
	Py_buffer &view = buffer.getView();
	if (view.ndim != 1 || view.shape[0] != length)
		return false;

	const char *format = view.format != nullptr ? view.format : "B";
	Py_ssize_t vstep = view.strides != nullptr ? view.strides[0] : view.itemsize;
	JPBufferSource src;
	if (vstep <= 0 || !classifyBufferSource(format, (int) view.itemsize, src))
		return false;

	jobject directBuf = frame.NewDirectByteBuffer(view.buf,
			(jlong) ((length - 1) * vstep + view.itemsize));
	frame.fillFlatIntoArray(pcls->getTypeCode(), src.kind, src.size, (jboolean) src.swapped,
			directBuf, length, (jint) vstep, dest, start, step);
	return true;
}

jintArray buildDimsArray(JPJavaFrame &frame, Py_buffer &view)
{
	auto jdims = (jintArray) frame.getContext()->_int->newArrayOf(frame, view.ndim);
	JPPrimitiveArrayAccessor<jintArray, jint*> accessor(frame, jdims,
			&JPJavaFrame::GetIntArrayElements, &JPJavaFrame::ReleaseIntArrayElements);
	jint *a = accessor.get();
	for (int i = 0; i < view.ndim; ++i)
		a[i] = (jint) view.shape[i];
	accessor.commit();
	return jdims;
}

bool tryFastMultiArrayBuffer(JPJavaFrame &frame, JPPrimitiveType *pcls,
		JPPyBuffer &buffer, jintArray jdims, jarray &out)
{
	Py_buffer &view = buffer.getView();
	if (!PyBuffer_IsContiguous(&view, 'C'))
		return false;

	char code[2] = {(char) tolower(pcls->getTypeCode()), 0};
	const char *format = view.format != nullptr ? view.format : "B";
	// getConverter() never actually returns nullptr for an unrecognized
	// format -- it raises ValueError via JP_RAISE_PYTHON() instead (see its
	// final `default: break;` case above). This check is dead but kept as a
	// defensive backstop in case that contract ever changes.
	jconverter converter = getConverter(format, (int) view.itemsize, code);
	if (converter == nullptr)  // GCOVR_EXCL_LINE
		return false;  // GCOVR_EXCL_LINE

	JPRawTransferMode mode = classifyRawTransfer(converter, pcls, format, (int) view.itemsize, code);
	if (mode == RAW_NONE)
		return false;

	Py_ssize_t total = 1;
	for (int i = 0; i < view.ndim; ++i)
		total *= view.shape[i];
	jobject directBuf = frame.NewDirectByteBuffer(view.buf, total * view.itemsize);
	// A local reference within frame's own scope -- caller keeps/converts
	// it as appropriate to their own frame lifetime, same as before.
	out = (jarray) frame.fillMultiArrayFromBuffer(pcls->getTypeCode(), (jint) mode, directBuf, jdims);
	return true;
}
