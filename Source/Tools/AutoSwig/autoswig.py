import argparse
import logging
import os
import re
import subprocess
import sys
from collections import OrderedDict
from contextlib import suppress

from clang.cindex import Config, CursorKind, AccessSpecifier, TypeKind

from walkcpp.generator import Generator
from walkcpp.module import Module
from walkcpp.passes import AstPass, AstAction
from walkcpp.utils import get_fully_qualified_name, is_builtin_type, desugar_type


builtin_to_cs = {
    TypeKind.VOID: 'void',
    TypeKind.BOOL: 'bool',
    TypeKind.CHAR_U: 'byte',
    TypeKind.UCHAR: 'byte',
    # TypeKind.CHAR16,
    # TypeKind.CHAR32,
    TypeKind.USHORT: 'ushort',
    TypeKind.UINT: 'uint',
    TypeKind.ULONG: 'uint',
    TypeKind.ULONGLONG: 'ulong',
    # TypeKind.UINT128,
    TypeKind.CHAR_S: 'char',
    TypeKind.SCHAR: 'char',
    # TypeKind.WCHAR,
    TypeKind.SHORT: 'short',
    TypeKind.INT: 'int',
    TypeKind.LONG: 'int',
    TypeKind.LONGLONG: 'long',
    # TypeKind.INT128,
    TypeKind.FLOAT: 'float',
    TypeKind.DOUBLE: 'double',
    # TypeKind.LONGDOUBLE,
    TypeKind.NULLPTR: 'null'
    # TypeKind.FLOAT128,
    # TypeKind.HALF,
}


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


def read_raw_code(node):
    extent = node.extent
    with open(extent.start.file.name) as fp:
        fp.seek(node.extent.start.offset, os.SEEK_SET)
        return fp.read(node.extent.end_int_data - node.extent.start.offset)


class DefineConstantsPass(AstPass):
    outputs = ['_constants.i']

    def on_begin(self):
        self.fp = open(os.path.join(self.module.args.output, '_constants.i'), 'w+')

    def on_end(self):
        self.fp.close()

    def get_constant_value(self, node):
        # Supports only single line constants so far
        ln = read_raw_code(node)
        if '=' in ln:
            ln = re.sub(rf'.*{node.spelling}\s*=\s*(.+);[\n\t\s]*', r'\1', ln)
        else:
            ln = ln.rstrip('\s\n;')
        return ln

    @AstPass.once
    def visit(self, node, action: AstAction):
        if node.c.spelling.endswith('/MathDefs.h'):
            return False
        if node.kind == CursorKind.VAR_DECL:
            fqn = get_fully_qualified_name(node)
            if fqn.startswith('Urho3D::IsFlagSet') or fqn.startswith('Urho3D::FlagSet'):
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

        elif node.kind in (CursorKind.CXX_METHOD, CursorKind.FUNCTION_DECL, CursorKind.FIELD_DECL, CursorKind.ENUM_CONSTANT_DECL):
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
                if node.kind == CursorKind.ENUM_CONSTANT_DECL:
                    fqn = name
                self.fp.write(f'%rename({idiomatic_name}) {fqn};\n')

        return True


