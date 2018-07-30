import argparse
import logging
import os
import re
import subprocess
import sys
from collections import OrderedDict

from clang.cindex import CursorKind, AccessSpecifier, TypeKind

from walkcpp.generator import Generator
from walkcpp.module import Module
from walkcpp.passes import AstPass, AstAction
from walkcpp.utils import get_fully_qualified_name, is_builtin_type, desugar_type


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


def rename_identifier(name, parent_name, is_private):
    name_parts = split_identifier(name)
    parent_parts = split_identifier(parent_name)
    # Remove name prefix if it consists of first letters of parent name words
    try:
        for i, c in enumerate(name_parts[0]):
            if c != parent_parts[i][0]:
                break
        else:
            del name_parts[0]
    except IndexError:
        pass
    name_parts = camel_case(name_parts)
    if is_private:
        name_parts[0] = name_parts[0][0].tolower() + name_parts[0][1:]
        name_parts[0] = '_' + name_parts[0]
    return ''.join(name_parts)


def find_identifier_parent_name(node):
    while node.kind not in (CursorKind.ENUM_DECL, CursorKind.CLASS_DECL, CursorKind.STRUCT_DECL, CursorKind.TRANSLATION_UNIT):
        node = node.parent
    name = node.spelling
    if node.kind == CursorKind.TRANSLATION_UNIT:
        name = os.path.basename(node.spelling)
        name = re.sub('\.h(pp|xx)?$', '', name)
    return name


class DefineConstantsPass(AstPass):
    def on_begin(self):
        self.fp = open(os.path.join(self.module.args.output, '_constants.i'), 'w+')

    def on_end(self):
        self.fp.close()

    def get_constant_value(self, node):
        # Supports only single line constants so far
        extent = node.extent
        with open(extent.start.file.name) as fp:
            fp.seek(node.extent.start.offset, os.SEEK_SET)
            ln = fp.read(node.extent.end_int_data - node.extent.start.offset)
            if '=' in ln:
                ln = re.sub(rf'.*{node.spelling}\s*=\s*(.+);[\n\t\s]*', r'\1', ln)
            else:
                ln = ln.rstrip('\s\n;')
            return ln

    @AstPass.once
    def visit(self, node, action: AstAction):
        if node.kind == CursorKind.VAR_DECL:
            fqn = get_fully_qualified_name(node)
            if fqn.startswith('Urho3D::IsFlagSet'):
                return False
            if fqn.startswith('Urho3D::FlagSet'):
                return False

            if node.type.spelling in ('const Urho3D::String', 'const char*'):
                node_expr = node.find_child(kind=CursorKind.UNEXPOSED_EXPR)
                idiomatic_name = camel_case(node.spelling)
                if node_expr:
                    value = self.get_constant_value(node_expr)
                    self.fp.write(f'CS_CONSTANT({fqn}, {idiomatic_name}, {value});\n')
                else:
                    self.fp.write(f'%rename({idiomatic_name}) {fqn};\n')
            elif is_builtin_type(node.type):
                literal = node.find_any_child(kind=CursorKind.INTEGER_LITERAL) or node.find_any_child(kind=CursorKind.FLOATING_LITERAL)
                idiomatic_name = camel_case(node.spelling)
                if literal:
                    value = self.get_constant_value(literal)
                    if node.type.get_canonical().kind in (TypeKind.CHAR_U,
                                                          TypeKind.UCHAR,
                                                          TypeKind.CHAR16,
                                                          TypeKind.CHAR32,
                                                          TypeKind.USHORT,
                                                          TypeKind.UINT,
                                                          TypeKind.ULONG,
                                                          TypeKind.ULONGLONG,
                                                          TypeKind.UINT128) and not value.endswith('U'):
                        value += 'U'
                    self.fp.write(f'CS_CONSTANT({fqn}, {idiomatic_name}, {value});\n')
                else:
                    self.fp.write(f'%rename({idiomatic_name}) {fqn};\n')

        elif node.kind in (CursorKind.CXX_METHOD, CursorKind.FUNCTION_DECL, CursorKind.FIELD_DECL):
            if node.kind in (CursorKind.CXX_METHOD, CursorKind.FUNCTION_DECL):
                if re.match(r'^operator[^\w]+.*$', node.spelling) is not None:
                    return False
            fqn = get_fully_qualified_name(node)
            if '::' in fqn:
                _, name = fqn.rsplit('::', 1)
            else:
                name = fqn
            idiomatic_name = camel_case(node.spelling)
            if name != idiomatic_name:
                self.fp.write(f'%rename({idiomatic_name}) {fqn};\n')

        return True


