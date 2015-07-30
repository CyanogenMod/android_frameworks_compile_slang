// RUN: %Slang -target-api 0 %s
// RUN: %scriptc-filecheck-wrapper --lang=Java %s

#pragma version(1)
#pragma rs java_package_name(foo)

// CHECK: mExportReduceIdx_mul_half
// Array variants of kernels with the half type are unsupported.
// CHECK-NOT: reduce_mul_half({{.*}} in[],
// CHECK: public void reduce_mul_half(Allocation ain, Allocation aout)
// CHECK: public void reduce_mul_half(Allocation ain, Allocation aout, Script.LaunchOptions sc)
half __attribute__((kernel("reduce"))) mul_half(half lhs, half rhs) {
  return lhs * rhs;
}

// CHECK: mExportReduceIdx_mul_half2
// CHECK-NOT: reduce_mul_half2({{.*}} in[],
// CHECK: public void reduce_mul_half2(Allocation ain, Allocation aout)
// CHECK: public void reduce_mul_half2(Allocation ain, Allocation aout, Script.LaunchOptions sc)
half2 __attribute__((kernel("reduce"))) mul_half2(half2 lhs, half2 rhs) {
  return lhs * rhs;
}

// CHECK: mExportReduceIdx_mul_half3
// CHECK-NOT: reduce_mul_half3({{.*}} in[],
// CHECK: public void reduce_mul_half3(Allocation ain, Allocation aout)
// CHECK: public void reduce_mul_half3(Allocation ain, Allocation aout, Script.LaunchOptions sc)
half3 __attribute__((kernel("reduce"))) mul_half3(half3 lhs, half3 rhs) {
  return lhs * rhs;
}

// CHECK: mExportReduceIdx_mul_half4
// CHECK-NOT: reduce_mul_half4({{.*}} in[],
// CHECK: public void reduce_mul_half4(Allocation ain, Allocation aout)
// CHECK: public void reduce_mul_half4(Allocation ain, Allocation aout, Script.LaunchOptions sc)
half4 __attribute__((kernel("reduce"))) mul_half4(half4 lhs, half4 rhs) {
  return lhs * rhs;
}

// CHECK: mExportReduceIdx_mul_bool
// CHECK: public boolean reduce_mul_bool(byte[] in)
// CHECK: public boolean reduce_mul_bool(byte[] in, int x1, int x2)
// CHECK: public void reduce_mul_bool(Allocation ain, Allocation aout)
// CHECK: public void reduce_mul_bool(Allocation ain, Allocation aout, Script.LaunchOptions sc)
bool __attribute__((kernel("reduce")))
mul_bool(bool lhs, bool rhs) {
  return lhs * rhs;
}

// CHECK: mExportReduceIdx_mul_char
// CHECK: public byte reduce_mul_char(byte[] in)
// CHECK: public byte reduce_mul_char(byte[] in, int x1, int x2)
// CHECK: public void reduce_mul_char(Allocation ain, Allocation aout)
// CHECK: public void reduce_mul_char(Allocation ain, Allocation aout, Script.LaunchOptions sc)
char __attribute__((kernel("reduce")))
mul_char(char lhs, char rhs) {
  return lhs * rhs;
}

// CHECK: mExportReduceIdx_mul_char2
// CHECK: public Byte2 reduce_mul_char2(byte[] in)
// CHECK: public Byte2 reduce_mul_char2(byte[] in, int x1, int x2)
// CHECK: public void reduce_mul_char2(Allocation ain, Allocation aout)
// CHECK: public void reduce_mul_char2(Allocation ain, Allocation aout, Script.LaunchOptions sc)
char2 __attribute__((kernel("reduce")))
mul_char2(char2 lhs, char2 rhs) {
  return lhs * rhs;
}

// CHECK: mExportReduceIdx_mul_char3
// CHECK: public Byte3 reduce_mul_char3(byte[] in)
// CHECK: public Byte3 reduce_mul_char3(byte[] in, int x1, int x2)
// CHECK: public void reduce_mul_char3(Allocation ain, Allocation aout)
// CHECK: public void reduce_mul_char3(Allocation ain, Allocation aout, Script.LaunchOptions sc)
char3 __attribute__((kernel("reduce")))
mul_char3(char3 lhs, char3 rhs) {
  return lhs * rhs;
}

