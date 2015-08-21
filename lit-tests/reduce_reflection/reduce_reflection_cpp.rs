// RUN: %Slang -target-api 0 -reflect-c++ %s
// RUN: %scriptc-filecheck-wrapper --lang=C++ %s

#pragma version(1)
#pragma rs java_package_name(foo)

// CHECK: void reduce_mul_half(android::RSC::sp<android::RSC::Allocation> ain,
// Array variants of kernels with the half type are unsupported.
// CHECK-NOT: half reduce_mul_half(const half in[],
half __attribute__((kernel("reduce"))) mul_half(half lhs, half rhs) {
  return lhs * rhs;
}

// CHECK: void reduce_mul_half2(android::RSC::sp<android::RSC::Allocation> ain,
// CHECK-NOT: Half2 reduce_mul_half2(const half in[],
half2 __attribute__((kernel("reduce"))) mul_half2(half2 lhs, half2 rhs) {
  return lhs * rhs;
}

// CHECK: void reduce_mul_half3(android::RSC::sp<android::RSC::Allocation> ain,
// CHECK-NOT: Half3 reduce_mul_half3(const half in[],
half3 __attribute__((kernel("reduce"))) mul_half3(half3 lhs, half3 rhs) {
  return lhs * rhs;
}

// CHECK: void reduce_mul_half4(android::RSC::sp<android::RSC::Allocation> ain,
// CHECK-NOT: Half4 reduce_mul_half4(const half in[],
half4 __attribute__((kernel("reduce"))) mul_half4(half4 lhs, half4 rhs) {
  return lhs * rhs;
}

// CHECK: void reduce_mul_bool(android::RSC::sp<android::RSC::Allocation> ain,
// CHECK: bool reduce_mul_bool(const bool in[],
bool __attribute__((kernel("reduce")))
mul_bool(bool lhs, bool rhs) {
  return lhs * rhs;
}

// CHECK: void reduce_mul_char(android::RSC::sp<android::RSC::Allocation> ain,
// CHECK: int8_t reduce_mul_char(const int8_t in[],
char __attribute__((kernel("reduce")))
mul_char(char lhs, char rhs) {
  return lhs * rhs;
}

// CHECK: void reduce_mul_char2(android::RSC::sp<android::RSC::Allocation> ain,
// CHECK: Byte2 reduce_mul_char2(const int8_t in[],
char2 __attribute__((kernel("reduce")))
mul_char2(char2 lhs, char2 rhs) {
  return lhs * rhs;
}

// CHECK: void reduce_mul_char3(android::RSC::sp<android::RSC::Allocation> ain,
// CHECK: Byte3 reduce_mul_char3(const int8_t in[],
char3 __attribute__((kernel("reduce")))
mul_char3(char3 lhs, char3 rhs) {
  return lhs * rhs;
}

// CHECK: void reduce_mul_char4(android::RSC::sp<android::RSC::Allocation> ain,
// CHECK: Byte4 reduce_mul_char4(const int8_t in[],
char4 __attribute__((kernel("reduce")))
mul_char4(char4 lhs, char4 rhs) {
  return lhs * rhs;
}

// CHECK: void reduce_mul_double(android::RSC::sp<android::RSC::Allocation> ain,
// CHECK: double reduce_mul_double(const double in[],
double __attribute__((kernel("reduce")))
mul_double(double lhs, double rhs) {
  return lhs * rhs;
}

// CHECK: void reduce_mul_double2(android::RSC::sp<android::RSC::Allocation> ain,
// CHECK: Double2 reduce_mul_double2(const double in[],
double2 __attribute__((kernel("reduce")))
mul_double2(double2 lhs, double2 rhs) {
  return lhs * rhs;
}

// CHECK: void reduce_mul_double3(android::RSC::sp<android::RSC::Allocation> ain,
// CHECK: Double3 reduce_mul_double3(const double in[],
double3 __attribute__((kernel("reduce")))
mul_double3(double3 lhs, double3 rhs) {
  return lhs * rhs;
}

// CHECK: void reduce_mul_double4(android::RSC::sp<android::RSC::Allocation> ain,
// CHECK: Double4 reduce_mul_double4(const double in[],
double4 __attribute__((kernel("reduce")))
mul_double4(double4 lhs, double4 rhs) {
  return lhs * rhs;
}

// CHECK: void reduce_mul_float(android::RSC::sp<android::RSC::Allocation> ain,
// CHECK: float reduce_mul_float(const float in[],
float __attribute__((kernel("reduce")))
mul_float(float lhs, float rhs) {
  return lhs * rhs;
}

// CHECK: void reduce_mul_float2(android::RSC::sp<android::RSC::Allocation> ain,
// CHECK: Float2 reduce_mul_float2(const float in[],
float2 __attribute__((kernel("reduce")))
mul_float2(float2 lhs, float2 rhs) {
  return lhs * rhs;
}

