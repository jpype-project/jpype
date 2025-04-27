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

import java.util.List;
import org.jpype.bridge.Interpreter;
import static org.testng.Assert.*;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;
import python.protocol.PyBuffer;
import static python.lang.PyBuiltIn.*;
/**
 *
 * @author nelson85
 */
public class PyByteArrayNGTest
{

  @BeforeClass
  public static void setUpClass() throws Exception
  {
    Interpreter.getInstance().start(new String[0]);
  }

  // Helper method to create a new instance of PyByteArray for testing
  private PyByteArray newInstance()
  {
    // Assume this method provides a concrete implementation of PyByteArray
    return PyByteArray.create(0); // Replace with actual instantiation logic
  }

  @Test
  public void testCreateWithLength()
  {
    int length = 10;
    PyByteArray instance = PyByteArray.create(length);

    // Assert that the created PyByteArray object is not null
    assertNotNull(instance, "PyByteArray object should not be null");

    // Optionally, verify the size of the bytearray (if accessible)
    int len2 = len(instance);
    assertEquals(len2, length, "PyByteArray size should match the specified length");
  }

  @Test
  public void testCreateFromIterable()
  {
    Iterable<PyObject> iterable = List.of(newInstance(), newInstance());
    PyByteArray instance = PyByteArray.of(iterable);

    // Assert that the created PyByteArray object is not null
    assertNotNull(instance, "PyByteArray object should not be null");

    // Optionally, verify the contents of the PyByteArray (if accessible)
    // Example: assertTrue(instance.containsAll(iterable));
  }

  @Test
  public void testCreateFromBuffer()
  {
    PyBuffer buffer = (PyBuffer) PyBytes.fromHex("48656c6c6f");
    PyByteArray instance = PyByteArray.of(buffer);

    // Assert that the created PyByteArray object is not null
    assertNotNull(instance, "PyByteArray object should not be null");

    // Optionally, verify the contents of the PyByteArray (if accessible)
    // Example: assertEquals(instance.getBuffer(), buffer);
  }

  @Test
  public void testType()
  {
    PyType pyType = PyByteArray.type();

    // Assert that the PyType object is not null
    assertNotNull(pyType, "PyType object should not be null");

    // Optionally, verify the type name (if accessible)
    // Example: assertEquals(pyType.getName(), "bytearray");
  }

  @Test
  public void testDecode()
  {
//    PyByteArray instance = newInstance();
//    PyObject encoding = new PyObject(); // Assume PyObject is properly instantiated
//    PyObject delete = null; // No bytes to delete
//
//    PyObject decoded = instance.decode(encoding, delete);
//
//    // Assert that the decoded PyObject is not null
//    assertNotNull(decoded, "Decoded PyObject should not be null");
//
//    // Optionally, verify the decoded content (if accessible)
//    // Example: assertEquals(decoded.toString(), "expectedString");
  }

  @Test
  public void testTranslate()
  {
//    PyByteArray instance = newInstance();
//    PyObject translationTable = new PyObject(); // Assume PyObject is properly instantiated
//    PyObject translated = instance.translate(translationTable);
//
//    // Assert that the translated PyObject is not null
//    assertNotNull(translated, "Translated PyObject should not be null");
//
//    // Optionally, verify the translated content (if accessible)
//    // Example: assertEquals(translated.toString(), "expectedTranslatedString");
  }

  @Test
  public void testFromHex()
  {
    CharSequence hexString = "48656c6c6f"; // Hex for "Hello"
    PyByteArray instance = PyByteArray.fromHex(hexString);

    // Assert that the created PyByteArray object is not null
    assertNotNull(instance, "PyByteArray object should not be null");

    // Optionally, verify the decoded content (if accessible)
    // Example: assertEquals(instance.toString(), "Hello");
  }

  @Test
  public void testSize()
  {
    PyByteArray instance = PyByteArray.create(10);

    // Assert that the size of the PyByteArray matches the specified length
    int len2 = len(instance);
    assertEquals(len2, 10, "PyByteArray size should match the specified length");
  }

}
