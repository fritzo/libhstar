"""Python reference implementation of reduction engine."""

# ----------------------------------------------------------------------------
# Terms

_terms = {}
_consts = {}
_pending = set()


def create_const(name):
    name = intern(name)
    term = name,
    assert term not in _terms, name
    _terms[term] = term
    _consts[name] = term
    return term


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


def reset():
    _terms.clear()
    _pending.clear()
    for term in _consts.itervalues():
        _terms[term] = term


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
        print('DEBUG {} {}'.format(head, args))
        if is_app(head):
            args.append(head[2])
            head = head[1]
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
    _terms[_APP, lhs, rhs] = head

    return head


def normalize(term, budget=[0]):
    if is_app(term):
        return app(term[1], term[2], budget=budget)
    else:
        return term


# ----------------------------------------------------------------------------
# Serialization

def parse_tokens(tokens):
    head = next(tokens)
    if head == _APP:
        lhs = parse_tokens(tokens)
        rhs = parse_tokens(tokens)
        return app(lhs, rhs)
    elif head in _consts:
        return _consts[head]
    else:
        raise ValueError('Unrecognized token: {}'.format(head))


def parse(string):
    tokens = iter(string.split())
    term = parse_tokens(tokens)
    extra = list(tokens)
    if extra:
        raise ValueError('Unexpected tokens: {}'.format(' '.join(extra)))
    return term


def serialize_tokens(term, tokens):
    assert term in _terms, term
    tag = term[0]
    if tag is _APP:
        tokens.append(_APP)
        serialize_tokens(term[1], tokens)
        serialize_tokens(term[2], tokens)
    elif tag in _consts:
        tokens.append(tag)
    else:
        raise ValueError('Unserializable term: {}'.format(term))


def serialize(term):
    tokens = []
    serialize_tokens(term, tokens)
    return ' '.join(tokens)
