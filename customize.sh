#!/system/bin/sh

# This module only ships an arm64-v8a build (SM8350 is arm64-only), so
# refuse to install on anything else instead of silently doing nothing.
if [ "$ARCH" != "arm64" ]; then
    abort "! This module only supports arm64 devices (ARCH=$ARCH)"
fi

ui_print "━━━━━━━━━━━━━━━━━━━━━━━━━"
ui_print "OplusFwk Bootclasspath Scrubber"
ui_print "━━━━━━━━━━━━━━━━━━━━━━━━━"
ui_print ""
ui_print "- Device: $(getprop ro.product.model)"
ui_print "- Android: $(getprop ro.build.version.release)"
ui_print "- Module path: $MODPATH"
ui_print ""
ui_print "- This only hides oplus-fwk.jar from BOOTCLASSPATH for the"
ui_print "  processes listed in jni/main.cpp's kTargetProcesses array."
ui_print "  It requires a working Zygisk implementation (Zygisk Next /"
ui_print "  built-in KernelSU Next Zygisk) to be enabled."
ui_print ""

if [ ! -f "$MODPATH/zygisk/arm64-v8a.so" ]; then
    abort "! zygisk/arm64-v8a.so is missing from this zip - did you run ndk-build and copy the .so in before zipping?"
fi

set_perm_recursive "$MODPATH" 0 0 0755 0644
set_perm "$MODPATH/zygisk/arm64-v8a.so" 0 0 0644

ui_print "- Initialization completed!"
ui_print ""

sed -i '/description/d' "$MODPATH/module.prop"
echo "description=Status: Installed ✅ | Scrubs oplus-fwk.jar from BOOTCLASSPATH for the allowlisted processes only." >> "$MODPATH/module.prop"
