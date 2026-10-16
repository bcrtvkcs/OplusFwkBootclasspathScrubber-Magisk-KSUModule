## v0.0.1

* feat: Initial release. Strips oplus-fwk.jar from BOOTCLASSPATH/DEX2OATBOOTCLASSPATH for an explicit allowlist of target processes (see kTargetProcesses in jni/main.cpp) via a Zygisk postAppSpecialize hook. Every other process keeps the real, unmodified classpath.