// CHECK: mExportReduceIdx_mul_char4
// CHECK: public Byte4 reduce_mul_char4(byte[] in)
// CHECK: public Byte4 reduce_mul_char4(byte[] in, int x1, int x2)
// CHECK: public void reduce_mul_char4(Allocation ain, Allocation aout)
// CHECK: public void reduce_mul_char4(Allocation ain, Allocation aout, Script.LaunchOptions sc)
char4 __attribute__((kernel("reduce")))
mul_char4(char4 lhs, char4 rhs) {
  return lhs * rhs;
}

// CHECK: mExportReduceIdx_mul_double
// CHECK: public double reduce_mul_double(double[] in)
// CHECK: public double reduce_mul_double(double[] in, int x1, int x2)
// CHECK: public void reduce_mul_double(Allocation ain, Allocation aout)
// CHECK: public void reduce_mul_double(Allocation ain, Allocation aout, Script.LaunchOptions sc)
double __attribute__((kernel("reduce")))
mul_double(double lhs, double rhs) {
  return lhs * rhs;
}

// CHECK: mExportReduceIdx_mul_double2
// CHECK: public Double2 reduce_mul_double2(double[] in)
// CHECK: public Double2 reduce_mul_double2(double[] in, int x1, int x2)
// CHECK: public void reduce_mul_double2(Allocation ain, Allocation aout)
// CHECK: public void reduce_mul_double2(Allocation ain, Allocation aout, Script.LaunchOptions sc)
double2 __attribute__((kernel("reduce")))
mul_double2(double2 lhs, double2 rhs) {
  return lhs * rhs;
}

// CHECK: mExportReduceIdx_mul_double3
// CHECK: public Double3 reduce_mul_double3(double[] in)
// CHECK: public Double3 reduce_mul_double3(double[] in, int x1, int x2)
// CHECK: public void reduce_mul_double3(Allocation ain, Allocation aout)
// CHECK: public void reduce_mul_double3(Allocation ain, Allocation aout, Script.LaunchOptions sc)
double3 __attribute__((kernel("reduce")))
mul_double3(double3 lhs, double3 rhs) {
  return lhs * rhs;
}

// CHECK: mExportReduceIdx_mul_double4
// CHECK: public Double4 reduce_mul_double4(double[] in)
// CHECK: public Double4 reduce_mul_double4(double[] in, int x1, int x2)
// CHECK: public void reduce_mul_double4(Allocation ain, Allocation aout)
// CHECK: public void reduce_mul_double4(Allocation ain, Allocation aout, Script.LaunchOptions sc)
double4 __attribute__((kernel("reduce")))
mul_double4(double4 lhs, double4 rhs) {
  return lhs * rhs;
}

// CHECK: mExportReduceIdx_mul_float
// CHECK: public float reduce_mul_float(float[] in)
// CHECK: public float reduce_mul_float(float[] in, int x1, int x2)
// CHECK: public void reduce_mul_float(Allocation ain, Allocation aout)
// CHECK: public void reduce_mul_float(Allocation ain, Allocation aout, Script.LaunchOptions sc)
float __attribute__((kernel("reduce")))
mul_float(float lhs, float rhs) {
  return lhs * rhs;
}

// CHECK: mExportReduceIdx_mul_float2
// CHECK: public Float2 reduce_mul_float2(float[] in)
// CHECK: public Float2 reduce_mul_float2(float[] in, int x1, int x2)
// CHECK: public void reduce_mul_float2(Allocation ain, Allocation aout)
// CHECK: public void reduce_mul_float2(Allocation ain, Allocation aout, Script.LaunchOptions sc)
float2 __attribute__((kernel("reduce")))
mul_float2(float2 lhs, float2 rhs) {
  return lhs * rhs;
}

