from hstar import engine
import pytest


SIMPLIFY_EXAMPLES = [
    ('I', 'I'),
    ('K', 'K'),
    ('B', 'B'),
    ('C', 'C'),
    ('S', 'S'),
    ('APP I I', 'I'),
    ('APP I K', 'K'),
    ('APP K I', 'APP K I'),
    ('APP K APP I I', 'APP K I'),
    ('APP APP K B C', 'B'),
    ('APP B I', 'APP B I'),
    ('APP APP B I K', 'APP APP B I K'),
    ('APP APP APP B I K B', 'APP K B'),
    ('APP C I', 'APP C I'),
    ('APP APP C I K', 'APP APP C I K'),
    ('APP APP APP C I K B', 'APP B K'),
    ('APP S I', 'APP S I'),
    ('APP APP S I K', 'APP APP S I K'),
    ('APP APP APP S I K B', 'APP APP APP S I K B'),
]


@pytest.mark.parametrize("string,expected", SIMPLIFY_EXAMPLES)
def test_simplfy(string, expected):
    term = engine.parse(string)
    actual = engine.serialize(term)
    assert actual == expected


NORMALIZE_EXAMPLES = [
    ('APP APP APP S I K B', 'APP APP APP S I K B', 0),
    pytest.mark.xfail(('APP APP APP S I K B', 'APP B APP K B', 1)),
    pytest.mark.xfail(('APP APP APP S I K B', 'APP B APP K B', 2)),
]


@pytest.mark.parametrize("string,expected,budget", NORMALIZE_EXAMPLES)
def test_normalize(string, expected, budget):
    term = engine.parse(string)
    term = engine.normalize(term, budget=[budget])
    actual = engine.serialize(term)
    assert actual == expected
