// import the cheribuildProject() step
library 'ctsrd-jenkins-scripts'

cheribuildProject(
        target: "newlib-baremetal",
        targetArchitectures: ["mips64", "mips64-purecap", "riscv64", "riscv64-purecap"],
        sdkCompilerOnly: true, // No need for a CheriBSD sysroot
        failFast: false, // complete all branches of the build
        extraArgs: '--install-prefix=/')
