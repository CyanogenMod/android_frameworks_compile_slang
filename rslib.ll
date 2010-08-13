
define <3 x float> @_Z15convert1_float3Dv3_h(<3 x i8> %u3) nounwind readnone {
  %conv = uitofp <3 x i8> %u3 to <3 x float>
  ret <3 x float> %conv
}

define <3 x i8> @_Z15convert1_uchar3Dv3_f(<3 x float> %f3) nounwind readnone {
  %conv = fptoui <3 x float> %f3 to <3 x i8>
  ret <3 x i8> %conv
}

declare float @llvm.powi.f32(float %val, i32 %exp)

define <3 x float> @_Z4powiDv3_fi(<3 x float> %f3, i32 %exp) nounwind readnone {
  %x = extractelement <3 x float> %f3, i32 0
  %y = extractelement <3 x float> %f3, i32 1
  %z = extractelement <3 x float> %f3, i32 2

  %retx = tail call float @llvm.powi.f32(float %x, i32 %exp)
  %rety = tail call float @llvm.powi.f32(float %y, i32 %exp)
  %retz = tail call float @llvm.powi.f32(float %z, i32 %exp)

  %tmp1 = insertelement <3 x float> %f3, float %retx, i32 0
  %tmp2 = insertelement <3 x float> %tmp1, float %rety, i32 1
  %ret = insertelement <3 x float> %tmp2, float %retz, i32 2

  ret <3 x float> %ret
}

declare float @llvm.pow.f32(float %val, float %exp)

define <3 x float> @_Z4pow3Dv3_ff(<3 x float> %f3, float %exp) nounwind readnone {
  %x = extractelement <3 x float> %f3, i32 0
  %y = extractelement <3 x float> %f3, i32 1
  %z = extractelement <3 x float> %f3, i32 2

  %retx = tail call float @llvm.pow.f32(float %x, float %exp)
  %rety = tail call float @llvm.pow.f32(float %y, float %exp)
  %retz = tail call float @llvm.pow.f32(float %z, float %exp)

  %tmp1 = insertelement <3 x float> %f3, float %retx, i32 0
  %tmp2 = insertelement <3 x float> %tmp1, float %rety, i32 1
  %ret = insertelement <3 x float> %tmp2, float %retz, i32 2

  ret <3 x float> %ret
}

