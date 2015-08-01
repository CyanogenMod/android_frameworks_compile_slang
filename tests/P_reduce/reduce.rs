// -target-api 0
#pragma version(1)
#pragma rs java_package_name(foo)

bool __attribute__((kernel("reduce")))
mul_bool(bool lhs, bool rhs) {
  return lhs * rhs;
}

char __attribute__((kernel("reduce")))
mul_char(char lhs, char rhs) {
  return lhs * rhs;
}

char2 __attribute__((kernel("reduce")))
mul_char2(char2 lhs, char2 rhs) {
  return lhs * rhs;
}

char3 __attribute__((kernel("reduce")))
mul_char3(char3 lhs, char3 rhs) {
  return lhs * rhs;
}

char4 __attribute__((kernel("reduce")))
mul_char4(char4 lhs, char4 rhs) {
  return lhs * rhs;
}

double __attribute__((kernel("reduce")))
mul_double(double lhs, double rhs) {
  return lhs * rhs;
}

double2 __attribute__((kernel("reduce")))
mul_double2(double2 lhs, double2 rhs) {
  return lhs * rhs;
}

double3 __attribute__((kernel("reduce")))
mul_double3(double3 lhs, double3 rhs) {
  return lhs * rhs;
}

double4 __attribute__((kernel("reduce")))
mul_double4(double4 lhs, double4 rhs) {
  return lhs * rhs;
}

float __attribute__((kernel("reduce")))
mul_float(float lhs, float rhs) {
  return lhs * rhs;
}

float2 __attribute__((kernel("reduce")))
mul_float2(float2 lhs, float2 rhs) {
  return lhs * rhs;
}

float3 __attribute__((kernel("reduce")))
mul_float3(float3 lhs, float3 rhs) {
  return lhs * rhs;
}

float4 __attribute__((kernel("reduce")))
mul_float4(float4 lhs, float4 rhs) {
  return lhs * rhs;
}

int __attribute__((kernel("reduce")))
mul_int(int lhs, int rhs) {
  return lhs * rhs;
}

int2 __attribute__((kernel("reduce")))
mul_int2(int2 lhs, int2 rhs) {
  return lhs * rhs;
}

int3 __attribute__((kernel("reduce")))
mul_int3(int3 lhs, int3 rhs) {
  return lhs * rhs;
}

int4 __attribute__((kernel("reduce")))
mul_int4(int4 lhs, int4 rhs) {
  return lhs * rhs;
}

long __attribute__((kernel("reduce")))
mul_long(long lhs, long rhs) {
  return lhs * rhs;
}

long2 __attribute__((kernel("reduce")))
mul_long2(long2 lhs, long2 rhs) {
  return lhs * rhs;
}

long3 __attribute__((kernel("reduce")))
mul_long3(long3 lhs, long3 rhs) {
  return lhs * rhs;
}

long4 __attribute__((kernel("reduce")))
mul_long4(long4 lhs, long4 rhs) {
  return lhs * rhs;
}

short __attribute__((kernel("reduce")))
mul_short(short lhs, short rhs) {
  return lhs * rhs;
}

short2 __attribute__((kernel("reduce")))
mul_short2(short2 lhs, short2 rhs) {
  return lhs * rhs;
}

short3 __attribute__((kernel("reduce")))
mul_short3(short3 lhs, short3 rhs) {
  return lhs * rhs;
}

short4 __attribute__((kernel("reduce")))
mul_short4(short4 lhs, short4 rhs) {
  return lhs * rhs;
}

uchar __attribute__((kernel("reduce")))
mul_uchar(uchar lhs, uchar rhs) {
  return lhs * rhs;
}

uchar2 __attribute__((kernel("reduce")))
mul_uchar2(uchar2 lhs, uchar2 rhs) {
  return lhs * rhs;
}

uchar3 __attribute__((kernel("reduce")))
mul_uchar3(uchar3 lhs, uchar3 rhs) {
  return lhs * rhs;
}

uchar4 __attribute__((kernel("reduce")))
mul_uchar4(uchar4 lhs, uchar4 rhs) {
  return lhs * rhs;
}

uint __attribute__((kernel("reduce")))
mul_uint(uint lhs, uint rhs) {
  return lhs * rhs;
}

uint2 __attribute__((kernel("reduce")))
mul_uint2(uint2 lhs, uint2 rhs) {
  return lhs * rhs;
}

uint3 __attribute__((kernel("reduce")))
mul_uint3(uint3 lhs, uint3 rhs) {
  return lhs * rhs;
}

uint4 __attribute__((kernel("reduce")))
mul_uint4(uint4 lhs, uint4 rhs) {
  return lhs * rhs;
}

ulong __attribute__((kernel("reduce")))
mul_ulong(ulong lhs, ulong rhs) {
  return lhs * rhs;
}

ulong2 __attribute__((kernel("reduce")))
mul_ulong2(ulong2 lhs, ulong2 rhs) {
  return lhs * rhs;
}

