# Copyright (c) 2025-2026 the rbfx project.
# This work is licensed under the terms of the MIT license.
# For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

"""GDB pretty-printers for Urho3D/rbfx types.

This module provides human-readable representations of Urho3D/rbfx objects in GDB.
It includes printers for:
  - StringHash: displays hash value and reverse lookup (if available)
  - Variant: shows type and value with best-effort type interpretation
  - SharedPtr/WeakPtr: displays pointer address and reference counts
  - Node: shows name and nameHash
  - Resource: displays resource name and path

Usage in GDB:
  (gdb) source /path/to/rbfx/script/gdb/rbfx_printers.py
  (gdb) python register_rbfx_printers()

The module auto-registers globally on import. If URHO3D_HASH_DEBUG is enabled
in the build, StringHash will also perform reverse lookups to show the original
string that created each hash.

Based on rbfx/script/rbfx.natvis but implemented via GDB Python API.
"""

from __future__ import print_function

import os
from pprint import pp

import gdb  # type: ignore
import gdb.printing  # type: ignore


_PRINTERS = None


# Cache for VariantType enum info (best-effort; may be unavailable in release builds).
_VARIANTTYPE_NAME_TO_VALUE = None
_VARIANTTYPE_VALUE_TO_NAME = None


def _normalize_enum_symbol_name(name):
    """Normalize enum symbol names to unqualified form.

    Depending on debug info and GDB version, enum field names may appear as
    'VAR_INT', 'Urho3D::VAR_INT', or even 'Urho3D::VariantType::VAR_INT'.
    For comparisons we always want the last component.
    """
    if name is None:
        return None
    try:
        s = str(name)
    except Exception:
        return name
    try:
        return s.rsplit('::', 1)[-1]
    except Exception:
        return s


# ============================================================================
# Helper Functions
# ============================================================================

def _try_int(x, default=0):
    try:
        return int(x)
    except Exception:
        return default


def _as_void_ptr(x):
    try:
        return x.cast(gdb.lookup_type('void').pointer())
    except Exception:
        return x


def _is_null_ptr(x):
    try:
        return _try_int(x) == 0
    except Exception:
        return False


def _find_member(val, member_name):
    """Find member by name, searching base classes as needed."""
    try:
        # Direct access first (works if member is in this type).
        return val[member_name]
    except Exception:
        pass

    try:
        t = val.type
    except Exception:
        return None

    try:
        for f in t.strip_typedefs().fields():
            # Base class
            if getattr(f, 'is_base_class', False):
                try:
                    base_val = val.cast(f.type)
                except Exception:
                    continue
                found = _find_member(base_val, member_name)
                if found is not None:
                    return found
            else:
                if f.name == member_name:
                    try:
                        return val[f]
                    except Exception:
                        try:
                            return val[f.name]
                        except Exception:
                            return None
    except Exception:
        return None

    return None


def _type_tag(t):
    try:
        tt = t.strip_typedefs()
    except Exception:
        tt = t
    try:
        return tt.tag
    except Exception:
        try:
            return str(tt)
        except Exception:
            return ''


def _find_base_cast_by_prefix(val, type_prefix):
    """Try to cast value to a base class whose type tag starts with prefix."""
    try:
        t = val.type.strip_typedefs()
    except Exception:
        return None

    try:
        if _type_tag(t).startswith(type_prefix):
            return val
    except Exception:
        pass

    try:
        for f in t.fields():
            if getattr(f, 'is_base_class', False):
                try:
                    base_val = val.cast(f.type)
                except Exception:
                    continue
                found = _find_base_cast_by_prefix(base_val, type_prefix)
                if found is not None:
                    return found
    except Exception:
        return None

    return None


def _find_eastl_hashtable_inside(val):
    """Find an EASTL hashtable-like subobject inside a container value."""
    # Direct base cast.
    for ns in ('eastl::hashtable<', 'ea::hashtable<'):
        ht = _find_base_cast_by_prefix(val, ns)
        if ht is not None:
            return ht

    # Common member names used by wrappers.
    for name in ('mTable', 'mHashtable', 'mHashTable', 'mBase', 'mImpl'):
        try:
            sub = _find_member(val, name)
            if sub is None:
                continue
            for ns in ('eastl::hashtable<', 'ea::hashtable<'):
                ht = _find_base_cast_by_prefix(sub, ns)
                if ht is not None:
                    return ht
            # Recurse one level.
            ht = _find_eastl_hashtable_inside(sub)
            if ht is not None:
                return ht
        except Exception:
            continue

    return None


