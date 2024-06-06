import doctest
import unittest


doctests = """
########### Tests mostly copied from test_setcomps.py ############

Test simple loop with conditional

    >>> sum({{i*i for i in range(100) if i&1 == 1}})
    166650

Test simple case

    >>> {{2*y + x + 1 for x in (0,) for y in (1,)}}
    frozenset({3})

Test simple nesting

    >>> list(sorted({{(i,j) for i in range(3) for j in range(4)}}))
    [(0, 0), (0, 1), (0, 2), (0, 3), (1, 0), (1, 1), (1, 2), (1, 3), (2, 0), (2, 1), (2, 2), (2, 3)]

Test nesting with the inner expression dependent on the outer

    >>> list(sorted({{(i,j) for i in range(4) for j in range(i)}}))
    [(1, 0), (2, 0), (2, 1), (3, 0), (3, 1), (3, 2)]

Test the idiom for temporary variable assignment in comprehensions.

    >>> sorted({{j*j for i in range(4) for j in [i+1]}})
    [1, 4, 9, 16]
    >>> sorted({{j*k for i in range(4) for j in [i+1] for k in [j+1]}})
    [2, 6, 12, 20]
    >>> sorted({{j*k for i in range(4) for j, k in [(i+1, i+2)]}})
    [2, 6, 12, 20]

Not assignment

    >>> sorted({{i*i for i in [*range(4)]}})
    [0, 1, 4, 9]
    >>> sorted({{i*i for i in (*range(4),)}})
    [0, 1, 4, 9]

Make sure the induction variable is not exposed

    >>> i = 20
    >>> sum({{i*i for i in range(100)}})
    328350

    >>> i
    20

Verify that syntax error's are raised for setcomps used as lvalues

    >>> {{y for y in (1,2)}} = 10          # doctest: +IGNORE_EXCEPTION_DETAIL
    Traceback (most recent call last):
       ...
    SyntaxError: ...

    >>> {{y for y in (1,2)}} += 10         # doctest: +IGNORE_EXCEPTION_DETAIL
    Traceback (most recent call last):
       ...
    SyntaxError: ...


Make a nested set comprehension that acts like set(range())

    >>> def srange(n):
    ...     return {{i for i in range(n)}}
    >>> list(sorted(srange(10)))
    [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]

Same again, only as a lambda expression instead of a function definition

    >>> lrange = lambda n:  {{i for i in range(n)}}
    >>> list(sorted(lrange(10)))
    [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]

Generators can call other generators:

    >>> def grange(n):
    ...     for x in {{i for i in range(n)}}:
    ...         yield x
    >>> list(sorted(grange(5)))
    [0, 1, 2, 3, 4]


Make sure that None is a valid return value

    >>> {{None for i in range(10)}}
    frozenset({None})

########### Tests for various scoping corner cases ############

Return lambdas that use the iteration variable as a default argument

    >>> items = {{(lambda i=i: i) for i in range(5)}}
    >>> {{x() for x in items}} == set(range(5))
    True

Same again, only this time as a closure variable

    >>> items = {{(lambda: i) for i in range(5)}}
    >>> {{x() for x in items}}
    frozenset({4})

Another way to test that the iteration variable is local to the list comp

    >>> items = {{(lambda: i) for i in range(5)}}
    >>> i = 20
    >>> {{x() for x in items}}
    frozenset({4})

And confirm that a closure can jump over the list comp scope

    >>> items = {{(lambda: y) for i in range(5)}}
    >>> y = 2
    >>> {{x() for x in items}}
    frozenset({2})

We also repeat each of the above scoping tests inside a function

    >>> def test_func():
    ...     items = {{(lambda i=i: i) for i in range(5)}}
    ...     return {{x() for x in items}}
    >>> test_func() == set(range(5))
    True

    >>> def test_func():
    ...     items = {{(lambda: i) for i in range(5)}}
    ...     return {{x() for x in items}}
    >>> test_func()
    frozenset({4})

    >>> def test_func():
    ...     items = {{(lambda: i) for i in range(5)}}
    ...     i = 20
    ...     return {{x() for x in items}}
    >>> test_func()
    frozenset({4})

    >>> def test_func():
    ...     items = {{(lambda: y) for i in range(5)}}
    ...     y = 2
    ...     return {{x() for x in items}}
    >>> test_func()
    frozenset({2})

"""


__test__ = {'doctests' : doctests}

def load_tests(loader, tests, pattern):
    tests.addTest(doctest.DocTestSuite())
    return tests


if __name__ == "__main__":
    unittest.main()
