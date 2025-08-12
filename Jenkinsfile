@Library('ctsrd-jenkins-scripts') _

// Set the default job properties (work around properties() not being additive but replacing)
setDefaultJobProperties([
        [$class: 'GithubProjectProperty', displayName: '', projectUrlStr: 'https://github.com/CTSRD-CHERI/newlib/'],
        copyArtifactPermission('*'),
        rateLimitBuilds(throttle: [count: 1, durationName: 'hour', userBoost: true]),
])

cheribuildProject(
        target: "newlib-baremetal",
        customGitCheckoutDir: "newlib", // source dir does not match the target name
        targetArchitectures: ["mips64", "mips64-purecap", "riscv64", "riscv64-purecap"],
        sdkCompilerOnly: true, // No need for a CheriBSD sysroot
        failFast: false, // complete all branches of the build
        extraArgs: '--install-prefix=/')
