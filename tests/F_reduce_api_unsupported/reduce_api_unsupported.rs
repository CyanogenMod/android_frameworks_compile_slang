// -target-api 23
#pragma version(1)
#pragma rs java_package_name(foo)

typedef struct foo {
   int x;
} foo;

foo __attribute__((kernel("reduce"))) addFoo(foo a, foo b) {
  foo result = { a.x + b.x };
  return result;
}
