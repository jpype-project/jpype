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

import org.jpype.bridge.Bridge;
import static org.testng.Assert.*;

/**
 *
 * @author nelson85
 */
public class PyByteArrayNGTest
{
    
    public PyByteArrayNGTest()
    {
    }

    @org.testng.annotations.BeforeClass
    public static void setUpClass() throws Exception
    {
        Bridge.create();
    }


    /**
     * Test of fromhex method, of class PyByteArray.
     */
    @org.testng.annotations.Test
    public void testFromhex()
    {
        System.out.println("fromhex");
        CharSequence str = "A5";
        PyByteArray result = PyByteArray.fromHex(str);
        assertTrue(result.isInstance(PyByteArray.type()));
    }

    /**
     * Test of decode method, of class PyByteArray.
     */
    @org.testng.annotations.Test
    public void testDecode()
    {
        System.out.println("decode");
        PyObject encoding = null;
        PyObject delete = null;
        PyByteArray instance = null;
        PyObject expResult = null;
        PyObject result = instance.decode(encoding, delete);
        assertEquals(result, expResult);
        // TODO review the generated test code and remove the default call to fail.
        fail("The test case is a prototype.");
    }

    /**
     * Test of translate method, of class PyByteArray.
     */
    @org.testng.annotations.Test
    public void testTranslate()
    {
        System.out.println("translate");
        PyObject table = null;
        PyByteArray instance = null;
        PyObject expResult = null;
        PyObject result = instance.translate(table);
        assertEquals(result, expResult);
        // TODO review the generated test code and remove the default call to fail.
        fail("The test case is a prototype.");
    }
    
}
