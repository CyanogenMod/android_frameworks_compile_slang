/* DEF_SUPPORT_TARGET(target, name, default_triple) */
#ifdef DEF_SUPPORT_TARGET
    DEF_SUPPORT_TARGET(ARM, "ARM", "armv7-none-linux-gnueabi")
    DEF_SUPPORT_TARGET(X86, "x86", "i686-unknown-linux")
    DEF_SUPPORT_TARGET(X64, "x64", "x86_64-unknown-linux")
#   undef DEF_SUPPORT_TARGET
#endif

/* @id below should be an unique identifier of a feature (i.e. different features should have different @id) even across different targets */

/* DEF_TARGET_FEATURE(target, id, key, description) */
#ifdef DEF_TARGET_FEATURE

#   ifndef HOOK_TARGET_FIRST_FEATURE
#       define HOOK_TARGET_FIRST_FEATURE DEF_TARGET_FEATURE
#   endif

#   ifndef HOOK_TARGET_LAST_FEATURE
#       define HOOK_TARGET_LAST_FEATURE DEF_TARGET_FEATURE
#   endif

    HOOK_TARGET_FIRST_FEATURE(ARM, FeatureNEON, "neon", "Enable NEON instructions")

    DEF_TARGET_FEATURE(ARM, FeatureThumb2, "thumb2", "Enable Thumb2 instructions")
    DEF_TARGET_FEATURE(ARM, ArchV4T, "v4t", "ARM v4T")
    DEF_TARGET_FEATURE(ARM, ArchV5T, "v5t", "ARM v5T")
    DEF_TARGET_FEATURE(ARM, ArchV5TE, "v5te", "ARM v5TE, v5TEj, v5TExp")
    DEF_TARGET_FEATURE(ARM, ArchV6, "v6", "ARM v6")
    DEF_TARGET_FEATURE(ARM, ArchV6T2, "v6t2", "ARM v6t2")
    DEF_TARGET_FEATURE(ARM, ArchV7A, "v7a", "ARM v7A")
    DEF_TARGET_FEATURE(ARM, FeatureVFP2, "vfp2", "Enable VFP2 instructions")

    HOOK_TARGET_LAST_FEATURE(ARM, FeatureVFP3, "vfp3", "Enable VFP3 instructions")


#   undef DEF_TARGET_FEATURE
#endif
#undef HOOK_TARGET_FIRST_FEATURE
#undef HOOK_TARGET_LAST_FEATURE
