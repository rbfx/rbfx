from clang_json.ast import AstPass


# TODO: ...
class GatherIfDefRanges(AstPass):
    def __init__(self, *args):
        super().__init__(*args)
        self._ifdef_ranges = {}

    def on_enter(self, node):
        if not node.source_path:
            return

        if node.source_path in self._ifdef_ranges:
            return

        range_start_stack = []
        with open(node.source_path) as fp:
            for line_no, ln in enumerate(fp):
                if ln.startswith('#if'):
                    range_start_stack.append((line_no, ln))
                elif ln.startswith('#endif'):
                    range_start, expr = range_start_stack.pop()
                    range_end = line_no
                    if 'URHO3D_' in expr:
                        try:
                            file_ranges = self._ifdef_ranges[node.source_path]
                        except KeyError:
                            file_ranges = self._ifdef_ranges[node.source_path] = []
                        file_ranges.append((expr, range_start, range_end))
