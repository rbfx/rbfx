import logging

import util
from clang_json.ast import AstPass


class DefineConstantsPass(AstPass):
    def on_enter(self, node):
        if node.kind != 'VarDecl':
            return

        if node.parent.kind != 'NamespaceDecl':
            return

        qual_type = node.raw_attr('type/qualType')
        if qual_type.startswith('const '):
            qual_type = qual_type[6:]

        # Events are mapped by another pass.
        if qual_type == 'Urho3D::StringHash' and (node.name.startswith('E_') or node.name.startswith('P_')):
            return

        # Arrays are not supported.
        if '[' in qual_type:
            return

        subsystem_name = util.get_subsystem_name(node.source_path)
        fqn = node.infer_fqn()

        if node.is_primitive_value:
            raw_value = node.value
            if self._lang == 'cs' and qual_type == 'float':
                # A workaround for SWIG bug https://github.com/swig/swig/issues/1917
                raw_value = f'(float){raw_value}'

            # TODO: Resolve constants of other constants, eg. M_MAX_UNSIGNED.
            if raw_value == 'M_MAX_UNSIGNED':
                raw_value = '0xFFFFFFFF'

            self._writer.write_pre(subsystem_name, f'%ignore {fqn};')
            self._writer.write_pre(subsystem_name, f'%{self._lang}const(1) {fqn};')
            self._writer.write_pre(subsystem_name, f'%constant {qual_type} {util.camel_case(node.name)} = {raw_value};')
        else:
            logging.debug(f'Constant {node.name} has no value')
            self._writer.write_pre(subsystem_name, f'%constant {qual_type} {util.camel_case(node.name)} = {fqn};')
            self._writer.write_pre(subsystem_name, f'%ignore {fqn};')