// CHECK: mExportReduceIdx_mul_float3
// CHECK: public Float3 reduce_mul_float3(float[] in)
// CHECK: public Float3 reduce_mul_float3(float[] in, int x1, int x2)
// CHECK: public void reduce_mul_float3(Allocation ain, Allocation aout)
// CHECK: public void reduce_mul_float3(Allocation ain, Allocation aout, Script.LaunchOptions sc)
float3 __attribute__((kernel("reduce")))
mul_float3(float3 lhs, float3 rhs) {
  return lhs * rhs;
}

// CHECK: mExportReduceIdx_mul_float4
// CHECK: public Float4 reduce_mul_float4(float[] in)
// CHECK: public Float4 reduce_mul_float4(float[] in, int x1, int x2)
// CHECK: public void reduce_mul_float4(Allocation ain, Allocation aout)
// CHECK: public void reduce_mul_float4(Allocation ain, Allocation aout, Script.LaunchOptions sc)
float4 __attribute__((kernel("reduce")))
mul_float4(float4 lhs, float4 rhs) {
  return lhs * rhs;
}

// CHECK: mExportReduceIdx_mul_int
// CHECK: public int reduce_mul_int(int[] in)
// CHECK: public int reduce_mul_int(int[] in, int x1, int x2)
// CHECK: public void reduce_mul_int(Allocation ain, Allocation aout)
// CHECK: public void reduce_mul_int(Allocation ain, Allocation aout, Script.LaunchOptions sc)
int __attribute__((kernel("reduce")))
mul_int(int lhs, int rhs) {
  return lhs * rhs;
}

// CHECK: mExportReduceIdx_mul_int2
// CHECK: public Int2 reduce_mul_int2(int[] in)
// CHECK: public Int2 reduce_mul_int2(int[] in, int x1, int x2)
// CHECK: public void reduce_mul_int2(Allocation ain, Allocation aout)
// CHECK: public void reduce_mul_int2(Allocation ain, Allocation aout, Script.LaunchOptions sc)
int2 __attribute__((kernel("reduce")))
mul_int2(int2 lhs, int2 rhs) {
  return lhs * rhs;
}

// CHECK: mExportReduceIdx_mul_int3
// CHECK: public Int3 reduce_mul_int3(int[] in)
// CHECK: public Int3 reduce_mul_int3(int[] in, int x1, int x2)
// CHECK: public void reduce_mul_int3(Allocation ain, Allocation aout)
// CHECK: public void reduce_mul_int3(Allocation ain, Allocation aout, Script.LaunchOptions sc)
int3 __attribute__((kernel("reduce")))
mul_int3(int3 lhs, int3 rhs) {
  return lhs * rhs;
}

// CHECK: mExportReduceIdx_mul_int4
// CHECK: public Int4 reduce_mul_int4(int[] in)
// CHECK: public Int4 reduce_mul_int4(int[] in, int x1, int x2)
// CHECK: public void reduce_mul_int4(Allocation ain, Allocation aout)
// CHECK: public void reduce_mul_int4(Allocation ain, Allocation aout, Script.LaunchOptions sc)
int4 __attribute__((kernel("reduce")))
mul_int4(int4 lhs, int4 rhs) {
  return lhs * rhs;
}

// CHECK: mExportReduceIdx_mul_long
// CHECK: public long reduce_mul_long(long[] in)
// CHECK: public long reduce_mul_long(long[] in, int x1, int x2)
// CHECK: public void reduce_mul_long(Allocation ain, Allocation aout)
// CHECK: public void reduce_mul_long(Allocation ain, Allocation aout, Script.LaunchOptions sc)
long __attribute__((kernel("reduce")))
mul_long(long lhs, long rhs) {
  return lhs * rhs;
}

// CHECK: mExportReduceIdx_mul_long2
// CHECK: public Long2 reduce_mul_long2(long[] in)
// CHECK: public Long2 reduce_mul_long2(long[] in, int x1, int x2)
// CHECK: public void reduce_mul_long2(Allocation ain, Allocation aout)
// CHECK: public void reduce_mul_long2(Allocation ain, Allocation aout, Script.LaunchOptions sc)
long2 __attribute__((kernel("reduce")))
mul_long2(long2 lhs, long2 rhs) {
  return lhs * rhs;
}

