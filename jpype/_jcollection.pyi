from typing import Any, Iterable, Collection, List, Set, Mapping, Tuple, TypeVar, Iterator, Generator, Union, overload, Dict

E = TypeVar('E')
K = TypeVar('K')
V = TypeVar('V')


class _JIterable(Iterable[E]):
    def __iter__(self) -> Iterator[E]: ...


class _JCollection(Collection[E]):
    def __len__(self) -> int: ...

    def __delitem__(self, i: E) -> None: ...

    def __contains__(self, i: Any) -> bool: ...

    def __iter__(self) -> Iterator[E]: ...


class _JList(List[E]):
    pass


class _JMap(Dict[K, V]):
    def __len__(self) -> int: ...

    def __iter__(self) -> Iterator[K]: ...

    def __getitem__(self, ndx: K) -> V: ...


class _JSet(Set[E]):
    pass


class _JMapEntry(Tuple[K, V]):
    pass


class _JIterator(Iterator[E]):
    def __next__(self) -> E: ...


class _JEnumeration(Iterator[E]):
    def __next__(self) -> E: ...
