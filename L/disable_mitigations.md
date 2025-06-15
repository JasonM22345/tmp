# Disabling kernel mitigations for Spectre 



## WARNING

Disabling these mitigations may allow:

* Spectre, Meltdown, and related attacks to succeed
* Cross-process or cross-VM data leaks
* Kernel memory to be read speculatively from userspace

Only do this:

* In a **controlled lab or research environment**
* On **isolated machines**, never in production or exposed systems

---

## Steps to Disable Kernel Spectre/Meltdown Mitigations on Linux

### 1. **Edit the GRUB Boot Parameters**

You'll modify the kernel command line via GRUB to disable Spectre, Meltdown, and other mitigations.

1. Open GRUB config for editing:

   ```bash
   sudo nano /etc/default/grub
   ```

2. Find the line starting with:

   ```bash
   GRUB_CMDLINE_LINUX_DEFAULT="..."
   ```

3. Append the following options inside the quotes:

   ```bash
   spectre_v2=off spec_store_bypass_disable=off nopti nospectre_v1 nospec_store_bypass noibrs noibpb no_stf_barrier nosrb spectre_v2_user=off
   ```

   Example:

   ```bash
   GRUB_CMDLINE_LINUX_DEFAULT="quiet splash spectre_v2=off spec_store_bypass_disable=off nopti nospectre_v1 nospec_store_bypass noibrs noibpb no_stf_barrier nosrb spectre_v2_user=off"
   ```

   **Key Flags Explained:**

   * `spectre_v1=off` / `spectre_v2=off`: Disables Spectre v1/v2 mitigations
   * `nopti`: Disables Kernel Page Table Isolation (Meltdown)
   * `noibrs`, `noibpb`, `nosrb`: Disables indirect branch and RSB mitigations
   * `spec_store_bypass_disable=off`: Disables Speculative Store Bypass mitigation

4. Save and exit the file (`Ctrl+O`, then `Enter`, then `Ctrl+X` for nano).

---

### 2. **Update GRUB and Reboot**

After editing:

```bash
sudo update-grub
sudo reboot
```

---

### 3. **Verify Mitigations Are Disabled**

After reboot, check the active mitigation status:

```bash
grep . /sys/devices/system/cpu/vulnerabilities/*
```

You should see output like:

```
/sys/devices/system/cpu/vulnerabilities/spectre_v1: Not affected
/sys/devices/system/cpu/vulnerabilities/spectre_v2: Mitigation: Disabled
/sys/devices/system/cpu/vulnerabilities/meltdown: Not affected
...
```

Or similar messages indicating no mitigations are active.

---

### 4. **(Optional) Prevent RSB Refilling**

Linux often does "RSB stuffing" on context switches to mitigate Spectre-RSB. To further control this:

* Use an older kernel (< 4.17) which may not do this
* Recompile the kernel with patches removed (`arch/x86/kernel/cpu/bugs.c`)

But for most testing, the GRUB flags above are sufficient to allow Spectre-RSB PoCs to work on older hardware.

---

## References
* https://unix.stackexchange.com/questions/554908/disable-spectre-and-meltdown-mitigations
