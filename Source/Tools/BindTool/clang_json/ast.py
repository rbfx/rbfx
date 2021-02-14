from util import find_value

AST_ACCESS_PRIVATE = 0
AST_ACCESS_PROTECTED = 1
AST_ACCESS_PUBLIC = 2
AST_ACCESS_STRING_TO_CONST = {
    'private': AST_ACCESS_PRIVATE,
    'protected': AST_ACCESS_PROTECTED,
    'public': AST_ACCESS_PUBLIC,
}

NAME_QUALIFYING_NODES = ['CXXRecordDecl', 'EnumDecl', 'NamespaceDecl']


def _get_type(type_node):
    return type_node['qualType']    # type_node.get('desugaredQualType', type_node['qualType'])


class AstPass(object):
    # Default. Continue iterating through children.
    PASS_CONTINUE = None
    # Do not iterate through children.
    PASS_BREAK = 1
    # Remove current node from AST.
    PASS_REMOVE = 2

    def __init__(self, parser, writer, lang):
        self._parser = parser
        self._writer = writer
        self._lang = lang

    def on_start(self): pass
    def on_finish(self): pass
    def on_enter(self, node): return AstPass.PASS_CONTINUE
    def on_exit(self, node): return AstPass.PASS_CONTINUE

    def walk_ast(self, ast_node):
        resolution = self.on_enter(ast_node)
        if resolution == AstPass.PASS_BREAK:
            return AstPass.PASS_CONTINUE
        if resolution == AstPass.PASS_REMOVE:
            return resolution

        remove_nodes = []
        for child in ast_node.children:
            resolution = self.walk_ast(child)
            if resolution == AstPass.PASS_REMOVE:
                remove_nodes.append(child)
        for remove_child in remove_nodes:
            ast_node.children.remove(remove_child)

        return self.on_exit(ast_node)


class AstNode(object):
    def __init__(self, parser, parent, json_node):
        self._json_node = json_node
        self.access = AST_ACCESS_PUBLIC
        self.name = json_node.get('name')
        self.parent = parent
        self.id = int(json_node.get('id', -1), 16)
        self.source_path = parser.get_source_code_path_abs(json_node)
        self.source_line = json_node.get('loc', {}).get('presumedLine')
        children = self.children = []
        access = getattr(self, 'default_access', AST_ACCESS_PUBLIC)
        for c in json_node.get('inner', []):
            if c.get('kind') == 'AccessSpecDecl':
                access = AST_ACCESS_STRING_TO_CONST[c['access']]
                continue
            n = AstNode.create_from_json_node(parser, self, c)
            if n is not None:
                n.access = access
                children.append(n)

    @property
    def kind(self):
        return self.__class__.__name__

    @staticmethod
    def create_from_json_node(parser, parent, json_node):
        kind = json_node.get('kind')

        # Special cases where we do not want new nodes created.
        if kind == 'OverrideAttr':
            parent.virtual = parent.override = True

        cls = KIND_TO_CLASS.get(kind)
        if cls:
            if kind == 'CXXRecordDecl' and not json_node.get('completeDefinition', False):
                return  # Do not keep forward declarations.
            return cls(parser, parent, json_node)

    def infer_fqn(self, short=False):
        name = getattr(self, 'name')
        if not name:
            return None
        parts = [name]
        n = self.parent
        while n is not None:
            kind = n.kind
            if kind in NAME_QUALIFYING_NODES:
                name = getattr(n, 'name')
                if name:
                    if kind == 'EnumDecl':
                        if not short or n.is_class:
                            parts.append(name)
                    else:
                        parts.append(name)
            n = n.parent
        return '::'.join(reversed(parts))

    def __str__(self):
        name = getattr(self, 'name')
        info = f'<{self.kind}> + {len(self.children)}'
        if name:
            return f'{name} {info}'
        return info

    def raw_attr(self, path):
        n = self._json_node
        for p in path.split('/'):
            n = n.get(p)
        return n


