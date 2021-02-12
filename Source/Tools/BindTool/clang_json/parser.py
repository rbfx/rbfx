import json
import logging
import os
import subprocess
import sys

from .ast import AstNode, AstPass, NAME_QUALIFYING_NODES


class MapObjectsPass(AstPass):
    """Create id:node and fqn:node maps for convenient access."""
    def on_enter(self, node):
        if node.kind in NAME_QUALIFYING_NODES:
            fqn = node.infer_fqn()
            self._parser.NODE_BY_FQN[fqn] = node
        self._parser.NODE_BY_ID[node.id] = node


class ResolveNodeReferences(AstPass):
    """Replace reference names (fqn, etc) with node references."""
    def on_enter(self, node):
        if node.kind == 'CXXRecordDecl':
            for base in node.bases:
                base.cls = self._parser.NODE_BY_FQN.get(base.cls, base.cls)


class ClangJsonAstParser(object):
    def __init__(self, source_file, compiler_parameters, clang=None):
        self._source_code = b''
        self._source_file = source_file
        self._compiler_parameters = compiler_parameters
        self._json_ast = None
        self._clang_exe = clang or ClangJsonAstParser._find_clang()
        self._ast = None
        self.vars = {}
        self.NODE_BY_ID = {}
        self.NODE_BY_FQN = {}

    def preprocess(self):
        args = ['/usr/bin/clang', '-E', '-C', '-CC']
        args.extend(self._compiler_parameters)
        args.append(self._source_file)
        self._source_code = subprocess.check_output(args)

    def parse_json_ast(self):
        assert self._clang_exe is not None
        args = [self._clang_exe, '-x', 'c++', '-Xclang', '-ast-dump=json', '-fsyntax-only']
        args.extend(self._compiler_parameters)
        args.append('-')
        ps = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
        ps.stdin.write(self._source_code)
        ps.stdin.close()
        try:
            self._json_ast = json.load(ps.stdout)
        finally:
            for ln in ps.stderr.read().decode().split('\n'):
                logging.error(ln)
        ps.wait()
        if ps.returncode != 0:
            sys.exit(ps.returncode)

    def create_ast(self):
        # Create AST.
        self._ast = AstNode.create_from_json_node(self, None, self._json_ast)
        # Create FQN and id maps.
        MapObjectsPass(self, None, None).walk_ast(self._ast)
        ResolveNodeReferences(self, None, None).walk_ast(self._ast)
        return self._ast

    def save_preprocessed_source_code(self, path):
        with open(path, 'w+b') as fp:
            fp.write(self._source_code)

    def save_ast(self, path):
        with open(path, 'w+') as fp:
            json.dump(self._json_ast, fp, indent=2)

    @staticmethod
    def _find_clang():
        suffix = '.exe' if os.name == 'nt' else ''
        for path in os.environ['PATH'].split(os.pathsep):
            exe_path = os.path.join(path, f'clang{suffix}')
            if os.path.isfile(exe_path):
                return exe_path

    def read_from_source(self, n):
        if 'offset' in n and 'tokLen' in n:
            return self._source_code[n['offset']:n['offset'] + n['tokLen']].decode('utf-8')
        elif 'begin' in n and 'end' in n:
            return self._source_code[n['begin']['offset']:n['end']['offset'] + n['end']['tokLen']].decode('utf-8')

    def get_source_code_path_abs(self, n):
        # clang has a bug where json ast reports wrong file containing node. Just find this info ourselves.
        offset = n.get('loc', {}).get('offset', -1)
        if offset < 0:
            return None
        a = self._source_code.rfind(b'\n# ', 0, offset)
        b = self._source_code.find(b'\n', a + 1, offset)
        marker = self._source_code[a:b]
        marker = marker.strip(b'#0123456789" \r\n')
        return os.path.abspath(marker.decode())
