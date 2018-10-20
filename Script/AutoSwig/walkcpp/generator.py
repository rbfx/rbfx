import itertools
import os
import sys

from clang.cindex import CursorKind, Index, conf, AccessSpecifier, TranslationUnit

from .passes import AstAction
from .utils import get_fully_qualified_name


_generator = None


class Node(object):
    def __init__(self, c, parent, access):
        self.c = c
        self.parent = parent
        self.children = []
        self.access = access
        self.fully_qualified_name = get_fully_qualified_name(c)
        if parent:
            self.contextual_unique_name = f'{parent.fully_qualified_name}::{c.displayname}'
        else:
            self.contextual_unique_name = ''

    def __repr__(self):
        kind = str(self.c.kind).split(".")[1]
        return f'{self.c.spelling or self.c.displayname} {kind}'

    def __getattr__(self, item):
        return getattr(self.c, item)

    def remove(self):
        if self.parent:
            self.parent.children.remove(self)
            self.parent = None

            fqn = self.fully_qualified_name
            if fqn:
                if fqn in _generator.types:
                    del _generator.types[fqn]

    def find_children(self, **kwargs):
        for c in self.children:
            if all([getattr(c, k) == v for k, v in kwargs.items()]):
                yield c

    def find_child(self, **kwargs):
        for child in self.find_children(**kwargs):
            return child

    def find_any_child(self, **kwargs):
        for c in self.children:
            if all([getattr(c, k) == v for k, v in kwargs.items()]):
                return c
            res = c.find_any_child(**kwargs)
            if res:
                return res


class Generator(object):
    two_stage_visit = [
        CursorKind.CLASS_DECL,
        CursorKind.STRUCT_DECL,
        CursorKind.ENUM_DECL,
        CursorKind.FUNCTION_DECL,
        CursorKind.CXX_METHOD,
        CursorKind.NAMESPACE,
        CursorKind.LINKAGE_SPEC,
        CursorKind.CONSTRUCTOR,
        CursorKind.DESTRUCTOR,
        CursorKind.CONVERSION_FUNCTION,
        CursorKind.FUNCTION_TEMPLATE,
        CursorKind.CLASS_TEMPLATE,
    ]

    def __init__(self):
        global _generator
        self.types = {}
        _generator = self

    def process(self, module: 'Module', args):
        compiler_parameters = []
        for param in itertools.chain(module.compiler_parameters, args.parameters):
            compiler_parameters.append(param)

        for include in itertools.chain(module.include_directories, args.includes):
            compiler_parameters.append(f'-I{include}')

        for define in itertools.chain(module.compile_definitions, args.defines):
            compiler_parameters.append(f'-D{define}')

        pass_factories = []
        module.register_passes(pass_factories)

        if not len(pass_factories):
            return

        # Instantiate passes
        passes = [p(self, module) for p in pass_factories]

        def walk_ast(p, node):
            if not p.visit(node, AstAction.ENTER):
                return

            node_children = node.children[:]    # node.children may be modified during iteration
            for child in node_children:
                walk_ast(p, child)

            if node.c.kind in self.two_stage_visit:
                p.visit(node, AstAction.LEAVE)

        trees = {}
        files = sorted(module.gather_files())

        for p in passes:
            p.on_begin()
            for file_path in files:
                root_node = trees[file_path] = self._build_tree(file_path, compiler_parameters)
                p.on_file_begin()
                walk_ast(p, root_node)
                p.on_file_end()
            p.on_end()

    @staticmethod
    def _build_tree(file_path, compiler_parameters):
        file_path = os.path.abspath(file_path)

        def build_tree_recursive(cursor, parent, access):
            node = Node(cursor, parent, access)

            for child in cursor.get_children():
                if child.kind == CursorKind.COMPOUND_STMT:
                    # Ignore nodes defining code logic
                    continue

                if not child.location.file or os.path.abspath(child.location.file.name) != file_path:
                    # Ignore nodes from other files
                    continue

                child_access = child.access_specifier
                if child_access == AccessSpecifier.INVALID:
                    child_access = access
                child_node = build_tree_recursive(child, node, child_access)
                node.children.append(child_node)
                assert child_node.parent == node
            return node

        index = Index(conf.lib.clang_createIndex(False, True))
        translation_unit = index.parse(file_path, compiler_parameters,
                                       options=TranslationUnit.PARSE_INCOMPLETE |
                                               TranslationUnit.PARSE_SKIP_FUNCTION_BODIES |
                                               TranslationUnit.PARSE_INCLUDE_BRIEF_COMMENTS_IN_CODE_COMPLETION)
        return build_tree_recursive(translation_unit.cursor, None, AccessSpecifier.NONE)