class DefineRefCountedPass(AstPass):
    outputs = ['_refcounted.i']
    parent_classes = {}

    def on_end(self):
        fp = open(os.path.join(self.module.args.output, '_refcounted.i'), 'w+')

        for cls_name in self.parent_classes.keys():
            if self.is_subclass_of(cls_name, 'Urho3D::RefCounted'):
                assert isinstance(cls_name, str)
                fp.write(f'URHO3D_REFCOUNTED({cls_name});\n')

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
    outputs = ['_properties.i']
    renames = {
        'Urho3D::Variant': {
            'GetType': 'GetVariantType'
        }
    }
    interfaces = [
        'Urho3D::Octant',
        'Urho3D::GPUObject',
    ]

    class Property:
        name = None
        access = 'public'
        getter = None
        setter = None
        getter_access = 'private'
        setter_access = 'private'
        type = ''

        def __init__(self, name):
            self.name = name

    def access_to_str(self, m):
        return 'public' if m.access == AccessSpecifier.PUBLIC else 'protected'

    def on_begin(self):
        self.fp = open(os.path.join(self.module.args.output, '_properties.i'), 'w+')
        self.fp.write('%include "attribute.i"\n')

    def on_end(self):
        self.fp.close()

    def sort_getters_and_setters(self, methods):
        sorted_methods = OrderedDict()

        for m in methods:
            if m.is_virtual_method() or m.is_pure_virtual_method() or m.is_static_method() or m.access == AccessSpecifier.PRIVATE or m.parent.is_abstract_record() or get_fully_qualified_name(m.parent) in self.interfaces:
                continue

            basename = re.sub('^Get|Set', '', m.spelling, flags=re.IGNORECASE)
            if not basename:
                continue

            if any([m.spelling == m2.spelling and m != m2 for m2 in methods]):
                continue

            if not basename.startswith('Is') and len(list(m.parent.find_children(spelling=basename))):
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
                prop.setter_access = self.access_to_str(m)
                prop.type = argument_type

            elif (m.spelling.startswith('Get') or m.spelling.startswith('Is')) and num_children == 0:
                if prop.setter:
                    if prop.setter.find_child(kind=CursorKind.PARM_DECL).type != m.type.get_result():
                        del sorted_methods[basename]
                        continue

                prop.getter = m
                prop.getter_access = self.access_to_str(m)
                prop.type = m.type.get_result()

            if prop.getter or prop.setter:
                sorted_methods[basename] = prop

        for k, prop in list(sorted_methods.items()):
            if prop.getter is None or prop.getter_access == 'private':
                # No properties without getter
                del sorted_methods[k]
            else:
                if prop.getter_access == prop.access:
                    prop.getter_access = ''
                if prop.setter:
                    if prop.setter_access == prop.access:
                        prop.setter_access = ''
                else:
                    prop.access = prop.getter_access
                    prop.getter_access = ''

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
                return True

            properties = self.sort_getters_and_setters(methods)
            if not len(properties):
                return True

            for basename, prop in properties.items():
                if not prop.getter:
                    continue
                # base_type = desugar_type(prop.type)
                # if base_type.kind in builtin_to_cs:
                #     base_type = builtin_to_cs[base_type.kind]
                # else:
                #     if 'FlagSet' in base_type.spelling:
                #         base_type = base_type.spelling[16:-7]
                #     else:
                #         base_type = prop.type.spelling
                #     base_type = base_type.replace('Urho3D::', 'Urho3DNet.')

                if prop.access == prop.getter.access:
                    prop.getter.access = ''

                self.fp.write(f"""
                %csmethodmodifiers {get_fully_qualified_name(prop.getter)} "
                {prop.access} $typemap(cstype, {prop.type.spelling}) {prop.name} {{
                    {prop.getter_access} get {{ return __{prop.getter.spelling}(); }}
                """)
                if prop.setter:
                    if prop.access == prop.setter.access:
                        prop.setter.access = ''
                    self.fp.write(f"""
                    {prop.setter_access} set {{ __{prop.setter.spelling}(value); }}
                    """)
                self.fp.write("""
                }
                private"
                """)
                self.fp.write(f'%rename(__{prop.getter.spelling}) {get_fully_qualified_name(prop.getter)};\n')
                if prop.setter:
                    self.fp.write(f'%rename(__{prop.setter.spelling}) {get_fully_qualified_name(prop.setter)};\n')
                    self.fp.write(f'%csmethodmodifiers {get_fully_qualified_name(prop.setter)} "private"\n')

        return True


class FindFlagEnums(AstPass):
    flag_enums = []

    def visit(self, node, action: AstAction):
        if node.kind == CursorKind.STRUCT_DECL:
            if node.spelling == 'IsFlagSet':
                enum = node.children[0].type.spelling
                self.flag_enums.append(enum)

        return True


class CleanEnumValues(AstPass):
    outputs = ['_enums.i']

    def on_begin(self):
        self.fp = open(os.path.join(self.module.args.output, '_enums.i'), 'w+')

    def on_end(self):
        self.fp.close()

    @AstPass.once
    def visit(self, node, action: AstAction):
        if node.kind == CursorKind.ENUM_DECL:
            if node.type.spelling in FindFlagEnums.flag_enums:
                self.fp.write(f'%csattributes {node.type.spelling} "[global::System.Flags]";\n')

            if node.type.spelling.startswith('Urho3D::'):
                for child in node.children:
                    code = read_raw_code(child)
                    if ' = SDL' in code:
                        # Enum uses symbols from SDL. Redefine it with raw enum values.
                        self.fp.write(f'%csconstvalue("{child.enum_value}") {child.spelling};\n')

        elif node.kind == CursorKind.TYPE_ALIAS_DECL:
            underlying_type = node.type.get_canonical()
            if underlying_type.spelling.startswith('Urho3D::FlagSet<'):
                enum_name = node.children[1].type.spelling
                target_name = node.spelling.strip("'")
                self.fp.write(f'using {target_name} = {enum_name};\n')
                self.fp.write(f'%typemap(ctype) {target_name} "size_t";\n')
                self.fp.write(f'%typemap(out) {target_name} "$result = (size_t)$1.AsInteger();"\n')

        return True