def _iter_eastl_hashtable_nodes(hashtable_val):
    """Yield node.mValue for each node in an EASTL hashtable."""
    try:
        bucket_count = max(0, _try_int(hashtable_val['mnBucketCount']))
        buckets = hashtable_val['mpBucketArray']
    except Exception:
        return

    for b in range(bucket_count):
        try:
            entry = (buckets + b).dereference()
        except Exception:
            return
        while not _is_null_ptr(entry):
            try:
                node = entry.dereference()
                yield node['mValue']
                entry = node['mpNext']
            except Exception:
                break


def _stringhash_reverse_lookup(hash_value_uint):
    """Best-effort reverse lookup using Urho3D::hashReverseMap (URHO3D_HASH_DEBUG)."""

    try:
        map_ptr = gdb.parse_and_eval('Urho3D::hashReverseMap')
    except Exception:
        return None

    try:
        if _is_null_ptr(map_ptr):
            return None
        map_val = map_ptr.dereference()
    except Exception:
        return None

    ht = _find_eastl_hashtable_inside(map_val)
    if ht is None:
        return None

    # We only need a single key match.
    for pair_val in _iter_eastl_hashtable_nodes(ht):
        try:
            k = pair_val['first']
            v = pair_val['second']
        except Exception:
            continue

        try:
            k_value = _find_member(k, 'value_')
            if k_value is None:
                continue
            if _try_int(k_value) == int(hash_value_uint):
                return v
        except Exception:
            continue

    return None


def _get_unique_ptr_pointer(unique_ptr_val):
    # ea::unique_ptr is EASTL unique_ptr in this codebase.
    # Field layout typically: mPair.mFirst is the raw pointer.
    try:
        mPair = _find_member(unique_ptr_val, 'mPair')
        if mPair is not None:
            p = _find_member(mPair, 'mFirst')
            if p is not None:
                return p
    except Exception:
        pass

    # Fallbacks for other unique_ptr layouts.
    for name in ['mpValue', 'mpPtr', 'mPtr', '_M_t', '__ptr_']:
        try:
            p = _find_member(unique_ptr_val, name)
            if p is not None:
                return p
        except Exception:
            pass

    return None


def _get_varianttype_maps():
    """Return (name_to_value, value_to_name) for Urho3D::VariantType enum.

    In stripped/release builds, debug info may be missing. In that case, return
    empty maps and let printers degrade gracefully.
    """
    global _VARIANTTYPE_NAME_TO_VALUE, _VARIANTTYPE_VALUE_TO_NAME
    if _VARIANTTYPE_NAME_TO_VALUE is not None and _VARIANTTYPE_VALUE_TO_NAME is not None:
        return (_VARIANTTYPE_NAME_TO_VALUE, _VARIANTTYPE_VALUE_TO_NAME)

    name_to_value = {}
    value_to_name = {}

    try:
        enum_ty = gdb.lookup_type('Urho3D::VariantType').strip_typedefs()
        if enum_ty.code != gdb.TYPE_CODE_ENUM:
            enum_ty = None
    except Exception:
        enum_ty = None

    if enum_ty is not None:
        try:
            for f in enum_ty.fields():
                if not getattr(f, 'name', None):
                    continue
                try:
                    v = int(f.enumval)
                except Exception:
                    continue
                name_to_value[f.name] = v
                # Prefer the first name for a given value.
                if v not in value_to_name:
                    value_to_name[v] = f.name
        except Exception:
            name_to_value = {}
            value_to_name = {}

    _VARIANTTYPE_NAME_TO_VALUE = name_to_value
    _VARIANTTYPE_VALUE_TO_NAME = value_to_name
    return (name_to_value, value_to_name)


def _varianttype_enum_name(type_id):
    """Return enum symbol name like 'VAR_INT' or None if unavailable."""
    _, value_to_name = _get_varianttype_maps()
    try:
        return value_to_name.get(int(type_id))
    except Exception:
        return None


def _varianttype_display_name(type_id):
    """Return a printable type name for Variant.

    If debug info is present: use the actual enum symbol name.
    Otherwise: return a numeric tag like 'type=17'.
    """
    n = _varianttype_enum_name(type_id)
    if n:
        return n
    try:
        return 'type=' + str(int(type_id))
    except Exception:
        return 'type=?'


# ============================================================================
# Pretty Printer Classes
# ============================================================================

