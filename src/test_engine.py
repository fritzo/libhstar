import hstar.engine
import functools


def for_each_kwargs(examples):

    def decorator(fun):

        def fun_one(i):
            fun(**examples[i])

        @functools.wraps(fun)
        def decorated():
            for i in xrange(len(examples)):
                yield fun_one, i

        return decorated
    return decorator


SIMPLIFY_EXAMPLES = [
    {'string': 'I', 'expected': 'I'},
    {'string': 'K', 'expected': 'K'},
    {'string': 'B', 'expected': 'B'},
    {'string': 'C', 'expected': 'C'},
    {'string': 'S', 'expected': 'S'},
    {'string': 'APP I I', 'expected': 'I'},
    {'string': 'APP I K', 'expected': 'K'},
    {'string': 'APP K I', 'expected': 'APP K I'},
    {'string': 'APP K APP I I', 'expected': 'APP K I'},
    {'string': 'APP APP K B C', 'expected': 'B'},
    {'string': 'APP B I', 'expected': 'APP B I'},
    {'string': 'APP APP B I K', 'expected': 'APP APP B I K'},
    {'string': 'APP APP APP B I K B', 'expected': 'APP K B'},
]


@for_each_kwargs(SIMPLIFY_EXAMPLES)
def test_simplfy(string, expected):
    term = hstar.engine.parse(string)
    actual = hstar.engine.serialize(term)
    assert actual == expected
