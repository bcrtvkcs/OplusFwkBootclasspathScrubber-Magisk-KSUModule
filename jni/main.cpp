// OplusFwkBootclasspathScrubber
//
// Problem:
//   crDroid's ooscamera_op9 port adds /system/framework/oplus-fwk.jar to
//   PRODUCT_BOOT_JARS, which makes derive_classpath bake it into the
//   system-wide BOOTCLASSPATH / DEX2OATBOOTCLASSPATH environment variables
//   at boot. Those variables are inherited by every Zygote-forked app
//   process, including sandboxed ones. Any app that calls
//   System.getenv("BOOTCLASSPATH") / getenv("BOOTCLASSPATH") and diffs it
//   against the stock AOSP boot jar list will flag oplus-fwk.jar as an
//   "injection", even though it's just a vendor framework jar shipped by
//   the ROM itself.
//
// What this module does:
//   For a small, explicit allowlist of target process/package names
//   (see kTargetProcesses below), postAppSpecialize() edits the
//   already-forked process's own environment block in place, removing the
//   oplus-fwk.jar entry from BOOTCLASSPATH and DEX2OATBOOTCLASSPATH before
//   any app code runs. Every other process on the device -- system_server,
//   OnePlus Camera, every other app -- keeps the untouched, real
//   BOOTCLASSPATH. Nothing is unmounted, nothing is removed from disk, and
//   the boot classloader itself (already built by Zygote before this hook
//   ever runs) is completely unaffected, so classes from oplus-fwk.jar
//   keep working normally for every process, this one included.
//
// What this module does NOT do:
//   It does not hide Zygisk's own presence, does not touch file-system
//   visibility (that's SuSFS's job), and does not alter bootclasspath.pb.
//   It only edits what a specific allowlisted process sees in its own
//   copy of two environment variables.

#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <jni.h>
#include "zygisk.hpp"

using zygisk::Api;
using zygisk::AppSpecializeArgs;

// ---------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------

// Process/package names this module scrubs the classpath env vars for.
// nice_name is matched as an EXACT string (not a substring), so a
// multi-process app's sub-processes (e.g. "pkg:service") are untouched
// unless you add them explicitly.
//
// Verify the real package name first if you add a new app here:
//   su -c "pm list packages | grep -i <keyword>"
static const char *const kTargetProcesses[] = {
    "com.reveny.nativecheck",
};

// Environment variables to scrub for the processes above.
static const char *const kTargetEnvVars[] = {
    "BOOTCLASSPATH",
    "DEX2OATBOOTCLASSPATH",
};

// The classpath entry to remove.
static const char *const kTargetJar = "/system/framework/oplus-fwk.jar";

// ---------------------------------------------------------------------
// In-place environment scrubbing
// ---------------------------------------------------------------------

// Removes one ':'-separated entry from the live value of `env_name`,
// editing the process's own environ memory in place (no malloc/setenv),
// so both getenv()/System.getenv() and a raw /proc/self/environ read see
// the change. Handles the entry appearing first, last, in the middle, or
// as the only entry, and zeroes the bytes vacated by the removal so no
// stale copy of the jar path is left sitting in the environ block for a
// raw byte scan to still find.
static void remove_classpath_entry(const char *env_name, const char *entry) {
    char *val = getenv(env_name);
    if (!val) return;

    const size_t entry_len = strlen(entry);

    for (;;) {
        char *pos = strstr(val, entry);
        if (!pos) break;

        // Make sure we matched a whole ':'-delimited entry, not just a
        // substring of some other path.
        bool starts_ok = (pos == val) || (pos[-1] == ':');
        bool ends_ok = (pos[entry_len] == ':') || (pos[entry_len] == '\0');
        if (!starts_ok || !ends_ok) {
            // Shouldn't happen for this specific jar name, but don't risk
            // corrupting the string if it ever does.
            break;
        }

        char *cut_start = pos;
        size_t cut_len = entry_len;

        if (pos != val && pos[-1] == ':') {
            // Swallow the separator before the entry: "...:oplus-fwk.jar"
            cut_start = pos - 1;
            cut_len += 1;
        } else if (pos[entry_len] == ':') {
            // Entry was first in the list: "oplus-fwk.jar:..."
            cut_len += 1;
        }
        // else: entry was the only thing in the variable, nothing extra to cut.

        size_t remaining_len = strlen(cut_start + cut_len);
        // Shift the remainder (including its NUL terminator) left over the cut.
        memmove(cut_start, cut_start + cut_len, remaining_len + 1);
        // Zero the now-unused tail bytes so a raw byte scan (e.g. `grep -a`
        // over /proc/self/environ) can't still find the old substring.
        memset(cut_start + remaining_len + 1, 0, cut_len);
    }
}

// ---------------------------------------------------------------------
// Module
// ---------------------------------------------------------------------

class OplusFwkBootclasspathScrubber : public zygisk::ModuleBase {
public:
    void onLoad(Api *api, JNIEnv *env) override {
        this->api = api;
        this->env = env;
    }

    void postAppSpecialize(const AppSpecializeArgs *args) override {
        if (!args->nice_name) return;

        const char *nice_name = env->GetStringUTFChars(args->nice_name, nullptr);
        if (!nice_name) return;

        bool is_target = false;
        for (const char *target : kTargetProcesses) {
            if (strcmp(nice_name, target) == 0) {
                is_target = true;
                break;
            }
        }
        env->ReleaseStringUTFChars(args->nice_name, nice_name);

        if (!is_target) return;

        for (const char *var : kTargetEnvVars) {
            remove_classpath_entry(var, kTargetJar);
        }
    }

private:
    Api *api = nullptr;
    JNIEnv *env = nullptr;
};

REGISTER_ZYGISK_MODULE(OplusFwkBootclasspathScrubber)