ulong3 __attribute__((kernel("reduce")))
mul_ulong3(ulong3 lhs, ulong3 rhs) {
  return lhs * rhs;
}

ulong4 __attribute__((kernel("reduce")))
mul_ulong4(ulong4 lhs, ulong4 rhs) {
  return lhs * rhs;
}

ushort __attribute__((kernel("reduce")))
mul_ushort(ushort lhs, ushort rhs) {
  return lhs * rhs;
}

ushort2 __attribute__((kernel("reduce")))
mul_ushort2(ushort2 lhs, ushort2 rhs) {
  return lhs * rhs;
}

ushort3 __attribute__((kernel("reduce")))
mul_ushort3(ushort3 lhs, ushort3 rhs) {
  return lhs * rhs;
}

ushort4 __attribute__((kernel("reduce")))
mul_ushort4(ushort4 lhs, ushort4 rhs) {
  return lhs * rhs;
}

struct indirect {
  bool elem_bool;
  char elem_char;
  char2 elem_char2;
  char3 elem_char3;
  char4 elem_char4;
  double elem_double;
  double2 elem_double2;
  double3 elem_double3;
  double4 elem_double4;
  float elem_float;
  float2 elem_float2;
  float3 elem_float3;
  float4 elem_float4;
  int elem_int;
  int2 elem_int2;
  int3 elem_int3;
  int4 elem_int4;
  long elem_long;
  long2 elem_long2;
  long3 elem_long3;
  long4 elem_long4;
  short elem_short;
  short2 elem_short2;
  short3 elem_short3;
  short4 elem_short4;
  uchar elem_uchar;
  uchar2 elem_uchar2;
  uchar3 elem_uchar3;
  uchar4 elem_uchar4;
  uint elem_uint;
  uint2 elem_uint2;
  uint3 elem_uint3;
  uint4 elem_uint4;
  ulong elem_ulong;
  ulong2 elem_ulong2;
  ulong3 elem_ulong3;
  ulong4 elem_ulong4;
  ushort elem_ushort;
  ushort2 elem_ushort2;
  ushort3 elem_ushort3;
  ushort4 elem_ushort4;
};

struct indirect __attribute__((kernel("reduce")))
mul_indirect(struct indirect lhs, struct indirect rhs) {
  lhs.elem_bool *= rhs.elem_bool;
  lhs.elem_char *= rhs.elem_char;
  lhs.elem_char2 *= rhs.elem_char2;
  lhs.elem_char3 *= rhs.elem_char3;
  lhs.elem_char4 *= rhs.elem_char4;
  lhs.elem_double *= rhs.elem_double;
  lhs.elem_double2 *= rhs.elem_double2;
  lhs.elem_double3 *= rhs.elem_double3;
  lhs.elem_double4 *= rhs.elem_double4;
  lhs.elem_float *= rhs.elem_float;
  lhs.elem_float2 *= rhs.elem_float2;
  lhs.elem_float3 *= rhs.elem_float3;
  lhs.elem_float4 *= rhs.elem_float4;
  lhs.elem_int *= rhs.elem_int;
  lhs.elem_int2 *= rhs.elem_int2;
  lhs.elem_int3 *= rhs.elem_int3;
  lhs.elem_int4 *= rhs.elem_int4;
  lhs.elem_long *= rhs.elem_long;
  lhs.elem_long2 *= rhs.elem_long2;
  lhs.elem_long3 *= rhs.elem_long3;
  lhs.elem_long4 *= rhs.elem_long4;
  lhs.elem_short *= rhs.elem_short;
  lhs.elem_short2 *= rhs.elem_short2;
  lhs.elem_short3 *= rhs.elem_short3;
  lhs.elem_short4 *= rhs.elem_short4;
  lhs.elem_uchar *= rhs.elem_uchar;
  lhs.elem_uchar2 *= rhs.elem_uchar2;
  lhs.elem_uchar3 *= rhs.elem_uchar3;
  lhs.elem_uchar4 *= rhs.elem_uchar4;
  lhs.elem_uint *= rhs.elem_uint;
  lhs.elem_uint2 *= rhs.elem_uint2;
  lhs.elem_uint3 *= rhs.elem_uint3;
  lhs.elem_uint4 *= rhs.elem_uint4;
  lhs.elem_ulong *= rhs.elem_ulong;
  lhs.elem_ulong2 *= rhs.elem_ulong2;
  lhs.elem_ulong3 *= rhs.elem_ulong3;
  lhs.elem_ulong4 *= rhs.elem_ulong4;
  lhs.elem_ushort *= rhs.elem_ushort;
  lhs.elem_ushort2 *= rhs.elem_ushort2;
  lhs.elem_ushort3 *= rhs.elem_ushort3;
  lhs.elem_ushort4 *= rhs.elem_ushort4;
  return lhs;
}