class _Urho3DStringHashPrinter(object):
    def __init__(self, val):
        self.val = val

    def to_string(self):
        try:
            value = _find_member(self.val, 'value_')
            if value is None:
                return 'StringHash(?)'
            hv = _try_int(value)
            rev = _stringhash_reverse_lookup(hv)
            if rev is not None:
                try:
                    return '#' + str(hv) + ' ' + str(rev)
                except Exception:
                    return '#' + str(hv)
            return '#' + str(hv)
        except Exception:
            return 'StringHash(?)'

    def children(self):
        try:
            value = _find_member(self.val, 'value_')
            if value is not None:
                yield ('[value]', value)

            if value is not None:
                rev = _stringhash_reverse_lookup(_try_int(value))
                if rev is not None:
                    yield ('[reverse]', rev)
        except Exception:
            return


class _Urho3DWeakPtrPrinter(object):
    def __init__(self, val):
        self.val = val

    def _refcount(self):
        return _find_member(self.val, 'refCount_')

    def _ptr(self):
        return _find_member(self.val, 'ptr_')

    def _expired(self):
        rc = self._refcount()
        if rc is None or _is_null_ptr(rc):
            return True
        try:
            refs = _find_member(rc.dereference(), 'refs_')
            return _try_int(refs) < 0
        except Exception:
            return True

    def to_string(self):
        try:
            rc = self._refcount()
            if rc is None or _is_null_ptr(rc):
                return '(empty)'
            if self._expired():
                return '(expired)'
            p = self._ptr()
            return str(_as_void_ptr(p))
        except Exception:
            return '(weakptr)'

    def children(self):
        try:
            rc = self._refcount()
            p = self._ptr()

            if rc is None or _is_null_ptr(rc):
                yield ('[empty]', 0)
                return

            try:
                rc_val = rc.dereference()
            except Exception:
                rc_val = None

            if rc_val is not None:
                refs = _find_member(rc_val, 'refs_')
                weak = _find_member(rc_val, 'weakRefs_')
                if weak is not None:
                    yield ('[weak]', weak)
                if refs is not None:
                    yield ('[strong]', refs)

                if _try_int(refs) >= 0 and p is not None:
                    yield ('[ptr]', p)
        except Exception:
            return


class _Urho3DSharedPtrPrinter(object):
    def __init__(self, val):
        self.val = val

    def _refcounted(self):
        return _find_member(self.val, 'refCounted_')

    def _ptr(self):
        # Some specializations store a separate ptr_.
        p = _find_member(self.val, 'ptr_')
        if p is not None:
            return p
        # Otherwise, the stored pointer itself is refCounted_.
        return _find_member(self.val, 'refCounted_')

    def _refcount_struct(self):
        refCounted = self._refcounted()
        if refCounted is None or _is_null_ptr(refCounted):
            return None

        # If refCounted_ isn't a RefCounted*, it still should be convertible.
        try:
            rc = _find_member(refCounted.dereference(), 'refCount_')
            if rc is not None:
                return rc
        except Exception:
            pass

        # Try casting to Urho3D::RefCounted.
        try:
            refCounted_ty = gdb.lookup_type('Urho3D::RefCounted').pointer()
            refCounted_cast = refCounted.cast(refCounted_ty)
            rc = _find_member(refCounted_cast.dereference(), 'refCount_')
            return rc
        except Exception:
            return None

    def _counts(self):
        rc = self._refcount_struct()
        if rc is None or _is_null_ptr(rc):
            return (None, None)
        try:
            rcv = rc.dereference()
            refs = _find_member(rcv, 'refs_')
            weak = _find_member(rcv, 'weakRefs_')
            return (refs, weak)
        except Exception:
            return (None, None)

    def to_string(self):
        try:
            p = self._ptr()
            if p is None or _is_null_ptr(p):
                return '(empty)'

            refs, weak = self._counts()
            if refs is None and weak is None:
                return str(_as_void_ptr(p))

            return str(_as_void_ptr(p)) + ' (refs=' + str(_try_int(refs, -1)) + ', weak=' + str(_try_int(weak, -1)) + ')'
        except Exception:
            return '(sharedptr)'

    def children(self):
        try:
            p = self._ptr()
            if p is None or _is_null_ptr(p):
                yield ('[empty]', 0)
                return

            refs, weak = self._counts()
            if refs is not None:
                yield ('[strong]', refs)
            if weak is not None:
                yield ('[weak]', weak)
            yield ('[ptr]', p)

            # Only try to show value if it is safe to dereference.
            try:
                yield ('[value]', p.dereference())
            except Exception:
                pass
        except Exception:
            return


