
class AssertRaises(object):
    def __init__(self, expected):
         self.expected = expected

    def __enter__(self):
         return self

    def __exit__(self, exc_type, exc_value, tb):
        if exc_type is None:
            try:
                 exc_name = self.expected.__name__
            except AttributeError:
                 exc_name = str(self.expected)
            raise AssertionError("{0} not raised".format(exc_name))
        if not issubclass(exc_type, self.expected):
             # let unexpected exceptions pass through
             return False
        return True

