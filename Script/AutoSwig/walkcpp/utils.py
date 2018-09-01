import pydoc
import re
import sys

from clang.cindex import CursorKind, TypeKind


_builtin_types = [
    TypeKind.VOID,
    TypeKind.BOOL,
    TypeKind.CHAR_U,
    TypeKind.UCHAR,
    # TypeKind.CHAR16,
    # TypeKind.CHAR32,
    TypeKind.USHORT,
    TypeKind.UINT,
    TypeKind.ULONG,
    TypeKind.ULONGLONG,
    # TypeKind.UINT128,
    TypeKind.CHAR_S,
    TypeKind.SCHAR,
    # TypeKind.WCHAR,
    TypeKind.SHORT,
    TypeKind.INT,
    TypeKind.LONG,
    TypeKind.LONGLONG,
    # TypeKind.INT128,
    TypeKind.FLOAT,
    TypeKind.DOUBLE,
    # TypeKind.LONGDOUBLE,
    TypeKind.NULLPTR,
    # TypeKind.FLOAT128,
    # TypeKind.HALF,
]


def split_arguments():
    args = []
    for cmd in sys.argv[1:]:
        if cmd == 'bind':
            if len(args):
                yield args
                args = []
        else:
            args.append(cmd)
    if len(args):
        yield args


def import_class(cls_name: str):
    return pydoc.locate(cls_name)


def get_fully_qualified_name(c):
    if c is None or not c.spelling or c.kind == CursorKind.TRANSLATION_UNIT:
        return None

    res = get_fully_qualified_name(c.semantic_parent)
    if res:
        return f'{res}::{c.spelling}'
    else:
        return c.spelling


def is_builtin_type(t):
    if t.kind in (TypeKind.POINTER, TypeKind.LVALUEREFERENCE, TypeKind.RVALUEREFERENCE) and \
       t.get_pointee().kind in _builtin_types:
        return True
    return t.kind in _builtin_types


def desugar_type(t):
    # TODO: cover more bases
    t = t.get_canonical()
    if t.kind in (TypeKind.POINTER, TypeKind.RVALUEREFERENCE, TypeKind.LVALUEREFERENCE):
        return desugar_type(t.get_pointee())
    return t


def sanitize_symbol(symbol):
    return re.sub(r'[^0-9a-z_]', '_', symbol, flags=re.IGNORECASE).strip('_')


def name_argument(name, index):
    if name:
        return name
    return f'arg{index}'
