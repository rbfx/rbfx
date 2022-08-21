import util
from clang_json.ast import AstPass


class DefineEventsPass(AstPass):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self._seen_subsystems = []

    def on_finish(self):
        for subsystem_name in self._seen_subsystems:
            self._writer.write_pre(subsystem_name, '} %}')

    def on_enter(self, node):
        if node.kind == 'VarDecl' and node.name.startswith('E_') and node.type == 'const Urho3D::StringHash':
            event_namespace = node.parent.children[node.parent.children.index(node) + 1]
            subsystem_name = util.get_subsystem_name(node.source_path)

            if subsystem_name not in self._seen_subsystems:
                self._writer.write_pre(
                    subsystem_name,
                    '%pragma(csharp) moduleimports=%{\n' +
                    'public static partial class E\n' +
                    '{'
                )
                self._seen_subsystems.append(subsystem_name)

            self._writer.write_pre(
                subsystem_name,
                f'    public class {event_namespace.name}Event {{\n' +
                f'        private StringHash _event = new StringHash("{event_namespace.name}");')

            for param in event_namespace.children:
                hash_string = self._parser.read_from_source(param._json_node['inner'][0]['inner'][0]['inner'][1]['range'])
                self._writer.write_pre(subsystem_name, f'        public StringHash {hash_string[1:-1]} = new StringHash({hash_string});')

            self._writer.write_pre(
                subsystem_name,
                f'        public {event_namespace.name}Event() {{ }}\n' +
                f'        public static implicit operator StringHash({event_namespace.name}Event e) {{ return e._event; }}\n' +
                '    }\n' +
                f'    public static {event_namespace.name}Event {event_namespace.name} = new {event_namespace.name}Event();'
            )
