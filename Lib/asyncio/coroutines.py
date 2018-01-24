__all__ = 'coroutine', 'iscoroutinefunction', 'iscoroutine'

import collections.abc
import functools
import inspect
import os
import sys
import traceback
import types

from . import base_futures
from . import constants
from . import format_helpers
from .log import logger


def _is_debug_mode():
    # If you set _DEBUG to true, @coroutine will wrap the resulting
    # generator objects in a CoroWrapper instance (defined below).  That
    # instance will log a message when the generator is never iterated
    # over, which may happen when you forget to use "await" or "yield from"
    # with a coroutine call.
    # Note that the value of the _DEBUG flag is taken
    # when the decorator is used, so to be of any use it must be set
    # before you define your coroutines.  A downside of using this feature
    # is that tracebacks show entries for the CoroWrapper.__next__ method
    # when _DEBUG is true.
    return sys.flags.dev_mode or (not sys.flags.ignore_environment and
                                  bool(os.environ.get('PYTHONASYNCIODEBUG')))


_DEBUG = _is_debug_mode()


class CoroWrapper:
    # Wrapper for coroutine object in _DEBUG mode.

    def __init__(self, gen, func=None):
        assert inspect.isgenerator(gen) or inspect.iscoroutine(gen), gen
        self.gen = gen
        self.func = func  # Used to unwrap @coroutine decorator
        self._source_traceback = format_helpers.extract_stack(sys._getframe(1))
        self.__name__ = getattr(gen, '__name__', None)
        self.__qualname__ = getattr(gen, '__qualname__', None)

    def __repr__(self):
        coro_repr = _format_coroutine(self)
        if self._source_traceback:
            frame = self._source_traceback[-1]
            coro_repr += f', created at {frame[0]}:{frame[1]}'

        return f'<{self.__class__.__name__} {coro_repr}>'

    def __iter__(self):
        return self

    def __next__(self):
        return self.gen.send(None)

    def send(self, value):
        return self.gen.send(value)

    def throw(self, type, value=None, traceback=None):
        return self.gen.throw(type, value, traceback)

    def close(self):
        return self.gen.close()

    @property
    def gi_frame(self):
        return self.gen.gi_frame

    @property
    def gi_running(self):
        return self.gen.gi_running

    @property
    def gi_code(self):
        return self.gen.gi_code

    def __await__(self):
        return self

    @property
    def gi_yieldfrom(self):
        return self.gen.gi_yieldfrom

    def __del__(self):
        # Be careful accessing self.gen.frame -- self.gen might not exist.
        gen = getattr(self, 'gen', None)
        frame = getattr(gen, 'gi_frame', None)
        if frame is not None and frame.f_lasti == -1:
            msg = f'{self!r} was never yielded from'
            tb = getattr(self, '_source_traceback', ())
            if tb:
                tb = ''.join(traceback.format_list(tb))
                msg += (f'\nCoroutine object created at '
                        f'(most recent call last, truncated to '
                        f'{constants.DEBUG_STACK_DEPTH} last lines):\n')
                msg += tb.rstrip()
            logger.error(msg)


def coroutine(func):
    """Decorator to mark coroutines.

    If the coroutine is not yielded from before it is destroyed,
    an error message is logged.
    """
    if inspect.iscoroutinefunction(func):
        # In Python 3.5 that's all we need to do for coroutines
        # defined with "async def".
        return func

    if inspect.isgeneratorfunction(func):
        coro = func
    else:
        @functools.wraps(func)
        def coro(*args, **kw):
            res = func(*args, **kw)
            if (base_futures.isfuture(res) or inspect.isgenerator(res) or
                    isinstance(res, CoroWrapper)):
                res = yield from res
            else:
                # If 'res' is an awaitable, run it.
                try:
                    await_meth = res.__await__
                except AttributeError:
                    pass
                else:
                    if isinstance(res, collections.abc.Awaitable):
                        res = yield from await_meth()
            return res

    coro = types.coroutine(coro)
    if not _DEBUG:
        wrapper = coro
    else:
        @functools.wraps(func)
        def wrapper(*args, **kwds):
            w = CoroWrapper(coro(*args, **kwds), func=func)
            if w._source_traceback:
                del w._source_traceback[-1]
            # Python < 3.5 does not implement __qualname__
            # on generator objects, so we set it manually.
            # We use getattr as some callables (such as
            # functools.partial may lack __qualname__).
            w.__name__ = getattr(func, '__name__', None)
            w.__qualname__ = getattr(func, '__qualname__', None)
            return w

    wrapper._is_coroutine = _is_coroutine  # For iscoroutinefunction().
    return wrapper


# A marker for iscoroutinefunction.
_is_coroutine = object()


def iscoroutinefunction(func):
    """Return True if func is a decorated coroutine function."""
    return (inspect.iscoroutinefunction(func) or
            getattr(func, '_is_coroutine', None) is _is_coroutine)


# Prioritize native coroutine check to speed-up
# asyncio.iscoroutine.
_COROUTINE_TYPES = (types.CoroutineType, types.GeneratorType,
                    collections.abc.Coroutine, CoroWrapper)
_iscoroutine_typecache = set()


def iscoroutine(obj):
    """Return True if obj is a coroutine object."""
    if type(obj) in _iscoroutine_typecache:
        return True

    if isinstance(obj, _COROUTINE_TYPES):
        # Just in case we don't want to cache more than 100
        # positive types.  That shouldn't ever happen, unless
        # someone stressing the system on purpose.
        if len(_iscoroutine_typecache) < 100:
            _iscoroutine_typecache.add(type(obj))
        return True
    else:
        return False


def _format_coroutine(coro):
    assert iscoroutine(coro)

    if not hasattr(coro, 'cr_code') and not hasattr(coro, 'gi_code'):
        # Most likely a built-in type or a Cython coroutine.

        # Built-in types might not have __qualname__ or __name__.
        coro_name = getattr(
            coro, '__qualname__',
            getattr(coro, '__name__', type(coro).__name__))
        coro_name = f'{coro_name}()'

        running = False
        try:
            running = coro.cr_running
        except AttributeError:
            try:
                running = coro.gi_running
            except AttributeError:
                pass

        if running:
            return f'{coro_name} running'
        else:
            return coro_name

    coro_name = None
    if isinstance(coro, CoroWrapper):
        func = coro.func
        coro_name = coro.__qualname__
        if coro_name is not None:
            coro_name = f'{coro_name}()'
    else:
        func = coro

    if coro_name is None:
        coro_name = format_helpers._format_callback(func, (), {})

    try:
        coro_code = coro.gi_code
    except AttributeError:
        coro_code = coro.cr_code

    try:
        coro_frame = coro.gi_frame
    except AttributeError:
        coro_frame = coro.cr_frame

    filename = coro_code.co_filename
    lineno = 0
    if (isinstance(coro, CoroWrapper) and
            not inspect.isgeneratorfunction(coro.func) and
            coro.func is not None):
        source = format_helpers._get_function_source(coro.func)
        if source is not None:
            filename, lineno = source
        if coro_frame is None:
            coro_repr = f'{coro_name} done, defined at {filename}:{lineno}'
        else:
            coro_repr = f'{coro_name} running, defined at {filename}:{lineno}'
    elif coro_frame is not None:
        lineno = coro_frame.f_lineno
        coro_repr = f'{coro_name} running at {filename}:{lineno}'
    else:
        lineno = coro_code.co_firstlineno
        coro_repr = f'{coro_name} done, defined at {filename}:{lineno}'

    return coro_repr
