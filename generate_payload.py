import os
os.makedirs('tests/bytecode_samples', exist_ok=True)

# Section 2 (Imports: fvm.ipld.put ve fvm.send) ve Section 10 (Code) içeren tam donanımlı Wasm payload
wasm_hex = (
    "0061736d" # Magic Number
    "01000000" # Version
    "021902"   # Section 2 (Import Section, size: 25, 2 imports)
      "0366766d0869706c642e7075740000" # Import 1: module "fvm", field "ipld.put", kind function (type 0)
      "0366766d0473656e640001"         # Import 2: module "fvm", field "send", kind function (type 1)
    "03020101" # Section 3 (Function Section)
    "0503010001" # Section 5 (Memory Section)
    "0a1801150041004180808001100041004100360200100140000b" # Section 10 (Code Section)
)

with open('tests/bytecode_samples/fvm_doomsday_actor.wasm', 'wb') as f:
    f.write(bytes.fromhex(wasm_hex))

print("SUCCESS: FVM Wasm payload with Section 2 Imports generated successfully!")
