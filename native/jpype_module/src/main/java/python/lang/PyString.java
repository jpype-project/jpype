/*
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may not
 *  use this file except in compliance with the License. You may obtain a copy of
 *  the License at
 * 
 *  http://www.apache.org/licenses/LICENSE-2.0
 * 
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 *  License for the specific language governing permissions and limitations under
 *  the License.
 * 
 *  See NOTICE file for details.
 */
package python.lang;

import python.protocol.PyIterable;
import org.jpype.bridge.BuiltIn;
import python.protocol.PyMapping;
import python.protocol.PySequence;

/**
 * Interface for Python objects acting as strings.
 *
 * This has some Java behaviors to facility use in Java code.
 *
 * @author nelson85
 */
public interface PyString extends PyObject, CharSequence
{

  static PyType type()
  {
    return (PyType) BuiltIn.eval("str", null, null);
  }

  public static PyString create(CharSequence s)
  {
    return BuiltIn.str(s);
  }

  PyString capitalize();

  PyString casefold();

  PyString center(int width);

  PyString center(int width, char fill);

  @Override
  char charAt(int index);

  int count(CharSequence str);

  int count(CharSequence str, int start);

  int count(CharSequence str, int start, int end);

  PyBytes encode();

  PyBytes encode(CharSequence encoding);

  PyBytes encode(CharSequence encoding, String errors);

  boolean endsWith(CharSequence suffix);

  boolean endsWith(CharSequence suffix, int start);

  boolean endsWith(CharSequence suffix, int start, int end);

  PyString expandTabs(int tabsize);

  int find(CharSequence str);

  int find(CharSequence str, int start);

  int find(CharSequence str, int start, int end);

  PyString format(PyTuple args, PyDict kwargs);

  PyString formatMap(PyMapping mapping);

  int index(CharSequence str);

  int index(CharSequence str, int start);

  int index(CharSequence str, int start, int end);

  boolean isAlnum();

  boolean isAlpha();

  boolean isAscii();

  boolean isDecimal();

  boolean isDigit();

  boolean isIdentifier();

  boolean isLower();

  boolean isNumeric();

  boolean isPrintable();

  boolean isSpace();

  boolean isTitle();

  boolean isUpper();

  PyString join(PyIterable iter);

  @Override
  int length();

  PyString lstrip();

  PyString lstrip(CharSequence characters);

  // Skipping maketrans
  PyTuple partition(CharSequence sep);

  PyString removePrefix(CharSequence prefix);

  PyString removeSuffix(CharSequence suffix);

  PyString replace(CharSequence old, CharSequence rep);

  PyString replace(CharSequence old, CharSequence rep, int count);

  int rfind(CharSequence sub);

  int rfind(CharSequence sub, int start);

  int rfind(CharSequence sub, int start, int end);

  int rindex(CharSequence sub);

  int rindex(CharSequence sub, int start);

  int rindex(CharSequence sub, int start, int end);

  PyTuple rpartition(CharSequence sep);

  PyList rsplit(CharSequence sep);

  PyList rsplit(CharSequence sep, int maxsplit);

  PyString rstrip(CharSequence chars);

  PyList split(CharSequence sep);

  PyList split(CharSequence sep, int maxsplit);

  PyList splitLines(boolean keepends);

  boolean startsWith(CharSequence prefix);

  boolean startsWith(CharSequence prefix, int start);

  boolean startsWith(CharSequence prefix, int start, int end);

  @Override
  PyString subSequence(int start, int end);

  PyString swapCase();

  PyString title();

  PyString translate(PyMapping m);

  PyString translate(PySequence seq);

  PyString upper();

  PyString zfill(int width);
}
