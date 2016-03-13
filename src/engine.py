"""Python reference implementation of reduction engine."""

import os

DEBUG = int(os.environ.get('HSTAR_DEBUG', 0))

# ----------------------------------------------------------------------------
# Terms

_terms = {}
_consts = {}
_pending = set()


def validate():
    for key, val in _terms.iteritems():
        assert val in _terms
        if key is not val:
            assert key != val
            assert key not in _pending
    for key, val in _consts.iteritems():
        assert val in _terms
        assert val not in _pending
    for term in _pending:
        assert term in _terms


def create_const(name):
    name = intern(name)
    term = name,
    assert term not in _terms, name
    _terms[term] = term
    _consts[name] = term
    return term


TOP = create_const('TOP')
BOT = create_const('BOT')
I = create_const('I')
K = create_const('K')
B = create_const('B')
C = create_const('C')
S = create_const('S')

_APP = intern('APP')


def is_app(term):
    return term[0] is _APP


def make_app(lhs, rhs, pending):
    term = _APP, lhs, rhs
    term = _terms.setdefault(term, term)
    if pending:
        _pending.add(term)
    else:
        _pending.discard(term)
    return term


EQUATIONS = [
    ('APP APP APP S I I APP APP S I I', 'BOT'),
]


def assume_equal(dep, rep):
    _terms[dep] = rep
    _pending.discard(dep)


def reset():
    _terms.clear()
    _pending.clear()
    for term in _consts.itervalues():
        _terms[term] = term

    if __debug__:
        validate()

    for lhs, rhs in EQUATIONS:
        lhs = parse(lhs)
        rhs = parse(rhs)
        assume_equal(lhs, rhs)

    if __debug__:
        validate()


# ----------------------------------------------------------------------------
# Reduction

def init_budget(value=0):
    return [value]


def try_consume_budget(budget):
    assert isinstance(budget, list), budget
    assert isinstance(budget[0], int) and budget[0] >= 0, budget
    if budget[0] > 0:
        budget[0] -= 1
        return True
    else:
        return False


def app(lhs, rhs, budget=[0]):
    '''
    Eagerly linear-beta reduce.
    '''
    # First check cache for most-reduced copy.
    try:
        term = _terms[_APP, lhs, rhs]
    except KeyError:
        head = lhs
        args = [rhs]
        pending = False
    else:
        if term not in _pending:
            return term
        assert is_app(term)
        head = term[1]
        args = [term[2]]
        pending = True

    # Head reduce.
    while True:
        if DEBUG:
            print('DEBUG {} {}'.format(head, args))
        if is_app(head):
            args.append(head[2])
            head = head[1]
        elif head is TOP:
            del args[:]
            break
        elif head is BOT:
            del args[:]
            break
        elif head is I:
            if len(args) < 1:
                break
            head = args.pop()
        elif head is K:
            if len(args) < 2:
                break
            x = args.pop()
            y = args.pop()
            head = x
        elif head is B:
            if len(args) < 3:
                break
            x = args.pop()
            y = args.pop()
            z = args.pop()
            head = app(x, app(y, z))
        elif head is C:
            if len(args) < 3:
                break
            x = args.pop()
            y = args.pop()
            z = args.pop()
            head = app(app(x, z), y)
        elif head is S:
            if len(args) < 3 or not try_consume_budget(budget):
                pending = True
                break
            x = args.pop()
            y = args.pop()
            z = args.pop()
            head = app(app(x, z), app(y, z))
        else:
            raise ValueError(head)

    # Simplify args.
    while args:
        arg = args.pop()
        arg = normalize(arg, budget=budget)
        pending = pending or arg in _pending
        head = make_app(head, arg, pending=pending)
        if not pending:
            _pending.discard(head)

    # Memoize result.
    term = make_app(lhs, rhs, pending=pending)
    if term is not head:
        _terms[term] = head
        _pending.discard(term)

    if __debug__:
        validate()

    return head


def normalize(term, budget=[0]):
    if is_app(term):
        return app(term[1], term[2], budget=budget)
    else:
        return term


# ----------------------------------------------------------------------------
# Serialization

def _parse_tokens(tokens):
    head = next(tokens)
    if head == _APP:
        lhs = _parse_tokens(tokens)
        rhs = _parse_tokens(tokens)
        return app(lhs, rhs)
    elif head in _consts:
        return _consts[head]
    else:
        raise ValueError('Unrecognized token: {}'.format(head))


def parse(string):
    tokens = iter(string.split())
    try:
        term = _parse_tokens(tokens)
    except StopIteration:
        raise ValueError('Early termination: {}'.format(string))
    extra = list(tokens)
    if extra:
        raise ValueError('Unexpected tokens: {}'.format(' '.join(extra)))
    return term


def _serialize_tokens(term, tokens):
    assert term in _terms, term
    tag = term[0]
    if tag is _APP:
        tokens.append(_APP)
        _serialize_tokens(term[1], tokens)
        _serialize_tokens(term[2], tokens)
    elif tag in _consts:
        tokens.append(tag)
    else:
        raise ValueError('Unserializable term: {}'.format(term))


def serialize(term):
    tokens = []
    _serialize_tokens(term, tokens)
    return ' '.join(tokens)
