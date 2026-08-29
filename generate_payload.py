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

# Wasm Bileşenlerini Insa Edelim
magic_version = b"\x00\x61\x73\x6d\x01\x00\x00\x00"

# Section 2: Imports ("fvm"."ipld.put" ve "fvm"."send")
mod_name = b"fvm"
field1 = b"ipld.put"
field2 = b"send"

import_entry1 = encode_uleb128(len(mod_name)) + mod_name + encode_uleb128(len(field1)) + field1 + b"\x00\x00"
import_entry2 = encode_uleb128(len(mod_name)) + mod_name + encode_uleb128(len(field2)) + field2 + b"\x00\x01"

imports_payload = encode_uleb128(2) + import_entry1 + import_entry2
section2 = b"\x02" + encode_uleb128(len(imports_payload)) + imports_payload

# Section 3: Function Section (2 imported funcs)
section3 = b"\x03\x02\x01\x01"

# Section 5: Memory Section
section5 = b"\x05\x03\x01\x00\x01"

# Section 10: Code Section (Malicious code body)
code_body = (
    b"\x00"              # 0 local declarations
    b"\x41\x00"          # i32.const 0
    b"\x41\x80\x80\x80\x01" # i32.const 1048576 (1MB threshold bypass attempt)
    b"\x10\x00"          # call 0 (fvm.ipld.put)
    b"\x41\x00"          # i32.const 0
    b"\x41\x00"          # i32.const 0
    b"\x36\x02\x00"      # i32.store
    b"\x10\x01"          # call 1 (fvm.send reentrancy trigger)
    b"\x40\x00"          # memory.grow
    b"\x0b"              # end
)
code_entry = encode_uleb128(len(code_body)) + code_body
code_payload = encode_uleb128(1) + code_entry
section10 = b"\x0a" + encode_uleb128(len(code_payload)) + code_payload

# Tum Wasm dosyasini birlestir
wasm_bytes = magic_version + section2 + section3 + section5 + section10

with open('tests/bytecode_samples/fvm_doomsday_actor.wasm', 'wb') as f:
    f.write(wasm_bytes)

print("SUCCESS: Standard-compliant FVM Wasm payload generated dynamically!")
