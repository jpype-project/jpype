/* ****************************************************************************
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * See NOTICE file for details.
 * ***************************************************************************/
package org.jpype.bridge;


import java.util.List;
import java.util.Map;
import python.lang.PyDict;
import python.lang.PyList;
import python.lang.PyString;
import python.lang.PyTuple;

/**
 *
 * @author nelson85
 */
public interface Builtin
{
  public PyTuple tuple(Object... obj);
  public PyDict dict(Map<Object,Object> map);
  public PyList list(List<Object> list);
  public PyString str(String str);


// It is okay if not everything is exposed as the user can always evaluate
// statements if something is missing.
  
//Creation
//   dict()
//   set()
//   tuple()
//   complex()
//   bytearray()
//   list()
//   memoryview()
//   open()
//   property()
//   slice()
  
//Casting? 
// These take one argument they act on so they could be considered object method
// place on PyOject?
//   ascii()
//   str()
//   int()
//   iter()
//   bytes()
//   classmethod()
//   float()
//   type()
  
  
// Int only methods?
//   bin()
//   hex()
//   oct()
  
// Methods?  move to PyObject
//   memoryview - asMemoryView()
//   bool() - asBool()
//   float() - asFloat()
//   int() - asInt()
//   callable() - isCallable
//   repr()
//   len()
//   id()
//   hash()
//   isinstance()
//   issubclass()
//   setattr()
//   getattr()
//   hasattr()
//   delattr()
//   dir()
//   vars()
  
// Belong on scopes?
//   compile()
//   eval()
//   exec()
//   globals()
//   locals()
  
// Math
//   abs()
//   pow()
//   divmod()
//   chr()
//   ord()
//   round()
  
// Iterator/generators
//   max()
//   min()
//   all()
//   sum()
//   map()
//   zip()
//   enumerate()
//   range()
//   reversed()
//   sorted()
//   any()
  
// Method for iterator
//  next()
//  filter()

//aiter()
//anext()
//breakpoint()
//format()
//frozenset()
//help()
//input()
//object()
//print()
//staticmethod()
//super()

  
  
}