class ConstValueMixin:
    CONST_EXPR_KINDS = ('FloatingLiteral', 'IntegerLiteral', 'ImplicitCastExpr', 'ConstantExpr')

    def __init__(self, parser, parent, json_node):
        def not_comments(n):
            return not n['kind'].endswith('Comment')
        self.value = find_value(json_node)
        self.is_primitive_value = self.value is not None
        if not self.value:
            self.value = parser.read_from_source(json_node['range']['end'])


class TranslationUnitDecl(AstNode):
    def __init__(self, parser, parent, json_node):
        super().__init__(parser, parent, json_node)


class NamespaceDecl(AstNode):
    def __init__(self, parser, parent, json_node):
        super().__init__(parser, parent, json_node)

    @property
    def is_anonymous(self):
        return not self.name


class CXXRecordBase(object):
    def __init__(self, base_node):
        self.access = base_node.get('access', 'private'),
        self.cls = _get_type(base_node['type'])


class CXXRecordDecl(AstNode):
    def __init__(self, parser, parent, json_node):
        self.tag = json_node['tagUsed']
        self.default_access = AST_ACCESS_STRING_TO_CONST['public' if self.tag == 'struct' else 'private']
        super().__init__(parser, parent, json_node)
        self.bases = []
        for base_node in json_node.get('bases', []):
            self.bases.append(CXXRecordBase(base_node))
        self.methods = [m for m in self.children if m.kind == 'CXXMethodDecl']


class CXXMethodDecl(AstNode):
    def __init__(self, parser, parent, json_node):
        super().__init__(parser, parent, json_node)
        self.params = [n for n in self.children if n.kind == 'ParmVarDecl']
        self.mangled_name = json_node.get('mangledName')
        if 'returnType' in json_node:
            self.return_type = _get_type(json_node['returnType'])
        else:
            # TODO: Attempt to get type string. This will most likely fail when function returns a function pointer!
            self.return_type = _get_type(json_node['type']).split('(')[0].strip()
        self.is_static = json_node.get('storageClass') == 'static'
        self.override = False
        self.virtual = json_node.get('virtual', False)


class CXXConstructorDecl(CXXMethodDecl):
    def __init__(self, parser, parent, json_node):
        super().__init__(parser, parent, json_node)


class CXXDestructorDecl(CXXMethodDecl):
    def __init__(self, parser, parent, json_node):
        super().__init__(parser, parent, json_node)


class VarNode(AstNode, ConstValueMixin):
    def __init__(self, parser, parent, json_node):
        super().__init__(parser, parent, json_node)
        ConstValueMixin.__init__(self, parser, parent, json_node)
        self.mangled_name = json_node.get('mangledName')
        self.type = _get_type(json_node['type'])


class ParmVarDecl(VarNode):
    def __init__(self, parser, parent, json_node):
        super().__init__(parser, parent, json_node)


class FieldDecl(VarNode):
    def __init__(self, parser, parent, json_node):
        super().__init__(parser, parent, json_node)


class VarDecl(VarNode):
    def __init__(self, parser, parent, json_node):
        super().__init__(parser, parent, json_node)


class EnumDecl(AstNode):
    def __init__(self, parser, parent, json_node):
        super().__init__(parser, parent, json_node)
        self.type = json_node.get('fixedUnderlyingType', {}).get('qualType')
        self.is_class = json_node.get('scopedEnumTag') == 'class'


class EnumConstantDecl(AstNode, ConstValueMixin):
    def __init__(self, parser, parent, json_node):
        super().__init__(parser, parent, json_node)
        ConstValueMixin.__init__(self, parser, parent, json_node)
        self.type = json_node.get('fixedUnderlyingType', {}).get('qualType')


class TypeAliasDecl(AstNode):
    def __init__(self, parser, parent, json_node):
        super().__init__(parser, parent, json_node)
        self.type = _get_type(json_node['type'])