class DefineEventsPass(AstPass):
    outputs = ['_events.i']
    re_param_name = re.compile(r'URHO3D_PARAM\(([^,]+),\s*([a-z0-9_]+)\);\s*', re.IGNORECASE)

    def on_begin(self):
        self.fp = open(os.path.join(self.module.args.output, '_events.i'), 'w+')
        self.fp.write("""%pragma(csharp) moduleimports=%{
            public static class E
            {
        """)

    def on_end(self):
        self.fp.write("} %}")
        self.fp.close()

    def visit(self, node, action: AstAction):
        if node.kind == CursorKind.VAR_DECL and node.type.spelling == 'const Urho3D::StringHash' and node.spelling.startswith('E_'):
            siblings: list = node.parent.children
            try:
                next_node = siblings[siblings.index(node) + 1]
            except IndexError:
                return False

            if next_node.kind == CursorKind.NAMESPACE:
                if next_node.spelling.lower() == node.spelling[2:].replace('_', '').lower():
                    self.fp.write(f"""
                    public class {next_node.spelling}Event {{
                        private StringHash _event = new StringHash("{next_node.spelling}");
                    """)

                    for param in next_node.children:
                        param_name = read_raw_code(param)
                        param_name = self.re_param_name.match(param_name).group(2)
                        var_name = camel_case(param_name)
                        self.fp.write(f"""
                        public StringHash {var_name} = new StringHash("{param_name}");""")

                    self.fp.write(f"""
                        public {next_node.spelling}Event() {{ }}
                        public static implicit operator StringHash({next_node.spelling}Event e) {{ return e._event; }}
                    }}
                    public static {next_node.spelling}Event {next_node.spelling} = new {next_node.spelling}Event();
                    """)
        return True


def find_program(name, paths=None):
    if paths is None:
        paths = []
    if isinstance(name, str):
        name = [name]
    paths = paths + os.environ['PATH'].split(os.pathsep)
    for path in paths:
        for n in name:
            full_path = os.path.join(path, n)
            if os.path.isfile(full_path):
                return full_path


class Urho3DModule(Module):
    exclude_headers = [
        r'/Urho3D/Precompiled.h',
        r'/Urho3D/Container/.+$',
        r'/Urho3D/Urho2D/.+$',
        r'/Urho3D/Graphics/[^/]+/.+\.h$',
    ]

    def __init__(self, args):
        super().__init__(args)
        self.name = 'Urho3D'
        self.compiler_parameters += ['-std=c++11']
        llvm_config = find_program('llvm-config', ['/usr/local/opt/llvm/bin'])
        self.compiler_parameters += \
            filter(lambda s: len(s), subprocess.check_output([llvm_config, '--cppflags']).decode().strip().split(' '))
        if sys.platform == 'linux':
            version = subprocess.check_output([llvm_config, '--version']).decode()
            self.include_directories += [f'/usr/lib/clang/{version}/include']

        self.include_directories += [
            os.path.dirname(self.args.input),
            os.path.join(os.path.dirname(self.args.input), 'ThirdParty')
        ]
        self.exclude_headers = [re.compile(pattern, re.IGNORECASE) for pattern in self.exclude_headers]

    def register_passes(self, passes: list):
        passes += [DefineEventsPass, FindFlagEnums, CleanEnumValues, DefineRefCountedPass, DefineConstantsPass]

    def gather_files(self):
        yield os.path.join(self.args.input, '../ThirdParty/SDL/include/SDL/SDL_joystick.h')
        yield os.path.join(self.args.input, '../ThirdParty/SDL/include/SDL/SDL_gamecontroller.h')
        yield os.path.join(self.args.input, '../ThirdParty/SDL/include/SDL/SDL_keycode.h')
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

    with suppress(KeyError):
        Config.library_file = os.environ['URHO3D_LIBCLANG_PATH']

    generator = Generator()
    module = Urho3DModule(args)
    generator.process(module, args)


if __name__ == '__main__':
    main()