// CHECK: mExportReduceIdx_mul_long3
// CHECK: public Long3 reduce_mul_long3(long[] in)
// CHECK: public Long3 reduce_mul_long3(long[] in, int x1, int x2)
// CHECK: public void reduce_mul_long3(Allocation ain, Allocation aout)
// CHECK: public void reduce_mul_long3(Allocation ain, Allocation aout, Script.LaunchOptions sc)
long3 __attribute__((kernel("reduce")))
mul_long3(long3 lhs, long3 rhs) {
  return lhs * rhs;
}

// CHECK: mExportReduceIdx_mul_long4
// CHECK: public Long4 reduce_mul_long4(long[] in)
// CHECK: public Long4 reduce_mul_long4(long[] in, int x1, int x2)
// CHECK: public void reduce_mul_long4(Allocation ain, Allocation aout)
// CHECK: public void reduce_mul_long4(Allocation ain, Allocation aout, Script.LaunchOptions sc)
long4 __attribute__((kernel("reduce")))
mul_long4(long4 lhs, long4 rhs) {
  return lhs * rhs;
}

// CHECK: mExportReduceIdx_mul_short
// CHECK: public short reduce_mul_short(short[] in)
// CHECK: public short reduce_mul_short(short[] in, int x1, int x2)
// CHECK: public void reduce_mul_short(Allocation ain, Allocation aout)
// CHECK: public void reduce_mul_short(Allocation ain, Allocation aout, Script.LaunchOptions sc)
short __attribute__((kernel("reduce")))
mul_short(short lhs, short rhs) {
  return lhs * rhs;
}

// CHECK: mExportReduceIdx_mul_short2
// CHECK: public Short2 reduce_mul_short2(short[] in)
// CHECK: public Short2 reduce_mul_short2(short[] in, int x1, int x2)
// CHECK: public void reduce_mul_short2(Allocation ain, Allocation aout)
// CHECK: public void reduce_mul_short2(Allocation ain, Allocation aout, Script.LaunchOptions sc)
short2 __attribute__((kernel("reduce")))
mul_short2(short2 lhs, short2 rhs) {
  return lhs * rhs;
}

// CHECK: mExportReduceIdx_mul_short3
// CHECK: public Short3 reduce_mul_short3(short[] in)
// CHECK: public Short3 reduce_mul_short3(short[] in, int x1, int x2)
// CHECK: public void reduce_mul_short3(Allocation ain, Allocation aout)
// CHECK: public void reduce_mul_short3(Allocation ain, Allocation aout, Script.LaunchOptions sc)
short3 __attribute__((kernel("reduce")))
mul_short3(short3 lhs, short3 rhs) {
  return lhs * rhs;
}

// CHECK: mExportReduceIdx_mul_short4
// CHECK: public Short4 reduce_mul_short4(short[] in)
// CHECK: public Short4 reduce_mul_short4(short[] in, int x1, int x2)
// CHECK: public void reduce_mul_short4(Allocation ain, Allocation aout)
// CHECK: public void reduce_mul_short4(Allocation ain, Allocation aout, Script.LaunchOptions sc)
short4 __attribute__((kernel("reduce")))
mul_short4(short4 lhs, short4 rhs) {
  return lhs * rhs;
}

// CHECK: mExportReduceIdx_mul_uchar
// CHECK: public short reduce_mul_uchar(byte[] in)
// CHECK: public short reduce_mul_uchar(byte[] in, int x1, int x2)
// CHECK: public void reduce_mul_uchar(Allocation ain, Allocation aout)
// CHECK: public void reduce_mul_uchar(Allocation ain, Allocation aout, Script.LaunchOptions sc)
uchar __attribute__((kernel("reduce")))
mul_uchar(uchar lhs, uchar rhs) {
  return lhs * rhs;
}

// CHECK: mExportReduceIdx_mul_uchar2
// CHECK: public Short2 reduce_mul_uchar2(byte[] in)
// CHECK: public Short2 reduce_mul_uchar2(byte[] in, int x1, int x2)
// CHECK: public void reduce_mul_uchar2(Allocation ain, Allocation aout)
// CHECK: public void reduce_mul_uchar2(Allocation ain, Allocation aout, Script.LaunchOptions sc)
uchar2 __attribute__((kernel("reduce")))
mul_uchar2(uchar2 lhs, uchar2 rhs) {
  return lhs * rhs;
}