// CHECK: void reduce_mul_float3(android::RSC::sp<android::RSC::Allocation> ain,
// CHECK: Float3 reduce_mul_float3(const float in[],
float3 __attribute__((kernel("reduce")))
mul_float3(float3 lhs, float3 rhs) {
  return lhs * rhs;
}

// CHECK: void reduce_mul_float4(android::RSC::sp<android::RSC::Allocation> ain,
// CHECK: Float4 reduce_mul_float4(const float in[],
float4 __attribute__((kernel("reduce")))
mul_float4(float4 lhs, float4 rhs) {
  return lhs * rhs;
}

// CHECK: void reduce_mul_int(android::RSC::sp<android::RSC::Allocation> ain,
// CHECK: int32_t reduce_mul_int(const int32_t in[],
int __attribute__((kernel("reduce")))
mul_int(int lhs, int rhs) {
  return lhs * rhs;
}

// CHECK: void reduce_mul_int2(android::RSC::sp<android::RSC::Allocation> ain,
// CHECK: Int2 reduce_mul_int2(const int32_t in[],
int2 __attribute__((kernel("reduce")))
mul_int2(int2 lhs, int2 rhs) {
  return lhs * rhs;
}

// CHECK: void reduce_mul_int3(android::RSC::sp<android::RSC::Allocation> ain,
// CHECK: Int3 reduce_mul_int3(const int32_t in[],
int3 __attribute__((kernel("reduce")))
mul_int3(int3 lhs, int3 rhs) {
  return lhs * rhs;
}

// CHECK: void reduce_mul_int4(android::RSC::sp<android::RSC::Allocation> ain,
// CHECK: Int4 reduce_mul_int4(const int32_t in[],
int4 __attribute__((kernel("reduce")))
mul_int4(int4 lhs, int4 rhs) {
  return lhs * rhs;
}

// CHECK: void reduce_mul_long(android::RSC::sp<android::RSC::Allocation> ain,
// CHECK: int64_t reduce_mul_long(const int64_t in[],
long __attribute__((kernel("reduce")))
mul_long(long lhs, long rhs) {
  return lhs * rhs;
}

// CHECK: void reduce_mul_long2(android::RSC::sp<android::RSC::Allocation> ain,
// CHECK: Long2 reduce_mul_long2(const int64_t in[],
long2 __attribute__((kernel("reduce")))
mul_long2(long2 lhs, long2 rhs) {
  return lhs * rhs;
}

// CHECK: void reduce_mul_long3(android::RSC::sp<android::RSC::Allocation> ain,
// CHECK: Long3 reduce_mul_long3(const int64_t in[],
long3 __attribute__((kernel("reduce")))
mul_long3(long3 lhs, long3 rhs) {
  return lhs * rhs;
}

// CHECK: void reduce_mul_long4(android::RSC::sp<android::RSC::Allocation> ain,
// CHECK: Long4 reduce_mul_long4(const int64_t in[],
long4 __attribute__((kernel("reduce")))
mul_long4(long4 lhs, long4 rhs) {
  return lhs * rhs;
}

// CHECK: void reduce_mul_short(android::RSC::sp<android::RSC::Allocation> ain,
// CHECK: int16_t reduce_mul_short(const int16_t in[],
short __attribute__((kernel("reduce")))
mul_short(short lhs, short rhs) {
  return lhs * rhs;
}

// CHECK: void reduce_mul_short2(android::RSC::sp<android::RSC::Allocation> ain,
// CHECK: Short2 reduce_mul_short2(const int16_t in[],
short2 __attribute__((kernel("reduce")))
mul_short2(short2 lhs, short2 rhs) {
  return lhs * rhs;
}

// CHECK: void reduce_mul_short3(android::RSC::sp<android::RSC::Allocation> ain,
// CHECK: Short3 reduce_mul_short3(const int16_t in[],
short3 __attribute__((kernel("reduce")))
mul_short3(short3 lhs, short3 rhs) {
  return lhs * rhs;
}

// CHECK: void reduce_mul_short4(android::RSC::sp<android::RSC::Allocation> ain,
// CHECK: Short4 reduce_mul_short4(const int16_t in[],
short4 __attribute__((kernel("reduce")))
mul_short4(short4 lhs, short4 rhs) {
  return lhs * rhs;
}

// CHECK: void reduce_mul_uchar(android::RSC::sp<android::RSC::Allocation> ain,
// CHECK: uint8_t reduce_mul_uchar(const uint8_t in[],
uchar __attribute__((kernel("reduce")))
mul_uchar(uchar lhs, uchar rhs) {
  return lhs * rhs;
}

// CHECK: void reduce_mul_uchar2(android::RSC::sp<android::RSC::Allocation> ain,
// CHECK: UByte2 reduce_mul_uchar2(const uint8_t in[],
uchar2 __attribute__((kernel("reduce")))
mul_uchar2(uchar2 lhs, uchar2 rhs) {
  return lhs * rhs;
}

