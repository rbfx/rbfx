# Copyright (c) 2025-2026 the rbfx project.
# This work is licensed under the terms of the MIT license.
# For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

"""GDB pretty-printers for EASTL containers.

This module provides human-readable representations of EASTL containers in GDB.
This is a reimplementation of EASTL.natvis (MSVC visualizers).

Usage in GDB:
  (gdb) source /path/to/rbfx/script/gdb/eastl_printers.py
"""

import gdb              # type: ignore
import gdb.printing     # type: ignore


class _DelegatingDerivedPrinter:
    """Delegate printing of a derived container to an existing base printer.

    Many EASTL fixed containers (fixed_vector, fixed_map, etc.) are thin wrappers
    derived from the regular containers. In debug info they appear as distinct
    types, so we explicitly match them and then cast to the base type to reuse
    the existing pretty-printers.
    """

    def __init__(self, val, base_tag_prefix: str, delegate_cls, display_name: str):
        self.val = val
        self._delegate_cls = delegate_cls
        self._display_name = display_name
        self._base = _find_base_cast(val, base_tag_prefix)

    def _delegate(self):
        if self._base is None:
            return None
        try:
            return self._delegate_cls(self._base)
        except Exception:
            return None

    def to_string(self):
        d = self._delegate()
        if d is None:
            return f"{self._display_name}<?>"
        try:
            # Preserve the delegate's human-friendly string.
            return d.to_string()
        except Exception:
            return f"{self._display_name}<?>"

    def display_hint(self):
        d = self._delegate()
        if d is None:
            return None
        try:
            return d.display_hint()
        except Exception:
            return None

    def children(self):
        d = self._delegate()
        if d is None:
            return
        try:
            yield from d.children()
        except Exception:
            return

# ============================================================================
# Helper Functions
# ============================================================================

def _int(v) -> int:
    try:
        return int(v)
    except Exception:
        return 0


def _value(i: int):
    return gdb.Value(i)


def _strip(t):
    try:
        return t.strip_typedefs()
    except Exception:
        return t


def _tag(t) -> str:
    t = _strip(t)
    return t.tag or str(t)


def _is_null_ptr(p) -> bool:
    try:
        return int(p) == 0
    except Exception:
        return False


def _find_base_cast(val, base_tag_prefix: str):
    """Return val casted to its base subobject whose tag starts with base_tag_prefix."""
    t = _strip(val.type)
    for f in getattr(t, "fields", lambda: [])():
        if not getattr(f, "is_base_class", False):
            continue
        base_t = _strip(f.type)
        base_name = _tag(base_t)
        if base_name.startswith(base_tag_prefix):
            try:
                return val.cast(base_t)
            except Exception:
                return None
    return None


def _compressed_pair_base(cp_val):
    """Return the compressed_pair_imp base subobject."""
    t = _strip(cp_val.type)
    for f in t.fields():
        if getattr(f, "is_base_class", False):
            return cp_val.cast(_strip(f.type))
    return cp_val


def _compressed_pair_first(cp_val):
    base = _compressed_pair_base(cp_val)
    try:
        return base["mFirst"]
    except Exception:
        pass

    # EBO case: first lives as a base class subobject.
    try:
        t1 = _strip(_strip(cp_val.type).template_argument(0))
        return base.cast(t1)
    except Exception:
        return base


def _compressed_pair_second(cp_val):
    base = _compressed_pair_base(cp_val)
    try:
        return base["mSecond"]
    except Exception:
        pass

    # EBO case: second lives as a base class subobject.
    try:
        t2 = _strip(_strip(cp_val.type).template_argument(1))
        return base.cast(t2)
    except Exception:
        return base


def _iterable_children_from_ptr(begin_ptr, count: int):
    for i in range(max(0, count)):
        try:
            yield f"[{i}]", (begin_ptr + i).dereference()
        except Exception:
            return


def _safe_deref(p):
    try:
        return p.dereference()
    except Exception:
        return None


def _try_read_string(ptr, length: int, elem_size: int):
    """Best-effort decode a (possibly non-char) string from inferior memory."""
    if ptr is None or _is_null_ptr(ptr) or length <= 0:
        return ""

    try:
        if elem_size == 1:
            try:
                cptr = ptr.cast(gdb.lookup_type("char").pointer())
            except Exception:
                cptr = ptr
            return cptr.string(length=length)

        # Wide char support is best-effort; gdb may not support these encodings.
        if elem_size == 2:
            return ptr.string(length=length, encoding="utf-16")
        if elem_size == 4:
            return ptr.string(length=length, encoding="utf-32")
    except Exception:
        return None
    return None


_IS_BIG_ENDIAN = None


def _target_is_big_endian() -> bool:
    global _IS_BIG_ENDIAN
    if _IS_BIG_ENDIAN is None:
        try:
            out = gdb.execute("show endian", to_string=True)
            _IS_BIG_ENDIAN = "big endian" in out.lower()
        except Exception:
            _IS_BIG_ENDIAN = False
    return bool(_IS_BIG_ENDIAN)


# ============================================================================
# String Printers
# ============================================================================