class _Urho3DNodePrinter(object):
    def __init__(self, val):
        self.val = val

    def to_string(self):
        try:
            impl = _find_member(self.val, 'impl_')
            if impl is None:
                return 'Node(name_ = ?)'
            p = _get_unique_ptr_pointer(impl)
            if p is None or _is_null_ptr(p):
                return 'Node(name_ = <null>)'
            try:
                name = _find_member(p.dereference(), 'name_')
                if name is not None:
                    return 'name_ = ' + str(name)
            except Exception:
                pass
            return 'Node(?)'
        except Exception:
            return 'Node(?)'

    def children(self):
        try:
            impl = _find_member(self.val, 'impl_')
            if impl is None:
                return
            p = _get_unique_ptr_pointer(impl)
            if p is None or _is_null_ptr(p):
                yield ('[impl]', 0)
                return
            impl_obj = p.dereference()
            name = _find_member(impl_obj, 'name_')
            nameHash = _find_member(impl_obj, 'nameHash_')
            if name is not None:
                yield ('name_', name)
            if nameHash is not None:
                yield ('nameHash_', nameHash)
        except Exception:
            return


class _Urho3DResourcePrinter(object):
    def __init__(self, val):
        self.val = val

    def to_string(self):
        try:
            name = _find_member(self.val, 'name_')
            if name is None:
                return 'name_ = ?'
            return 'name_ = ' + str(name)
        except Exception:
            return 'Resource(?)'

    def children(self):
        try:
            name = _find_member(self.val, 'name_')
            if name is not None:
                yield ('name_', name)
            nameHash = _find_member(self.val, 'nameHash_')
            if nameHash is not None:
                yield ('nameHash_', nameHash)
            absName = _find_member(self.val, 'absoluteFileName_')
            if absName is not None:
                yield ('absoluteFileName_', absName)
        except Exception:
            return


class _Urho3DCustomVariantValueImplPrinter(object):
    def __init__(self, val):
        self.val = val

    def to_string(self):
        try:
            v = _find_member(self.val, 'value_')
            if v is None:
                return '(custom)'
            return str(v)
        except Exception:
            return '(custom)'

    def children(self):
        try:
            v = _find_member(self.val, 'value_')
            if v is not None:
                yield ('value_', v)
        except Exception:
            return


