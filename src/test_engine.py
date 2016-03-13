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


@pytest.mark.parametrize('string,expected', SIMPLIFY_EXAMPLES)
def test_simplfy_pristine_db(string, expected):
    engine.reset()
    term = engine.parse(string)
    actual = engine.serialize(term)
    assert actual == expected


@pytest.mark.parametrize('string,expected', SIMPLIFY_EXAMPLES)
def test_simplfy_dirty_db(string, expected):
    term = engine.parse(string)
    actual = engine.serialize(term)
    assert actual == expected


@pytest.mark.parametrize('string,expected', SIMPLIFY_EXAMPLES)
def test_normalize_zero_budget_pristine_db(string, expected):
    engine.reset()
    term = engine.parse(string)
    term = engine.normalize(term, budget=[0])
    actual = engine.serialize(term)
    assert actual == expected


NORMALIZE_EXAMPLES = [
    ('APP APP APP S I K B', 'APP APP APP S I K B', 0),
    ('APP APP APP S I K B', 'APP B APP K B', 1),
    ('APP APP APP S I K B', 'APP B APP K B', 2),
    ('APP APP APP S I K B', 'APP APP APP S I K B', 0),
]


@pytest.mark.parametrize('string,expected,budget', NORMALIZE_EXAMPLES)
def test_normalize_pristine_db(string, expected, budget):
    engine.reset()
    term = engine.parse(string)
    term = engine.normalize(term, budget=[budget])
    actual = engine.serialize(term)
    assert actual == expected
