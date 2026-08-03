from __future__ import annotations

import struct
from typing import Generator, Iterable, List, Tuple

import libfdt


STRING_PROPERTY_NAMES = {
    "bootargs",
    "clock-output-names",
    "compatible",
    "device_type",
    "label",
    "method",
    "model",
    "name",
    "stdout-path",
    "status",
}


class FdtProperty:
    def __init__(self, name: str):
        self._name = name

    def get_name(self) -> str:
        return self._name


class FdtPropertyWords(FdtProperty):
    def __init__(self, name: str, words: Iterable[int]):
        super().__init__(name)
        self.words = tuple(int(w) for w in words)


class FdtPropertyStrings(FdtProperty):
    def __init__(self, name: str, strings: Iterable[str]):
        super().__init__(name)
        self.strings = tuple(strings)


class FdtPropertyBytes(FdtProperty):
    def __init__(self, name: str, data: bytes):
        super().__init__(name)
        self.data = bytes(data)


class FdtNode:
    def __init__(self, name: str):
        self._name = name
        self._items: List[object] = []
        self._children: List[FdtNode] = []

    def get_name(self) -> str:
        return self._name

    def add_item(self, item: object) -> None:
        self._items.append(item)
        if isinstance(item, FdtNode):
            self._children.append(item)

    def __iter__(self):
        return iter(self._items)

    def walk(self, base_path: str = "/") -> Generator[Tuple[str, "FdtNode"], None, None]:
        for child in self._children:
            child_path = _join_path(base_path, child.get_name())
            yield (child_path, child)
            yield from child.walk(child_path)


class Fdt:
    def __init__(self, blob: bytes):
        self._fdt = libfdt.FdtRo(blob)
        self._root = self._build_node(self._fdt.path_offset("/"))

    def get_rootnode(self) -> FdtNode:
        return self._root

    def _build_node(self, node_offset: int) -> FdtNode:
        node = FdtNode(self._fdt.get_name(node_offset))

        try:
            prop_offset = self._fdt.first_property_offset(node_offset, quiet=(libfdt.NOTFOUND,))
        except libfdt.FdtException:
            prop_offset = -1

        while prop_offset >= 0:
            prop = self._fdt.get_property_by_offset(prop_offset)
            node.add_item(_wrap_property(prop.name, bytes(prop)))
            try:
                prop_offset = self._fdt.next_property_offset(prop_offset, quiet=(libfdt.NOTFOUND,))
            except libfdt.FdtException:
                prop_offset = -1

        try:
            child_offset = self._fdt.first_subnode(node_offset, quiet=(libfdt.NOTFOUND,))
        except libfdt.FdtException:
            child_offset = -1

        while child_offset >= 0:
            child = self._build_node(child_offset)
            node.add_item(child)
            try:
                child_offset = self._fdt.next_subnode(child_offset, quiet=(libfdt.NOTFOUND,))
            except libfdt.FdtException:
                child_offset = -1

        return node


class FdtBlobParse:
    def __init__(self, dtb_file):
        self._blob = dtb_file.read()

    def to_fdt(self) -> Fdt:
        return Fdt(self._blob)


def _join_path(base_path: str, name: str) -> str:
    if base_path == "/":
        return "/" + name
    return base_path.rstrip("/") + "/" + name


def _wrap_property(name: str, raw: bytes) -> FdtProperty:
    if len(raw) == 0:
        return FdtPropertyBytes(name, raw)

    if (_is_string_property_name(name) or len(raw) % 4 != 0) and _looks_like_string_property(raw):
        strings = [part.decode("utf-8") for part in raw.rstrip(b"\x00").split(b"\x00")]
        return FdtPropertyStrings(name, strings)

    if len(raw) % 4 == 0:
        words = struct.unpack(">" + "I" * (len(raw) // 4), raw)
        return FdtPropertyWords(name, words)

    return FdtPropertyBytes(name, raw)


def _looks_like_string_property(raw: bytes) -> bool:
    if b"\x00" not in raw:
        return False

    if raw[-1] != 0:
        return False

    try:
        for part in raw.rstrip(b"\x00").split(b"\x00"):
            part.decode("utf-8")
    except UnicodeDecodeError:
        return False
    return True


def _is_string_property_name(name: str) -> bool:
    return (
        name in STRING_PROPERTY_NAMES
        or name.endswith("-names")
        or name.endswith("-name")
        or name.endswith("-path")
    )
