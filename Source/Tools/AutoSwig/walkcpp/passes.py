import logging
from enum import Enum

from clang.cindex import CursorKind, TypeKind

from .utils import get_fully_qualified_name, is_builtin_type, desugar_type


class AstAction(Enum):
    ENTER = 1
    LEAVE = 2


class AstPass(object):
    def __init__(self, generator, module):
        self.generator = generator
        self.module = module

    def on_begin(self):
        """Called once per module on very start of ast processing."""
        pass

    def on_end(self):
        """Called once per module on very end of ast processing."""
        pass

    def on_file_begin(self):
        """Called on start of file ast processing."""
        pass

    def on_file_end(self):
        """Called on end of file ast processing."""
        pass

    def visit(self, node, action: AstAction):
        """
        Called for every ast node. For some nodes this method may be called twice, on start and on end. Time of call is
        indicated by `action` parameter. Return `True` if pass should descend to children of this node, return `False`
        to skip processing of children.
        """
        raise NotImplementedError()

    @staticmethod
    def once(fn):
        def wrapper(self, node, action):
            if action != AstAction.ENTER:
                return
            return fn(self, node, action)
        return wrapper


class GatherSymbolsPass(AstPass):
    """Walk AST and gather known types."""
    type_kinds = [
        CursorKind.CLASS_DECL,
        CursorKind.STRUCT_DECL,
        CursorKind.ENUM_DECL,
        CursorKind.CLASS_TEMPLATE,
    ]

    def visit(self, node, action):
        if action != AstAction.ENTER:
            return True

        if node.kind in self.type_kinds and node.is_definition() and node.fully_qualified_name:
            if node.fully_qualified_name in self.generator.types:
                raise Exception(f'{node.fully_qualified_name} is already known')
            self.generator.types[node.fully_qualified_name] = node

        return True


class UnknownSymbolsPass(AstPass):
    """Walk AST and remove any nodes that use unsupported types. Gather list of unknown types."""

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.unknown_types = []

    def is_type_acceptable(self, t):
        t = desugar_type(t)
        if is_builtin_type(t):
            return True

        if t.kind == TypeKind.UNEXPOSED:
            # Unsupported
            return False

        if t.kind in (TypeKind.FUNCTIONPROTO, TypeKind.CONSTANTARRAY, TypeKind.INCOMPLETEARRAY, TypeKind.UNEXPOSED):
            # TODO: Unsupported for now
            return False

        type_decl = t.get_declaration()
        symbol_name = get_fully_qualified_name(type_decl)
        assert symbol_name
        if symbol_name in self.generator.types:
            return True

        if type_decl not in self.unknown_types:
            self.unknown_types.append(type_decl)
            logging.warning(f'Ignored unknown type {symbol_name}')

        # Unknown types will be wrapped as plain pointers
        return True

    def visit(self, node, action):
        if action != AstAction.ENTER:
            return True

        if node.kind == CursorKind.FIELD_DECL or node.kind == CursorKind.VAR_DECL:
            if not self.is_type_acceptable(node.type):
                node.remove()
                return False
        if node.kind == CursorKind.PARM_DECL:
            if not self.is_type_acceptable(node.type):
                node.parent.remove()
                return False
        if node.kind == CursorKind.FUNCTION_DECL or node.kind == CursorKind.CXX_METHOD:
            if not self.is_type_acceptable(node.type.get_result()):
                node.remove()
                return False

        return True