class _Urho3DVariantPrinter(object):

    def __init__(self, val):
        self.val = val

    def _type_id(self):
        t = _find_member(self.val, 'type_')
        return _try_int(t, 0)

    def _value(self):
        return _find_member(self.val, 'value_')

    def _format_value(self, type_name, value_u):
        try:
            if value_u is None:
                return None

            type_name = _normalize_enum_symbol_name(type_name)

            if type_name == 'VAR_NONE':
                return None
            if type_name == 'VAR_INT':
                return _find_member(value_u, 'int_')
            if type_name == 'VAR_BOOL':
                return _find_member(value_u, 'bool_')
            if type_name == 'VAR_FLOAT':
                return _find_member(value_u, 'float_')
            if type_name == 'VAR_VECTOR2':
                return _find_member(value_u, 'vector2_')
            if type_name == 'VAR_VECTOR3':
                return _find_member(value_u, 'vector3_')
            if type_name == 'VAR_VECTOR4':
                return _find_member(value_u, 'vector4_')
            if type_name == 'VAR_QUATERNION':
                return _find_member(value_u, 'quaternion_')
            if type_name == 'VAR_COLOR':
                return _find_member(value_u, 'color_')
            if type_name == 'VAR_STRING':
                return _find_member(value_u, 'string_')
            if type_name == 'VAR_BUFFER':
                return _find_member(value_u, 'buffer_')
            if type_name == 'VAR_VOIDPTR':
                return _find_member(value_u, 'voidPtr_')
            if type_name == 'VAR_RESOURCEREF':
                return _find_member(value_u, 'resourceRef_')
            if type_name == 'VAR_RESOURCEREFLIST':
                return _find_member(value_u, 'resourceRefList_')
            if type_name == 'VAR_VARIANTVECTOR':
                return _find_member(value_u, 'variantVector_')
            if type_name == 'VAR_VARIANTMAP':
                return _find_member(value_u, 'variantMap_')
            if type_name == 'VAR_INTRECT':
                return _find_member(value_u, 'intRect_')
            if type_name == 'VAR_INTVECTOR2':
                return _find_member(value_u, 'intVector2_')
            if type_name == 'VAR_PTR':
                return _find_member(value_u, 'weakPtr_')
            if type_name == 'VAR_MATRIX3':
                p = _find_member(value_u, 'matrix3_')
                return p.dereference() if p is not None and not _is_null_ptr(p) else p
            if type_name == 'VAR_MATRIX3X4':
                p = _find_member(value_u, 'matrix3x4_')
                return p.dereference() if p is not None and not _is_null_ptr(p) else p
            if type_name == 'VAR_MATRIX4':
                p = _find_member(value_u, 'matrix4_')
                return p.dereference() if p is not None and not _is_null_ptr(p) else p
            if type_name == 'VAR_DOUBLE':
                return _find_member(value_u, 'double_')
            if type_name == 'VAR_STRINGVECTOR':
                return _find_member(value_u, 'stringVector_')
            if type_name == 'VAR_RECT':
                return _find_member(value_u, 'rect_')
            if type_name == 'VAR_INTVECTOR3':
                return _find_member(value_u, 'intVector3_')
            if type_name == 'VAR_INT64':
                return _find_member(value_u, 'int64_')
            if type_name == 'VAR_CUSTOM':
                storage = _find_member(value_u, 'storage_')
                if storage is None:
                    storage = _find_member(value_u, 'storage')
                if storage is not None:
                    try:
                        custom_ty = gdb.lookup_type('Urho3D::CustomVariantValue').pointer()
                        # storage_ is byte array; take its address.
                        ptr = storage.address.cast(custom_ty)
                        return ptr.dereference()
                    except Exception:
                        return None
                return None
            if type_name == 'VAR_VARIANTCURVE':
                return _find_member(value_u, 'variantCurve_')
            if type_name == 'VAR_STRINGVARIANTMAP':
                return _find_member(value_u, 'stringVariantMap_')
        except Exception:
            return None

        return None

    def to_string(self):
        try:
            tid = self._type_id()

            # Prefer real enum symbol name when debug info is available.
            raw_enum_name = _varianttype_enum_name(tid)
            enum_name = _normalize_enum_symbol_name(raw_enum_name)
            display_name = raw_enum_name or enum_name or _varianttype_display_name(tid)

            if tid == 0 or enum_name == 'VAR_NONE':
                return '(none)'

            value_u = self._value()
            value = self._format_value(enum_name, value_u) if enum_name is not None else None

            if value is None:
                return '(' + display_name + ')'

            # Pointer-like types:
            if enum_name in ('VAR_VOIDPTR',):
                return '(void*) ' + str(_as_void_ptr(value))

            return '(' + display_name + ') ' + str(value)
        except Exception:
            return '(variant)'

    def children(self):
        try:
            tid = self._type_id()
            value_u = self._value()
            raw_enum_name = _varianttype_enum_name(tid)
            enum_name = _normalize_enum_symbol_name(raw_enum_name)
            value = self._format_value(enum_name, value_u) if enum_name is not None else None
            if value is None:
                return
            # Follow natvis: scalar types show [value], complex types show expanded item.
            yield ('[value]', value)
        except Exception:
            return


# ============================================================================
# Printer Registration
# ============================================================================

def build_pretty_printers():
    pp = gdb.printing.RegexpCollectionPrettyPrinter('rbfx-urho3d')

    # Urho3D types
    pp.add_printer('Urho3D::StringHash', '^Urho3D::StringHash$', _Urho3DStringHashPrinter)
    pp.add_printer('Urho3D::Variant', '^Urho3D::Variant$', _Urho3DVariantPrinter)

    pp.add_printer('Urho3D::CustomVariantValueImpl', '^Urho3D::CustomVariantValueImpl<.*>$', _Urho3DCustomVariantValueImplPrinter)

    pp.add_printer('Urho3D::SharedPtr', '^Urho3D::SharedPtr<.*>$', _Urho3DSharedPtrPrinter)
    pp.add_printer('Urho3D::WeakPtr', '^Urho3D::WeakPtr<.*>$', _Urho3DWeakPtrPrinter)

    pp.add_printer('Urho3D::Node', '^Urho3D::Node$', _Urho3DNodePrinter)
    pp.add_printer('Urho3D::Resource', '^Urho3D::Resource$', _Urho3DResourcePrinter)

    return pp


def register_rbfx_printers(objfile=None):
    """Register Urho3D/rbfx pretty printers.

    If objfile is None, registers globally (recommended for interactive use).
    """
    global _PRINTERS
    if _PRINTERS is None:
        _PRINTERS = build_pretty_printers()

    # Older GDBs don't support replace=.
    try:
        gdb.printing.register_pretty_printer(objfile, _PRINTERS, replace=True)
    except TypeError:
        gdb.printing.register_pretty_printer(objfile, _PRINTERS)


# Convenience: if sourced from .gdbinit, auto-register globally.
try:
    register_rbfx_printers()
except Exception as e:
    # Avoid breaking gdb startup on older builds lacking python support.
    gdb.write('Error registering urho3d/rbfx printers: ' + str(e) + '\n')