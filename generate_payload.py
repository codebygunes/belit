import os

os.makedirs('tests/bytecode_samples', exist_ok=True)

# Derlenmiş zararlı FVM Wasm payload'unun makine kodları (Hex dump)
# İçeriği: fvm.ipld.put (2MB payload ile), fvm.send (izinsiz çağrı) ve Memory Out-of-Bounds (sınır aşımı)
wasm_hex = (
    "0061736d01000000" # Wasm Magic & Version
    "010c0260027f7f017f6000017f" # Type Section
    "0219020366766d0869706c642e70757400000366766d0473656e640001" # Imports: fvm.ipld.put, fvm.send
    "03020101" # Function Section
    "0503010001" # Memory Section
    "070801046d61696e0002" # Export Section
    "0a1c011a004100418080800110001a4100410036020010011a0b" # Code Section (Malicious logic)
)

with open('tests/bytecode_samples/fvm_doomsday_actor.wasm', 'wb') as f:
    f.write(bytes.fromhex(wasm_hex))

print("SUCCESS: fvm_doomsday_actor.wasm has been generated successfully!")
