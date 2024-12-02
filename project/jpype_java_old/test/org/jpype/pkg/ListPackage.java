/* ****************************************************************************
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
**************************************************************************** */
package org.jpype.pkg;

/**
 *
 * @author nelson85
 */
public class ListPackage
{

  public static void main(String[] args)
  {
    JPypePackage pkg = new JPypePackage("java.lang");
    System.out.println(pkg.contents.size());
    for (String s : pkg.getContents())
    {
      System.out.println(s);
    }
    System.out.println(pkg.getObject("Class"));
  }

}
