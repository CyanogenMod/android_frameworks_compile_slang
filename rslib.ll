define <4 x i8> @convert_iint4(<4 x float> %f4) nounwind readnone {
%conv = fptosi <4 x float> %f4 to <4 x i8>
ret <4 x i8> %conv
}
