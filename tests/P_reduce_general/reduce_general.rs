// -target-api 0 -Wall -Werror
#pragma version(1)
#pragma rs java_package_name(all)

#pragma rs reduce(addint) \
  initializer(aiInit) accumulator(aiAccum)

#pragma rs reduce(dp) \
  initializer(dpInit) accumulator(dpAccum) combiner(dpSum)

#pragma rs reduce(findMinAndMax) \
  initializer(fMMInit) accumulator(fMMAccumulator) \
  combiner(fMMCombiner) outconverter(fMMOutConverter)

#pragma rs reduce(fz) \
  initializer(fzInit) \
  accumulator(fzAccum) combiner(fzCombine) \
       halter(fzFound)

#pragma rs reduce(fz2) \
  initializer(fz2Init) \
  accumulator(fz2Accum) combiner(fz2Combine) \
  halter(fz2Found)

#pragma rs reduce(histogram) initializer(hsgInit) \
  accumulator(hsgAccum) combiner(hsgCombine)

#pragma rs reduce(mode) initializer(hsgInit) \
  accumulator(hsgAccum) combiner(hsgCombine) \
  outconverter(modeOutConvert)

void dummy() {}
