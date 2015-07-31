// -target-api 0
#pragma version(1)
#pragma rs java_package_name(foo)

double __attribute__((kernel("reduce"))) kernel(float arg1, float arg2) {
  return arg1 + arg2;
}
