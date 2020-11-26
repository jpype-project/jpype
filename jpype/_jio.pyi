from types import TracebackType
from typing import ContextManager, Optional, Type


class _JCloseable(ContextManager['_JCloseable']):
    def __enter__(self) -> '_JCloseable': ...

    def __exit__(self, exception_type: Optional[Type[BaseException]], exception_value: Optional[BaseException],
                 traceback: Optional[TracebackType]) -> bool: ...
