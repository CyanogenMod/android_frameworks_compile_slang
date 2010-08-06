define <3 x float> @convert1_float3(<3 x i8> %u3) nounwind readnone {
  %conv = uitofp <3 x i8> %u3 to <3 x float>
  ret <3 x float> %conv
}

define <3 x i8> @convert1_uchar3(<3 x float> %f3) nounwind readnone {
  %conv = fptoui <3 x float> %f3 to <3 x i8>
  ret <3 x i8> %conv
}


