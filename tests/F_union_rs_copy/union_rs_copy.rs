#pragma version(1)
#pragma rs java_package_name(foo)

static union goodUnion {
    int i;
    float f;
    char c;
} g1, g2;

static union badUnion {
    rs_font rsf;
    int i;
} b1, b2;

void copy() {
    g1 = g2;
    b1 = b2;
}
