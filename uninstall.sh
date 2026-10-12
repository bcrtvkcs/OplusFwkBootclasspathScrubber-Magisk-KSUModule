#!/system/bin/sh

# Purely a Zygisk hook with no persistent system changes (no bind-mounts,
# no file edits outside this module's own directory), so removing the
# module directory is enough - nothing else to clean up.
echo "OplusFwkBootclasspathScrubber: module uninstalled" >> /dev/kmsg
