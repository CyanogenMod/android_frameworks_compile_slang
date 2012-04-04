// -target-api 15
#pragma version(1)
#pragma rs java_package_name(foo)

rs_allocation a[2];

struct rsStruct {
    rs_allocation a;
} s;

static rs_allocation aOk[2];

static struct noExport {
    rs_allocation a;
} sOk;

