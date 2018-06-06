//*****************************************************************************
//Copyright 2004-2008 Steve Menard
//
//Licensed under the Apache License, Version 2.0 (the "License");
//you may not use this file except in compliance with the License.
//You may obtain a copy of the License at
//
//http://www.apache.org/licenses/LICENSE-2.0
//
//Unless required by applicable law or agreed to in writing, software
//distributed under the License is distributed on an "AS IS" BASIS,
//WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//See the License for the specific language governing permissions and
//limitations under the License.
//
//*****************************************************************************
package jpype.xml;

import org.xml.sax.helpers.*;
import org.xml.sax.*;

public class DelegateHandler extends DefaultHandler
{
    private EntityResolver er;
    private DTDHandler dh;
    private ContentHandler ch;
    private ErrorHandler eh;
    
    public DelegateHandler(EntityResolver er, DTDHandler dh, ContentHandler ch, ErrorHandler eh)
    {
        this.er = er;
        this.dh = dh;
        this.ch = ch;
        this.eh = eh;
    }
    
    public void characters(char[] ch, int start, int length) throws SAXException
    {
        if (ch != null)
        {
            this.ch.characters(ch, start, length);
        }
    }

    public void endDocument() throws SAXException
    {
        if (ch != null)
        {
            ch.endDocument();
        }
    }

    public void endElement(String namespaceURI, String localName, String qName) throws SAXException
    {
        if (ch != null)
        {
            ch.endElement(namespaceURI, localName, qName);
        }
    }

    public void endPrefixMapping(String prefix) throws SAXException
    {
        if (ch != null)
        {
            ch.endPrefixMapping(prefix);
        }
    }

    public void ignorableWhitespace(char[] ch, int start, int length) throws SAXException
    {
        if (ch != null)
        {
            this.ch.ignorableWhitespace(ch, start, length);
        }
    }

    public void processingInstruction(String target, String data) throws SAXException
    {
        if (ch != null)
        {
            ch.processingInstruction(target, data);
        }
    }

    public void setDocumentLocator(Locator locator)
    {
        if (ch != null)
        {
           ch.setDocumentLocator(locator); 
        }
    }

    public void skippedEntity(String name) throws SAXException
    {
        if (ch != null)
        {
            ch.skippedEntity(name);
        }
    }

    public void startDocument() throws SAXException
    {
        if (ch != null)
        {
            ch.startDocument();
        }
    }

    public void startElement(String namespaceURI, String localName, String qName, Attributes atts) throws SAXException
    {
        if (ch != null)
        {
            ch.startElement(namespaceURI, localName, qName, atts);
        }
    }

    public void startPrefixMapping(String prefix, String uri) throws SAXException 
    {
        if (ch != null)
        {
            ch.startPrefixMapping(prefix, uri);
        }
    }

}