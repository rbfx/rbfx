from enum import Enum


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

