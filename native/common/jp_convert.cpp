#include "jpype.h"

namespace
{

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
} // namespace

jconverter getConverter(const char* from, int itemsize, const char* to)
{
	// If not specified then the type is bytes
	if (from == NULL)
		from = "B";
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
			return 0;
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
			return 0;
		case 'h':
			switch (to[0])
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
			return 0;
		case 'H':
			switch (to[0])
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
			return 0;
		case 'i':
		case 'l':
			switch (to[0])
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
			return 0;
		case 'I':
		case 'L':
			switch (to[0])
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
			return 0;
		case 'q':
			switch (to[0])
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
			return 0;
		case 'Q':
			switch (to[0])
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
			return 0;
		case 'f':
			switch (to[0])
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
			return 0;
		case 'd':
			switch (to[0])
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
			return 0;
		case 'n':
		case 'N':
		case 'P':
		default: return 0;
	}
}
