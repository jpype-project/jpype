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
from jpype import *

startJVM(getDefaultJVMPath(), "-ea")

# XML test
Element = JPackage("org").w3c.dom.Element

def output(el, prefix="") :
    if not Element.__isinstance__(el) :
        return
        
    #print prefix, "<", el.getTagName(), 
    
    atts = el.getAttributes()
    for i in range(atts.getLength()) :
        a = atts.item(i);
        #print a.getNodeName(), '="%s"' % a.getNodeValue(), 
    #print '>'
    
    nl = el.getChildNodes()
    for i in range(nl.getLength()) :
        output(nl.item(i), prefix+"  ")


    #print prefix, "</", el.getTagName(), ">"

t = time.time()
count = 30
for i in range(count) :    
    build = javax.xml.parsers.DocumentBuilderFactory.newInstance().newDocumentBuilder()
    doc = build.parse("d:/darkwolf/jpype/test/sample/big.xml")
    
    el = doc.getDocumentElement()
    output(el)

t2 = time.time()
print count, "iterations in", t2-t, "seconds"

shutdownJVM()