class DefineRefCountedPass(AstPass):
    parent_classes = {}

    def on_end(self):
        fp = open(os.path.join(self.module.args.output, '_refcounted.i'), 'w+')

        for cls_name in self.parent_classes.keys():
            if self.is_subclass_of(cls_name, 'Urho3D::RefCounted'):
                assert isinstance(cls_name, str)
                parent, name = cls_name.rsplit('::', 1)
                fp.write(f'URHO3D_REFCOUNTED({parent}, {name});\n')

        fp.close()

    def is_subclass_of(self, class_name, base_name):
        if class_name == base_name:
            return True
        subclasses = self.parent_classes[class_name]
        if base_name in subclasses:
            return True
        for subclass in subclasses:
            if subclass in self.parent_classes:
                if self.is_subclass_of(subclass, base_name):
                    return True
        return False

    @AstPass.once
    def visit(self, node, action: AstAction):
        if node.kind == CursorKind.CLASS_DECL or node.kind == CursorKind.STRUCT_DECL:
            for base in node.find_children(kind=CursorKind.CXX_BASE_SPECIFIER):
                base = base.type.get_declaration()
                super_fqn = get_fully_qualified_name(node)
                sub_fqn = get_fully_qualified_name(base)

                try:
                    bases_set = self.parent_classes[super_fqn]
                except KeyError:
                    bases_set = self.parent_classes[super_fqn] = set()
                # Create tree of base classes
                bases_set.add(sub_fqn)
        return True


class DefinePropertiesPass(AstPass):
    renames = {
        'Urho3D::Variant': {
            'GetType': 'GetVariantType'
        }
    }

    class Property:
        name = None
        access = 'public'
        getter = None
        setter = None
        getter_access = ''
        setter_access = ''
        type = ''

        def __init__(self, name):
            self.name = name

    def access_to_str(self, access):
        return 'public' if access == AccessSpecifier.PUBLIC else 'protected'

    def on_begin(self):
        self.fp = open(os.path.join(self.module.args.output, '_properties.i'), 'w+')

    def on_end(self):
        self.fp.close()

    def sort_getters_and_setters(self, methods):
        sorted_methods = OrderedDict()

        for m in methods:
            if m.is_virtual_method() or m.is_static_method() or m.access == AccessSpecifier.PRIVATE:
                continue

            basename = re.sub('^Is|Get|Set', '', m.spelling, flags=re.IGNORECASE)
            if not basename:
                continue

            if any([m.spelling == m2.spelling and m != m2 for m2 in methods]):
                continue

            try:
                prop = sorted_methods[basename]
            except KeyError:
                prop = self.Property(basename)

            num_children = len(list(m.find_children(kind=CursorKind.PARM_DECL)))
            if m.spelling.startswith('Set') and num_children == 1:
                argument_type = m.find_child(kind=CursorKind.PARM_DECL).type
                if prop.getter:
                    if prop.getter.type.get_result() != argument_type:
                        del sorted_methods[basename]
                        continue

                prop.setter = m
                prop.setter_access = self.access_to_str(m.access)
                prop.type = argument_type
                if prop.access == prop.setter_access:
                    prop.setter_access = ''

            elif (m.spelling.startswith('Get') or m.spelling.startswith('Is')) and num_children == 0:
                if prop.setter:
                    if prop.setter.find_child(kind=CursorKind.PARM_DECL).type != m.type.get_result():
                        del sorted_methods[basename]
                        continue

                prop.getter = m
                prop.getter_access = self.access_to_str(m.access)
                prop.type = m.type.get_result()
                if prop.access == prop.getter_access:
                    prop.getter_access = ''

            if prop.getter or prop.setter:
                sorted_methods[basename] = prop

        return sorted_methods

    def insert_rename(self, cls_fqn, name):
        try:
            return self.renames[cls_fqn][name]
        except KeyError:
            return name

    @AstPass.once
    def visit(self, node, action: AstAction):
        if node.kind == CursorKind.CLASS_DECL or node.kind == CursorKind.STRUCT_DECL:
            methods = list(node.find_children(kind=CursorKind.CXX_METHOD))
            methods = list(filter(lambda m: m.spelling.startswith('Get') or
                                            m.spelling.startswith('Set') or
                                            m.spelling.startswith('Is'), methods))
            if not len(methods):
                return False

            properties = self.sort_getters_and_setters(methods)
            if not len(properties):
                return False

            cls_fqn = get_fully_qualified_name(node)
            for basename, prop in properties.items():
                if prop.getter:
                    self.fp.write(f'%csmethodmodifiers {get_fully_qualified_name(prop.getter)} "private";\n')
                if prop.setter:
                    self.fp.write(f'%csmethodmodifiers {get_fully_qualified_name(prop.setter)} "private";\n')

            for basename, prop in properties.items():
                if prop.setter:
                    enclosing_method = get_fully_qualified_name(prop.setter)
                    enclosing_method_return_type = 'void'
                elif prop.getter:
                    enclosing_method = get_fully_qualified_name(prop.getter)
                    enclosing_method_return_type = prop.type.spelling
                else:
                    raise Exception()

                self.fp.write(f'%typemap(csout) {enclosing_method_return_type} {enclosing_method} %{{\n')
                self.fp.write(f'  $typemap(csout, {enclosing_method_return_type})\n')

                self.fp.write(f'  {prop.access} $typemap(cstype, {prop.type.spelling}) {prop.name} {{\n')
                if prop.getter:
                    self.fp.write(f'    {prop.getter_access} get {{ return '
                                  f'{self.insert_rename(cls_fqn, prop.getter.spelling)}(); }}\n')
                elif prop.setter:
                    self.fp.write(f'    {prop.setter_access} set {{ '
                                  f'{self.insert_rename(cls_fqn, prop.setter.spelling)}(value); }}\n')
                self.fp.write('  }\n')
                self.fp.write('%}\n')
            return False
        return True


