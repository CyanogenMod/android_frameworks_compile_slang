// -target-api 13
#pragma version(1)
#pragma rs java_package_name(foo)

#if RS_VERSION != 13
#error Invalid RS_VERSION
#endif

