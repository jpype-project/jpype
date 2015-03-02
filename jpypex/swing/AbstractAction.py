from jpype import java, javax, JProxy

class AbstractAction(object):
    ACCELERATOR_KEY    = javax.swing.Action.ACCELERATOR_KEY
    ACTION_COMMAND_KEY = javax.swing.Action.ACTION_COMMAND_KEY
    DEFAULT            = javax.swing.Action.DEFAULT
    LONG_DESCRIPTION   = javax.swing.Action.LONG_DESCRIPTION
    MNEMONIC_KEY       = javax.swing.Action.MNEMONIC_KEY
    NAME               = javax.swing.Action.NAME
    SHORT_DESCRIPTION  = javax.swing.Action.SHORT_DESCRIPTION
    SMALL_ICON         = javax.swing.Action.SMALL_ICON

    def __init__(self, cb, name=None, icon=None):
        object.__init__(self)

        self.__proxy = JProxy(javax.swing.Action, inst=self)
        self.__values = {}
        self.__cb = cb
        self.__listeners = []
        self.__enabled = True

        if name is not None:
            self.putValue(AbstractAction.NAME, name)

        if icon is not None:
            self.putValue(AbstractAction.SMALL_ICON, icon)

    proxy = property(lambda self: self.__proxy)

    def addPropertyChangeListener(self, listener):
        self.__listeners.append(listener)

    def getValue(self, key):
        return self.__values.get(key, None)

    def isEnabled(self):
        if self.__enabled:
            return True
        return False

    def putValue(self, key, value):
        oldVal = self.__values.get(key, None)
        if oldVal != value:
            self.__values[key] = value
            self.__notify(key, oldVal, value)

    def removePropertyChangeListener(self, listener):
        self.__listeners.remove(listener)

    def setEnabled(self, b):
        if (b and not self.__enabled) or (not b and self.__enabled):
            self.__enabled = b
            self.__notify("enabled", java.lang.Boolean(not b),
                          java.lang.Boolean(b))

    def actionPerformed(self, ev):
        self.__cb(ev)

    def __notify(self, k, oldVal, newVal):
        ev = java.beans.PropertyChangeEvent(self.__proxy, k, oldVal, newVal)
        for i in self.__listeners:
            i.propertyChange(ev)
