import jpype
from jpype.types import *
from jpype import java
import pandas

jpype.startJVM()


def canConvert(cls, obj):
    return cls.__javaclass__.canConvertToJava(obj)


def runTest(cases, types, abbrev=lambda x: x):
    columns = ['Name', 'Type']
    columns.extend([abbrev(str(t.class_.getName())) for t in types])
    rows = []
    for name, case in cases:
        m = {}
        q = str(type(case)).replace('<class ', '').replace('>', '')
        if 'jpype' in q:
            q = "Java " + abbrev(q.replace('jpype._jclass.', ''))
        else:
            q = "Py " + q

        m['Name'] = abbrev(name)
        m['Type'] = abbrev(q)
        for t in types:
            c1 = abbrev(str(t.class_.getName()))
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
    return out


cases1 = [
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

    ("java.lang.Boolean(True)", java.lang.Boolean(True)),
    ("java.lang.Character(1)", java.lang.Character(1)),
    ("java.lang.Short(1)", java.lang.Short(1)),
    ("java.lang.Integer(1)", java.lang.Integer(1)),
    ("java.lang.Long(1)", java.lang.Long(1)),
    ("java.lang.Float(2.1)", java.lang.Float(2.1)),
    ("java.lang.Double(3.5)", java.lang.Double(3.5)),
    ("java.lang.Integer(1)", java.lang.Integer(1)),
    ("JObject(1)", JObject(1)),
    ("JObject(1.1)", JObject(1.1)),
    ("JObject(1, JInt)", JObject(1, JInt)),
]

cases2 = [
    ("bytes('c')", bytes('c', 'utf8')),
    ("str('c')", str('c')),
    ("str('abc')", str('abc')),
    ("JString('b')", JString('b')),
    ("JObject('a')", JObject('a')),
    ("java.lang.Object", java.lang.Object),
    ("java.lang.Object.class_", java.lang.Object.class_),
    ("java.lang.Object()", java.lang.Object()),
    ("JObject()", JObject()),
    ("JObject(1, JObject)", JObject(1, JObject)),
    ("JObject(None, JObject)", JObject(None, JObject)),
    ("java.lang.Throwable", java.lang.Throwable('d')),
    ("java.lang.Exception", java.lang.Exception('d')),
    ('java.lang.Byte(1)', java.lang.Byte(1)),
    ('JObject(java.lang.Byte(1),java.lang.Number)',
        JObject(java.lang.Byte(1), java.lang.Number)),
    ("dict()", dict()),
    ("list()", list()),
    ("[1,2]", [1, 2]),
    ("[.1,.2]", [.1, .2]),
    ('JArray(JInt)([1,2,3])', JArray(JInt)([1, 2, 3])),
]

types1 = [
    JBoolean,
    JChar,
    JShort,
    JInt,
    JLong,
    JFloat,
    JDouble,
    java.lang.Character,
    java.lang.Byte,
    java.lang.Short,
    java.lang.Integer,
    java.lang.Long,
    java.lang.Float,
    java.lang.Double,
    java.lang.Object,
    #    java.lang.Class,
    #    java.lang.String,
    #    java.lang.Exception,
    #    JArray(JInt)
]

types2 = [
    JChar,
    java.lang.String,
    java.lang.Object,
    java.lang.Number,
    java.lang.Class,
    java.lang.Exception,
    JArray(JInt),
    JArray(JDouble)
]


def abbrev(s):
    return s.replace("java.lang.", "j.l.")


print("Primitives and Boxed")
print(runTest(cases1, types1, abbrev=abbrev))
print()

print("Objects and casts")
print(runTest(cases2, types2, abbrev=abbrev))