class _EastlBasicStringPrinter:
    """Pretty-printer for eastl::basic_string<T>.

    Handles both SSO (Small String Optimization) and heap-allocated strings.
    Detects endianness and string encoding (char, wchar_t, etc.).
    Displays string content when possible, or metadata (length/capacity) on decode failure.
    """
    def __init__(self, val):
        self.val = val

    def _layout(self):
        # basic_string has: eastl::compressed_pair<Layout, allocator_type> mPair;
        return _compressed_pair_first(self.val["mPair"])

    def _is_heap(self, layout) -> bool:
        try:
            remaining = _int(layout["sso"]["mRemainingSizeField"]["mnRemainingSize"]) & 0xFF
        except Exception:
            return False

        # EASTL uses a mask bit inside SSO's remaining-size byte.
        # Little-endian: 0x80, Big-endian: 0x01.
        mask = 0x01 if _target_is_big_endian() else 0x80
        return (remaining & mask) != 0

    def _heap_capacity(self, layout) -> int:
        cap_raw = _int(layout["heap"]["mnCapacity"])
        cap_type = layout["heap"]["mnCapacity"].type
        bits = int(getattr(cap_type, "sizeof", 0) * 8) or 64
        msb_mask = 1 << (bits - 1)

        # Little endian stores cap|MSB.
        if cap_raw & msb_mask:
            return cap_raw & (~msb_mask)
        # Big endian stores (cap<<1)|1.
        if cap_raw & 1:
            return cap_raw >> 1
        return cap_raw

    def _sso_capacity(self, layout) -> int:
        value_type = _strip(_strip(self.val.type).template_argument(0))
        elem_size = int(getattr(value_type, "sizeof", 1)) or 1
        heap_layout_size = int(getattr(layout["heap"].type, "sizeof", 0))
        # EASTL: (sizeof(HeapLayout) - sizeof(char)) / sizeof(value_type)
        return max(0, (heap_layout_size - 1) // elem_size)

    def _sso_size(self, layout) -> int:
        remaining = _int(layout["sso"]["mRemainingSizeField"]["mnRemainingSize"]) & 0xFF
        sso_cap = self._sso_capacity(layout)
        if _target_is_big_endian():
            # Big endian: remaining byte stores (cap - size) << 2.
            return max(0, sso_cap - (remaining >> 2))
        # Little endian: remaining byte stores (cap - size).
        return max(0, sso_cap - remaining)

    def _data_ptr_and_len(self):
        layout = self._layout()
        if self._is_heap(layout):
            p = layout["heap"]["mpBegin"]
            n = _int(layout["heap"]["mnSize"])
            cap = self._heap_capacity(layout)
            return p, n, cap, True

        # SSO
        data_arr = layout["sso"]["mData"]
        value_type = _strip(_strip(self.val.type).template_argument(0))
        p = data_arr.address.cast(value_type.pointer())
        n = self._sso_size(layout)
        cap = self._sso_capacity(layout)
        return p, n, cap, False

    def to_string(self) -> str:
        try:
            p, n, _cap, is_heap = self._data_ptr_and_len()
        except Exception:
            return "eastl::basic_string<?>"

        if _is_null_ptr(p):
            return "\"\""

        # Best-effort decode.
        try:
            value_type = _strip(_strip(self.val.type).template_argument(0))
            elem_size = int(getattr(value_type, "sizeof", 1)) or 1
            if elem_size == 1:
                s = p.string(length=n)
            elif elem_size == 2:
                s = p.string(length=n, encoding="utf-16")
            else:
                s = p.string(length=n, encoding="utf-32")
            return s
        except Exception:
            return f"eastl::basic_string(len={n}, heap={int(is_heap)})"

    def display_hint(self):
        return "string"

    def children(self):
        try:
            p, n, cap, is_heap = self._data_ptr_and_len()
        except Exception:
            return

        yield "[length]", _value(n)
        yield "[capacity]", _value(cap)
        yield "[uses heap]", _value(1 if is_heap else 0)
        yield "[data]", p


class _EastlFixedStringPrinter(_DelegatingDerivedPrinter):
    def __init__(self, val):
        super().__init__(val, "eastl::basic_string<", _EastlBasicStringPrinter, "eastl::fixed_string")


class _EastlFixedSubstringPrinter(_DelegatingDerivedPrinter):
    def __init__(self, val):
        super().__init__(val, "eastl::basic_string<", _EastlBasicStringPrinter, "eastl::fixed_substring")


# ============================================================================
# Smart Pointer Printers
# ============================================================================

class _EastlUniquePtrPrinter:
    def __init__(self, val):
        self.val = val

    def _ptr(self):
        try:
            return self.val["mPair"]["mFirst"]
        except Exception:
            return None

    def to_string(self) -> str:
        p = self._ptr()
        if p is None or _is_null_ptr(p):
            return "eastl::unique_ptr(nullptr)"
        try:
            return f"eastl::unique_ptr({p})"
        except Exception:
            return "eastl::unique_ptr<?>"

    def children(self):
        p = self._ptr()
        if p is None:
            return
        yield "[pointer]", p
        if not _is_null_ptr(p):
            v = _safe_deref(p)
            if v is not None:
                yield "[value]", v


class _EastlSharedPtrPrinter:
    def __init__(self, val):
        self.val = val

    def _ptr(self):
        try:
            return self.val["mpValue"]
        except Exception:
            return None

    def _refcount(self):
        try:
            return self.val["mpRefCount"]
        except Exception:
            return None

    def to_string(self) -> str:
        p = self._ptr()
        if p is None or _is_null_ptr(p):
            return "eastl::shared_ptr(nullptr)"
        try:
            return f"eastl::shared_ptr({p})"
        except Exception:
            return "eastl::shared_ptr<?>"

    def children(self):
        p = self._ptr()
        if p is not None:
            yield "[pointer]", p
            if not _is_null_ptr(p):
                v = _safe_deref(p)
                if v is not None:
                    yield "[value]", v

        rc = self._refcount()
        if rc is not None and not _is_null_ptr(rc):
            try:
                yield "[reference count]", rc["mRefCount"]
            except Exception:
                pass
            try:
                yield "[weak reference count]", rc["mWeakRefCount"]
            except Exception:
                pass


class _EastlWeakPtrPrinter:
    def __init__(self, val):
        self.val = val

    def _ptr(self):
        try:
            return self.val["mpValue"]
        except Exception:
            return None

    def _refcount(self):
        try:
            return self.val["mpRefCount"]
        except Exception:
            return None

    def to_string(self) -> str:
        rc = self._refcount()
        p = self._ptr()
        try:
            if rc is not None and not _is_null_ptr(rc) and _int(rc["mRefCount"]) > 0 and p is not None:
                return f"eastl::weak_ptr({p})"
        except Exception:
            pass
        return "eastl::weak_ptr(nullptr)"

    def children(self):
        rc = self._refcount()
        p = self._ptr()
        if rc is not None:
            yield "[refcount]", rc
        if p is not None:
            yield "[pointer]", p


# ============================================================================
# Utility Type Printers
# ============================================================================

class _EastlPairPrinter:
    def __init__(self, val):
        self.val = val

    def to_string(self) -> str:
        try:
            return f"({self.val['first']}, {self.val['second']})"
        except Exception:
            return "eastl::pair<?>"

    def children(self):
        try:
            yield "first", self.val["first"]
            yield "second", self.val["second"]
        except Exception:
            return


# ============================================================================
# String View Printer
# ============================================================================

class _EastlBasicStringViewPrinter:
    def __init__(self, val):
        self.val = val

    def _ptr_len(self):
        p = self.val["mpBegin"]
        n = _int(self.val["mnCount"])
        return p, n

    def to_string(self) -> str:
        try:
            p, n = self._ptr_len()
        except Exception:
            return "eastl::basic_string_view<?>"

        value_type = _strip(_strip(self.val.type).template_argument(0))
        elem_size = int(getattr(value_type, "sizeof", 1)) or 1
        s = _try_read_string(p, n, elem_size)
        if s is not None:
            return s
        return f"eastl::basic_string_view(len={n})"

    def display_hint(self):
        return "string"

    def children(self):
        try:
            p, n = self._ptr_len()
        except Exception:
            return
        yield "[data]", p
        yield "[length]", _value(n)


# ============================================================================
# Deque and Queue Printers
# ============================================================================

class _EastlDequeBasePrinter:
    def __init__(self, val):
        self.val = val

    def _subarray_size(self) -> int:
        # DequeBase<T, Allocator, unsigned kDequeSubarraySize>
        try:
            k = int(_strip(self.val.type).template_argument(2))
            if k > 0:
                return k
        except Exception:
            pass
        try:
            return int(self.val["kSubarraySize"])
        except Exception:
            return 0

    def _size(self) -> int:
        k = self._subarray_size()
        if k <= 0:
            return 0
        b = self.val["mItBegin"]
        e = self.val["mItEnd"]
        return _int((e["mpCurrentArrayPtr"] - b["mpCurrentArrayPtr"]) * k + (e["mpCurrent"] - e["mpBegin"]) - (b["mpCurrent"] - b["mpBegin"]))

    def to_string(self) -> str:
        try:
            return f"eastl::deque(size={max(0, self._size())})"
        except Exception:
            return "eastl::deque<?>"

    def children(self):
        k = self._subarray_size()
        if k <= 0:
            return
        b = self.val["mItBegin"]
        arr = b["mpCurrentArrayPtr"]
        begin_offset = _int(b["mpCurrent"] - b["mpBegin"])
        n = max(0, self._size())
        yield "[size]", _value(n)
        for i in range(n):
            off = begin_offset + i
            try:
                block_ptr = (arr + (off // k)).dereference()  # T*
                yield f"[{i}]", (block_ptr + (off % k)).dereference()
            except Exception:
                return


class _EastlDequePrinter:
    def __init__(self, val):
        self.val = val
        self.base = _find_base_cast(val, "eastl::DequeBase<")

    def to_string(self):
        if self.base is None:
            return "eastl::deque<?>"
        return _EastlDequeBasePrinter(self.base).to_string()

    def children(self):
        if self.base is None:
            return
        yield from _EastlDequeBasePrinter(self.base).children()


class _EastlDequeIteratorPrinter:
    def __init__(self, val):
        self.val = val

    def _subarray_size(self) -> int:
        try:
            # DequeIterator<T, Pointer, Reference, unsigned kDequeSubarraySize>
            k = int(_strip(self.val.type).template_argument(3))
            return k
        except Exception:
            return 0

    def to_string(self):
        try:
            return str(self.val["mpCurrent"].dereference())
        except Exception:
            return "eastl::DequeIterator<?>"

    def children(self):
        try:
            cur = self.val["mpCurrent"]
            begin = self.val["mpBegin"]
            end = self.val["mpEnd"]
            arr = self.val["mpCurrentArrayPtr"]
        except Exception:
            return

        v = _safe_deref(cur)
        if v is not None:
            yield "Value", v

        k = self._subarray_size()
        try:
            if cur == begin:
                if k > 0:
                    prev_block = (arr - 1).dereference()
                    yield "Previous", (prev_block + (k - 1)).dereference()
            else:
                yield "Previous", (cur - 1).dereference()
        except Exception:
            pass

        try:
            if cur + 1 == end:
                next_block = (arr + 1).dereference()
                yield "Next", next_block.dereference()
            else:
                yield "Next", (cur + 1).dereference()
        except Exception:
            pass

        try:
            yield "Begin", _value(1 if cur == begin else 0)
            yield "End", _value(1 if cur + 1 == end else 0)
        except Exception:
            return


class _EastlQueuePrinter:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        try:
            return str(self.val["c"])
        except Exception:
            return "eastl::queue<?>"

    def children(self):
        try:
            yield "c", self.val["c"]
        except Exception:
            return


# ============================================================================
# Linked List Printers
# ============================================================================

class _EastlListNodePrinter:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        try:
            return str(self.val["mValue"])
        except Exception:
            return "eastl::ListNode<?>"

    def children(self):
        try:
            yield "Value", self.val["mValue"]
        except Exception:
            return
        try:
            yield "Next", self.val["mpNext"]
        except Exception:
            pass
        try:
            yield "Previous", self.val["mpPrev"]
        except Exception:
            pass


class _EastlListIteratorPrinter:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        try:
            return str(self.val["mpNode"].dereference())
        except Exception:
            return "eastl::ListIterator<?>"

    def children(self):
        try:
            yield "node", self.val["mpNode"]
        except Exception:
            return


# ============================================================================
# Singly Linked List Printers
# ============================================================================

class _EastlSListBasePrinter:
    def __init__(self, val):
        self.val = val

    def _node_ptr_type(self):
        try:
            value_type = _strip(_strip(self.val.type).template_argument(0))
            node_t = gdb.lookup_type(f"eastl::SListNode<{_tag(value_type)}>")
            return node_t.pointer()
        except Exception:
            return None

    def _head(self):
        try:
            base_node = _compressed_pair_first(self.val["mNodeAllocator"])
            return base_node["mpNext"]
        except Exception:
            return None

    def to_string(self):
        return "eastl::slist"  # size is O(n); avoid walking in to_string.

    def children(self):
        head = self._head()
        if head is None or _is_null_ptr(head):
            return

        node_ptr_t = self._node_ptr_type()
        if node_ptr_t is None:
            return

        # Walk until nullptr.
        i = 0
        node = head.cast(node_ptr_t)
        while node is not None and not _is_null_ptr(node):
            try:
                yield f"[{i}]", node.dereference()["mValue"]
            except Exception:
                return
            i += 1
            try:
                node = node.dereference()["mpNext"].cast(node_ptr_t)
            except Exception:
                return


class _EastlSListPrinter:
    def __init__(self, val):
        self.val = val
        self.base = _find_base_cast(val, "eastl::SListBase<")

    def to_string(self):
        if self.base is None:
            return "eastl::slist<?>"
        return _EastlSListBasePrinter(self.base).to_string()

    def children(self):
        if self.base is None:
            return
        yield from _EastlSListBasePrinter(self.base).children()


class _EastlFixedSListPrinter(_DelegatingDerivedPrinter):
    def __init__(self, val):
        super().__init__(val, "eastl::slist<", _EastlSListPrinter, "eastl::fixed_slist")


class _EastlSListNodePrinter:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        try:
            return str(self.val["mValue"])
        except Exception:
            return "eastl::SListNode<?>"

    def children(self):
        try:
            yield "Value", self.val["mValue"]
        except Exception:
            return
        try:
            yield "Next", self.val["mpNext"]
        except Exception:
            return


class _EastlSListIteratorPrinter:
    def __init__(self, val):
        self.val = val

    def _node_ptr_type(self):
        try:
            value_type = _strip(_strip(self.val.type).template_argument(0))
            return gdb.lookup_type(f"eastl::SListNode<{value_type}>").pointer()
        except Exception:
            return None

    def _node_ptr(self):
        try:
            node = self.val["mpNode"]
        except Exception:
            return None

        try:
            node_type = self._node_ptr_type()
            if node_type is not None:
                return node.cast(node_type)
        except Exception:
            pass
        return node

    def to_string(self):
        try:
            node = self._node_ptr()
            if node is None:
                return "eastl::SListIterator<?>"
            return str(node.dereference())
        except Exception:
            return "eastl::SListIterator<?>"

    def children(self):
        node = self._node_ptr()
        if node is None:
            return

        yield "node", node
        try:
            yield "Value", node.dereference()["mValue"]
        except Exception:
            return
        try:
            yield "Next", node.dereference()["mpNext"]
        except Exception:
            return


# ============================================================================
# Intrusive List Printers
# ============================================================================

class _EastlIntrusiveListBasePrinter:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        return "eastl::intrusive_list"

    def children(self):
        try:
            anchor = self.val["mAnchor"]
            node = anchor["mpNext"]
        except Exception:
            return

        i = 0
        while node is not None and int(node) != int(anchor.address):
            try:
                yield f"[{i}]", node.dereference()
            except Exception:
                return
            i += 1
            try:
                node = node.dereference()["mpNext"]
            except Exception:
                return


class _EastlIntrusiveListIteratorPrinter:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        try:
            return str(self.val["mpNode"])
        except Exception:
            return "eastl::intrusive_list_iterator<?>"

    def children(self):
        try:
            yield "node", self.val["mpNode"]
        except Exception:
            return


# ============================================================================
# Hash Node and Iterator Printers
# ============================================================================

class _EastlHashNodePrinter:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        try:
            return str(self.val["mValue"])
        except Exception:
            return "eastl::hash_node<?>"

    def children(self):
        try:
            yield "mValue", self.val["mValue"]
        except Exception:
            return
        try:
            yield "mpNext", self.val["mpNext"]
        except Exception:
            return


class _EastlHashtableIteratorBasePrinter:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        try:
            return str(self.val["mpNode"].dereference()["mValue"])
        except Exception:
            return "eastl::hashtable_iterator_base<?>"

    def children(self):
        try:
            yield "value", self.val["mpNode"].dereference()["mValue"]
        except Exception:
            return


# ============================================================================
# Reverse Iterator Printer
# ============================================================================

class _EastlReverseIteratorPrinter:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        try:
            it = self.val["mIterator"]
            return str((it - 1).dereference())
        except Exception:
            return "eastl::reverse_iterator<?>"

    def children(self):
        try:
            it = self.val["mIterator"]
            yield "base", it
            yield "value", (it - 1).dereference()
        except Exception:
            return


# ============================================================================
# Bitset Printer
# ============================================================================

class _EastlBitsetPrinter:
    def __init__(self, val):
        self.val = val

    def _size(self) -> int:
        try:
            return int(_strip(self.val.type).template_argument(0))
        except Exception:
            return 0

    def _word_type(self):
        try:
            return _strip(_strip(self.val.type).template_argument(1))
        except Exception:
            return None

    def _bits_per_word(self) -> int:
        wt = self._word_type()
        try:
            if wt is not None:
                return int(getattr(wt, "sizeof", 0) * 8)
        except Exception:
            pass
        return 0

    def _words(self):
        # bitset derives from BitsetBase<NW, WordType> which stores word_type mWord[NW].
        try:
            return self.val["mWord"]
        except Exception:
            pass

        try:
            base = _find_base_cast(self.val, "eastl::BitsetBase<")
            if base is not None:
                return base["mWord"]
        except Exception:
            pass
        return None

    def _iter_set_bits(self, limit: int = 64):
        n = self._size()
        b = self._bits_per_word()
        words = self._words()
        if n <= 0 or b <= 0 or words is None:
            return [], 0

        try:
            mask = (1 << b) - 1
        except Exception:
            mask = None

        set_bits = []
        total = 0

        try:
            word_count = (n + b - 1) // b
            word_count = max(1, word_count)
        except Exception:
            word_count = 1

        for wi in range(word_count):
            try:
                w = int(words[wi])
            except Exception:
                break

            if mask is not None:
                w &= mask

            while w:
                lsb = w & -w
                bit = int(lsb.bit_length() - 1)
                idx = wi * b + bit
                if idx >= n:
                    break
                total += 1
                if len(set_bits) < limit:
                    set_bits.append(idx)
                w ^= lsb

        return set_bits, total

    def to_string(self):
        n = self._size()
        bits, total = self._iter_set_bits(limit=16)
        if total == 0:
            return f"eastl::bitset(count={n}, set={{}})"
        shown = ",".join(str(i) for i in bits)
        suffix = "" if total <= len(bits) else f",... (+{total - len(bits)})"
        return f"eastl::bitset(count={n}, set={{{shown}{suffix}}})"

    def children(self):
        n = self._size()
        b = self._bits_per_word()
        words = self._words()
        if n <= 0 or b <= 0 or words is None:
            return
        yield "[count]", _value(n)

        # For small bitsets, expose every bit as an array.
        if n <= 256:
            for i in range(n):
                try:
                    word = int(words[i // b])
                    bit = (word >> (i % b)) & 1
                    yield f"[{i}]", _value(bit)
                except Exception:
                    return
            return

        # For large bitsets, list only set indices (much more usable).
        bits, total = self._iter_set_bits(limit=128)
        yield "[set_count]", _value(total)
        for i, idx in enumerate(bits):
            yield f"[set][{i}]", _value(idx)


# ============================================================================
# Ring Buffer Printer
# ============================================================================

class _EastlRingBufferPrinter:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        try:
            return str(self.val["c"])
        except Exception:
            return "eastl::ring_buffer<?>"

    def children(self):
        try:
            yield "c", self.val["c"]
        except Exception:
            return


# ============================================================================
# Compressed Pair Printer
# ============================================================================

class _EastlCompressedPairImpPrinter:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        try:
            a = self.val["mFirst"]
        except Exception:
            a = None
        try:
            b = self.val["mSecond"]
        except Exception:
            b = None
        if a is not None and b is not None:
            return f"({a}, {b})"
        if a is not None:
            return f"({a})"
        if b is not None:
            return f"({b})"
        return "(empty)"

    def children(self):
        try:
            yield "first", self.val["mFirst"]
        except Exception:
            pass
        try:
            yield "second", self.val["mSecond"]
        except Exception:
            pass


# ============================================================================
# Optional Printer
# ============================================================================

class _EastlOptionalPrinter:
    def __init__(self, val):
        self.val = val

    def _engaged(self) -> bool:
        try:
            return bool(self.val["engaged"])
        except Exception:
            return False

    def _value(self):
        t = _strip(_strip(self.val.type).template_argument(0))
        try:
            storage = self.val["val"]
            p = storage.address.cast(t.pointer())
            return p.dereference()
        except Exception:
            return None

    def to_string(self):
        if not self._engaged():
            return "nullopt"
        v = self._value()
        return str(v) if v is not None else "eastl::optional<?>"

    def children(self):
        if not self._engaged():
            return
        v = self._value()
        if v is not None:
            yield "value", v


# ============================================================================
# Ratio and Chrono Printers
# ============================================================================

class _EastlRatioPrinter:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        try:
            t = _strip(self.val.type)
            return f"{t.template_argument(0)} to {t.template_argument(1)}"
        except Exception:
            return "eastl::ratio<?>"


class _EastlChronoDurationPrinter:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        try:
            return f"{self.val['mRep']}"
        except Exception:
            return "eastl::chrono::duration<?>"

    def children(self):
        try:
            yield "rep", self.val["mRep"]
        except Exception:
            return


# ============================================================================
# Function and Utility Wrapper Printers
# ============================================================================

class _EastlFunctionPrinter:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        try:
            if _is_null_ptr(self.val["mInvokeFuncPtr"]):
                return "empty"
            return str(self.val["mInvokeFuncPtr"])
        except Exception:
            return "eastl::function<?>"


class _EastlReferenceWrapperPrinter:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        try:
            return str(self.val["val"].dereference())
        except Exception:
            return "eastl::reference_wrapper<?>"


class _EastlAnyPrinter:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        try:
            if _is_null_ptr(self.val["m_handler"]):
                return "empty"
            return str(self.val["m_storage"]["external_storage"])
        except Exception:
            return "eastl::any<?>"


# ============================================================================
# Atomic Printers
# ============================================================================

class _EastlAtomicPrinter:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        try:
            return str(self.val["mAtomic"])
        except Exception:
            return "eastl::atomic<?>"

    def children(self):
        try:
            yield "mAtomic", self.val["mAtomic"]
        except Exception:
            return


class _EastlAtomicFlagPrinter:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        try:
            return str(self.val["mFlag"]["mAtomic"])
        except Exception:
            return "eastl::atomic_flag"


# ============================================================================
# Variant and Tuple Printers
# ============================================================================

class _EastlVariantPrinter:
    def __init__(self, val):
        self.val = val

    def _index(self) -> int:
        try:
            return int(self.val["mIndex"])
        except Exception:
            return -1

    def _value(self):
        idx = self._index()
        if idx < 0:
            return None
        try:
            t = _strip(_strip(self.val.type).template_argument(idx))
        except Exception:
            return None
        try:
            storage = self.val["mStorage"]["mBuffer"]["mCharData"]
            p = storage.address.cast(t.pointer())
            return p.dereference()
        except Exception:
            return None

    def to_string(self):
        idx = self._index()
        if idx < 0:
            return "[valueless_by_exception]"
        v = self._value()
        return f"eastl::variant(index={idx}, value={v})" if v is not None else f"eastl::variant(index={idx})"

    def children(self):
        idx = self._index()
        yield "index", _value(idx)
        v = self._value()
        if v is not None:
            yield "value", v


def _tuple_leaf_value(leaf_val, value_type):
    try:
        return leaf_val["mValue"]
    except Exception:
        pass
    try:
        return leaf_val.cast(value_type)
    except Exception:
        return leaf_val


class _EastlTuplePrinter:
    def __init__(self, val):
        self.val = val

    def _arity(self) -> int:
        t = _strip(self.val.type)
        n = 0
        while True:
            try:
                _ = t.template_argument(n)
                n += 1
            except Exception:
                return n

    def to_string(self):
        return f"eastl::tuple(size={self._arity()})"

    def children(self):
        try:
            impl = self.val["mImpl"]
        except Exception:
            return

        n = self._arity()
        for i in range(n):
            try:
                value_type = _strip(_strip(self.val.type).template_argument(i))
            except Exception:
                continue

            leaf = None
            for f in _strip(impl.type).fields():
                if not getattr(f, "is_base_class", False):
                    continue
                bt = _strip(f.type)
                name = _tag(bt)
                if name.startswith(f"eastl::Internal::TupleLeaf<{i},"):
                    try:
                        leaf = impl.cast(bt)
                        break
                    except Exception:
                        leaf = None
                        break

            if leaf is None:
                continue

            yield f"[{i}]", _tuple_leaf_value(leaf, value_type)


# ============================================================================
# Vector and Array Printers
# ============================================================================

class _EastlVectorBasePrinter:
    def __init__(self, val):
        self.val = val

    def _begin_end_cap(self):
        mp_begin = self.val["mpBegin"]
        mp_end = self.val["mpEnd"]
        cap_ptr = _compressed_pair_first(self.val["mCapacityAllocator"])
        return mp_begin, mp_end, cap_ptr

    def to_string(self) -> str:
        try:
            b, e, _c = self._begin_end_cap()
            n = _int(e - b)
            return f"eastl::vector(size={n})"
        except Exception:
            return "eastl::vector<?>"

    def display_hint(self):
        return "array"

    def children(self):
        try:
            b, e, c = self._begin_end_cap()
            n = max(0, _int(e - b))
            cap = max(0, _int(c - b))
        except Exception:
            return

        yield "[size]", _value(n)
        yield "[capacity]", _value(cap)
        for name, child in _iterable_children_from_ptr(b, n):
            yield name, child


class _EastlVectorPrinter:
    def __init__(self, val):
        self.val = val
        self.base = _find_base_cast(val, "eastl::VectorBase<")

    def to_string(self):
        if self.base is None:
            return "eastl::vector<?>"
        return _EastlVectorBasePrinter(self.base).to_string()

    def display_hint(self):
        return "array"

    def children(self):
        if self.base is None:
            return
        yield from _EastlVectorBasePrinter(self.base).children()


class _EastlFixedVectorPrinter(_DelegatingDerivedPrinter):
    def __init__(self, val):
        super().__init__(val, "eastl::vector<", _EastlVectorPrinter, "eastl::fixed_vector")


# ============================================================================
# Vector and Array Printers
# ============================================================================

class _EastlArrayPrinter:
    def __init__(self, val):
        self.val = val

    def _size(self) -> int:
        try:
            # eastl::array<T, N>
            n = int(_strip(self.val.type).template_argument(1))
            return max(0, n)
        except Exception:
            return 0

    def to_string(self) -> str:
        return f"eastl::array(size={self._size()})"

    def display_hint(self):
        return "array"

    def children(self):
        n = self._size()
        if n <= 0:
            return

        try:
            value_type = _strip(_strip(self.val.type).template_argument(0))
        except Exception:
            value_type = None

        try:
            # mValue is a C-array member: value_type mValue[N].
            # In GDB Python, mValue.address is a pointer-to-array (value_type (*)[N]).
            # Cast to value_type* so pointer arithmetic advances by elements.
            p = self.val["mValue"].address
            if value_type is not None:
                p = p.cast(value_type.pointer())
        except Exception:
            return
        for name, child in _iterable_children_from_ptr(p, n):
            yield name, child


class _EastlSpanPrinter:
    def __init__(self, val):
        self.val = val

    def _size_t_max(self):
        try:
            t = gdb.lookup_type("size_t")
            bits = int(getattr(t, "sizeof", 8) * 8)
            return (1 << bits) - 1
        except Exception:
            # Best-effort fallback for typical 64-bit targets.
            return (1 << 64) - 1

    def _looks_like_dynamic_extent(self, extent: int) -> bool:
        # EASTL span uses dynamic_extent == size_t(-1).
        if extent < 0:
            return True
        if extent == self._size_t_max():
            return True
        return False

    def _data_size(self):
        # span has mStorage.mpData + size (dynamic) or compile-time size.
        try:
            p = self.val["mStorage"]["mpData"]
        except Exception:
            p = None

        try:
            extent = int(_strip(self.val.type).template_argument(1))
            if (extent >= 0) and (not self._looks_like_dynamic_extent(extent)):
                # Sanity cap: if we somehow got an absurd static extent, treat it as dynamic.
                if extent <= (1 << 30):
                    return p, extent
        except Exception:
            pass

        try:
            return p, _int(self.val["mStorage"]["mnSize"])
        except Exception:
            return p, 0

    def to_string(self) -> str:
        _p, n = self._data_size()
        return f"eastl::span(size={n})"

    def display_hint(self):
        return "array"

    def children(self):
        p, n = self._data_size()
        if p is None or _is_null_ptr(p):
            return
        for name, child in _iterable_children_from_ptr(p, n):
            yield name, child


# ============================================================================
# Doubly Linked List Printers
# ============================================================================

class _EastlListBasePrinter:
    def __init__(self, val):
        self.val = val

    def _anchor(self):
        # compressed_pair<base_node_type, allocator_type> mNodeAllocator
        base_node = _compressed_pair_first(self.val["mNodeAllocator"])
        return base_node

    def to_string(self) -> str:
        # size can be O(n); list optionally caches.
        try:
            if "mSize" in [f.name for f in _strip(self.val.type).fields()]:
                n = _int(self.val["mSize"])
                return f"eastl::list(size={n})"
        except Exception:
            pass
        return "eastl::list( size=? )"

    def children(self):
        anchor = self._anchor()
        anchor_ptr = anchor.address
        cur = anchor["mpNext"]

        try:
            value_type = _strip(_strip(self.val.type).template_argument(0))
            node_t = gdb.lookup_type(f"eastl::ListNode<{_tag(value_type)}>")
            node_ptr_t = node_t.pointer()
        except Exception:
            node_ptr_t = None

        i = 0
        # Prevent debugger hangs on corrupted lists.
        limit = 10000
        while not _is_null_ptr(cur) and int(cur) != int(anchor_ptr) and i < limit:
            try:
                if node_ptr_t is not None:
                    node = cur.cast(node_ptr_t).dereference()
                    yield f"[{i}]", node["mValue"]
                else:
                    yield f"[{i}]", cur
            except Exception:
                return
            try:
                cur = cur.dereference()["mpNext"]
            except Exception:
                return
            i += 1


class _EastlListPrinter:
    def __init__(self, val):
        self.val = val
        self.base = _find_base_cast(val, "eastl::ListBase<")

    def to_string(self):
        if self.base is None:
            return "eastl::list<?>"
        return _EastlListBasePrinter(self.base).to_string()

    def children(self):
        if self.base is None:
            return
        yield from _EastlListBasePrinter(self.base).children()


class _EastlFixedListPrinter(_DelegatingDerivedPrinter):
    def __init__(self, val):
        super().__init__(val, "eastl::list<", _EastlListPrinter, "eastl::fixed_list")


# ============================================================================
# Tree (Red-Black Tree) and Associative Container Printers
# ============================================================================

class _EastlRBTreePrinter:
    def __init__(self, val):
        self.val = val

    def _node_value_type(self):
        try:
            # rbtree<Key, Value, ...> -> Value is template arg 1.
            return _strip(_strip(self.val.type).template_argument(1))
        except Exception:
            return None

    def _node_ptr_type(self):
        vt = self._node_value_type()
        if vt is None:
            return None
        try:
            node_t = gdb.lookup_type(f"eastl::rbtree_node<{_tag(vt)}>")
            return node_t.pointer()
        except Exception:
            return None

    def _is_map_like(self) -> bool:
        # Heuristic: map/multimap store pair<...> values.
        try:
            vt = self._node_value_type()
            if vt is None:
                return False
            return _tag(vt).startswith("eastl::pair<")
        except Exception:
            return False

    def to_string(self) -> str:
        try:
            n = _int(self.val["mnSize"])
            return f"eastl::rbtree(size={n})"
        except Exception:
            return "eastl::rbtree<?>"

    def children(self):
        try:
            size = max(0, _int(self.val["mnSize"]))
            anchor = self.val["mAnchor"]
            anchor_ptr = anchor.address
            cur = anchor["mpNodeLeft"]
        except Exception:
            return

        yield "[size]", _value(size)

        node_ptr_t = self._node_ptr_type()
        map_like = self._is_map_like()

        # In-order successor traversal.
        def leftmost(p):
            while not _is_null_ptr(p):
                try:
                    q = p.dereference()["mpNodeLeft"]
                except Exception:
                    break
                if _is_null_ptr(q):
                    break
                p = q
            return p

        def successor(p):
            d = p.dereference()
            r = d["mpNodeRight"]
            if not _is_null_ptr(r):
                return leftmost(r)
            while True:
                parent = d["mpNodeParent"]
                if _is_null_ptr(parent):
                    return parent
                if int(parent) == int(anchor_ptr):
                    return parent
                pd = parent.dereference()
                if int(p) == int(pd["mpNodeLeft"]):
                    return parent
                p = parent
                d = pd

        i = 0
        limit = min(size + 2, 200000)  # avoid hangs on corruption
        while not _is_null_ptr(cur) and int(cur) != int(anchor_ptr) and i < limit:
            try:
                if node_ptr_t is None:
                    yield f"[{i}]", cur
                else:
                    node = cur.cast(node_ptr_t).dereference()
                    v = node["mValue"]
                    if map_like:
                        try:
                            k = v["first"]
                            yield f"[{k}]", v["second"]
                        except Exception:
                            yield f"[{i}]", v
                    else:
                        yield f"[{i}]", v
            except Exception:
                return
            cur = successor(cur)
            i += 1


class _EastlMapSetPrinter:
    def __init__(self, val):
        self.val = val
        self.base = _find_base_cast(val, "eastl::rbtree<")

    def to_string(self):
        if self.base is None:
            return "eastl::(map/set)<?>"
        return _EastlRBTreePrinter(self.base).to_string()

    def children(self):
        if self.base is None:
            return
        yield from _EastlRBTreePrinter(self.base).children()


class _EastlFixedMapPrinter(_DelegatingDerivedPrinter):
    def __init__(self, val):
        super().__init__(val, "eastl::map<", _EastlMapSetPrinter, "eastl::fixed_map")


class _EastlFixedMultiMapPrinter(_DelegatingDerivedPrinter):
    def __init__(self, val):
        super().__init__(val, "eastl::multimap<", _EastlMapSetPrinter, "eastl::fixed_multimap")


class _EastlFixedSetPrinter(_DelegatingDerivedPrinter):
    def __init__(self, val):
        super().__init__(val, "eastl::set<", _EastlMapSetPrinter, "eastl::fixed_set")


class _EastlFixedMultiSetPrinter(_DelegatingDerivedPrinter):
    def __init__(self, val):
        super().__init__(val, "eastl::multiset<", _EastlMapSetPrinter, "eastl::fixed_multiset")


# ============================================================================
# Hash Table Printer
# ============================================================================

class _EastlHashtablePrinter:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        try:
            n = _int(self.val["mnElementCount"])
            b = _int(self.val["mnBucketCount"])
            return f"eastl::hashtable(size={n}, buckets={b})"
        except Exception:
            return "eastl::hashtable<?>"

    def children(self):
        try:
            bucket_count = max(0, _int(self.val["mnBucketCount"]))
            elem_count = max(0, _int(self.val["mnElementCount"]))
            buckets = self.val["mpBucketArray"]
        except Exception:
            return

        yield "[size]", _value(elem_count)
        yield "[bucket_count]", _value(bucket_count)

        # If this is a hash_map-like hashtable, attempt key/value naming.
        is_map_like = False
        try:
            value_t = _strip(_strip(self.val.type).template_argument(0))
            is_map_like = _tag(value_t).startswith("eastl::pair<")
        except Exception:
            pass

        i = 0
        for b in range(bucket_count):
            try:
                entry = (buckets + b).dereference()
            except Exception:
                return
            while not _is_null_ptr(entry):
                try:
                    v = entry.dereference()["mValue"]
                    if is_map_like:
                        try:
                            k = v["first"]
                            yield f"[{k}]", v["second"]
                        except Exception:
                            yield f"[{i}]", v
                    else:
                        yield f"[{i}]", v
                except Exception:
                    return
                i += 1
                try:
                    entry = entry.dereference()["mpNext"]
                except Exception:
                    break


class _EastlHashContainerPrinter:
    def __init__(self, val):
        self.val = val
        self.base = _find_base_cast(val, "eastl::hashtable<")

    def to_string(self):
        if self.base is None:
            return "eastl::hash_*<?>"
        return _EastlHashtablePrinter(self.base).to_string()

    def children(self):
        if self.base is None:
            return
        yield from _EastlHashtablePrinter(self.base).children()


class _EastlFixedHashMapPrinter(_DelegatingDerivedPrinter):
    def __init__(self, val):
        super().__init__(val, "eastl::hash_map<", _EastlHashContainerPrinter, "eastl::fixed_hash_map")


class _EastlFixedHashMultiMapPrinter(_DelegatingDerivedPrinter):
    def __init__(self, val):
        super().__init__(val, "eastl::hash_multimap<", _EastlHashContainerPrinter, "eastl::fixed_hash_multimap")


class _EastlFixedHashSetPrinter(_DelegatingDerivedPrinter):
    def __init__(self, val):
        super().__init__(val, "eastl::hash_set<", _EastlHashContainerPrinter, "eastl::fixed_hash_set")


class _EastlFixedHashMultiSetPrinter(_DelegatingDerivedPrinter):
    def __init__(self, val):
        super().__init__(val, "eastl::hash_multiset<", _EastlHashContainerPrinter, "eastl::fixed_hash_multiset")


# ============================================================================
# Printer Registration
# ============================================================================

_PRINTERS = None


def build_eastl_pretty_printer():
    pp = gdb.printing.RegexpCollectionPrettyPrinter("eastl")

    # Smart pointers
    pp.add_printer("unique_ptr", r"^eastl::unique_ptr<.*>$", _EastlUniquePtrPrinter)
    pp.add_printer("shared_ptr", r"^eastl::shared_ptr<.*>$", _EastlSharedPtrPrinter)
    pp.add_printer("weak_ptr", r"^eastl::weak_ptr<.*>$", _EastlWeakPtrPrinter)

    # Utilities
    pp.add_printer("pair", r"^eastl::pair<.*>$", _EastlPairPrinter)
    pp.add_printer("reference_wrapper", r"^eastl::reference_wrapper<.*>$", _EastlReferenceWrapperPrinter)
    pp.add_printer("optional", r"^eastl::optional<.*>$", _EastlOptionalPrinter)
    pp.add_printer("compressed_pair_imp", r"^eastl::compressed_pair_imp<.*>$", _EastlCompressedPairImpPrinter)

    # Strings
    pp.add_printer("basic_string", r"^eastl::basic_string<.*>$", _EastlBasicStringPrinter)
    pp.add_printer("basic_string_view", r"^eastl::basic_string_view<.*>$", _EastlBasicStringViewPrinter)

    # Fixed strings
    pp.add_printer("fixed_string", r"^eastl::fixed_string<.*>$", _EastlFixedStringPrinter)
    pp.add_printer("fixed_substring", r"^eastl::fixed_substring<.*>$", _EastlFixedSubstringPrinter)

    # Arrays / views
    pp.add_printer("array", r"^eastl::array<.*>$", _EastlArrayPrinter)
    pp.add_printer("span", r"^eastl::span<.*>$", _EastlSpanPrinter)

    # Deques / adaptors
    pp.add_printer("DequeBase", r"^eastl::DequeBase<.*>$", _EastlDequeBasePrinter)
    pp.add_printer("deque", r"^eastl::deque<.*>$", _EastlDequePrinter)
    pp.add_printer("DequeIterator", r"^eastl::DequeIterator<.*>$", _EastlDequeIteratorPrinter)
    pp.add_printer("queue", r"^eastl::queue<.*>$", _EastlQueuePrinter)
    pp.add_printer("stack", r"^eastl::stack<.*>$", _EastlQueuePrinter)
    pp.add_printer("priority_queue", r"^eastl::priority_queue<.*>$", _EastlQueuePrinter)

    # Vectors
    pp.add_printer("VectorBase", r"^eastl::VectorBase<.*>$", _EastlVectorBasePrinter)
    pp.add_printer("vector", r"^eastl::vector<.*>$", _EastlVectorPrinter)

    # Fixed vector
    pp.add_printer("fixed_vector", r"^eastl::fixed_vector<.*>$", _EastlFixedVectorPrinter)

    # Lists
    pp.add_printer("ListBase", r"^eastl::ListBase<.*>$", _EastlListBasePrinter)
    pp.add_printer("list", r"^eastl::list<.*>$", _EastlListPrinter)
    pp.add_printer("ListNode", r"^eastl::ListNode<.*>$", _EastlListNodePrinter)
    pp.add_printer("ListIterator", r"^eastl::ListIterator<.*>$", _EastlListIteratorPrinter)

    # Fixed list
    pp.add_printer("fixed_list", r"^eastl::fixed_list<.*>$", _EastlFixedListPrinter)

    # Singly-linked lists
    pp.add_printer("SListBase", r"^eastl::SListBase<.*>$", _EastlSListBasePrinter)
    pp.add_printer("slist", r"^eastl::slist<.*>$", _EastlSListPrinter)
    pp.add_printer("SListNode", r"^eastl::SListNode<.*>$", _EastlSListNodePrinter)
    pp.add_printer("SListIterator", r"^eastl::SListIterator<.*>$", _EastlSListIteratorPrinter)

    # Fixed slist
    pp.add_printer("fixed_slist", r"^eastl::fixed_slist<.*>$", _EastlFixedSListPrinter)

    # Intrusive list
    pp.add_printer("intrusive_list_base", r"^eastl::intrusive_list_base$", _EastlIntrusiveListBasePrinter)
    pp.add_printer("intrusive_list_iterator", r"^eastl::intrusive_list_iterator<.*>$", _EastlIntrusiveListIteratorPrinter)

    # Trees / ordered associative containers
    pp.add_printer("rbtree", r"^eastl::rbtree<.*>$", _EastlRBTreePrinter)
    pp.add_printer("map", r"^eastl::map<.*>$", _EastlMapSetPrinter)
    pp.add_printer("multimap", r"^eastl::multimap<.*>$", _EastlMapSetPrinter)
    pp.add_printer("set", r"^eastl::set<.*>$", _EastlMapSetPrinter)
    pp.add_printer("multiset", r"^eastl::multiset<.*>$", _EastlMapSetPrinter)

    # Fixed ordered containers
    pp.add_printer("fixed_map", r"^eastl::fixed_map<.*>$", _EastlFixedMapPrinter)
    pp.add_printer("fixed_multimap", r"^eastl::fixed_multimap<.*>$", _EastlFixedMultiMapPrinter)
    pp.add_printer("fixed_set", r"^eastl::fixed_set<.*>$", _EastlFixedSetPrinter)
    pp.add_printer("fixed_multiset", r"^eastl::fixed_multiset<.*>$", _EastlFixedMultiSetPrinter)

    # Hash containers
    pp.add_printer("hashtable", r"^eastl::hashtable<.*>$", _EastlHashtablePrinter)
    pp.add_printer("hash_map", r"^eastl::hash_map<.*>$", _EastlHashContainerPrinter)
    pp.add_printer("hash_multimap", r"^eastl::hash_multimap<.*>$", _EastlHashContainerPrinter)
    pp.add_printer("hash_set", r"^eastl::hash_set<.*>$", _EastlHashContainerPrinter)
    pp.add_printer("hash_multiset", r"^eastl::hash_multiset<.*>$", _EastlHashContainerPrinter)

    # Fixed hash containers
    pp.add_printer("fixed_hash_map", r"^eastl::fixed_hash_map<.*>$", _EastlFixedHashMapPrinter)
    pp.add_printer("fixed_hash_multimap", r"^eastl::fixed_hash_multimap<.*>$", _EastlFixedHashMultiMapPrinter)
    pp.add_printer("fixed_hash_set", r"^eastl::fixed_hash_set<.*>$", _EastlFixedHashSetPrinter)
    pp.add_printer("fixed_hash_multiset", r"^eastl::fixed_hash_multiset<.*>$", _EastlFixedHashMultiSetPrinter)

    # Hash iterator/node helpers
    pp.add_printer("hash_node", r"^eastl::hash_node<.*>$", _EastlHashNodePrinter)
    pp.add_printer("hashtable_iterator_base", r"^eastl::hashtable_iterator_base<.*>$", _EastlHashtableIteratorBasePrinter)

    # Iterators
    pp.add_printer("reverse_iterator", r"^eastl::reverse_iterator<.*>$", _EastlReverseIteratorPrinter)

    # Bitset
    pp.add_printer("bitset", r"^eastl::bitset<.*>$", _EastlBitsetPrinter)

    # ring_buffer
    pp.add_printer("ring_buffer", r"^eastl::ring_buffer<.*>$", _EastlRingBufferPrinter)

    # chrono
    pp.add_printer("ratio", r"^eastl::ratio<.*>$", _EastlRatioPrinter)
    pp.add_printer("chrono_duration", r"^eastl::chrono::duration<.*>$", _EastlChronoDurationPrinter)

    # function / any
    pp.add_printer("function", r"^eastl::function<.*>$", _EastlFunctionPrinter)
    pp.add_printer("any", r"^eastl::any$", _EastlAnyPrinter)

    # atomics
    pp.add_printer("atomic", r"^eastl::atomic<.*>$", _EastlAtomicPrinter)
    pp.add_printer("atomic_flag", r"^eastl::atomic_flag$", _EastlAtomicFlagPrinter)

    # variant / tuple
    pp.add_printer("variant", r"^eastl::variant<.*>$", _EastlVariantPrinter)
    pp.add_printer("tuple", r"^eastl::tuple<.*>$", _EastlTuplePrinter)

    return pp


def register_eastl_printers(objfile=None) -> None:
    """Register EASTL pretty printers.

    If objfile is None, registers globally (recommended for interactive use).
    """
    global _PRINTERS
    if _PRINTERS is None:
        _PRINTERS = build_eastl_pretty_printer()

    # Some older GDB builds may not support the 'replace' kwarg.
    try:
        gdb.printing.register_pretty_printer(objfile, _PRINTERS, replace=True)
    except TypeError:
        gdb.printing.register_pretty_printer(objfile, _PRINTERS)


# Convenience: if sourced from .gdbinit, auto-register globally.
try:
    register_eastl_printers()
except Exception as e:
    # Avoid breaking gdb startup on older builds lacking python support.
    gdb.write('Error registering EASTL printers: ' + str(e) + '\n')
