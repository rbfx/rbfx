from clang_json.ast import AstPass
import util


class DefineRefCountedPass(AstPass):
    def on_enter(self, node):
        if node.kind == 'CXXRecordDecl':
            if util.has_base_class(node, 'Urho3D::RefCounted'):
                fqn = node.infer_fqn()
                if fqn:
                    self._writer.write_pre('refcounted', f'URHO3D_REFCOUNTED({fqn});')
            return AstPass.PASS_BREAK
