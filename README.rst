=========================================
llvm-rs-cc: Compiler for ScriptC language
=========================================


Introduction
------------

llvm-rs-cc compiles a program in the ScriptC language to generate the
following files.

* Bitcode file. Note that the bitcode here denotes the LLVM (Low-Level
  Virtual Machine) bitcode representation, which will be consumed on
  an Android device by libbcc (in
  platform/frameworks/compile/libbcc.git) to generate device-specific
  executables.

* Reflected APIs for Java. As a result, Android's Java developers can
  invoke those APIs from their code.

Note that although ScriptC is C99-like, we enhance it with several
distinct, effective features for Android programming. We will use
example usage below to illustrate these features.

llvm-rs-cc is being run on a host and is highly-optimizing. As a
result, libbcc on the device can be lightweight and focus on
machine-dependent code generation given some bitcode.

llvm-rs-cc is a driver on top of libslang. The archictecture of
libslang and libbcc is depicted in the following figure::

    libslang   libbcc
        |   \   |
        |    \  |
     clang     llvm


Usage
-----

* *-o $(PRIVATE_RS_OUTPUT_DIR)/res/raw*

  This option specifies the directory for outputting a .bc file.

* *-p $(PRIVATE_RS_OUTPUT_DIR)/src*

  The option *-p* denotes the directory for outputting the reflected Java files.

* *-d $(PRIVATE_RS_OUTPUT_DIR)*

  This option *-d* sets the directory for writing dependences.

* *-MD*

  Note that *-MD* will tell llvm-rs-cc to output dependences.

* *-a*

  Specifies additional dependence target.

Example Command
---------------

First::

  $ cd <Android_Root_Directory>

Using frameworks/base/libs/rs/java/Fountain as a simple app in both
Java and ScriptC, we can find the following command line in the build
log::

  $ out/host/linux-x86/bin/llvm-rs-cc \
    -o out/target/common/obj/APPS/Fountain_intermediates/src/renderscript/res/raw \
    -p out/target/common/obj/APPS/Fountain_intermediates/src/renderscript/src \
    -d out/target/common/obj/APPS/Fountain_intermediates/src/renderscript \
    -a out/target/common/obj/APPS/Fountain_intermediates/src/RenderScript.stamp \
    -MD \
    -I frameworks/base/libs/rs/scriptc \
    -I external/clang/lib/Headers \
    frameworks/base/libs/rs/java/Fountain/src/com/android/fountain/fountain.rs

This command will generate:

* **fountain.bc**

* **ScriptC_fountain.java**

* **ScriptField_Point.java**

The **Script\*.java** files above will be documented below.


Example Program: fountain.rs
----------------------------

fountain.rs is in ScriptC language, which is based on the standard
C99. However, llvm-rs-cc goes beyond "clang -std=c99" and provides the
following important features:

1. Pragma
---------

* *#pragma rs java_package_name([PACKAGE_NAME])*

  The ScriptC_[SCRIPT_NAME].java has to be packaged so that Java
  developers can invoke those APIs.

  To do that, a ScriptC programmer should specify the package name, so
  that llvm-rs-cc knows the package expression and hence the directory
  for outputting ScriptC_[SCRIPT_NAME].java.

  In fountain.rs, we have::

    #pragma rs java_package_name(com.android.fountain)

  In ScriptC.fountain.java, we have::

    package com.android.fountain

  Note that the ScriptC_fountain.java will be generated inside
  ./com/android/fountain/.

* #pragma version(1)

  This pragma is for evolving the language. Currently we are at
  version 1 of the language.


2. Basic Reflection: Export Variables and Functions
---------------------------------------------------

llvm-rs-cc automatically export the "externalizable and defined" functions and
variables to Android's Java side. That is, scripts are accessible from
Java.

For instance, for::

  int foo = 0;

In ScriptC_fountain.java, llvm-rs-cc will reflect it to::

  void set_foo(int v)...

  int get_foo()...

This access takes the form of generated classes which provide access
to the functions and global variables within a script. In summary,
global variables and functions within a script that are not declared
static will generate get, set, or invoke methods.  This provides a way
to set the data within a script and call to its functions.

Take the addParticles function in fountain.rs as an example::

  void addParticles(int rate, float x, float y, int index, bool newColor) {
    ...
  }

llvm-rs-cc will genearte ScriptC_fountain.java as follows::

  void invoke_addParticles(int rate, float x, float y,
                           int index, bool newColor) {
    ...
  }


3. Export User-Defined Structs
------------------------------

In fountain.rs, we have::

  typedef struct __attribute__((packed, aligned(4))) Point {
    float2 delta;
    float2 position;
    uchar4 color;
  } Point_t;

  Point_t *point;

