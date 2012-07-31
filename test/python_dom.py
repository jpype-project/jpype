#*****************************************************************************
#   Copyright 2004-2008 Steve Menard
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#   
#*****************************************************************************
import time
from xml.dom import minidom

def output(el, prefix="") :
    if el.nodeType != el.ELEMENT_NODE :
        return
        
    #print prefix, "<", el.tagName, 
    
    atts = el.attributes
    for i in range(atts.length) :
        a = atts.item(i);
        #print a.nodeName, '="%s"' % a.nodeValue, 
    #print '>'
    
    nl = el.childNodes
    for i in range(nl.length) :
        output(nl.item(i), prefix+"  ")


    #print prefix, "</", el.tagName, ">"

t = time.time()    
count = 30
for i in range(count) :    
    doc = minidom.parse("d:/darkwolf/jpype/test/sample/big.xml")

    el = doc.documentElement
    output(el)

t2 = time.time()
print count, "iterations in", t2-t, "seconds"

