import os
os.makedirs('tests/bytecode_samples', exist_ok=True)

# Kusursuz hesaplanmis FVM payload. 
# Hata duzeltildi: Body size 0x15 yerine 0x16 yapilarak END (0x0b) komutu kapsama alindi.
wasm_hex = "0061736d010000000a1801160041004180808001100041004100360200100140000b"

with open('tests/bytecode_samples/fvm_doomsday_actor.wasm', 'wb') as f:
    f.write(bytes.fromhex(wasm_hex))

print("SUCCESS: 1-byte alignment fixed. Wasm payload generated.")
