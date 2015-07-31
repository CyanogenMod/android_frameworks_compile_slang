// -target-api 0
#pragma version(1)
#pragma rs java_package_name(foo)

void __attribute__((kernel("reduce"))) kernel(int arg1, int arg2) {
  return;
}
