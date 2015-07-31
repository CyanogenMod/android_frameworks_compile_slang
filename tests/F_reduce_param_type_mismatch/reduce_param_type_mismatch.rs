// -target-api 0
#pragma version(1)
#pragma rs java_package_name(foo)

int __attribute__((kernel("reduce"))) kernel1(int arg1, float arg2) {
  return 0;
}
