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


jconverter getConverter(const char *from, int itemsize, const char *to) {
    // "From" codes are documented at https://docs.python.org/3/library/struct.html#format-characters.
    // "To" codes correspond to the codes under "Type Signatures" in https://docs.oracle.com/javase/6/docs/technotes/guides/jni/spec/types.html#wp428
    // Note that from "l" and "L" codes deviate from the documentation to follow the numpy convention.
    std::string from_actual;
    if (from) {
        from_actual = from;
    } else {
        // If not specified then the type is bytes
        from_actual = "B";
    }

    char byte_order_code; // One of "@=<>!".
    char data_type_code;  // One of "cbB?hHiIlLqQfdspP".

    std::string all_byte_order_codes = "@=<>!";
    if (all_byte_order_codes.find(from_actual[0]) != std::string::npos) {
        byte_order_code = from_actual[0];
        data_type_code = from_actual[1];
    } else {
        byte_order_code = '@';
        data_type_code = from_actual[0];
    }

    std::string unsupported_byte_order_codes = "<>!";
    if (unsupported_byte_order_codes.find(byte_order_code) != std::string::npos) {
        // We don't try to implement endian swapping, or figure out the host's endianness.
        throw std::invalid_argument("Unable convert to specific requested endianness");
    }

    if (byte_order_code == '@') {
        // Native size
        switch (data_type_code) {
        case '?':
        case 'c':
            switch (to[0]) {
            case 'z': return &Convert<char>::toZ;
            case 'b': return &Convert<char>::toB;
            case 'c': return &Convert<char>::toC;
            case 's': return &Convert<char>::toS;
            case 'i': return &Convert<char>::toI;
            case 'j': return &Convert<char>::toJ;
            case 'f': return &Convert<char>::toF;
            case 'd': return &Convert<char>::toD;
            default: break;
            }
        case 'b':
            switch (to[0]) {
            case 'z': return &Convert<signed char>::toZ;
            case 'b': return &Convert<signed char>::toB;
            case 'c': return &Convert<signed char>::toC;
            case 's': return &Convert<signed char>::toS;
            case 'i': return &Convert<signed char>::toI;
            case 'j': return &Convert<signed char>::toJ;
            case 'f': return &Convert<signed char>::toF;
            case 'd': return &Convert<signed char>::toD;
            default: break;
            }
        case 'B':
            switch (to[0]) {
            case 'z': return &Convert<unsigned char>::toZ;
            case 'b': return &Convert<unsigned char>::toB;
            case 'c': return &Convert<unsigned char>::toC;
            case 's': return &Convert<unsigned char>::toS;
            case 'i': return &Convert<unsigned char>::toI;
            case 'j': return &Convert<unsigned char>::toJ;
            case 'f': return &Convert<unsigned char>::toF;
            case 'd': return &Convert<unsigned char>::toD;
            default: break;
            }
        case 'h':
            switch (to[0]) {
            case 'z': return &Convert<short>::toZ;
            case 'b': return &Convert<short>::toB;
            case 'c': return &Convert<short>::toC;
            case 's': return &Convert<short>::toS;
            case 'i': return &Convert<short>::toI;
            case 'j': return &Convert<short>::toJ;
            case 'f': return &Convert<short>::toF;
            case 'd': return &Convert<short>::toD;
            default: break;
            }
        case 'H':
            switch (to[0]) {
            case 'z': return &Convert<unsigned short>::toZ;
            case 'b': return &Convert<unsigned short>::toB;
            case 'c': return &Convert<unsigned short>::toC;
            case 's': return &Convert<unsigned short>::toS;
            case 'i': return &Convert<unsigned short>::toI;
            case 'j': return &Convert<unsigned short>::toJ;
            case 'f': return &Convert<unsigned short>::toF;
            case 'd': return &Convert<unsigned short>::toD;
            default: break;
            }
        case 'i':
            switch (to[0]) {
            case 'z': return &Convert<int>::toZ;
            case 'b': return &Convert<int>::toB;
            case 'c': return &Convert<int>::toC;
            case 's': return &Convert<int>::toS;
            case 'i': return &Convert<int>::toI;
            case 'j': return &Convert<int>::toJ;
            case 'f': return &Convert<int>::toF;
            case 'd': return &Convert<int>::toD;
            default: break;
            }
        case 'I':
            switch (to[0]) {
            case 'z': return &Convert<unsigned int>::toZ;
            case 'b': return &Convert<unsigned int>::toB;
            case 'c': return &Convert<unsigned int>::toC;
            case 's': return &Convert<unsigned int>::toS;
            case 'i': return &Convert<unsigned int>::toI;
            case 'j': return &Convert<unsigned int>::toJ;
            case 'f': return &Convert<unsigned int>::toF;
            case 'd': return &Convert<unsigned int>::toD;
            default: break;
            }
        case 'l':
            switch (to[0]) {
            case 'z': return &Convert<long>::toZ;
            case 'b': return &Convert<long>::toB;
            case 'c': return &Convert<long>::toC;
            case 's': return &Convert<long>::toS;
            case 'i': return &Convert<long>::toI;
            case 'j': return &Convert<long>::toJ;
            case 'f': return &Convert<long>::toF;
            case 'd': return &Convert<long>::toD;
            default: break;
            }
        case 'L':
            switch (to[0]) {
            case 'z': return &Convert<unsigned long>::toZ;
            case 'b': return &Convert<unsigned long>::toB;
            case 'c': return &Convert<unsigned long>::toC;
            case 's': return &Convert<unsigned long>::toS;
            case 'i': return &Convert<unsigned long>::toI;
            case 'j': return &Convert<unsigned long>::toJ;
            case 'f': return &Convert<unsigned long>::toF;
            case 'd': return &Convert<unsigned long>::toD;
            default: break;
            }
        case 'q':
            switch (to[0]) {
            case 'z': return &Convert<long long>::toZ;
            case 'b': return &Convert<long long>::toB;
            case 'c': return &Convert<long long>::toC;
            case 's': return &Convert<long long>::toS;
            case 'i': return &Convert<long long>::toI;
            case 'j': return &Convert<long long>::toJ;
            case 'f': return &Convert<long long>::toF;
            case 'd': return &Convert<long long>::toD;
            default: break;
            }
        case 'Q':
            switch (to[0]) {
            case 'z': return &Convert<unsigned long long>::toZ;
            case 'b': return &Convert<unsigned long long>::toB;
            case 'c': return &Convert<unsigned long long>::toC;
            case 's': return &Convert<unsigned long long>::toS;
            case 'i': return &Convert<unsigned long long>::toI;
            case 'j': return &Convert<unsigned long long>::toJ;
            case 'f': return &Convert<unsigned long long>::toF;
            case 'd': return &Convert<unsigned long long>::toD;
            default: break;
            }
        case 'n':
        case 'N':
        case 'f':
            switch (to[0]) {
            case 'z': return &Convert<float>::toZ;
            case 'b': return &Convert<float>::toB;
            case 'c': return &Convert<float>::toC;
            case 's': return &Convert<float>::toS;
            case 'i': return &Convert<float>::toI;
            case 'j': return &Convert<float>::toJ;
            case 'f': return &Convert<float>::toF;
            case 'd': return &Convert<float>::toD;
            default: break;
            }
        case 'd':
            switch (to[0]) {
            case 'z': return &Convert<double>::toZ;
            case 'b': return &Convert<double>::toB;
            case 'c': return &Convert<double>::toC;
            case 's': return &Convert<double>::toS;
            case 'i': return &Convert<double>::toI;
            case 'j': return &Convert<double>::toJ;
            case 'f': return &Convert<double>::toF;
            case 'd': return &Convert<double>::toD;
            default: break;
            }
        case 'P':
        default: break;
        }
    } else {
        // "Standard size" types
        switch (data_type_code) {
        case '?':
        case 'c':
        case 'b':
            switch (to[0]) {
            case 'z': return &Convert<int8_t>::toZ;
            case 'b': return &Convert<int8_t>::toB;
            case 'c': return &Convert<int8_t>::toC;
            case 's': return &Convert<int8_t>::toS;
            case 'i': return &Convert<int8_t>::toI;
            case 'j': return &Convert<int8_t>::toJ;
            case 'f': return &Convert<int8_t>::toF;
            case 'd': return &Convert<int8_t>::toD;
            default: break;
            }
        case 'B':
            switch (to[0]) {
            case 'z': return &Convert<uint8_t>::toZ;
            case 'b': return &Convert<uint8_t>::toB;
            case 'c': return &Convert<uint8_t>::toC;
            case 's': return &Convert<uint8_t>::toS;
            case 'i': return &Convert<uint8_t>::toI;
            case 'j': return &Convert<uint8_t>::toJ;
            case 'f': return &Convert<uint8_t>::toF;
            case 'd': return &Convert<uint8_t>::toD;
            default: break;
            }
        case 'h':
            switch (to[0]) {
            case 'z': return &Convert<int16_t>::toZ;
            case 'b': return &Convert<int16_t>::toB;
            case 'c': return &Convert<int16_t>::toC;
            case 's': return &Convert<int16_t>::toS;
            case 'i': return &Convert<int16_t>::toI;
            case 'j': return &Convert<int16_t>::toJ;
            case 'f': return &Convert<int16_t>::toF;
            case 'd': return &Convert<int16_t>::toD;
            default: break;
            }
        case 'H':
            switch (to[0]) {
            case 'z': return &Convert<uint16_t>::toZ;
            case 'b': return &Convert<uint16_t>::toB;
            case 'c': return &Convert<uint16_t>::toC;
            case 's': return &Convert<uint16_t>::toS;
            case 'i': return &Convert<uint16_t>::toI;
            case 'j': return &Convert<uint16_t>::toJ;
            case 'f': return &Convert<uint16_t>::toF;
            case 'd': return &Convert<uint16_t>::toD;
            default: break;
            }
        case 'i':
            switch (to[0]) {
            case 'z': return &Convert<int32_t>::toZ;
            case 'b': return &Convert<int32_t>::toB;
            case 'c': return &Convert<int32_t>::toC;
            case 's': return &Convert<int32_t>::toS;
            case 'i': return &Convert<int32_t>::toI;
            case 'j': return &Convert<int32_t>::toJ;
            case 'f': return &Convert<int32_t>::toF;
            case 'd': return &Convert<int32_t>::toD;
            default: break;
            }
        case 'I':
            switch (to[0]) {
            case 'z': return &Convert<uint32_t>::toZ;
            case 'b': return &Convert<uint32_t>::toB;
            case 'c': return &Convert<uint32_t>::toC;
            case 's': return &Convert<uint32_t>::toS;
            case 'i': return &Convert<uint32_t>::toI;
            case 'j': return &Convert<uint32_t>::toJ;
            case 'f': return &Convert<uint32_t>::toF;
            case 'd': return &Convert<uint32_t>::toD;
            default: break;
            }
        case 'l':
            // Despite the Python standard format documentation, we follow np.dtype('=l').itemsize == 8
            switch (to[0]) {
            case 'z': return &Convert<int64_t>::toZ;
            case 'b': return &Convert<int64_t>::toB;
            case 'c': return &Convert<int64_t>::toC;
            case 's': return &Convert<int64_t>::toS;
            case 'i': return &Convert<int64_t>::toI;
            case 'j': return &Convert<int64_t>::toJ;
            case 'f': return &Convert<int64_t>::toF;
            case 'd': return &Convert<int64_t>::toD;
            default: break;
            }
        case 'L':
            // Despite the standard format documentation, we follow np.dtype('=L').itemsize == 8
            switch (to[0]) {
            case 'z': return &Convert<uint64_t>::toZ;
            case 'b': return &Convert<uint64_t>::toB;
            case 'c': return &Convert<uint64_t>::toC;
            case 's': return &Convert<uint64_t>::toS;
            case 'i': return &Convert<uint64_t>::toI;
            case 'j': return &Convert<uint64_t>::toJ;
            case 'f': return &Convert<uint64_t>::toF;
            case 'd': return &Convert<uint64_t>::toD;
            default: break;
            }
        case 'q':
            switch (to[0]) {
            case 'z': return &Convert<int64_t>::toZ;
            case 'b': return &Convert<int64_t>::toB;
            case 'c': return &Convert<int64_t>::toC;
            case 's': return &Convert<int64_t>::toS;
            case 'i': return &Convert<int64_t>::toI;
            case 'j': return &Convert<int64_t>::toJ;
            case 'f': return &Convert<int64_t>::toF;
            case 'd': return &Convert<int64_t>::toD;
            default: break;
            }
        case 'Q':
            switch (to[0]) {
            case 'z': return &Convert<uint64_t>::toZ;
            case 'b': return &Convert<uint64_t>::toB;
            case 'c': return &Convert<uint64_t>::toC;
            case 's': return &Convert<uint64_t>::toS;
            case 'i': return &Convert<uint64_t>::toI;
            case 'j': return &Convert<uint64_t>::toJ;
            case 'f': return &Convert<uint64_t>::toF;
            case 'd': return &Convert<uint64_t>::toD;
            default: break;
            }
        case 'f':
            switch (to[0]) {
            case 'z': return &Convert<float>::toZ;
            case 'b': return &Convert<float>::toB;
            case 'c': return &Convert<float>::toC;
            case 's': return &Convert<float>::toS;
            case 'i': return &Convert<float>::toI;
            case 'j': return &Convert<float>::toJ;
            case 'f': return &Convert<float>::toF;
            case 'd': return &Convert<float>::toD;
            default: break;
            }
        case 'd':
            switch (to[0]) {
            case 'z': return &Convert<double>::toZ;
            case 'b': return &Convert<double>::toB;
            case 'c': return &Convert<double>::toC;
            case 's': return &Convert<double>::toS;
            case 'i': return &Convert<double>::toI;
            case 'j': return &Convert<double>::toJ;
            case 'f': return &Convert<double>::toF;
            case 'd': return &Convert<double>::toD;
            default: break;
            }
        case 'n':
        case 'N':
        case 'P':
        default: break;
        }
    }

    std::stringstream message;
    message << "Unable to convert from " << from_actual << " to " << to;
    throw std::invalid_argument(message.str());
}
