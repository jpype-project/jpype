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
#ifndef JP_MODIFIER_H
#define JP_MODIFIER_H

#ifdef __cplusplus
extern "C"
{
#endif

namespace JPModifier
{

/* https://docs.oracle.com/javase/specs/jvms/se11/html/jvms-4.html#jvms-4.1-200-E.1
 * https://docs.oracle.com/javase/specs/jvms/se11/html/jvms-4.html#jvms-4.5-200-A.1
 * https://docs.oracle.com/javase/specs/jvms/se11/html/jvms-4.html#jvms-4.6-200-A.1
 */

inline bool isPublic(jlong modifier)
{
	return (modifier & 0x0001) == 0x0001;
}

inline bool isPrivate(jlong modifier)
{
	return (modifier & 0x0002) == 0x0002;
}

inline bool isProtected(jlong modifier)
{
	return (modifier & 0x0004) == 0x0004;
}

inline bool isStatic(jlong modifier)
{
	return (modifier & 0x0008) == 0x0008;
}

inline bool isFinal(jlong modifier)
{
	return (modifier & 0x0010) == 0x0010;
}

inline bool isSuper(jlong modifier)
{
	return (modifier & 0x0020) == 0x0020;
}

inline bool isVolatile(jlong modifier)
{
	return (modifier & 0x0040) == 0x0040;
} // fields

inline bool isBridge(jlong modifier)
{
	return (modifier & 0x0040) == 0x0040;
} // methods

inline bool isTransient(jlong modifier)
{
	return (modifier & 0x0080) == 0x0080;
} // fields

inline bool isVarArgs(jlong modifier)
{
	return (modifier & 0x0080) == 0x0080;
} //methods

inline bool isNative(jlong modifier)
{
	return (modifier & 0x0100) == 0x0100;
}

inline bool isInterface(jlong modifier)
{
	return (modifier & 0x0200) == 0x0200;
}

inline bool isAbstract(jlong modifier)
{
	return (modifier & 0x0400) == 0x0400;
}

inline bool isStrict(jlong modifier)
{
	return (modifier & 0x0800) == 0x0800;
}

inline bool isSynthetic(jlong modifier)
{
	return (modifier & 0x1000) == 0x1000;
}

inline bool isAnnotation(jlong modifier)
{
	return (modifier & 0x2000) == 0x2000;
}

inline bool isEnum(jlong modifier)
{
	return (modifier & 0x4000) == 0x4000;
}

inline bool isModule(jlong modifier)
{
	return (modifier & 0x8000) == 0x8000;
}

/* JPype flags (must match ModifierCodes sections)
 */
inline bool isSpecial(jlong modifier)
{
	return (modifier & 0x00010000) == 0x00010000;
}

inline bool isThrowable(jlong modifier)
{
	return (modifier & 0x00020000) == 0x00020000;
}

inline bool isSerializable(jlong modifier)
{
	return (modifier & 0x00040000) == 0x00040000;
}

inline bool isAnonymous(jlong modifier)
{
	return (modifier & 0x00080000) == 0x00080000;
}

inline bool isFunctional(jlong modifier)
{
	return (modifier & 0x00100000) == 0x00100000;
}

inline bool isCallerSensitive(jlong modifier)
{
	return (modifier & 0x00200000) == 0x00200000;
}

inline bool isPrimitiveArray(jlong modifier)
{
	return (modifier & 0x00400000) == 0x00400000;
}

inline bool isComparable(jlong modifier)
{
	return (modifier & 0x00800000) == 0x00800000;
}

inline bool isBuffer(jlong modifier)
{
	return (modifier & 0x01000000) == 0x01000000;
}

inline bool isConstructor(jlong modifier)
{
	return (modifier & 0x10000000) == 0x10000000;
}

inline bool isBeanAccessor(jlong modifier)
{
	return (modifier & 0x20000000) == 0x20000000;
}

inline bool isBeanMutator(jlong modifier)
{
	return (modifier & 0x40000000) == 0x40000000;
}
}

#ifdef __cplusplus
}
#endif
#endif /* JP_MODIFIER_H */