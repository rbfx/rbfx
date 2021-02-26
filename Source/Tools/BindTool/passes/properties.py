import logging

from clang_json.ast import AstPass, AST_ACCESS_PUBLIC
import util


class DiscoverInterfacesPass(AstPass):
    """Interface methods can not be used as properties."""
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self._parser.vars['interfaces'] = {}

    def on_enter(self, node):
        if not node.kind == 'CXXRecordDecl':
            return

        if len(node.bases) > 1:
            for base in node.bases[1:]:
                if isinstance(base.cls, str):
                    continue

                fqn = base.cls.infer_fqn()
                if fqn not in self._parser.vars['interfaces']:
                    self._parser.vars['interfaces'][fqn] = [m.name for m in base.cls.methods]

        return AstPass.PASS_BREAK


class DefinePropertiesPass(AstPass):
    def on_enter(self, node):
        if not node.kind == 'CXXRecordDecl':
            return

        fqn = node.infer_fqn()
        if fqn in self._parser.vars['interfaces']:
            logging.warning(f'Interface class {fqn} can not contain properties.')
            return
        properties = {}
        subsystem_name = util.get_subsystem_name(node.source_path)

        # Gather restricted names
        restricted_names = set()
        restricted_methods = set()
        restricted_methods.add('GetType')     # Clashes with C#'s System.Object.GetType.

        for n in node.children:
            # Do not consider protected members as restricted. This is not strictly correct, because
            if n.access == AST_ACCESS_PUBLIC and n.kind in ('CXXMethodDecl', 'CXXConstructorDecl', 'FieldDecl', 'VarDecl', 'EnumDecl'):
                if n.name:
                    restricted_names.add(util.camel_case(n.name))

        # Blacklist interface methods from property generation. If interface method is turned to a property it can not
        # be renamed or made private, because it breaks compilation.
        for base_class, interface_methods in self._parser.vars['interfaces'].items():
            if util.has_base_class(node, base_class):
                restricted_methods.update(interface_methods)

        # Blacklist methods where getter has any parameters or setter has more than one parameter.
        for n in node.children:
            if n.kind == 'CXXMethodDecl':
                if (n.name.startswith('Get') or n.name.startswith('Is')) and len(n.params) > 0:
                    restricted_methods.add(n.name)
                elif n.name.startswith('Set') and len(n.params) != 1:
                    restricted_methods.add(n.name)

        # Gather getters
        for n in node.children:
            if n.kind == 'CXXMethodDecl' and not n.is_static:
                if n.access != AST_ACCESS_PUBLIC:  # SWIG cant turn protected getters/setters to properties.
                    logging.warning(f'{fqn}::{n.name} can not be a property: access.')
                    continue
                if n.virtual:
                    logging.warning(f'{fqn}::{n.name} can not be a property: virtual.')
                    continue
                name = n.name
                is_getter = name.startswith('Get') or name.startswith('Is')
                # Getter must have no parameters
                is_getter = is_getter and len(n.params) == 0
                if is_getter:
                    if name.startswith('Is'):
                        prop_name = name
                        name = name[2:]
                    else:
                        name = prop_name = name[3:]

                    if name in restricted_names or n.name in restricted_methods:
                        logging.warning(f'{fqn}::{n.name} can not be a property: name clash.')
                        continue

                    if name in properties:
                        # Const and non-const getters may be available.
                        continue
                    properties[name] = [n, None, n.return_type, prop_name]

        # Gather setters with a compatible parameter type
        for n in node.children:
            if n.access != AST_ACCESS_PUBLIC:  # SWIG cant turn protected getters/setters to properties.
                continue

            if n.kind == 'CXXMethodDecl' and not n.is_static:
                name = n.name
                is_setter = name.startswith('Set') or name in properties
                if is_setter:
                    # Setters have a single parameter
                    if len(n.params) > 0:
                        if name.startswith('Set'):
                            name = name[3:]
                        prop = properties.get(name)
                        if prop is not None:
                            setter_param_type = n.params[0].type
                            if prop[2] != setter_param_type:
                                if setter_param_type.startswith('const '):
                                    setter_param_type = setter_param_type[6:]
                                setter_param_type = setter_param_type.rstrip('& ')
                            if prop[2] == setter_param_type:
                                if n.virtual:
                                    logging.warning(f'{fqn}::{n.name} can not be a property: virtual.')
                                    del properties[name]
                                elif len(n.params) != 1:
                                    logging.warning(f'{fqn}::{n.name} can not be a property: setter has too many parameters.')
                                    del properties[name]
                                else:
                                    prop[1] = n

        # Print properties
        for name, (getter_node, setter_node, param_type, prop_name) in properties.items():
            cls_fqn = node.infer_fqn()
            prop_name = util.camel_case(prop_name)
            if prop_name == node.name:
                # Property name can not be same as class name.
                prop_name += 'Value'
            if len(prop_name) > 3 and prop_name.endswith('Ptr') and node.name == 'Variant' and prop_name != 'VoidPtr':
                # Drop Ptr suffix for some Variant properties. We use Ptr versions so that returned containers would
                # be modifiable, but we do not care for this detail in C#.
                prop_name = prop_name[:-3]
            param_type = param_type.rstrip('&')
            if param_type.startswith('const '):
                param_type = param_type[6:]
            param_type = param_type.strip()
            line = f'%csattribute({cls_fqn}, %arg({param_type}), {prop_name}, {getter_node.name}'
            if setter_node:
                line += f', {setter_node.name}'
            line += ');'
            self._writer.write_pre(subsystem_name, line)
