# AMD CPU Microcode (2025-05 snapshot)



## Microcode update mechanisms

* **Boot-time patching via AGESA** – Every BIOS/UEFI firmware embeds an AGESA module that hands each core a patch blob seconds after reset. See AMD’s public change-log for AGESA 1.2.0.3C → fixes Zen 5 signature bug ➡️ [Tom’s Hardware report](https://www.tomshardware.com/pc-components/motherboards/amd-patches-critical-zen-5-microcode-bug-partners-deliver-new-bios-with-agesa-1-2-0-3c) ([Tom's Hardware][1])
* **Early OS loader** – Linux places **`/boot/amd-ucode.img`** first in the initrd list; kernel hook re-programs Patch-RAM before `start_kernel()`. How-to on the [Arch Wiki](https://wiki.archlinux.org/title/Microcode) ([ArchWiki][2])
* **Hot-load on Windows Server** – Microsoft ships signed `mcupdate_GenuineAMD.dll` via Windows Update for EPYC/Ryzen-Pro. AMD explains the flow in security bulletin [AMD-SB-3019](https://www.amd.com/en/resources/product-security/bulletin/amd-sb-3019.html) ([AMD][3])
* **Programming path** – The PSP decrypts and authenticates the blob, then writes the physical base address to **MSR 0xC001\_0020** and size to **MSR 0xC001\_0021**; finally a `wrmsr` triggers the in-place copy to Patch-RAM (constants listed in the [Haiku MSR header](https://github.com/haiku/haiku/blob/master/headers/private/kernel/arch/x86/arch_cpu.h) ) ([GitHub][4])



## Vulnerabilities

* **Zenbleed (CVE-2023-20593)** – SIMD register bleed-through on Zen 2; fixed by microcode rev 0x8301030. [AMD-SB-7008](https://www.amd.com/en/resources/product-security/bulletin/amd-sb-7008.html) ([AMD][5])
* **Inception / SRSO (CVE-2023-20569)** – Phantom return prediction leak on Zen 1-4; bulletin [AMD-SB-7005](https://www.amd.com/en/resources/product-security/bulletin/amd-sb-7005.html) ([AMD][6])
* **Predictive Store Forwarding (PSF)** – Crosstask value reveal; mitigation via `PSFD` chicken-bit (see AMD white-paper “Security Analysis of PSF” [pdf](https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/white-papers/security-analysis-of-amd-predictive-store-forwarding.pdf)) ([AMD][7])
* **Signature-Forge (CVE-2024-56161)** – Weak hash in Zen 1-5 patch loader lets attackers craft arbitrary microcode; full advisory on [GitHub GHSA-4xq7-4mgh-gp6w](https://github.com/google/security-research/security/advisories/GHSA-4xq7-4mgh-gp6w) ([GitHub][8])



## Microcode encryption and authentication mechanisms

* **K8 / K10** – Plain-text payload + 32-bit additive checksum (see historic post “[Opteron Exposed](https://seclists.org/interesting-people/2004/Jul/255)”) ([SecLists][9])
* **Zen 1-4** – AES-CMAC encrypted blobs; authenticity checked by a proprietary 32-byte hash (now known forgeable – see Signature-Forge above).
* **Zen 5 & patched Zen 1-4** – Container re-signed with 3072-bit RSA-PSS; distribution began in AGESA 1.2.0.3C ([Tom’s Hardware](https://www.tomshardware.com/pc-components/motherboards/amd-patches-critical-zen-5-microcode-bug-partners-deliver-new-bios-with-agesa-1-2-0-3c)) ([Tom's Hardware][1])



## Microcode Debugging

* **Check your revision** – `dmesg | grep microcode` on Linux shows per-core patch IDs; Cross-platform `cpuid` reads EAX of Fn 0x80000008. [Arch Wiki microcode page](https://wiki.archlinux.org/title/Microcode) ([ArchWiki][2])
* **Dump & verify BIOS blobs** – `MCExtractor` GitHub repo parses AMD containers and reports CPUID, build-date, checksum → [github.com/platomav/MCExtractor](https://github.com/platomav/MCExtractor) ([GitHub][10])
* **Inspect linux-firmware containers** – `amd_ucode_info.py` shows encryption key-ID, family tags, size → [github.com/AMDESE/amd\_ucode\_info](https://github.com/AMDESE/amd_ucode_info) ([GitHub][11])
* **Low-level tracing** – IBS-OP / PMU events + BKDG MSRs (`0xC001_100`, …) let researchers correlate µcode paths (example table in USENIX paper [sec17-koppe.pdf](https://www.usenix.org/system/files/conference/usenixsecurity17/sec17-koppe.pdf)) ([USENIX][12])



## Reverse engineering and community work

* **Historic K8 dump (2004)** – “[Opteron Exposed](https://seclists.org/interesting-people/2004/Jul/255)” showed header layout & lack of signatures. ([SecLists][9])
* **USENIX 2017 study** – Full methodology for tracing and rewriting AMD micro-ops (pdf link: [https://www.usenix.org/system/files/conference/usenixsecurity17/sec17-koppe.pdf](https://www.usenix.org/system/files/conference/usenixsecurity17/sec17-koppe.pdf)) ([USENIX][12])
* **Modern tooling**

  * MCExtractor – BIOS/UEFI patch extraction ([GitHub](https://github.com/platomav/MCExtractor)) ([GitHub][10])
  * amd\_ucode\_info – linux-firmware container parser ([GitHub](https://github.com/AMDESE/amd_ucode_info)) ([GitHub][11])
* **2025 Google PoC** – Arbitrary Zen 1-4 microcode injection demonstrated in [GHSA-4xq7-4mgh-gp6w](https://github.com/google/security-research/security/advisories/GHSA-4xq7-4mgh-gp6w) ([GitHub][8])



## Other helpful microcode tips

* **Always flash new BIOS first** – Board vendors often post AGESA months before Linux firmware updates (track via vendor-news feeds; e.g., MSI’s AM5 BIOS list in the Tom’s article above). ([Tom's Hardware][1])
* **Embed `amd-ucode.img` in early initrd** – Essential for kernels < 6.2 on Zenbleed-affected hosts ([Arch Wiki guide](https://wiki.archlinux.org/title/Microcode)) ([ArchWiki][2])
* **Temporary mitigations** – Until patched, disable PSF (`psf=off`) and SRSO (`lsrs=off`) via kernel parameters (see AMD PSF white-paper linked earlier). ([AMD][7])
* **Archive old blobs** – Keep historical microcode versions (e.g., `CPUMicrocodes` repo) to roll back if performance issues appear after updates.
* **Cross-socket sanity** – Match µcode revision to AMD “Family 19h Revision Guide” before mixing CPUs in multi-socket servers for SEV-SNP stability.

*Report compiled  31 May 2025.*

[1]: https://www.tomshardware.com/pc-components/motherboards/amd-patches-critical-zen-5-microcode-bug-partners-deliver-new-bios-with-agesa-1-2-0-3c?utm_source=chatgpt.com "AMD patches critical Zen 5 microcode bug - partners deliver new BIOS with AGESA 1.2.0.3C"
[2]: https://wiki.archlinux.org/title/Microcode?utm_source=chatgpt.com "Microcode - ArchWiki"
[3]: https://www.amd.com/en/resources/product-security/bulletin/amd-sb-3019.html?utm_source=chatgpt.com "AMD SEV Confidential Computing Vulnerability"
[4]: https://github.com/haiku/haiku/blob/master/headers/private/kernel/arch/x86/arch_cpu.h?utm_source=chatgpt.com "haiku/headers/private/kernel/arch/x86/arch_cpu.h at master - GitHub"
[5]: https://www.amd.com/en/resources/product-security/bulletin/amd-sb-7008.html?utm_source=chatgpt.com "Cross-Process Information Leak - AMD"
[6]: https://www.amd.com/en/resources/product-security/bulletin/amd-sb-7005.html?utm_source=chatgpt.com "Return Address Security Bulletin - AMD"
[7]: https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/white-papers/security-analysis-of-amd-predictive-store-forwarding.pdf?utm_source=chatgpt.com "[PDF] SECURITY ANALYSIS OF AMD PREDICTIVE STORE FORWARDING"
[8]: https://github.com/google/security-research/security/advisories/GHSA-4xq7-4mgh-gp6w?utm_source=chatgpt.com "AMD: Microcode Signature Verification Vulnerability - GitHub"
[9]: https://seclists.org/interesting-people/2004/Jul/255?utm_source=chatgpt.com "Opteron Exposed: Reverse Engineering AMD K8 Microcode Updates"
[10]: https://github.com/platomav/MCExtractor?utm_source=chatgpt.com "platomav/MCExtractor: Intel, AMD, VIA & Freescale Microcode ..."
[11]: https://github.com/AMDESE/amd_ucode_info?utm_source=chatgpt.com "AMDESE/amd_ucode_info: Parse and display information ... - GitHub"
[12]: https://www.usenix.org/system/files/conference/usenixsecurity17/sec17-koppe.pdf?utm_source=chatgpt.com "[PDF] Reverse Engineering x86 Processor Microcode - USENIX"
