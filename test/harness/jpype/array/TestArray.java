// *****************************************************************************
//Copyright 2004-2008 Steve Menard
//
//Licensed under the Apache License, Version 2.0 (the "License");
//you may not use this file except in compliance with the License.
//You may obtain a copy of the License at
//
//  http://www.apache.org/licenses/LICENSE-2.0
//
//Unless required by applicable law or agreed to in writing, software
//distributed under the License is distributed on an "AS IS" BASIS,
//WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//See the License for the specific language governing permissions and
//limitations under the License.
//
//*****************************************************************************
package jpype.array;

public class TestArray
{
    public TestArray()
    {
        
    }
    
    public int[] i = {12234,1234,234,1324,424,234,234,142,5,251,242,35,235,62,1235,46,245132,51, 2, 3, 4};
    
    public Object[] getSubClassArray()
    {
      return new String[] { "aaa", "bbb" };
    }

    public Object getArrayAsObject()
    {
      return new String[] { "aaa", "bbb" };
    }
    
    public char[] getCharArray()
    {
    	return new char[] { 'a', 'v', 'c', 'd' };
    }

    public byte[] getByteArray()
    {
    	String s = "avcd";
    	return s.getBytes();
    }
}