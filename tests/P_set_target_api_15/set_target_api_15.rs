// -target-api 15
#pragma version(1)
#pragma rs java_package_name(foo)

#if RS_VERSION != 15
#error Invalid RS_VERSION
#endif

