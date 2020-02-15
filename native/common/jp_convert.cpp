/*
 * Copyright 2020 nelson85.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <jpype.h>

static jvalue bool2bool(char* c)
{
	jvalue v;
	v.z = (*(uint8_t*) c) == 0 ? false : true;
	return v;
}

static jvalue short2bool(char* c)
{
	jvalue v;
	v.z = (*(uint16_t*) c) == 0 ? false : true;
	return v;
}

static jvalue int2bool(char* c)
{
	jvalue v;
	v.z = (*(uint32_t*) c) == 0 ? false : true;
	return v;
}

static jvalue long2bool(char* c)
{
	jvalue v;
	v.z = (*(uint64_t*) c) == 0 ? false : true;
	return v;
}

static jvalue float2bool(char* c)
{
	jvalue v;
	v.z = (*(float*) c) == 0 ? false : true;
	return v;
}

static jvalue double2bool(char* c)
{
	jvalue v;
	v.z = (*(double*) c) == 0 ? false : true;
	return v;
}

static jvalue byte2byte(char* c)
{
	jvalue v;
	v.b = (jbyte) (*(int8_t*) c);
	return v;
}

static jvalue short2byte(char* c)
{
	jvalue v;
	v.b = (jbyte) (*(int16_t*) c);
	return v;
}

static jvalue ushort2byte(char* c)
{
	jvalue v;
	v.b = (jbyte) (*(uint16_t*) c);
	return v;
}

static jvalue int2byte(char* c)
{
	jvalue v;
	v.b = (jbyte) (*(int32_t*) c);
	return v;
}

static jvalue uint2byte(char* c)
{
	jvalue v;
	v.b = (jbyte) (*(uint32_t*) c);
	return v;
}

static jvalue long2byte(char* c)
{
	jvalue v;
	v.b = (jbyte) (*(int64_t*) c);
	return v;
}

static jvalue ulong2byte(char* c)
{
	jvalue v;
	v.b = (jbyte) (*(uint64_t*) c);
	return v;
}

static jvalue float2byte(char* c)
{
	jvalue v;
	v.b = (jbyte) (*(float*) c);
	return v;
}

static jvalue double2byte(char* c)
{
	jvalue v;
	v.b = (jbyte) (*(double*) c);
	return v;
}

static jvalue byte2short(char* c)
{
	jvalue v;
	v.s = (jshort) (*(int8_t*) c);
	return v;
}

static jvalue ubyte2short(char* c)
{
	jvalue v;
	v.s = (jshort) (*(uint8_t*) c);
	return v;
}

static jvalue short2short(char* c)
{
	jvalue v;
	v.s = (jshort) (*(int16_t*) c);
	return v;
}

static jvalue ushort2short(char* c)
{
	jvalue v;
	v.s = (jshort) (*(uint16_t*) c);
	return v;
}

static jvalue int2short(char* c)
{
	jvalue v;
	v.s = (jshort) (*(int32_t*) c);
	return v;
}

static jvalue uint2short(char* c)
{
	jvalue v;
	v.s = (jshort) (*(uint32_t*) c);
	return v;
}

static jvalue long2short(char* c)
{
	jvalue v;
	v.s = (jshort) (*(int64_t*) c);
	return v;
}

static jvalue ulong2short(char* c)
{
	jvalue v;
	v.s = (jshort) (*(uint64_t*) c);
	return v;
}

static jvalue float2short(char* c)
{
	jvalue v;
	v.s = (jshort) (*(float*) c);
	return v;
}

static jvalue double2short(char* c)
{
	jvalue v;
	v.s = (jshort) (*(double*) c);
	return v;
}

static jvalue byte2int(char* c)
{
	jvalue v;
	v.i = (jint) (*(int8_t*) c);
	return v;
}

static jvalue ubyte2int(char* c)
{
	jvalue v;
	v.i = (jint) (*(uint8_t*) c);
	return v;
}

static jvalue short2int(char* c)
{
	jvalue v;
	v.i = (jint) (*(int16_t*) c);
	return v;
}

static jvalue ushort2int(char* c)
{
	jvalue v;
	v.i = (jint) (*(uint16_t*) c);
	return v;
}

static jvalue int2int(char* c)
{
	jvalue v;
	v.i = (jint) (*(int32_t*) c);
	return v;
}

static jvalue uint2int(char* c)
{
	jvalue v;
	v.i = (jint) (*(uint32_t*) c);
	return v;
}

static jvalue long2int(char* c)
{
	jvalue v;
	v.i = (jint) (*(int64_t*) c);
	return v;
}

static jvalue ulong2int(char* c)
{
	jvalue v;
	v.i = (jint) (*(uint64_t*) c);
	return v;
}

static jvalue float2int(char* c)
{
	jvalue v;
	v.i = (jint) (*(float*) c);
	return v;
}

static jvalue double2int(char* c)
{
	jvalue v;
	v.i = (jint) (*(double*) c);
	return v;
}

static jvalue byte2long(char* c)
{
	jvalue v;
	v.j = (jlong) (*(int8_t*) c);
	return v;
}

static jvalue ubyte2long(char* c)
{
	jvalue v;
	v.j = (jlong) (*(uint8_t*) c);
	return v;
}

static jvalue short2long(char* c)
{
	jvalue v;
	v.j = (jlong) (*(int16_t*) c);
	return v;
}

static jvalue ushort2long(char* c)
{
	jvalue v;
	v.j = (jlong) (*(uint16_t*) c);
	return v;
}

static jvalue int2long(char* c)
{
	jvalue v;
	v.j = (jlong) (*(int32_t*) c);
	return v;
}

static jvalue uint2long(char* c)
{
	jvalue v;
	v.j = (jlong) (*(uint32_t*) c);
	return v;
}

static jvalue long2long(char* c)
{
	jvalue v;
	v.j = (jlong) (*(int64_t*) c);
	return v;
}

static jvalue ulong2long(char* c)
{
	jvalue v;
	v.j = (jlong) (*(uint64_t*) c);
	return v;
}

static jvalue float2long(char* c)
{
	jvalue v;
	v.j = (jlong) (*(float*) c);
	return v;
}

static jvalue double2long(char* c)
{
	jvalue v;
	v.j = (jlong) (*(double*) c);
	return v;
}

static jvalue byte2float(char* c)
{
	jvalue v;
	v.f = (jfloat) (*(int8_t*) c);
	return v;
}

static jvalue ubyte2float(char* c)
{
	jvalue v;
	v.f = (jfloat) (*(uint8_t*) c);
	return v;
}

static jvalue short2float(char* c)
{
	jvalue v;
	v.f = (jfloat) (*(int16_t*) c);
	return v;
}

static jvalue ushort2float(char* c)
{
	jvalue v;
	v.f = (jfloat) (*(uint16_t*) c);
	return v;
}

static jvalue int2float(char* c)
{
	jvalue v;
	v.f = (jfloat) (*(int32_t*) c);
	return v;
}

static jvalue uint2float(char* c)
{
	jvalue v;
	v.f = (jfloat) (*(uint32_t*) c);
	return v;
}

static jvalue long2float(char* c)
{
	jvalue v;
	v.f = (jfloat) (*(int64_t*) c);
	return v;
}

static jvalue ulong2float(char* c)
{
	jvalue v;
	v.f = (jfloat) (*(uint64_t*) c);
	return v;
}

static jvalue float2float(char* c)
{
	jvalue v;
	v.f = (jfloat) (*(float*) c);
	return v;
}

static jvalue double2float(char* c)
{
	jvalue v;
	v.f = (jfloat) (*(double*) c);
	return v;
}

static jvalue byte2double(char* c)
{
	jvalue v;
	v.d = (jdouble) (*(int8_t*) c);
	return v;
}

static jvalue ubyte2double(char* c)
{
	jvalue v;
	v.d = (jdouble) (*(uint8_t*) c);
	return v;
}

static jvalue short2double(char* c)
{
	jvalue v;
	v.d = (jdouble) (*(int16_t*) c);
	return v;
}

static jvalue ushort2double(char* c)
{
	jvalue v;
	v.j = (jdouble) (*(uint16_t*) c);
	return v;
}

static jvalue int2double(char* c)
{
	jvalue v;
	v.d = (jdouble) (*(int32_t*) c);
	return v;
}

static jvalue uint2double(char* c)
{
	jvalue v;
	v.d = (jdouble) (*(uint32_t*) c);
	return v;
}

static jvalue long2double(char* c)
{
	jvalue v;
	v.d = (jdouble) (*(int64_t*) c);
	return v;
}

static jvalue ulong2double(char* c)
{
	jvalue v;
	v.d = (jdouble) (*(uint64_t*) c);
	return v;
}

static jvalue float2double(char* c)
{
	jvalue v;
	v.d = (jdouble) (*(float*) c);
	return v;
}

static jvalue double2double(char* c)
{
	jvalue v;
	v.d = (jdouble) (*(double*) c);
	return v;
}

typedef jvalue (*jconverter)(char*) ;

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
	switch (to[0])
	{
		case 'z':
			switch (from[0])
			{
				case 'c':
				case 'B':
				case 'b':
				case '?': return &bool2bool;
				case 'h':
				case 'H': return &short2bool;
				case 'i':
				case 'I':
				case 'l':
				case 'L': return &int2bool;
				case 'q':
				case 'Q': return &long2bool;
				case 'f': return &float2bool;
				case 'd': return &double2bool;
				case 'n':
				case 'N':
				case 'P':
				default: return 0;
			}
		case 'b':
			switch (from[0])
			{
				case '?':
				case 'c':
				case 'B':
				case 'b': return &byte2byte;
				case 'h': return &short2byte;
				case 'H': return &ushort2byte;
				case 'i':
				case 'l': return &int2byte;
				case 'I':
				case 'L': return &uint2byte;
				case 'q': return &long2byte;
				case 'Q': return &ulong2byte;
				case 'f': return &float2byte;
				case 'd': return &double2byte;
				case 'n':
				case 'N':
				case 'P':
				default: return 0;
			}
		case 's':
			switch (from[0])
			{
				case '?':
				case 'c':
				case 'b': return &byte2short;
				case 'B': return &ubyte2short;
				case 'h': return &short2short;
				case 'H': return &ushort2short;
				case 'i':
				case 'l': return &int2short;
				case 'I':
				case 'L': return &uint2short;
				case 'q': return &long2short;
				case 'Q': return &ulong2short;
				case 'f': return &float2short;
				case 'd': return &double2short;
				case 'n':
				case 'N':
				case 'P':
				default: return 0;
			}
		case 'i':
			switch (from[0])
			{
				case '?':
				case 'c':
				case 'b': return &byte2int;
				case 'B': return &ubyte2int;
				case 'h': return &short2int;
				case 'H': return &ushort2int;
				case 'i':
				case 'l': return &int2int;
				case 'I':
				case 'L': return &uint2int;
				case 'q': return &long2int;
				case 'Q': return &ulong2int;
				case 'f': return &float2int;
				case 'd': return &double2int;
				case 'n':
				case 'N':
				case 'P':
				default: return 0;
			}
		case 'j':
			switch (from[0])
			{
				case '?':
				case 'c':
				case 'b': return &byte2long;
				case 'B': return &ubyte2long;
				case 'h': return &short2long;
				case 'H': return &ushort2long;
				case 'i':
				case 'l': return &int2long;
				case 'I':
				case 'L': return &uint2long;
				case 'q': return &long2long;
				case 'Q': return &ulong2long;
				case 'f': return &float2long;
				case 'd': return &double2long;
				case 'n':
				case 'N':
				case 'P':
				default: return 0;
			}
		case 'f':
			switch (from[0])
			{
				case '?':
				case 'c':
				case 'b': return &byte2float;
				case 'B': return &ubyte2float;
				case 'h': return &short2float;
				case 'H': return &ushort2float;
				case 'i':
				case 'l': return &int2float;
				case 'I':
				case 'L': return &uint2float;
				case 'q': return &long2float;
				case 'Q': return &ulong2float;
				case 'f': return &float2float;
				case 'd': return &double2float;
				case 'n':
				case 'N':
				case 'P':
				default: return 0;
			}
		case 'd':
			switch (from[0])
			{
				case '?':
				case 'c':
				case 'b': return &byte2double;
				case 'B': return &ubyte2double;
				case 'h': return &short2double;
				case 'H': return &ushort2double;
				case 'i':
				case 'l': return &int2double;
				case 'I':
				case 'L': return &uint2double;
				case 'q': return &long2double;
				case 'Q': return &ulong2double;
				case 'f': return &float2double;
				case 'd': return &double2double;
				case 'n':
				case 'N':
				case 'P':
				default: return 0;
			}
		default:
			return 0;
	}
}
