#*****************************************************************************
#   Copyright 2004-2008 Steve Menard
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#	   http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#   
#*****************************************************************************
import _jclass, _jpype

def _initialize() :
	_jclass.registerClassCustomizer(PropertiesCustomizer())

def _is_java_method(attribute):
        return isinstance(attribute, _jpype._JavaMethod)

class PropertiesCustomizer(object) :
	def canCustomize(self, name, jc) :
		return True
		
	def customize(self, name, jc, bases, members) :
		gets = {}
		sets = {}
		
		for i in members :
			if not _is_java_method(members[i]):
				continue
                        access, bare_attrib = i[:3], i[3:]
                        attribute = bare_attrib[:1].lower() + bare_attrib[1:]
                        if attribute in members:
                                if _is_java_method(members[attribute]):
                                        continue
			if access == 'get' :
				if len(i) > 3 and members[i].isBeanAccessor() :
                                        gets[attribute] = members[i]
			elif access == 'set' :
				if len(i) > 3 and members[i].isBeanMutator() :
                                        sets[attribute] = members[i]
				
		for i in gets :
			members[i] = property(gets[i], sets.get(i, None))
