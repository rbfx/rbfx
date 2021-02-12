from clang_json.ast import AstPass


class TrimNodesPass(AstPass):
    KEEP_NAMESPACES = ['Urho3D', 'ImGui']
    KEEP_PREFIXES = ['SDL_', 'SDLK_', 'Im']

    def __init__(self, *args):
        super().__init__(*args)

    def on_enter(self, node):
        if node.parent is None:
            assert node.kind == 'TranslationUnitDecl'
            return

        if node.parent.kind != 'TranslationUnitDecl':
            # Only handle top level nodes
            return AstPass.PASS_BREAK

        if node.kind == 'NamespaceDecl':
            if node.name not in TrimNodesPass.KEEP_NAMESPACES:
                return AstPass.PASS_REMOVE
        else:
            name = getattr(node, 'name')
            if name and not any([name.startswith(prefix) for prefix in TrimNodesPass.KEEP_PREFIXES]):
                return AstPass.PASS_REMOVE
