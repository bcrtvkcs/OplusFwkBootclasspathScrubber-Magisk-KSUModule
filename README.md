# OplusFwk Bootclasspath Scrubber

A Zygisk module for crDroid on the OnePlus 9 Pro (SM8350) / `ooscamera_op9`
setup, where `oplus-fwk.jar` lives in `PRODUCT_BOOT_JARS` and therefore gets
baked into the system-wide `BOOTCLASSPATH` / `DEX2OATBOOTCLASSPATH`
environment variables by `derive_classpath` at boot.

## What it does

For an explicit allowlist of process/package names (`kTargetProcesses` in
`jni/main.cpp`, defaults to `com.reveny.nativecheck`), the module's
`postAppSpecialize` hook edits that process's own already-forked copy of
`BOOTCLASSPATH` and `DEX2OATBOOTCLASSPATH` in memory, removing the
`/system/framework/oplus-fwk.jar` entry before any app code runs. This is
done with an in-place edit of the process's own `environ` block (not
`setenv`), so both `getenv()` / `System.getenv()` calls and a raw
`/proc/self/environ` read see the change consistently.

Every other process - `system_server`, OnePlus Camera, every other
installed app - keeps the real, unmodified classpath. The boot classloader
itself is built by Zygote before this hook ever runs, so classes from
`oplus-fwk.jar` keep loading and working normally everywhere, including in
the targeted process; only what that one process's `getenv()` calls report
is edited.

## What it does NOT do

- Does not hide Zygisk's own presence (a detector can still separately
  report "Zygisk detected" - that's a different check and out of scope
  here).
- Does not touch the filesystem, `bootclasspath.pb`, or any mount (that's
  SuSFS's / a build-system fix's job, see "Alternative" below).
- Does not affect any process not explicitly listed in `kTargetProcesses`.

## Building

This repo ships source only - you need to compile `zygisk/arm64-v8a.so`
yourself with the Android NDK before zipping the module:

```bash
cd jni
ndk-build
# produces libs/arm64-v8a/liboplusfwk_scrubber.so
mkdir -p ../zygisk
cp libs/arm64-v8a/liboplusfwk_scrubber.so ../zygisk/arm64-v8a.so
cd ..
zip -r OplusFwkBootclasspathScrubber.zip module.prop customize.sh uninstall.sh zygisk jni README.md -x "jni/obj/*" "jni/libs/*"
```

(`jni/` itself does not need to ship inside the flashable zip; it's only
kept here for reference/rebuilds. Feel free to drop it from the zip you
actually flash.)

`jni/zygisk.hpp` is the unmodified official header from
[topjohnwu/zygisk-module-sample](https://github.com/topjohnwu/zygisk-module-sample).

## Installing

1. Flash the zip via KernelSU / KernelSU Next (Zygisk must be enabled -
   either the built-in KernelSU Next Zygisk or Zygisk Next).
2. Reboot.

## Verifying

```bash
su -c "grep -a oplus-fwk /proc/\$(pidof com.reveny.nativecheck)/environ"
# expected: no output

su -c "grep -a oplus-fwk /proc/\$(pidof com.oplus.camera)/environ"
# expected: still shows oplus-fwk.jar - untouched on purpose
```

Then open the target app and re-scan: the "Found Injection" entry for
`oplus-fwk.jar` should be gone. The separate "Detected Zygisk" finding is
unrelated to this module and will still fire, since it's reported by a
different check and Zygisk is of course still present and doing the work
above.

## Verify the target package name first

If you add a different detector app to `kTargetProcesses`, confirm its
real package/process name before relying on it:

```bash
su -c "pm list packages | grep -i <keyword>"
```

## Alternative (build-system-level) fix

The more thorough fix is to stop shipping `oplus-fwk.jar` via
`PRODUCT_BOOT_JARS` in the device/vendor tree at all, and instead expose
it as a standalone shared library under `/system/etc/permissions/`
(`<library name="oplus-fwk" ...>`) that only `com.oplus.camera` links
against via `<uses-library>`. That keeps `BOOTCLASSPATH` stock-AOSP for
every app on the device, system-wide, with no per-process patch needed -
at the cost of a full rebuild and the risk that some OnePlus component
turns out to implicitly depend on `oplus-fwk.jar` being globally on the
classpath. This module is the low-risk, reboot-free alternative to that.
