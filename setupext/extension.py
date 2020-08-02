from setuptools import Extension as _Extension


class Extension(_Extension):
    def __init__(self, name, sources, extra=[], py=[], **kwargs):
        self.extra = extra
        self.py = py
        super().__init__(name, sources, **kwargs)
