"""Python bindings for KOML"""

import ctypes
import enum
import typing

koml = ctypes.cdll.LoadLibrary('koml/koml.so')

class _Data(ctypes.Union):
	_fields_ = [
		('i32', ctypes.c_int),
		('f32', ctypes.c_float),
		('string', ctypes.c_char_p),
		('boolean', ctypes.c_ubyte),
	]

class KOMLSymbol(ctypes.Structure):
	_fields_ = [
		('stride', ctypes.c_ulonglong),
		('type', ctypes.c_int),
		('data', _Data),
	]

class KOMLTable(ctypes.Structure):
	_fields_ = [
		('symbols', ctypes.c_void_p),
		('hashes', ctypes.c_void_p),
		('length', ctypes.c_ulonglong),
	]

	def value(self, key: str) -> typing.Any:
		sym_p = koml.koml_table_symbol(ctypes.pointer(ktable), key.encode())
		if sym_p == 0:
			raise KeyError(key)
		sym = ctypes.cast(sym_p, ctypes.POINTER(KOMLSymbol)).contents
		if sym.type == KOMLType.INT:
			return sym.data.i32
		elif sym.type == KOMLType.FLOAT:
			return sym.data.f32
		elif sym.type == KOMLType.STRING:
			return sym.data.string.decode()
		elif sym.type == KOMLType.BOOLEAN:
			return sym.data.boolean
		else:
			raise AssertionError('unhandled type: %r' % sym.type)

class KOMLType(enum.IntEnum):
	UNKNOWN = 0
	INT = 1
	FLOAT = 2
	STRING = 3
	BOOLEAN = 4

def loads(b: bytes) -> KOMLTable:
	ktable = KOMLTable()
	assert koml.koml_table_load(ctypes.pointer(ktable), b, len(b)) == 0
	return ktable

if __name__ == '__main__':
	with open('test.koml', 'rb') as f:
		ktable = loads(f.read())
	print('a = %r; c = %r; f = %r; s = %r; b = %r; test = %r' % (
			ktable.value('section:a'),
			ktable.value('section.sub:c'),
			ktable.value('section:f'),
			ktable.value('section:s'),
			ktable.value('section.sub:b'),
			ktable.value('test')));