// CHECK: mExportReduceIdx_mul_uchar3
// CHECK: public Short3 reduce_mul_uchar3(byte[] in)
// CHECK: public Short3 reduce_mul_uchar3(byte[] in, int x1, int x2)
// CHECK: public void reduce_mul_uchar3(Allocation ain, Allocation aout)
// CHECK: public void reduce_mul_uchar3(Allocation ain, Allocation aout, Script.LaunchOptions sc)
uchar3 __attribute__((kernel("reduce")))
mul_uchar3(uchar3 lhs, uchar3 rhs) {
  return lhs * rhs;
}

// CHECK: mExportReduceIdx_mul_uchar4
// CHECK: public Short4 reduce_mul_uchar4(byte[] in)
// CHECK: public Short4 reduce_mul_uchar4(byte[] in, int x1, int x2)
// CHECK: public void reduce_mul_uchar4(Allocation ain, Allocation aout)
// CHECK: public void reduce_mul_uchar4(Allocation ain, Allocation aout, Script.LaunchOptions sc)
uchar4 __attribute__((kernel("reduce")))
mul_uchar4(uchar4 lhs, uchar4 rhs) {
  return lhs * rhs;
}

// CHECK: mExportReduceIdx_mul_uint
// CHECK: public long reduce_mul_uint(int[] in)
// CHECK: public long reduce_mul_uint(int[] in, int x1, int x2)
// CHECK: public void reduce_mul_uint(Allocation ain, Allocation aout)
// CHECK: public void reduce_mul_uint(Allocation ain, Allocation aout, Script.LaunchOptions sc)
uint __attribute__((kernel("reduce")))
mul_uint(uint lhs, uint rhs) {
  return lhs * rhs;
}

// CHECK: mExportReduceIdx_mul_uint2
// CHECK: public Long2 reduce_mul_uint2(int[] in)
// CHECK: public Long2 reduce_mul_uint2(int[] in, int x1, int x2)
// CHECK: public void reduce_mul_uint2(Allocation ain, Allocation aout)
// CHECK: public void reduce_mul_uint2(Allocation ain, Allocation aout, Script.LaunchOptions sc)
uint2 __attribute__((kernel("reduce")))
mul_uint2(uint2 lhs, uint2 rhs) {
  return lhs * rhs;
}

// CHECK: mExportReduceIdx_mul_uint3
// CHECK: public Long3 reduce_mul_uint3(int[] in)
// CHECK: public Long3 reduce_mul_uint3(int[] in, int x1, int x2)
// CHECK: public void reduce_mul_uint3(Allocation ain, Allocation aout)
// CHECK: public void reduce_mul_uint3(Allocation ain, Allocation aout, Script.LaunchOptions sc)
uint3 __attribute__((kernel("reduce")))
mul_uint3(uint3 lhs, uint3 rhs) {
  return lhs * rhs;
}

// CHECK: mExportReduceIdx_mul_uint4
// CHECK: public Long4 reduce_mul_uint4(int[] in)
// CHECK: public Long4 reduce_mul_uint4(int[] in, int x1, int x2)
// CHECK: public void reduce_mul_uint4(Allocation ain, Allocation aout)
// CHECK: public void reduce_mul_uint4(Allocation ain, Allocation aout, Script.LaunchOptions sc)
uint4 __attribute__((kernel("reduce")))
mul_uint4(uint4 lhs, uint4 rhs) {
  return lhs * rhs;
}

// CHECK: mExportReduceIdx_mul_ulong
// CHECK: public long reduce_mul_ulong(long[] in)
// CHECK: public long reduce_mul_ulong(long[] in, int x1, int x2)
// CHECK: public void reduce_mul_ulong(Allocation ain, Allocation aout)
// CHECK: public void reduce_mul_ulong(Allocation ain, Allocation aout, Script.LaunchOptions sc)
ulong __attribute__((kernel("reduce")))
mul_ulong(ulong lhs, ulong rhs) {
  return lhs * rhs;
}

