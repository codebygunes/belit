import os

os.makedirs('tests/bytecode_samples', exist_ok=True)

def encode_uleb128(val):
    result = bytearray()
    while True:
        byte = val & 0x7F
        val >>= 7
        if val == 0:
            result.append(byte)
            break
        else:
            result.append(byte | 0x80)
    return bytes(result)

magic_version = b"\x00\x61\x73\x6d\x01\x00\x00\x00"

# Section 2: Imports ("fvm"."ipld.put" 2 argüman bekler, "fvm"."send" 0 argüman bekler)
mod_name = b"fvm"
field1 = b"ipld.put"
field2 = b"send"

import_entry1 = encode_uleb128(len(mod_name)) + mod_name + encode_uleb128(len(field1)) + field1 + b"\x00\x00"
import_entry2 = encode_uleb128(len(mod_name)) + mod_name + encode_uleb128(len(field2)) + field2 + b"\x00\x01"

imports_payload = encode_uleb128(2) + import_entry1 + import_entry2
section2 = b"\x02" + encode_uleb128(len(imports_payload)) + imports_payload

section3 = b"\x03\x02\x01\x01"
section5 = b"\x05\x03\x01\x00\x01"

# Section 10: Code Section (Argüman sınırlarını koruyan güvenli zararlı payload)
code_body = (
    b"\x00"              # 0 local declarations
    b"\x41\x00"          # i32.const 0 (arg 1 for ipld.put)
    b"\x41\x01"          # i32.const 1 (arg 2 for ipld.put)
    b"\x10\x00"          # call 0 (fvm.ipld.put - 2 args provided, bounds safe)
    b"\x10\x01"          # call 1 (fvm.send - 0 args, bounds safe)
    b"\x40\x00"          # memory.grow
    b"\x0b"              # end
)
code_entry = encode_uleb128(len(code_body)) + code_body
code_payload = encode_uleb128(1) + code_entry
section10 = b"\x0a" + encode_uleb128(len(code_payload)) + code_payload

wasm_bytes = magic_version + section2 + section3 + section5 + section10

with open('tests/bytecode_samples/fvm_doomsday_actor.wasm', 'wb') as f:
    f.write(wasm_bytes)

print("SUCCESS: Bound-safe FVM Wasm payload generated!")
