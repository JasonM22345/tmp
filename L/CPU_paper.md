# Summary of “CustomProcessingUnit: Reverse Engineering and Customization of Intel Microcode”

---

1. **Topic**  
The paper introduces the CustomProcessingUnit, the first full-featured framework for static and dynamic analysis of Intel x86 microcode (specifically for Goldmont CPUs), enabling reverse engineering, tracing, and customization.

2. **Problem**  
Intel microcode is opaque, encrypted, and signed, preventing third-party auditing or modification, thereby limiting the ability to verify security, understand CPU internals, or explore customization.

3. **Current Status Quo**  
While reverse engineering older or less secure microarchitectures (e.g., Intel P6, AMD Opteron) has been done, modern Intel CPUs are highly protected with encrypted microcode and hidden update processes.

4. **Contribution Overview**  
The authors reverse-engineer the microcode of Intel Goldmont CPUs and reconstruct internal debug interfaces to develop tools for decompilation (Ghidra module) and dynamic patching (UEFI application).

5. **Technical Innovation**  
They introduce CustomProcessingUnit, which supports microcode tracing, patching, and live debugging. It can disassemble, modify, and reroute instruction-level microcode using undocumented instructions (udbgrd, udbgwr).

6. **Reverse-Engineering the Update Algorithm**  
The authors decrypt and reconstruct Intel’s microcode update algorithm, revealing it uses RSA-signed and RC4-encrypted updates with CPU-resident secrets, stored in the L2 cache during decryption.

7. **Case Study 1: Pointer Authentication Code (PAC) for x86**  
They implement PAC functionality using microcode, bringing ARM-style pointer protection to Intel CPUs, and evaluate its security including resistance to PACMAN-style speculative attacks.

8. **Case Study 2: µSoftware Breakpoints**  
A new form of software breakpoint is introduced and implemented directly in microcode, offering approximately 1000x speedup over traditional breakpoints by avoiding OS interrupts and context switches.

9. **Case Study 3: Constant-Time Division**  
The div instruction is patched at the microcode level to execute in constant time, mitigating timing side-channel attacks and outperforming existing software-only constant-time division routines.

10. **Security Analysis**  
The paper examines the robustness of Intel’s microcode encryption and signature validation, finding it mostly secure but noting that decrypted microcode resides temporarily in the cache (potentially exploitable).

11. **Framework Accessibility and Ethics**  
Although the tools are powerful, their functionality is restricted to CPUs in Red Unlock mode, a debug state requiring a known exploit and only achievable on a limited set of devices, minimizing malicious risk.

12. **Conclusion**  
The work democratizes microcode research, offering open-source tools for audit and experimentation. It exposes the hidden microcode layer, evaluates its security, and demonstrates practical benefits from its controlled modification.

---

## Key Technical Terms to Explain

| Term                  | Explanation                                                                 |
|-----------------------|-----------------------------------------------------------------------------|
| Microcode             | A layer between machine instructions and hardware execution, translating complex instructions into simpler micro-operations (µops). |
| Goldmont (GLM)        | A low-power Intel microarchitecture (Atom family) targeted in this study due to known vulnerabilities allowing deeper access. |
| Red Unlock            | A debug mode achieved via exploits that provides low-level access to internal CPU components, bypassing standard protections. |
| udbgrd / udbgwr       | Undocumented Intel instructions used to read/write internal CPU structures, crucial for microcode customization. |
| Match Registers       | CPU registers that redirect execution from default microcode to patched versions stored in RAM. |
| RC4 and RSA           | Cryptographic primitives used by Intel to encrypt and sign microcode updates. |
| CRBUS and LDAT        | Internal buses and interfaces providing access to CPU state and debug data. |
| PAC (Pointer Authentication Code) | A technique (borrowed from ARM) for preventing pointer manipulation by signing pointers cryptographically. |
| PACMAN Attack         | A speculative execution attack that brute-forces valid PACs using side-channel leakage. |
| µSoftware Breakpoints | Novel breakpoints implemented directly in microcode for ultra-low latency debugging. |
| UEFI                  | Unified Extensible Firmware Interface, used here to run early boot-level instrumentation. |
| Ghidra SLEIGH         | A processor specification language used to define instruction semantics in the Ghidra reverse engineering tool. |

---

## Questions a Reader Might Have

1. Scalability: Can the techniques used on Goldmont be extended to other Intel microarchitectures like Skylake, Alder Lake, etc.?
2. Generalizability: How much of the decrypted microcode update logic is shared across CPU generations?
3. Stability: Is there any risk of CPU instability or bricking from applying these microcode patches?
4. Patch Scope: How limited are the 128 triads and 64 hooks in MSRAM for more ambitious microcode rewrites?
5. Tool Integration: Could this be extended into a higher-level scripting interface for prototyping microcode transformations?
6. Real-World Exploitation: Could any nation-state or advanced actor abuse these methods beyond Red Unlock boundaries?
7. Formal Verification: Could microcode be formally verified now that it’s accessible, or is it still too low-level?
8. Vendor Reaction: What are the possible future countermeasures Intel might introduce in response to this level of access?

---

## Future Work Proposed or Implied

- Extension to More Architectures: Port CustomProcessingUnit to newer microarchitectures beyond Goldmont.
- Microcode Rehosting: Simulate or emulate microcode execution for research and testing outside of hardware.
- Security Verification Tools: Build formal verification tools to statically check for microcode-level vulnerabilities.
- Compiler Integration: Develop compilers that output or optimize code considering customizable microcode behavior.
- Custom CPU Features: Implement microcoded features for real-time systems, embedded platforms, or hypervisors.
- Expanding PAC Protections: Enhance and generalize PAC-like mechanisms into broader control-flow integrity systems.

---

## Criticisms of the Paper

1. **Narrow Target Scope**  
The framework is limited to Goldmont-class CPUs that can be Red-Unlocked, a small subset of consumer devices. Broader applicability is unclear.

2. **Risk of Misuse**  
While ethical concerns were addressed, releasing tools that support microcode rewriting could still pose risks if adapted to bypass other protections on more devices.

3. **Limited Validation of Attacks**  
The attempted L2-cache leakage and PACMAN attack replications are informative, but not comprehensive enough to assess risk across all platforms.

4. **Performance Evaluation**  
Case study performance gains (e.g., µsoftware breakpoints and div) are compelling, but broader benchmarks across diverse workloads would strengthen the argument.

5. **PAC Implementation Trade-offs**  
The PAC implementation sacrifices compatibility and may introduce other attack vectors (e.g., Spectre) when PACMAN is mitigated.

6. **Documentation Dependency**  
The reverse engineering relies heavily on prior tools and undocumented behaviors, which may change or disappear in future CPUs.

7. **Patch Reversibility and Debugging**  
Limited discussion is provided on what happens if a microcode patch fails, how is rollback or recovery handled?

---