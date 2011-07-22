// -target-api 12
#pragma version(1)
#pragma rs java_package_name(foo)

#if RS_VERSION != 12
#error Invalid RS_VERSION
#endif

