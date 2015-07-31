// -target-api 0
#pragma version(1)
#pragma rs java_package_name(foo)

/* 0 arguments */

int __attribute__((kernel("reduce"))) kernel0(void) {
  return 0;
}

/* 1 argument */

int __attribute__((kernel("reduce"))) kernel1(int arg1) {
  return 0;
}

/* 3 arguments */

int __attribute__((kernel("reduce"))) kernel3(int arg1, int arg2, int arg3) {
  return 0;
}

/* 4 arguments */

int __attribute__((kernel("reduce"))) kernel4(int arg1, int arg2, int arg3, int arg4) {
  return 0;
}