class Urho3DModule(Module):
    exclude_headers = [
        r'/Urho3D/Precompiled.h',
        r'/Urho3D/Container/.+$',
        r'/Urho3D/Urho2D/.+$',
        r'/Urho3D/Math/MathDefs\.h$',
        r'/Urho3D/Graphics/[^/]+/.+\.h$',
    ]

    def __init__(self, args):
        super().__init__(args)
        self.name = 'Urho3D'
        self.compiler_parameters += ['-std=c++11']
        self.compiler_parameters += \
            filter(lambda s: len(s), subprocess.check_output(['llvm-config', '--cppflags']).decode().strip().split(' '))
        self.include_directories += [
            '/usr/lib/clang/6.0.1/include',
            os.path.dirname(self.args.input),
            os.path.join(os.path.dirname(self.args.input), 'ThirdParty')
        ]
        self.exclude_headers = [re.compile(pattern, re.IGNORECASE) for pattern in self.exclude_headers]

    def register_passes(self, passes: list):
        passes += [DefinePropertiesPass, DefineRefCountedPass, DefineConstantsPass]

    def gather_files(self):
        for root, dirs, files in os.walk(self.args.input):
            for file in files:
                if not file.endswith('.h'):
                    continue
                file_path = os.path.join(root, file)
                if not any([ex.search(file_path) is not None for ex in self.exclude_headers]):
                    yield file_path


def main():
    logging.basicConfig(level=logging.DEBUG)

    bind = argparse.ArgumentParser()
    bind.add_argument('-I', action='append', dest='includes', default=[])
    bind.add_argument('-D', action='append', dest='defines', default=[])
    bind.add_argument('-O', action='append', dest='parameters', default=[])
    bind.add_argument('input')
    bind.add_argument('output')
    if len(sys.argv) == 2 and os.path.isfile(sys.argv[1]):
        # Load options from a file
        program_args = open(sys.argv[1]).read().split('\n')
        while '' in program_args:
            program_args.remove('')
    else:
        program_args = sys.argv[1:]
    args = bind.parse_args(program_args)

    generator = Generator()
    module = Urho3DModule(args)
    generator.process(module, args)


if __name__ == '__main__':
    main()
