import util
from clang_json.ast import AstPass


class DefineEnumValuesPass(AstPass):
    def on_enter(self, node):
        if node.kind == 'EnumDecl':
            name = node.name

            # Dear ImGui flag enums
            if name and name.endswith('Flags_'):
                fqn = node.infer_fqn()
                subsystem_name = util.get_subsystem_name(node.source_path)
                self._writer.write_pre(subsystem_name, f'%rename({name.rstrip("_")}) {name};')
                self._writer.write_pre(subsystem_name, f'%typemap(csattributes) {fqn} "[global::System.Flags]";')

        elif node.kind == 'TypeAliasDecl' and node.type.startswith('Urho3D::FlagSet<'):
            subsystem_name = util.get_subsystem_name(node.source_path)
            name = node.name
            fqn = node._json_node['inner'][0]['inner'][0]['inner'][0]['type']['qualType']
            self._writer.write_pre(subsystem_name, f'%typemap(csattributes) {fqn} "[global::System.Flags]";')
            # Trick swig into treating FlagSet<> as a plain enum.
            self._writer.write_pre(subsystem_name, f'using {name} = {fqn};')
            self._writer.write_pre(subsystem_name, f'%typemap(ctype) {name} "size_t";')
            self._writer.write_pre(subsystem_name, f'%typemap(out) {name} "$result = (size_t)$1.AsInteger();"')

        elif node.kind == 'EnumConstantDecl':
            if not node.is_primitive_value:
                return
            subsystem_name = util.get_subsystem_name(node.source_path)

            # Short fqn must be used because SWIG does not support long (one that includes enum type) version.
            fqn = node.infer_fqn(short=True)
            if not fqn:
                return

            if node.value:
                self._writer.write_pre(subsystem_name, f'%{self._lang}constvalue("{node.value}") {fqn};')

            name = node.name
            qual_type = node.type
            if qual_type and name.startswith(qual_type):
                new_name = name[len(qual_type):].strip('_')
                # Get rid of type name repeating in enum constant name
                self._writer.write_pre(subsystem_name, f'%rename({new_name}) {fqn};')
