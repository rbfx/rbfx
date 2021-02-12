from clang_json.ast import AstPass
import util


class DefineRefCountedPass(AstPass):
    def on_enter(self, node):
        if node.kind == 'CXXRecordDecl':
            if util.has_base_class(node, 'Urho3D::RefCounted'):
                fqn = node.infer_fqn()
                if fqn:
                    subsystem_name = util.get_subsystem_name(node.source_path)
                    self._writer.write_pre(subsystem_name, f'URHO3D_REFCOUNTED({fqn});')
            return AstPass.PASS_BREAK
