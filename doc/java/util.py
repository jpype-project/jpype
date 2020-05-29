import jpype
from typing import Union


class Iterable:
    """ Customized interface for a container which can be iterated.

    JPype wraps ``java.lang.Iterable`` as the Python iterator
    interface.
    """

    def __iter__(self):
        """ Iterate over the members on this collect. """
        ...


class Collection:
    """ Customized interface representing a collection of items.

    JPype wraps ``java.util.Collection`` as a Python collection.
    """

    def __len__(self) -> int:
        """ Get the length of this collection.

        Use ``len(collection)`` to find the number of items in this
        collection.

        """
        ...

    def __delitem__(self, item):
        """ Collections do not support remove by index. """
        ...

    def __contains__(self, item) -> bool:
        """ Check if this collection contains this item.

        Use ``item in collection`` to check if the item is 
        present.

        Args:
           item: is the item to check for.  This must be a Java
           object or an object which can be automatically converted
           such as a string.

        Returns:
           bool: True if the item is in the collection.
        """
        ...


class List(Collection):
    """ Customized container holding items in a specified order.

    JPype customizes ``java.lang.List`` to be equivalent to a Python list.
    Java list fulfill the contract for ``collection.abc.MutableSequence``.
    """

    def __getitem__(self, ndx):
        """ Access an item or set of items.

        Use ``list[idx]`` to access a single item or ``list[i0:i1]`` to obtain
        a slice of the list.  Slices are views thus changing the view will
        alter the original list.  Slice stepping is not supported for Java
        lists.
        """
        ...

    def __setitem__(self, index: Union[int, slice], value):
        """ Set an item on the list.

        Use ``list[idx]=value`` to set a value on the list or 
        ``list[i0:i1] = values`` to replace a section of a list with
        another list of values.
        """
        ...

    def __delitem__(self, idx: Union[int, slice]):
        """ Delete an item by index.

        Use ``del list[idx]`` to remove ont itme from the list or
        ``del list[i0:i1]`` to remove a section of the list.
        """
        ...

    def __reversed__(self):
        """ Obtain an iterator that walks the list in reverse order.

        Use ``reversed(list)`` to traverse a list backwards.
        """
        ...

    def index(self, obj) -> int:
        """ Find the index that an item appears.

        Args:
            obj: A Java object or Python object which automatically
                converts to Java.

        Returns:
            int: The index where the item first appears in the list.

        Raises:
           ValueError: If the item is not on the list.
        """
        ...

    def count(self, obj):
        """ Count the number of times an object appears in a list.

        Args:
            obj: A Java object or Python object which automatically
                converts to Java.

        Returns:
            int: The number of times this object appears.
        """
        ...

    def insert(self, idx: int, obj):
        """ Insert an object at a specific position.

        Args:
            idx: The index to insert the item in front of.
            obj: The object to insert.

        Raises:
            TypeError: If the object cannot be converted to Java.
        """
        ...

    def append(self, obj):
        """ Append an object to the list.

        Args:
            obj: The object to insert.

        Raises:
            TypeError: If the object cannot be converted to Java.
        """
        ...

    def reverse(self):
        """ Reverse the order of the list in place.

        This is equivalent to ``java.util.Collections.reverse(list)``.
        """

    def extend(self, lst):
        """ Extends a list by adding a set of elements to the end.

        Args:
           lst: A Sequence holding items to be appended.

        Raises:
           TypeError: If the list to be added cannot be converted to Java.
        """
        ...

    def pop(self, idx=-1):
        """ Remove an item from the list.

        Args:
           idx (int, optional): Position to remove the item from.  If not
             specified the item is removed from the end of the list.

        Returns:
           The item or raises if index is outside of the list.
        """
        ...

    def __iadd__(self, obj):
        """ Add an items to the end of the list.

        Use ``list += obj`` to append one item.  This is simply an alias
        for add.
        """
        ...

    def __add__(self, obj):
        """ Combine two lists.

        Use ``list + seq`` to create a new list with additional members.
        This is only supported if the list can be cloned.
        """
        ...

    def remove(self, obj):
        """ Remove an item from the list by finding the first
        instance that matches.

        This overrides the Java method to provide the Python remove.
        Use ``lst.remove_`` to obtain the Java remove method.

        Args:
           obj: Must be a Java object or Python object that can 
             convert to Java automatically.

        Raises:
            ValueError: If the item is not present on the list.
        """
        ...


class Map:
    """ Customized container holding pairs of items like a dictionary.

    JPype customizes ``java.lang.List`` to be equivalent to a Python list.
    Java maps fulfill the contract for ``collection.abc.Mapping``.
    """

    def __len__(self):
        """ Get the number of items in this map.

        Use ``len(map)`` to get the number of items in the map.
        """
        ...

    def __iter__(self):
        """ Iterate the keys of the map.
        """
        ...

    def __delitem__(self, i):
        """ Remove an item by its key.

        Raises:
           TypeError: If the key cannot be converted to Java.
        """
        ...

    def __getitem__(self, ndx):
        """ Get a value by its key.

        Use ``map[key]`` to get the value associate with a key. 

        Raises:
           KeyError: If the key is not found in the map or the key
             cannot be converted to Java.
        """
        ...

    def __setitem__(self, key, value):
        """ Set a value associated with a key..

        Use ``map[key]=value`` to set the value associate with a key. 

        Raises:
           TypeError: If the key or value cannot be converted to Java.
        """
        ...

    def items(self):
        """ Get a list of entries in the map.

        The map entries are customized to appear as tuples with two 
        items.  Maps can traversed as key value pairs using ``map.items()``
        """
        ...

    def keys(self) -> list:
        """ Get a list of keys for this map.

        Use ``map.keySet()`` to obtain the keys as Java views them.

        Returns:
           list: A Python list holding all of the items.
        """
        ...

    def __contains__(self, item):
        """ Check if a key is in the map.

        Use ``item in map`` to verify if the map contains the item.
        This will return true whether on not the associated value
        is an object or None.

        Returns:
          True is the key is found.
        """
        ...


class Set(object):
    """ Customized Java Sets.

    Java sets only provide the ability to delete items.
    """
    ...


class Iterator:
    """ Customized Java Iterator.

    Java iterators act just like Python iterators for the 
    purposed of list comprehensions and foreach loops.
    """
    ...


class Enumeration:
    """ Customized Java enumeration.

    Enumerations are used rarely in Java, but can be iterated like a Java
    iterable using Python.
    """
    ...
