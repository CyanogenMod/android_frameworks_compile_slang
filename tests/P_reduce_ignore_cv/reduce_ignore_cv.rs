// -target-api 0
#pragma version(1)
#pragma rs java_package_name(foo)

typedef struct foo {
  int x[10];
} foo;

foo __attribute__((kernel("reduce"))) add_foo_a(const foo a, const foo b) {
  foo tmp = a;
  for (int i = 0; i < 10; ++i)
    tmp.x[i] += b.x[i];
  return tmp;
}

