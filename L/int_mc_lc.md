
Intel's microcode lifecycle is a comprehensive process encompassing development, deployment, and updates to ensure processor reliability and security. Here's a chronological overview:


## Microcode Development and Manufacturing

* Intel develops microcode using proprietary toolchains and compilers tailored for its processors.
* Microcode is embedded into processors during manufacturing at Intel's fabrication facilities.


## Microcode Update Mechanisms 

Intel provides multiple avenues for updating microcode post-manufacturing:

### Firmware Interface Table (FIT) Update

* Upon system boot, the processor checks the FIT in the BIOS SPI flash for available microcode updates. If a suitable update is found, it's loaded before executing the BIOS firmware.
* Core Loading Variance**: Depending on the processor architecture, the microcode update may load on all cores or only the Bootstrap Processor (BSP).&#x20;

### Early BIOS Microcode Update

* Occurs before memory initialization during BIOS startup.
* If the FIT update isn't present or applicable, BIOS can load the microcode early in the boot process. 

### Late BIOS Microcode Update

* Required for enabling certain features like Intel Software Guard Extensions (Intel SGX).
* Performed after memory initialization and system management mode (SMM) setup.&#x20;

### Early OS Microcode Update

* The operating system checks for newer microcode versions and loads them during early boot stages if available.
* Common in Linux distributions via the `/lib/firmware/intel-ucode` directory. 

### Runtime Microcode Update

* Applied while the system is fully operational, without requiring a reboot.
* Not all microcode updates are suitable for runtime loading; compatibility depends on the update's characteristics and the processor's current state. ([Intel Customer Support][2])



## Security and Authentication

* Microcode updates are authenticated by the processor to ensure integrity and compatibility.
* Updates are encrypted and include checksums to prevent tampering.&#x20;



## Vulnerabilities and Mitigations

Intel addresses various vulnerabilities through microcode updates:

* **Spectre and Meltdown**: Mitigated via microcode changes that introduce new controls and behaviors.
* **CROSSTalk (SRBDS)**: Addressed by microcode updates that prevent data leakage between cores.&#x20;
* **Vmin Shift Instability**: Identified in 13th and 14th Gen CPUs, mitigated through microcode updates (e.g., 0x12B) that adjust voltage behaviors.



## Reverse Engineering and Community work

There have been a few papers reverse engineerign intel's microcodes. 

Like: 

* CustomProcessingUnit:
   Develops framework for static and dynamic analysis of Intel microcode, particularly focusing on the Goldmont microarchitecture. Decompilation and analysis using a Ghidra processor module.
* Reverse Engineering x86 Processor Microcode:
  Analysis of AMD's K8 and K10 microcode to understand update mechanisms and potential for custom microcode development. Demonstrated the feasibility of developing custom microcode updates and highlighted the security implications.

  https://www.usenix.org/system/files/conference/usenixsecurity17/sec17-koppe.pdf
* Decompiler Internals: Microcode:
  Discussed the intermediate language used in the Hex-Rays Decompiler, shedding light on microcode representation and its role in binary analysis.
  https://i.blackhat.com/us-18/Thu-August-9/us-18-Guilfanov-Decompiler-Internals-Microcode-wp.pdf
  
There are some tools which may be relevant
* iucode-tool
  Allows modifiying and vieiwing intel microcode
  https://gitlab.com/iucode-tool/iucode-tool
* microcode_ctl
  Decodes and sends new microcode to the kernel driver to be uploaded to Intel IA32 processors. (Pentium Pro, PII, Celeron, PIII, Xeon, Pentium 4 etc)
  https://linux.die.net/man/8/microcode_ctl



For detailed technical guidance and the latest updates, refer to Intel's official documentation:

* [Microcode Update Guidance](https://www.intel.com/content/www/us/en/developer/articles/technical/software-security-guidance/best-practices/microcode-update-guidance.html)
* [Loading Microcode from the OS](https://www.intel.com/content/www/us/en/developer/articles/technical/software-security-guidance/secure-coding/loading-microcode-os.html)


Sources

[1]: https://www.intel.com/content/www/us/en/developer/articles/technical/software-security-guidance/secure-coding/loading-microcode-os.html "Loading Microcode from the OS - Intel"
[2]: https://cdrdv2.intel.com/v1/dl/getContent/782715 "[PDF] Runtime Microcode Update - Intel"
[3]: https://www.intel.com/content/www/us/en/developer/articles/technical/software-security-guidance/resources/q1-2024-intel-tcb-recovery-guidance.html "Q1 2024 Intel TCB Recovery Guidance"
[4]: https://xcp-ng.org/blog/2020/06/12/intel-microcode-security-update-crosstalk "Intel microcode security update (CROSSTalk)"
[5]: https://www.theverge.com/2024/10/4/24262287/intel-13th-14th-gen-crash-raptor-lake-root-cause-fix "Intel says its Raptor Lake crashing chip nightmare is over"