// CHECK: void reduce_mul_uchar3(android::RSC::sp<android::RSC::Allocation> ain,
// CHECK: UByte3 reduce_mul_uchar3(const uint8_t in[],
uchar3 __attribute__((kernel("reduce")))
mul_uchar3(uchar3 lhs, uchar3 rhs) {
  return lhs * rhs;
}

// CHECK: void reduce_mul_uchar4(android::RSC::sp<android::RSC::Allocation> ain,
// CHECK: UByte4 reduce_mul_uchar4(const uint8_t in[],
uchar4 __attribute__((kernel("reduce")))
mul_uchar4(uchar4 lhs, uchar4 rhs) {
  return lhs * rhs;
}

// CHECK: void reduce_mul_uint(android::RSC::sp<android::RSC::Allocation> ain,
// CHECK: uint32_t reduce_mul_uint(const uint32_t in[],
uint __attribute__((kernel("reduce")))
mul_uint(uint lhs, uint rhs) {
  return lhs * rhs;
}

// CHECK: void reduce_mul_uint2(android::RSC::sp<android::RSC::Allocation> ain,
// CHECK: UInt2 reduce_mul_uint2(const uint32_t in[],
uint2 __attribute__((kernel("reduce")))
mul_uint2(uint2 lhs, uint2 rhs) {
  return lhs * rhs;
}

// CHECK: void reduce_mul_uint3(android::RSC::sp<android::RSC::Allocation> ain,
// CHECK: UInt3 reduce_mul_uint3(const uint32_t in[],
uint3 __attribute__((kernel("reduce")))
mul_uint3(uint3 lhs, uint3 rhs) {
  return lhs * rhs;
}

// CHECK: void reduce_mul_uint4(android::RSC::sp<android::RSC::Allocation> ain,
// CHECK: UInt4 reduce_mul_uint4(const uint32_t in[],
uint4 __attribute__((kernel("reduce")))
mul_uint4(uint4 lhs, uint4 rhs) {
  return lhs * rhs;
}

// CHECK: void reduce_mul_ulong(android::RSC::sp<android::RSC::Allocation> ain,
// CHECK: uint64_t reduce_mul_ulong(const uint64_t in[],
ulong __attribute__((kernel("reduce")))
mul_ulong(ulong lhs, ulong rhs) {
  return lhs * rhs;
}

// CHECK: void reduce_mul_ulong2(android::RSC::sp<android::RSC::Allocation> ain,
// CHECK: ULong2 reduce_mul_ulong2(const uint64_t in[],
ulong2 __attribute__((kernel("reduce")))
mul_ulong2(ulong2 lhs, ulong2 rhs) {
  return lhs * rhs;
}

// CHECK: void reduce_mul_ulong3(android::RSC::sp<android::RSC::Allocation> ain,
// CHECK: ULong3 reduce_mul_ulong3(const uint64_t in[],
ulong3 __attribute__((kernel("reduce")))
mul_ulong3(ulong3 lhs, ulong3 rhs) {
  return lhs * rhs;
}

// CHECK: void reduce_mul_ulong4(android::RSC::sp<android::RSC::Allocation> ain,
// CHECK: ULong4 reduce_mul_ulong4(const uint64_t in[],
ulong4 __attribute__((kernel("reduce")))
mul_ulong4(ulong4 lhs, ulong4 rhs) {
  return lhs * rhs;
}

// CHECK: void reduce_mul_ushort(android::RSC::sp<android::RSC::Allocation> ain,
// CHECK: uint16_t reduce_mul_ushort(const uint16_t in[],
ushort __attribute__((kernel("reduce")))
mul_ushort(ushort lhs, ushort rhs) {
  return lhs * rhs;
}

// CHECK: void reduce_mul_ushort2(android::RSC::sp<android::RSC::Allocation> ain,
// CHECK: UShort2 reduce_mul_ushort2(const uint16_t in[],
ushort2 __attribute__((kernel("reduce")))
mul_ushort2(ushort2 lhs, ushort2 rhs) {
  return lhs * rhs;
}

// CHECK: void reduce_mul_ushort3(android::RSC::sp<android::RSC::Allocation> ain,
// CHECK: UShort3 reduce_mul_ushort3(const uint16_t in[],
ushort3 __attribute__((kernel("reduce")))
mul_ushort3(ushort3 lhs, ushort3 rhs) {
  return lhs * rhs;
}

// CHECK: void reduce_mul_ushort4(android::RSC::sp<android::RSC::Allocation> ain,
// CHECK: UShort4 reduce_mul_ushort4(const uint16_t in[],
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

// CHECK: void reduce_mul_indirect(android::RSC::sp<android::RSC::Allocation> ain,
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