llvm-rs-cc generates one ScriptField*.java file for each user-defined
struct. I.e., in this case llvm-rs-cc will reflect to two files,
ScriptC_fountain.java and ScriptField_Point.java.

Note that when the type of exportable variable is struct, ScriptC
developers should avoid anonymous structs. This is because llvm-rs-cc
uses the struct name to name the file, instead of the typedef name.

For the generated Java files, using ScriptC_fountain.java as an
example we have::

  void bind_point(ScriptField_Point v)

This binds your object with the allocated memory.

You can bind the struct(e.g., Point), using the setter and getter
method in ScriptField_Point.java.

After binding, you could get the object from this method::

  ScriptField_Point get_point()

In ScriptField_Point_s.java::

    ...
    // Copying the Item, which is the object that stores every
    // fields of struct, to the *index*\-th entry of byte array.
    //
    // In general, this method would not be invoked directly
    // but is used to implement the setter.
    void copyToArray(Item i, int index)

    // The setter of Item array,
    // index: the index of the Item array
    // copyNow: If true, it will be copied to the *index*\-th entry
    // of byte array.
    void set(Item i, int index, boolean copyNow)

    // The getter of Item array, which gets the *index*-th element
    // of byte array.
    Item get(int index)

    set_delta(int index, Float2 v, boolean copyNow)

    // The following is the individual setters and getters of
    // each field of a struct.
    public void set_delta(int index, Float2 v, boolean copyNow)
    public void set_position(int index, Float2 v, boolean copyNow)
    public void set_color(int index, Short4 v, boolean copyNow)
    public Float2 get_delta(int index)
    public Float2 get_position(int index)
    public Short4 get_color(int index)

    // Copying all Item array to byte array (i.e., memory allocation).
    void copyAll()
    ...


4. Summarize the Java Reflection above
--------------------------------------

Let us summarize the high-level design of reflection next.

* In terms of script's global functions, they can be called from Java.
  These calls operate asynchronously and no assumptions should be made
  upon with a function called will actually complete operation.  If it
  is necessary to wait for a function to complete the java application
  may call the runtime finish method which will wait for all the script
  threads to complete.  Two special functions also exist:

  * The function **init** present will be called once after the script
    is loaded.  This is useful to initialize data or anything else the
    script may need before it can be used.  The init may not depend on
    globals initialized from Java as it will be called before these
    can be initialized.

  * The function **root** is a special function for graphics.  Which a
    script must redraw its contents this function will be called.  No
    assumptions should be made as to when this function will be
    called.  It will only be called if the script is bound as root.
    Also calls to this will be synchronized with data updates and
    other invocations from Java.  Thus the script will not change due
    to external influence during a run of **root**.  The return value
    indicates to the runtime if the function should be called again to
    redraw in the future.  A return value of 0 indicates that no
    redraw is necessary until something changes.  Any positive integer
    indicates a time in ms that the runtime should wait before calling
    root again to render another frame.

* In terms of script's global data, global variables can be written
  from Java.  The Java class will cache the value or object set and
  provide return methods to retrieve this value.  If a script updates
  the value, this update will not propagate back to the Java class.
  Initializers if present will also initialize the cached Java value.
  This provides a convenient way to declare constants within a script and
  make them accessible from the java runtime.  If the script declares a
  variable const, only the get methods will be generated.

  Globals within a script are considered local to the script.  They
  cannot be accessed by other scripts and are in effect always 'static'
  in the traditional C sense.  Static here is used to control if a
  accessor is generated.  Static continues to mean *not
  externally visible* and thus prevents the generation of
  accessors.  Globals are persistent across invocations to a script and
  thus may be used to hold data from run to run.

  Globals of two types may be reflected into the Java class.  The first
  type is basic non-pointer types.  Types defined in rs_types.rsh may be
  used.  For the non-pointer class get and set methods are generated in
  Java.  Globals of single pointer types behave differently.  These may
  use more complex types.  Simple structures composed of the types in
  rs_types.rsh may also be used.  These globals generate bind points in
  java.  If the type is a structure they also generate a **Field** class
  used to pack and unpack the contents of the structure.  Binding an
  allocation to one of these bind points in Java effectively sets the
  pointer in the script.  Bind points marked const indicate to the
  runtime that the script will not modify the contents of an allocation.
  This may allow the runtime to make more effective use of threads.


5. Vector Types
---------------

Vector types such as float2, float4, and uint4 are included to support
vector processing in environments where the processors provide vector
instructions.

On non-vector systems the same code will continue to run but without
the performance advantage.  Function overloading is also supported.
This allows the runtime to support vector version of the basic math
routines without the need for special naming.  For instance,

* *float sin(float);*

* *float2 sin(float2);*

* *float4 sin(float4);*
