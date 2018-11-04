import sys
from jpype import javax, JObject

__JMenuBar = javax.swing.JMenuBar
__JMenu = javax.swing.JMenu

def buildMenuBar(menuDef):
    mb = __JMenuBar()

    for i in menuDef:
        jm = buildMenu(i[0], i[1])
        mb.add(JObject(jm, __JMenu))

    return mb

def buildMenu(name, menuDef):
    jm = __JMenu(name)

    for i in menuDef:
        if i is None:
            jm.addSeparator()
        elif isinstance(i, list) or isinstance(i, tuple):
            jm2 = buildMenu(i[0], i[1])
            jm.add(jm2)
        else:
            jm.add(i.proxy)

    return jm
