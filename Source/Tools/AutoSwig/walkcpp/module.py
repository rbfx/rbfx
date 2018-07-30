
class Module(object):
    def __init__(self, args):
        self.name = None
        self.compiler_parameters = ['-x', 'c++-header', '-E', '-I.']
        self.include_directories = []
        self.compile_definitions = []
        self.symbol_registry = {}
        self.args = args

    def gather_files(self):
        """Generates a list of files to parse."""
        raise NotImplementedError()

    def register_passes(self, passes):
        """Add passes that should run on C++ AST."""
        pass