// CHECK: mExportReduceIdx_mul_ulong2
// CHECK: public Long2 reduce_mul_ulong2(long[] in)
// CHECK: public Long2 reduce_mul_ulong2(long[] in, int x1, int x2)
// CHECK: public void reduce_mul_ulong2(Allocation ain, Allocation aout)
// CHECK: public void reduce_mul_ulong2(Allocation ain, Allocation aout, Script.LaunchOptions sc)
ulong2 __attribute__((kernel("reduce")))
mul_ulong2(ulong2 lhs, ulong2 rhs) {
  return lhs * rhs;
}

// CHECK: mExportReduceIdx_mul_ulong3
// CHECK: public Long3 reduce_mul_ulong3(long[] in)
// CHECK: public Long3 reduce_mul_ulong3(long[] in, int x1, int x2)
// CHECK: public void reduce_mul_ulong3(Allocation ain, Allocation aout)
// CHECK: public void reduce_mul_ulong3(Allocation ain, Allocation aout, Script.LaunchOptions sc)
ulong3 __attribute__((kernel("reduce")))
mul_ulong3(ulong3 lhs, ulong3 rhs) {
  return lhs * rhs;
}

// CHECK: mExportReduceIdx_mul_ulong4
// CHECK: public Long4 reduce_mul_ulong4(long[] in)
// CHECK: public Long4 reduce_mul_ulong4(long[] in, int x1, int x2)
// CHECK: public void reduce_mul_ulong4(Allocation ain, Allocation aout)
// CHECK: public void reduce_mul_ulong4(Allocation ain, Allocation aout, Script.LaunchOptions sc)
ulong4 __attribute__((kernel("reduce")))
mul_ulong4(ulong4 lhs, ulong4 rhs) {
  return lhs * rhs;
}

// CHECK: mExportReduceIdx_mul_ushort
// CHECK: public int reduce_mul_ushort(short[] in)
// CHECK: public int reduce_mul_ushort(short[] in, int x1, int x2)
// CHECK: public void reduce_mul_ushort(Allocation ain, Allocation aout)
// CHECK: public void reduce_mul_ushort(Allocation ain, Allocation aout, Script.LaunchOptions sc)
ushort __attribute__((kernel("reduce")))
mul_ushort(ushort lhs, ushort rhs) {
  return lhs * rhs;
}

// CHECK: mExportReduceIdx_mul_ushort2
// CHECK: public Int2 reduce_mul_ushort2(short[] in)
// CHECK: public Int2 reduce_mul_ushort2(short[] in, int x1, int x2)
// CHECK: public void reduce_mul_ushort2(Allocation ain, Allocation aout)
// CHECK: public void reduce_mul_ushort2(Allocation ain, Allocation aout, Script.LaunchOptions sc)
ushort2 __attribute__((kernel("reduce")))
mul_ushort2(ushort2 lhs, ushort2 rhs) {
  return lhs * rhs;
}

// CHECK: mExportReduceIdx_mul_ushort3
// CHECK: public Int3 reduce_mul_ushort3(short[] in)
// CHECK: public Int3 reduce_mul_ushort3(short[] in, int x1, int x2)
// CHECK: public void reduce_mul_ushort3(Allocation ain, Allocation aout)
// CHECK: public void reduce_mul_ushort3(Allocation ain, Allocation aout, Script.LaunchOptions sc)
ushort3 __attribute__((kernel("reduce")))
mul_ushort3(ushort3 lhs, ushort3 rhs) {
  return lhs * rhs;
}

// CHECK: mExportReduceIdx_mul_ushort4
// CHECK: public Int4 reduce_mul_ushort4(short[] in)
// CHECK: public Int4 reduce_mul_ushort4(short[] in, int x1, int x2)
// CHECK: public void reduce_mul_ushort4(Allocation ain, Allocation aout)
// CHECK: public void reduce_mul_ushort4(Allocation ain, Allocation aout, Script.LaunchOptions sc)
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

// CHECK: mExportReduceIdx_mul_indirect
// CHECK: public void reduce_mul_indirect(Allocation ain, Allocation aout)
// CHECK: public void reduce_mul_indirect(Allocation ain, Allocation aout, Script.LaunchOptions sc)
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
