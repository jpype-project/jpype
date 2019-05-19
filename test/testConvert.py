import jpype
from jpype.types import *
from jpype import java
import pandas

jpype.startJVM()


def canConvert(cls, obj):
    return cls.__javaclass__.canConvertToJava(obj)

types = [ 
    JBoolean,
    JChar,
    JShort,
    JInt,
    JLong,
    JFloat,
    JDouble,
    java.lang.Object,
    java.lang.Integer,
    java.lang.Class,
    java.lang.String,
    java.lang.Exception,
    JArray(JInt)
    ]

objects = [
 ("None", None), 
 ("True", True),
 ("False", False),
 ("int(1)", int(1)),
 ("int(2)", int(2)),
 ("float(12.3)", float(12.3)),
 ("JBoolean(True)", JBoolean(True)),
 ("JChar(1)", JChar(1)),
 ("JShort(1)", JShort(1)),
 ("JInt(1)", JInt(1)),
 ("JLong(1)", JLong(1)),
 ("JFloat(2.1)", JFloat(2.1)),
 ("JDouble(3.5)", JDouble(3.5)),
 ("str('c')", str('c')),
 ("str('abc')", str('abc')),
 ("JString('b')", JString('b')),
 ("java.lang.Object", java.lang.Object),
 ("java.lang.Object.class_", java.lang.Object.class_),
 ("java.lang.Object()", java.lang.Object()),
 ("JObject()", JObject()),
 ("JObject(1)", JObject(1)),
 ("java.lang.Integer(1)", java.lang.Integer(1)),
 ("JObject(1, JInt)", JObject(1, JInt)),
 ("JObject(1, JObject)", JObject(1, JObject)),
 ("java.lang.Throwable", java.lang.Throwable('d')),
 ("java.lang.Exception", java.lang.Exception('d')),
 ("JObject(j.l.rte('e'), j.l.e)", 
     JObject(java.lang.RuntimeException('e'), java.lang.Exception)),
 ("dict()", dict()),
 ("list()", list()),
 ('JArray(JInt)([1,2,3])', JArray(JInt)([1,2,3])),
]

columns = ['Name','Type']
columns.extend([str(t.class_.getName()).replace('java.lang.','j.l.') for t in types])
rows = []
for name, case in objects:
    m = {}
    q = str(type(case)).replace('<class ','').replace('>','')
    if 'jpype' in q:
        q="Java "+q.replace('jpype._jclass.','')
    else:
        q="Py "+q

    m['Name'] = name.replace('java.lang.','j.l.')
    m['Type'] = q.replace('java.lang.','j.l.')
    for t in types:
        c1 = str(t.class_.getName()).replace('java.lang.','j.l.')
        r = canConvert(t, case)
        if r == "exact":
            m[c1] = 'X'
        if r == "implicit":
            m[c1] = 'I'
        if r == "explicit":
            m[c1] = 'E'
        if r == "none":
            m[c1] = ''
    rows.append(m)

out = pandas.DataFrame(rows, columns=columns)
print(out)
