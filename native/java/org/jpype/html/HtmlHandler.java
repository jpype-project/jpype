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
package org.jpype.html;

public interface HtmlHandler
{

  void cdata(String text);

  void comment(String contents);

  void endDocument();

  void endElement(String name);

  void startDocument();

  void startElement(String name, String attr);

  void text(String text);

  public Object getResult();

  public void directive(String content);

}
