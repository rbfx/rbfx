import os
import subprocess


def cpp_demangle(name):
    return subprocess.check_output(['c++filt', name]).decode('utf-8').strip()


def split_identifier(identifier):
    """Splits string at _ or between lower case and uppercase letters."""
    prev_split = 0
    parts = []

    if '_' in identifier:
        parts = [s.lower() for s in identifier.split('_')]
    else:
        for i in range(len(identifier) - 1):
            if identifier[i + 1].isupper():
                parts.append(identifier[prev_split:i + 1].lower())
                prev_split = i + 1
        last = identifier[prev_split:]
        if last:
            parts.append(last.lower())
    return parts


def camel_case(identifier):
    identifier = identifier.strip('_')
    return_string = False
    if isinstance(identifier, str):
        if identifier.isupper() and '_' not in identifier:
            identifier = identifier.lower()
        name_parts = split_identifier(identifier)
        return_string = True
    elif isinstance(identifier, (list, tuple)):
        name_parts = identifier
    else:
        raise ValueError('identifier must be a list, tuple or string.')

    for i in range(len(name_parts)):
        name_parts[i] = name_parts[i][0].upper() + name_parts[i][1:]

    if return_string:
        return ''.join(name_parts)
    return name_parts


def get_subsystem_name(src_path):
    cwd = os.path.abspath('.')
    rel_path = os.path.relpath(src_path, cwd)
    subsystem = rel_path[:rel_path.index('/')]
    if subsystem == '..':
        subsystem = 'global'
    return subsystem


def has_base_class(node, base_class_name):
    for base in node.bases:
        if base.cls is None or isinstance(base.cls, str):
            continue
        if base.cls.infer_fqn() == base_class_name:
            return True
        elif has_base_class(base.cls, base_class_name):
            return True
    return False


def find_value(n):
    inner = n.get('inner', ())
    if len(inner) == 1:
        n = inner[0]
        if n['kind'] in ('FloatingLiteral', 'IntegerLiteral', 'ImplicitCastExpr', 'ConstantExpr'):
            value = n.get('value')
            if value:
                return value
            return find_value(n)