KIND_TO_CLASS = {cls.__name__: cls for cls in [
    TranslationUnitDecl, NamespaceDecl, CXXRecordDecl, CXXConstructorDecl, CXXDestructorDecl, CXXMethodDecl,
    ParmVarDecl, FieldDecl, VarDecl, EnumDecl, EnumConstantDecl, TypeAliasDecl
]}

# TranslationUnitDecl
#   NamespaceDecl
#     CXXRecordDecl
#       CXXConstructorDecl
#       CXXDestructorDecl
#       CXXMethodDecl
#         OverrideAttr
#         ParmVarDecl
#       FieldDecl
#     VarDecl
# BlockCommandComment
# VerbatimBlockComment
# VerbatimBlockLineComment
# VerbatimLineComment
# TParamCommandComment
# TextComment
# ParagraphComment
# ParamCommandComment
# FullComment

# ClassTemplateDecl
# ClassTemplatePartialSpecializationDecl
# ClassTemplateSpecializationDecl
# TemplateTypeParmDecl
# TypeAliasDecl
# TypedefDecl
# UsingDecl
# UsingDirectiveDecl
# UsingShadowDecl
# FunctionDecl
# FunctionTemplateDecl
# AccessSpecDecl
# AlignedAttr
# AllocAlignAttr
# AllocSizeAttr
# AlwaysInlineAttr
# ArrayInitIndexExpr
# ArrayInitLoopExpr
# ArraySubscriptExpr
# AsmLabelAttr
# BinaryOperator
# BuiltinAttr
# BuiltinType
# CallExpr
# CharacterLiteral
# ConditionalOperator
# ConstantArrayType
# ConstantExpr
# ConstAttr
# ConstructorUsingShadowDecl
# CStyleCastExpr
# CXXBindTemporaryExpr
# CXXBoolLiteralExpr
# CXXConversionDecl
# CXXCtorInitializer
# CXXDeductionGuideDecl
# CXXDefaultArgExpr
# CXXDefaultInitExpr
# CXXDependentScopeMemberExpr
# CXXMemberCallExpr
# CXXNullPtrLiteralExpr
# CXXOperatorCallExpr
# CXXStaticCastExpr
# CXXTemporaryObjectExpr
# CXXThisExpr
# CXXTypeidExpr
# DeclRefExpr
# DecltypeType
# DependentNameType
# DependentScopeDeclRefExpr
# DeprecatedAttr
# ElaboratedType
# EmptyDecl
# ExprWithCleanups
# FinalAttr
# FloatingLiteral
# FormatArgAttr
# FormatAttr
# FriendDecl
# FunctionProtoType
# GNUNullExpr
# HTMLEndTagComment
# HTMLStartTagComment
# ImplicitCastExpr
# ImplicitValueInitExpr
# IndirectFieldDecl
# InitListExpr
# InjectedClassNameType
# InlineCommandComment
# IntegerLiteral
# LinkageSpecDecl
# LValueReferenceType
# MaterializeTemporaryExpr
# MemberExpr
# MemberPointerType
# ModeAttr
# NonNullAttr
# NonTypeTemplateParmDecl
# NoThrowAttr
# OpaqueValueExpr
# PackExpansionType
# ParenExpr
# ParenListExpr
# ParenType
# PointerType
# PureAttr
# QualType
# RecordType
# RestrictAttr
# ReturnsNonNullAttr
# ReturnsTwiceAttr
# StaticAssertDecl
# StringLiteral
# SubstNonTypeTemplateParmExpr
# SubstTemplateTypeParmType
# TemplateArgument
# TemplateSpecializationType
# TemplateTypeParmType
# TypedefType
# UnaryExprOrTypeTraitExpr
# UnaryOperator
# UnaryTransformType
# UnresolvedLookupExpr
# VisibilityAttr
# WarnUnusedResultAttr
