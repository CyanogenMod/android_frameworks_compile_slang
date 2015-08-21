// -target-api 0 -reflect-c++

#pragma version(1)
#pragma rs java_package_name(foo)

int __attribute__((kernel("reduce"))) add(int a, int b) {
  return a + b;
}
