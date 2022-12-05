#uses "classes/QualityGates/Tools/CppCheck/TestFixture"

// #include "checkio.h"
// #include "platform.h"
// #include "settings.h"
// #include "testsuite.h"
// #include "tokenize.h"


struct TestIO : TestFixture
{

  void run()
  {
    settings.inconclusive = false;
    settings.addEnabled("portability");

    TEST_CASE(wrongMode_simple);
    TEST_CASE(wrongMode_complex);
    TEST_CASE(useClosedFile);
    TEST_CASE(fileIOwithoutPositioning);
    TEST_CASE(seekOnAppendedFile);
    TEST_CASE(fflushOnInputStream);

    TEST_CASE(testScanf1); // Scanf without field limiters

    TEST_CASE(testScanfArgument);
    TEST_CASE(testPrintfArgument);
    TEST_CASE(testPosixPrintfScanfParameterPosition);  // #4900

    //TEST_CASE(testMicrosoftPrintfArgument); // ticket #4902
    //TEST_CASE(testMicrosoftScanfArgument);

    TEST_CASE(testTernary); // ticket #6182
    TEST_CASE(testUnsignedConst); // ticket #6132

    TEST_CASE(testAstType); // #7014

    TEST_CASE(testPrintfTypeAlias1);
  }

  void wrongMode_simple()
  {
    // Read mode
    check("void foo(file& f) {\n"
          "    string name = \"dummy\";\n"
          "    blob data = 0;\n"
          "    f = fopen(name, \"r\");\n"
          "    fread(f, data);\n"
          "    rewind(f);\n"
          "    fwrite(f, data);\n"
          "}");
    ASSERT_EQUALS("[scripts/test.ctl:7]: (error) Write operation on a file that was opened only for reading.\n", errout.str());

    check("void foo(file& f) {\n"
          "    string name = \"dummy\";\n"
          "    blob data = 0;\n"
          "    f = fopen(name, \"r+\");\n"
          "    fwrite(f, data);\n"
          "}");
    ASSERT_EQUALS("", errout.str());


    // Write mode
    check("void foo(file& f) {\n"
          "    string name = \"dummy\";\n"
          "    blob data = 0;\n"
          "    f = fopen(name, \"w\");\n"
          "    fwrite(f, data);\n"
          "    rewind(f);\n"
          "    fread(f, data);\n"
          "}");
    ASSERT_EQUALS("[scripts/test.ctl:7]: (error) Read operation on a file that was opened only for writing.\n", errout.str());

    check("void foo(file& f) {\n"
          "    string name = \"dummy\";\n"
          "    blob data = 0;\n"
          "    f = fopen(name, \"w+\");\n"
          "    fread(f, data);\n"
          "}");
    ASSERT_EQUALS("", errout.str());

    check("void foo(file& f) {\n"
          "    blob data = 0;\n"
          "    f = tmpfile();\n"
          "    fread(f, data);\n"
          "}");
    ASSERT_EQUALS("", errout.str());

    // Append mode
    check("void foo(file& f) {\n"
          "    string name = \"dummy\";\n"
          "    blob data = 0;\n"
          "    f = fopen(name, \"a\");\n"
          "    fwrite(f, data);\n"
          "    rewind(f);\n"
          "    fread(f, data);\n"
          "}");
    ASSERT_EQUALS("[scripts/test.ctl:6]: (warning) Repositioning operation performed on a file opened in append mode has no effect.\n"
                  "[scripts/test.ctl:7]: (error) Read operation on a file that was opened only for writing.\n", errout.str());

    check("void foo(file& f) {\n"
          "    string name = \"dummy\";\n"
          "    blob data = 0;\n"
          "    f = fopen(name, \"a+\");\n"
          "    fread(f, data);\n"
          "}");
    ASSERT_EQUALS("", errout.str());

    // Variable declared locally
    check("void foo() {\n"
          "    string name = \"dummy\";\n"
          "    blob data = 0;\n"
          "    file f = fopen(name, \"r\");\n"
          "    fwrite(f, data);\n"
          "    fclose(f);\n"
          "}");
    ASSERT_EQUALS("[scripts/test.ctl:5]: (error) Write operation on a file that was opened only for reading.\n", errout.str());

    // Call unknown function
    check("void foo(file& f) {\n"
          "    string name = \"dummy\";\n"
          "    blob data = 0;\n"
          "    f = fopen(name, \"a\");\n"
          "    fwrite(f, data);\n"
          "    bar(f);\n"
          "    fread(f, data);\n"
          "}");
    ASSERT_EQUALS("", errout.str());

    // Crash tests
    check("void foo(file& f) {\n"
          "    string name = \"dummy\";\n"
          "    blob data = 0;\n"
          "    f = fopen(name, mode);\n" // No assertion failure (#3830)
          "    fwrite(f, data);\n"
          "}");
    ASSERT_EQUALS("[scripts/test.ctl:4]: (warning) Undefined variable: mode\n", errout.str());

    check("void fopen(string const &filepath, string const &mode);"); // #3832
  }

  void wrongMode_complex()
  {
    check("void foo(file f, bool a) {\n"
          "    string name = \"dummy\";\n"
          "    blob data = 0;\n"
          "    if(a) f = fopen(name, \"w\");\n"
          "    else  f = fopen(name, \"r\");\n"
          "    if(a) fwrite(f, data);\n"
          "    else  fread(f, data);\n"
          "}");
    ASSERT_EQUALS("", errout.str());

    check("void foo(bool a) {\n"
          "    file f;\n"
          "    string name = \"dummy\";\n"
          "    blob data = 0;\n"
          "    if(a) f = fopen(name, \"w\");\n"
          "    else  f = fopen(name, \"r\");\n"
          "    if(a) fwrite(f, data);\n"
          "    else  fread(f, data);\n"
          "}");
    ASSERT_EQUALS("", errout.str());

    check("void foo(bool a) {\n"
          "    string name = \"dummy\";\n"
          "    blob data = 0;\n"
          "    file f = fopen(name, \"w\");\n"
          "    if(a) fwrite(f, data);\n"
          "    else  fread(f, data);\n"
          "}");
    ASSERT_EQUALS("[scripts/test.ctl:6]: (error) Read operation on a file that was opened only for writing.\n", errout.str());
  }

  void useClosedFile()
  {
    check("void foo(file& f) {\n"
          "    blob data = 0;\n"
          "    fclose(f);\n"
          "    fwrite(f, data);\n"
          "    rewind(f);\n"
          "}");
    ASSERT_EQUALS("[scripts/test.ctl:4]: (error) Used file that is not opened.\n"
                  "[scripts/test.ctl:5]: (error) Used file that is not opened.\n",
                  errout.str());

    check("void foo(file& f) {\n"
          "    blob data = 0;\n"
          "    if(!ferror(f)) {\n"
          "        fclose(f);\n"
          "        return;"
          "    }\n"
          "    fwrite(f, data);\n"
          "}");
    ASSERT_EQUALS("", errout.str());

    check("void foo(file& f) {\n"
          "    string name = \"dummy\";\n"
          "    blob data = 0;\n"
          "    fclose(f);\n"
          "    f = fopen(name, \"r\");\n"
          "    fread(f, data);\n"
          "}");
    ASSERT_EQUALS("", errout.str());

    check("void foo(file& f, file& g) {\n"
          "    string name = \"dummy\";\n"
          "    blob data = 0;\n"
          "    f = fopen(name, \"r\");\n"
          "    f = g;\n"
          "    fwrite(f, data);\n"
          "}");
    ASSERT_EQUALS("", errout.str());

    check("void foo() {\n"
          "    file f;\n"
          "    blob data = 0;\n"
          "    fwrite(f, data);\n"
          "}");
    ASSERT_EQUALS("[scripts/test.ctl:4]: (information, inconclusive) Uninitialized variable: f\n"
                  "[scripts/test.ctl:4]: (error) Used file that is not opened.\n",
                  errout.str());

    check("void foo() {\n" // #3965
          "    dyn_anytype fs = makeDynAnytype();\n"
          "    string name = \"dummy\";\n"
          "    string mode = \"\";\n"
          "    dynAppend(fs, fopen(name, mode));\n"
          "    fclose(fs[1]);\n"
          "}");
    ASSERT_EQUALS("", errout.str());

    // #4368: multiple functions
    check("file fp = nullptr;\n"
          "\n"
          "void close()\n"
          "{\n"
          "  fclose(fp);\n"
          "}\n"
          "\n"
          "void dump()\n"
          "{\n"
          "  if (fp == nullptr) return;\n"
          "  fprintf(fp, \"Here's the output.\\n\");\n"
          "}\n"
          "\n"
          "int main()\n"
          "{\n"
          "  fp = fopen(\"test.txt\", \"w\");\n"
          "  dump();\n"
          "  close();\n"
          "  return 0;\n"
          "}");
    ASSERT_EQUALS("", errout.str());

    check("file fp = nullptr;\n"
          "\n"
          "void close()\n"
          "{\n"
          "  fclose(fp);\n"
          "}\n"
          "\n"
          "void dump()\n"
          "{\n"
          "  fclose(fp);\n"
          "  fprintf(fp, \"Here's the output.\\n\");\n"
          "}");
    ASSERT_EQUALS("[scripts/test.ctl:11]: (error) Used file that is not opened.\n", errout.str());

    // #4466
    check("void chdcd_parse_nero(file infile, int mode) {\n"
          "    switch (mode) {\n"
          "        case 0x0300:\n"
          "            fclose(infile);\n"
          "            return;\n"
          "        case 0x0500:\n"
          "            fclose(infile);\n"
          "            return;\n"
          "    }\n"
          "}");
    ASSERT_EQUALS("", errout.str());

    check("void chdcd_parse_nero(file infile, int mode) {\n" //Should behave like above (but doesn't)
          "    switch (mode) {\n"
          "        case 0x0300:\n"
          "            fclose(infile);\n"
          "            exit(0);\n"
          "        case 0x0500:\n"
          "            fclose(infile);\n"
          "            return;\n"
          "    }\n"
          "}");
    ASSERT_EQUALS("", errout.str());

    // #4649
    check("struct files{file f1; file f2;};\n"
          "void foo(string name, string mode) {\n"
          "    files a;\n"
          "    a.f1 = fopen(name,mode);\n"
          "    a.f2 = fopen(name,mode);\n"
          "    fclose(a.f1);\n"
          "    fclose(a.f2);\n"
          "}");
    ASSERT_EQUALS("", errout.str());

    // #1473
    check("void foo() {\n"
          "    file a = fopen(\"aa\", \"r\");\n"
          "    while (fclose(a)) {}\n"
          "}");
    ASSERT_EQUALS("[scripts/test.ctl:3]: (error) Used file that is not opened.\n", errout.str());

    // #6823
    check("void foo() {\n"
          "    dyn_anytype fs = makeDynAnytype();\n"
          "    dynAppend(fs, fopen(\"1\", \"w\"));\n"
          "    dynAppend(fs, fopen(\"2\", \"w\"));\n"
          "    fclose(fs[1]);\n"
          "    fclose(fs[2]);\n"
          "}");
    ASSERT_EQUALS("", errout.str());
  }

  void fileIOwithoutPositioning()
  {
    check("void foo(file f) {\n"
          "    blob data = 0;\n"
          "    fwrite(f, data);\n"
          "    fread(f, data);\n"
          "}");
    ASSERT_EQUALS("[scripts/test.ctl:4]: (error) Read and write operations without a call to a positioning function (fseek or rewind) or fflush in between result in undefined behaviour.\n", errout.str());

    check("void foo(file f) {\n"
          "    blob data = 0;\n"
          "    fread(f, data);\n"
          "    fwrite(f, data);\n"
          "}");
    ASSERT_EQUALS("[scripts/test.ctl:4]: (error) Read and write operations without a call to a positioning function (fseek or rewind) or fflush in between result in undefined behaviour.\n", errout.str());

    check("void foo(file f, bool read) {\n"
          "    blob data = 0;\n"
          "    if(read)\n"
          "        fread(f, data);\n"
          "    else\n"
          "        fwrite(f, data);\n"
          "}");
    ASSERT_EQUALS("", errout.str());

    check("void foo(file f) {\n"
          "    blob data = 0;\n"
          "    fread(f, data);\n"
          "    fflush(f);\n"
          "    fwrite(f, data);\n"
          "}");
    ASSERT_EQUALS("", errout.str());

    check("void foo(file f) {\n"
          "    blob data = 0;\n"
          "    fread(f, data);\n"
          "    rewind(f);\n"
          "    fwrite(f, data);\n"
          "}");
    ASSERT_EQUALS("", errout.str());

    check("void foo(file f, int pos) {\n"
          "    blob data = 0;\n"
          "    fread(f, data);\n"
          "    fseek(f, pos, SEEK_SET);\n" //SEEK_SET should be known. is also referenced in the fseek documentation
          "    fwrite(f, data);\n"
          "}");
    ASSERT_EQUALS("", errout.str());

    check("void foo(file f) {\n"
          "    blob data = 0;\n"
          "    fread(f, data);\n"
          "    fseek(f, 0, SEEK_SET);\n" //SEEK_SET should be known. is also referenced in the fseek documentation
          "    fwrite(f, data);\n"
          "}");
    ASSERT_EQUALS("", errout.str());

    check("void foo(file f) {\n"
          "    blob data = 0;\n"
          "    fread(f, data);\n"
          "    long pos = ftell(f);\n"
          "    fwrite(f, data);\n"
          "}");
    ASSERT_EQUALS("[scripts/test.ctl:5]: (error) Read and write operations without a call to a positioning function (fseek or rewind) or fflush in between result in undefined behaviour.\n", errout.str());

    // #6452 - member functions
    check("class FileStream\n"
          "{\n"
          "    void insert(const blob &writeData, int readCount) {\n"
          "        blob data = 0;\n"
          "        int charactersRead = fread(f, data, readCount);\n"
          "        fseek(f, 0, SEEK_CUR);\n"
          "        fwrite(f, writeData);\n"
          "    }\n"
          "    file f;\n"
          "};");
    ASSERT_EQUALS("", errout.str());

    check("class FileStream {\n"
          "    void insert(const blob &writeData, int readCount){\n"
          "        blob data = 0;\n"
          "        int charactersRead = fread(f, data, readCount);\n"
          "        unknown(0);\n"
          "        fwrite(f, writeData);\n"
          "    }\n"
          "    file f;\n"
          "};\n");
    ASSERT_EQUALS("", errout.str());

    check("class FileStream {\n"
          "    void insert(const blob &writeData, int readCount){\n"
          "        blob data = 0;\n"
          "        int bytesRead = fread(f, data, readCount);   \n"
          "        known(0);\n"
          "        fwrite(f, writeData);\n"
          "    }\n"
          "    file f;\n"
          "};\n"
          "void known(int i){}");
    ASSERT_EQUALS("[scripts/test.ctl:6]: (error) Read and write operations without a call to a positioning function (fseek or rewind) or fflush in between result in undefined behaviour.\n", errout.str());

    check("class FileStream {\n"
          "    void insert(int readCount){\n"
          "       blob data = 0;\n"
          "       int bytesRead = fread(f, X.data(), readCount);\n"
          "       known(0);\n"
          "       fwrite(f, X.data());\n"
          "    }\n"
          "    file f;\n"
          "};\n"
          "void known(int i){}");
    ASSERT_EQUALS("[scripts/test.ctl:6]: (error) Read and write operations without a call to a positioning function (fseek or rewind) or fflush in between result in undefined behaviour.\n", errout.str());
  }

  void seekOnAppendedFile()
  {
    check("void foo() {\n"
          "    file f = fopen(\"\", \"a+\");\n"
          "    fseek(f, 0, SEEK_SET);\n"
          "}");
    ASSERT_EQUALS("", errout.str());

    check("void foo() {\n"
          "    file f = fopen(\"\", \"w\");\n"
          "    fseek(f, 0, SEEK_SET);\n"
          "}");
    ASSERT_EQUALS("", errout.str());

    check("void foo() {\n"
          "    file f = fopen(\"\", \"a\");\n"
          "    fseek(f, 0, SEEK_SET);\n"
          "}");
    ASSERT_EQUALS("[test.cpp:3]: (warning) Repositioning operation performed on a file opened in append mode has no effect.\n", errout.str());

    check("void foo() {\n"
          "    file f = fopen(\"\", \"a\");\n"
          "    fflush(f);\n"
          "}");
    ASSERT_EQUALS("", errout.str()); // #5578

    check("void foo() {\n"
          "    file f = fopen(\"\", \"a\");\n"
          "    fclose(f);\n"
          "    f = fopen(\"\", \"r\");\n"
          "    fseek(f, 0, SEEK_SET);\n"
          "}");
    ASSERT_EQUALS("", errout.str()); // #6566
  }

  void fflushOnInputStream()
  {
    check("void foo(file& f, string path) {\n"
          "    f = fopen(path, \"r\");\n"
          "    fflush(f);\n"
          "}");
    ASSERT_EQUALS("[scripts/test.ctl:3]: (portability) fflush() called on input stream 'f' may result in undefined behaviour on non-linux systems.\n", errout.str());

    check("void foo(file& f, string path) {\n"
          "    f = fopen(path, \"w\");\n"
          "    fflush(f);\n"
          "}");
    ASSERT_EQUALS("", errout.str());

    check("void foo(file& f) {\n"
          "    fflush(f);\n"
          "}");
    ASSERT_EQUALS("", errout.str());
  }

  void testScanf1()
  {
    check("void foo(string foo, string bar) {\n"
          "    int a;\n"
          "    file f = fopen(\"test\", \"r\");\n"
          "    a = fscanf(f, \"aa %s\", bar);\n"
          "    sscanf(foo, \"%[^~]\", bar);\n"
          "    fclose(f);\n"
          "}");
    ASSERT_EQUALS("", errout.str());
  }

  void testScanfArgument()
  {
    check("void foo(file &f, string foo, string &ref, string ip_port, int &a, int &port) {\n"
          "    sscanf(foo, \"%1d\", a);\n"
          "    fscanf(f, \"%7ms\", ref);\n" // #3461
          "    sscanf(ip_port, \"%*[^:]:%4d\", port);\n" // #3468
          "}");
    ASSERT_EQUALS("", errout.str());

    check("void foo(file &f, int foo, string bar) {\n"
          "    fscanf(f, \"%1d\", foo, bar);\n"
          "}");
    ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) fscanf format string requires 1 parameter but 2 are given.\n", errout.str());

    check("void foo(string foo, int bar) {\n"
          "    sscanf(foo, \"%1d%1d\", bar);\n"
          "}");
    ASSERT_EQUALS("[test.cpp:2]: (error) sscanf format string requires 2 parameters but only 1 is given.\n", errout.str());
  }

  void testPrintfArgument()
  {
    check("void foo(file f, string str1, string str2) {\n"
          "    fprintf(f,\"%u%s\");\n"
          "    sprintf(str1, \"%-*.*s\", 32, str2);\n" // #3364
          "}");
    ASSERT_EQUALS("[scripts/test.ctl:2]: (error) fprintf format string requires 2 parameters but only 0 are given.\n"
      "[scripts/test.ctl:3]: (error) sprintf format string requires 3 parameters but only 2 are given.\n",
      errout.str());

    check("void foo(file f, int indent, string str1) {\n"
          "    fprintf(f,\"%PRId64\", 123);\n"
          "    fprintf(f, \"error: %m\");\n" // #3339
          "    fprintf(f, \"%*cText.\", indent, ' ');\n" // #3313
          "    sprintf(str1, \"%*\", 32);\n" // #3364
          "}");
    ASSERT_EQUALS("", errout.str());

    check("struct Bar\n"
          "{\n"
          "    int i;\n"
          "};\n"
          "struct Baz\n"
          "{\n"
          "    int i;\n"
          "};\n"
          "class Foo\n"
          "{\n"
          "    double d;\n"
          "    vector<Bar> bar;\n"
          "    Baz baz;\n"
          "};\n"
          "dyn_int a;\n"
          "vector<Foo> f;\n"
          "void foo(string str1, const Foo foo)\n"
          "{\n"
          "    sprintf(str1, \"%d %f %f %d %f %f\",\n"
          "            foo.d, foo.bar[0].i, a[0],\n"
          "            f[0].d, f[0].baz.i, f[0].bar[0].i);\n"
          "}\n");
    ASSERT_EQUALS("[scripts/test.ctl:19]: (warning) %d in format string (no. 1) requires 'int' but the argument type is 'double'.\n"
                  "[scripts/test.ctl:19]: (warning) %f in format string (no. 2) requires 'double' but the argument type is 'signed int'.\n"
                  "[scripts/test.ctl:19]: (warning) %f in format string (no. 3) requires 'double' but the argument type is 'int'.\n"
                  "[scripts/test.ctl:19]: (warning) %d in format string (no. 4) requires 'int' but the argument type is 'double'.\n"
                  "[scripts/test.ctl:19]: (warning) %f in format string (no. 5) requires 'double' but the argument type is 'int'.\n"
                  "[scripts/test.ctl:19]: (warning) %f in format string (no. 6) requires 'double' but the argument type is 'int'.\n", errout.str());

    check("short f() { return 0; }\n"
          "void foo(string str1) { sprintf(str1, \"%d %u %lu %I64u %I64d %f %Lf %p\", f(), f(), f(), f(), f(), f(), f(), f()); }");
    ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) %u in format string (no. 2) requires 'unsigned int' but the argument type is 'signed short'.\n"
                  "[scripts/test.ctl:2]: (warning) %lu in format string (no. 3) requires 'unsigned long' but the argument type is 'signed short'.\n"
                  "[scripts/test.ctl:2]: (warning) %I64u in format string (no. 4) requires 'unsigned __int64' but the argument type is 'signed short'.\n"
                  "[scripts/test.ctl:2]: (warning) %I64d in format string (no. 5) requires '__int64' but the argument type is 'signed short'.\n"
                  "[scripts/test.ctl:2]: (warning) %f in format string (no. 6) requires 'double' but the argument type is 'signed short'.\n"
                  "[scripts/test.ctl:2]: (warning) %Lf in format string (no. 7) requires 'long double' but the argument type is 'signed short'.\n"
                  "[scripts/test.ctl:2]: (warning) %p in format string (no. 8) requires an address but the argument type is 'signed short'.\n", errout.str());

    check("int f() { return 0; }\n"
          "void foo(string str1) { sprintf(str1, \"%d %u %lu %I64u %I64d %f %Lf %p\", f(), f(), f(), f(), f(), f(), f(), f()); }");
    ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) %u in format string (no. 2) requires 'unsigned int' but the argument type is 'signed int'.\n"
                  "[scripts/test.ctl:2]: (warning) %lu in format string (no. 3) requires 'unsigned long' but the argument type is 'signed int'.\n"
                  "[scripts/test.ctl:2]: (warning) %I64u in format string (no. 4) requires 'unsigned __int64' but the argument type is 'signed int'.\n"
                  "[scripts/test.ctl:2]: (warning) %I64d in format string (no. 5) requires '__int64' but the argument type is 'signed int'.\n"
                  "[scripts/test.ctl:2]: (warning) %f in format string (no. 6) requires 'double' but the argument type is 'signed int'.\n"
                  "[scripts/test.ctl:2]: (warning) %Lf in format string (no. 7) requires 'long double' but the argument type is 'signed int'.\n"
                  "[scripts/test.ctl:2]: (warning) %p in format string (no. 8) requires an address but the argument type is 'signed int'.\n", errout.str());

    check("uint f() { return 0; }\n"
          "void foo(string str1) { sprintf(str1, \"%u %d %ld %I64d %I64u %f %Lf %p\", f(), f(), f(), f(), f(), f(), f(), f()); }");
    ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) %d in format string (no. 2) requires 'int' but the argument type is 'unsigned int'.\n"
                  "[scripts/test.ctl:2]: (warning) %ld in format string (no. 3) requires 'long' but the argument type is 'unsigned int'.\n"
                  "[scripts/test.ctl:2]: (warning) %I64d in format string (no. 4) requires '__int64' but the argument type is 'unsigned int'.\n"
                  "[scripts/test.ctl:2]: (warning) %I64u in format string (no. 5) requires 'unsigned __int64' but the argument type is 'unsigned int'.\n"
                  "[scripts/test.ctl:2]: (warning) %f in format string (no. 6) requires 'double' but the argument type is 'unsigned int'.\n"
                  "[scripts/test.ctl:2]: (warning) %Lf in format string (no. 7) requires 'long double' but the argument type is 'unsigned int'.\n"
                  "[scripts/test.ctl:2]: (warning) %p in format string (no. 8) requires an address but the argument type is 'unsigned int'.\n", errout.str());

    check("long f() { return 0; }\n"
          "void foo(string str1) { sprintf(str1, \"%ld %u %lu %I64u %I64d %f %Lf %p\", f(), f(), f(), f(), f(), f(), f(), f()); }");
    ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) %u in format string (no. 2) requires 'unsigned int' but the argument type is 'signed long'.\n"
                  "[scripts/test.ctl:2]: (warning) %lu in format string (no. 3) requires 'unsigned long' but the argument type is 'signed long'.\n"
                  "[scripts/test.ctl:2]: (warning) %I64u in format string (no. 4) requires 'unsigned __int64' but the argument type is 'signed long'.\n"
                  "[scripts/test.ctl:2]: (warning) %I64d in format string (no. 5) requires '__int64' but the argument type is 'signed long'.\n"
                  "[scripts/test.ctl:2]: (warning) %f in format string (no. 6) requires 'double' but the argument type is 'signed long'.\n"
                  "[scripts/test.ctl:2]: (warning) %Lf in format string (no. 7) requires 'long double' but the argument type is 'signed long'.\n"
                  "[scripts/test.ctl:2]: (warning) %p in format string (no. 8) requires an address but the argument type is 'signed long'.\n", errout.str());

    check("ulong f() { return 0; }\n"
          "void foo(string str1) { sprintf(str1, \"%lu %d %ld %I64d %I64u %f %Lf %p\", f(), f(), f(), f(), f(), f(), f(), f()); }");
    ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) %d in format string (no. 2) requires 'int' but the argument type is 'unsigned long'.\n"
                  "[scripts/test.ctl:2]: (warning) %ld in format string (no. 3) requires 'long' but the argument type is 'unsigned long'.\n"
                  "[scripts/test.ctl:2]: (warning) %I64d in format string (no. 4) requires '__int64' but the argument type is 'unsigned long'.\n"
                  "[scripts/test.ctl:2]: (warning) %I64u in format string (no. 5) requires 'unsigned __int64' but the argument type is 'unsigned long'.\n"
                  "[scripts/test.ctl:2]: (warning) %f in format string (no. 6) requires 'double' but the argument type is 'unsigned long'.\n"
                  "[scripts/test.ctl:2]: (warning) %Lf in format string (no. 7) requires 'long double' but the argument type is 'unsigned long'.\n"
                  "[scripts/test.ctl:2]: (warning) %p in format string (no. 8) requires an address but the argument type is 'unsigned long'.\n", errout.str());

    check("float f() { return 0; }\n"
          "void foo(string str1) { sprintf(str1, \"%f %d %ld %u %lu %I64d %I64u %Lf %p\", f(), f(), f(), f(), f(), f(), f(), f(), f()); }");
    ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) %d in format string (no. 2) requires 'int' but the argument type is 'float'.\n"
                  "[scripts/test.ctl:2]: (warning) %ld in format string (no. 3) requires 'long' but the argument type is 'float'.\n"
                  "[scripts/test.ctl:2]: (warning) %u in format string (no. 4) requires 'unsigned int' but the argument type is 'float'.\n"
                  "[scripts/test.ctl:2]: (warning) %lu in format string (no. 5) requires 'unsigned long' but the argument type is 'float'.\n"
                  "[scripts/test.ctl:2]: (warning) %I64d in format string (no. 6) requires '__int64' but the argument type is 'float'.\n"
                  "[scripts/test.ctl:2]: (warning) %I64u in format string (no. 7) requires 'unsigned __int64' but the argument type is 'float'.\n"
                  "[scripts/test.ctl:2]: (warning) %Lf in format string (no. 8) requires 'long double' but the argument type is 'float'.\n"
                  "[scripts/test.ctl:2]: (warning) %p in format string (no. 9) requires an address but the argument type is 'float'.\n", errout.str());

    check("double f() { return 0; }\n"
          "void foo(string str1) { sprintf(str1, \"%f %d %ld %u %lu %I64d %I64u %Lf %p\", f(), f(), f(), f(), f(), f(), f(), f(), f()); }");
    ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) %d in format string (no. 2) requires 'int' but the argument type is 'double'.\n"
                  "[scripts/test.ctl:2]: (warning) %ld in format string (no. 3) requires 'long' but the argument type is 'double'.\n"
                  "[scripts/test.ctl:2]: (warning) %u in format string (no. 4) requires 'unsigned int' but the argument type is 'double'.\n"
                  "[scripts/test.ctl:2]: (warning) %lu in format string (no. 5) requires 'unsigned long' but the argument type is 'double'.\n"
                  "[scripts/test.ctl:2]: (warning) %I64d in format string (no. 6) requires '__int64' but the argument type is 'double'.\n"
                  "[scripts/test.ctl:2]: (warning) %I64u in format string (no. 7) requires 'unsigned __int64' but the argument type is 'double'.\n"
                  "[scripts/test.ctl:2]: (warning) %Lf in format string (no. 8) requires 'long double' but the argument type is 'double'.\n"
                  "[scripts/test.ctl:2]: (warning) %p in format string (no. 9) requires an address but the argument type is 'double'.\n", errout.str());

    check("int f() { return 0; }\n"
          "void foo(string str1) { sprintf(str1, \"%I64d %I64u %I64x %d\", f(), f(), f(), f()); }");
    ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) %I64d in format string (no. 1) requires '__int64' but the argument type is 'signed int'.\n"
                  "[scripts/test.ctl:2]: (warning) %I64u in format string (no. 2) requires 'unsigned __int64' but the argument type is 'signed int'.\n"
                  "[scripts/test.ctl:2]: (warning) %I64x in format string (no. 3) requires 'unsigned __int64' but the argument type is 'signed int'.\n", errout.str());

    check("struct Fred { int i; };\n"
          "Fred f;\n"
          "void foo(string str1) { sprintf(str1, \"%d %u %lu %f %Lf %p\", f.i, f.i, f.i, f.i, f.i, f.i); }");
    ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) %u in format string (no. 2) requires 'unsigned int' but the argument type is 'signed int'.\n"
                  "[scripts/test.ctl:2]: (warning) %lu in format string (no. 3) requires 'unsigned long' but the argument type is 'signed int'.\n"
                  "[scripts/test.ctl:2]: (warning) %f in format string (no. 4) requires 'double' but the argument type is 'signed int'.\n"
                  "[scripts/test.ctl:2]: (warning) %Lf in format string (no. 5) requires 'long double' but the argument type is 'signed int'.\n"
                  "[scripts/test.ctl:2]: (warning) %p in format string (no. 6) requires an address but the argument type is 'signed int'.\n", errout.str());

    check("struct Fred { uint u; };\n"
          "Fred f;\n"
          "void foo(string str1) { sprintf(str1, \"%u %d %ld %f %Lf %p\", f.u, f.u, f.u, f.u, f.u, f.u); }");
    ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) %d in format string (no. 2) requires 'int' but the argument type is 'unsigned int'.\n"
                  "[scripts/test.ctl:2]: (warning) %ld in format string (no. 3) requires 'long' but the argument type is 'unsigned int'.\n"
                  "[scripts/test.ctl:2]: (warning) %f in format string (no. 4) requires 'double' but the argument type is 'unsigned int'.\n"
                  "[scripts/test.ctl:2]: (warning) %Lf in format string (no. 5) requires 'long double' but the argument type is 'unsigned int'.\n"
                  "[scripts/test.ctl:2]: (warning) %p in format string (no. 6) requires an address but the argument type is 'unsigned int'.\n", errout.str());

    check("struct Fred { uint ui() { return 0; } };\n"
          "Fred f;\n"
          "void foo(string str1) { sprintf(str1, \"%u %d %ld %f %Lf %p\", f.ui(), f.ui(), f.ui(), f.ui(), f.ui(), f.ui()); }");
    ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) %d in format string (no. 2) requires 'int' but the argument type is 'unsigned int'.\n"
                  "[scripts/test.ctl:2]: (warning) %ld in format string (no. 3) requires 'long' but the argument type is 'unsigned int'.\n"
                  "[scripts/test.ctl:2]: (warning) %f in format string (no. 4) requires 'double' but the argument type is 'unsigned int'.\n"
                  "[scripts/test.ctl:2]: (warning) %Lf in format string (no. 5) requires 'long double' but the argument type is 'unsigned int'.\n"
                  "[scripts/test.ctl:2]: (warning) %p in format string (no. 6) requires an address but the argument type is 'unsigned int'.\n", errout.str());

    check("struct Fred { int i; };\n"
          "Fred bar() { return Fred(); }\n"
          "void foo(string str1) { sprintf(str1, \"%d %u %lu %f %Lf %p\", bar().i, bar().i, bar().i, bar().i, bar().i, bar().i); }");
    ASSERT_EQUALS("[scripts/test.ctl:3]: (warning) %u in format string (no. 2) requires 'unsigned int' but the argument type is 'signed int'.\n"
                  "[scripts/test.ctl:3]: (warning) %lu in format string (no. 3) requires 'unsigned long' but the argument type is 'signed int'.\n"
                  "[scripts/test.ctl:3]: (warning) %f in format string (no. 4) requires 'double' but the argument type is 'signed int'.\n"
                  "[scripts/test.ctl:3]: (warning) %Lf in format string (no. 5) requires 'long double' but the argument type is 'signed int'.\n"
                  "[scripts/test.ctl:3]: (warning) %p in format string (no. 6) requires an address but the argument type is 'signed int'.\n", errout.str());

    check("struct Base { int length() { } };\n"
          "struct Derived : Base { };\n"
          "void foo(Derived d, string str1)\n"
          "{\n"
          "    sprintf(str1, \"%f\", d.length());\n"
          "}\n");
    ASSERT_EQUALS("[scripts/test.ctl:5]: (warning) %f in format string (no. 1) requires 'double' but the argument type is 'signed int'.\n", errout.str());

    check("vector<int> v;\n"
          "void foo(string str1) {\n"
          "    sprintf(str1, \"%d %u %f\", v[0], v[0], v[0]);\n"
          "}\n");
    ASSERT_EQUALS("[scripts/test.ctl:3]: (warning) %u in format string (no. 2) requires 'unsigned int' but the argument type is 'signed int'.\n"
                  "[scripts/test.ctl:3]: (warning) %f in format string (no. 3) requires 'double' but the argument type is 'signed int'.\n", errout.str());

    // #4999 (crash)
    check("int bar(int a){ return a; }\n"
          "void foo(string str1) {\n"
          "    sprintf(str1, \"%d\", bar(0));\n"
          "}\n");
    ASSERT_EQUALS("", errout.str());

    check("void foo(string str1, int i) {\n"
          "    dyn_long l = makeDynLong();\n"
          "    sprintf(str1, \"%d %x %u %f\", l[i], l[i], l[i], l[i]);\n"
          "}\n");
    ASSERT_EQUALS("[scripts/test.ctl:3]: (warning) %d in format string (no. 1) requires 'int' but the argument type is 'signed long'.\n"
                  "[scripts/test.ctl:3]: (warning) %x in format string (no. 2) requires 'unsigned int' but the argument type is 'signed long'.\n"
                  "[scripts/test.ctl:3]: (warning) %u in format string (no. 3) requires 'unsigned int' but the argument type is 'signed long'.\n"
                  "[scripts/test.ctl:3]: (warning) %f in format string (no. 4) requires 'double' but the argument type is 'signed long'.\n", errout.str());

    check("void f(string str1)\n"   // #5104
          "{\n"
          "    vector<short> v1 = makeDynInt(1, 0);\n"
          "    sprintf(str1, \"%d\", v1[0]);\n"
          "    vector<int> v2 = makeDynInt(1, 0);\n"
          "    sprintf(str1, \"%d\", v2[0]);\n"
          "    vector<int> v3 = makeDynUInt(1, 0);\n"
          "    sprintf(str1, \"%u\", v3[0]);\n"
          "    vector<uint> v4 = makeDynUInt(1, 0);\n"
          "    sprintf(str1, \"%x\", v4[0]);\n"
          "    vector<double> v5 = makeDynDouble(1, 0);\n"
          "    sprintf(str1, \"%f\", v5[0]);\n"
          "    vector<bool> v6 = makeDynBool(1, 0);\n"
          "    sprintf(str1, \"%u\", v6[0]);\n"
          "    vector<string> v7 = makeDynString(1, 0);\n"
          "    sprintf(str1, \"%s\", v7[0]);\n"
          "}\n");
    ASSERT_EQUALS("", errout.str());

    check("vector<char> v;\n" // #5151
          "void foo(string str1) {\n"
          "   sprintf(str1, \"%c %u %f\", v.at(32), v.at(32), v.at(32));\n"
          "}\n");
    ASSERT_EQUALS("[scripts/test.ctl:3]: (warning) %u in format string (no. 2) requires 'unsigned int' but the argument type is 'char'.\n"
                  "[scripts/test.ctl:3]: (warning) %f in format string (no. 3) requires 'double' but the argument type is 'char'.\n", errout.str());

    // #5486
    check("void foo(string str1)\n"
          "{\n"
          "    int test = 0;\n"
          "    sprintf(str1, \"%zd\", test);\n"
          "}\n");
    ASSERT_EQUALS("", errout.str());

    // #6009
    check("string StringByReturnValue(){}\n"
          "int IntByReturnValue(){}\n"
          "void MyFunction(string str1)\n"
          "{\n"
          "    sprintf(str1, \"%s - %s\", StringByReturnValue(), IntByReturnValue());\n"
          "}\n");
    ASSERT_EQUALS("[scripts/test.ctl:4]: (warning) %s in format string (no. 1) requires 'char *' but the argument type is 'std::string'.\n"
                  "[scripts/test.ctl:4]: (warning) %s in format string (no. 2) requires 'char *' but the argument type is 'signed int'.\n", errout.str());

    // Ticket #7445
    check("struct S { uint x; };\n"
          "S s = new S(0);\n"
          "void foo(string str1)\n"
          "{\n"
          "    sprintf(str1, \"%d\", s.x);\n"
          "}\n");
    ASSERT_EQUALS("", errout.str());

    // Ticket #7601
    check("void foo(string str1, char c, short s, int i, uint ui, long l, ulong ul)\n"
          "{\n"
          "    sprintf(str1, \"%hhd %hhd %hhd %hhd %hhd %hhd\", c, s, i, ui, l, ul);\n"
          "}\n");
    ASSERT_EQUALS("[test.cpp:2]: (warning) %hhd in format string (no. 2) requires 'char' but the argument type is 'short'.\n"
                  "[test.cpp:2]: (warning) %hhd in format string (no. 3) requires 'char' but the argument type is 'int'.\n"
                  "[test.cpp:2]: (warning) %hhd in format string (no. 4) requires 'char' but the argument type is 'uint'.\n"
                  "[test.cpp:2]: (warning) %hhd in format string (no. 5) requires 'char' but the argument type is 'long'.\n"
                  "[test.cpp:2]: (warning) %hhd in format string (no. 6) requires 'char' but the argument type is 'ulong'.\n",
                  errout.str());

    check("void foo(string str1, char c, short s, int i, uint ui, long l, ulong ul)\n"
          "{\n"
          "    sprintf(str1, \"%hd %hd %hd %hd %hd %hd\", c, s, i, ui, l, ul);\n"
          "}\n");
    ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) %hd in format string (no. 1) requires 'short' but the argument type is 'char'.\n"
                  "[scripts/test.ctl:2]: (warning) %hd in format string (no. 3) requires 'short' but the argument type is 'int'.\n"
                  "[scripts/test.ctl:2]: (warning) %hd in format string (no. 4) requires 'short' but the argument type is 'uint'.\n"
                  "[scripts/test.ctl:2]: (warning) %hd in format string (no. 5) requires 'short' but the argument type is 'long'.\n"
                  "[scripts/test.ctl:2]: (warning) %hd in format string (no. 6) requires 'short' but the argument type is 'ulong'.\n",
                  errout.str());

    // #7837 - Use ValueType for function call
    check("struct S\n"
          "{\n"
          "    double f;\n"
          "};\n"
          "void foo(string str1, S x)\n"
          "{\n"
          "    sprintf(str1, \"%f\", x.f(4.0));\n"
          "}\n");
    ASSERT_EQUALS("", errout.str());
  }

  void testPosixPrintfScanfParameterPosition()   // #4900  - No support for parameters in format strings
  {
    check("void foo(string str1)\n"
          "{\n"
          "  int bar;\n"
          "  sprintf(str1, \"%1$d\", 1);\n"
          "  sprintf(str1, \"%1$d, %d, %1$d\", 1, 2);\n"
          "  sscanf(str1, \"%1$d\", bar);\n"
          "}\n");
    ASSERT_EQUALS("", errout.str());

    check("void foo(string str1) {\n"
          "  int bar;\n"
          "  sprintf(str1, \"%1$d\");\n"
          "  sprintf(str1, \"%1$d, %d, %4$d\", 1, 2, 3);\n"
          "  sscanf(str1, \"%2$d\", bar);\n"
          "  sprintf(str1, \"%0$f\", 0.0);\n"
          "}");
    ASSERT_EQUALS("[scripts/test.ctl:3]: (error) sprintf format string requires 1 parameter but only 0 are given.\n"
                  "[scripts/test.ctl:4]: (warning) sprintf: referencing parameter 4 while 3 arguments given\n"
                  "[scripts/test.ctl:5]: (warning) sscanf: referencing parameter 2 while 1 arguments given\n"
                  "[scripts/test.ctl:6]: (warning) sprintf: parameter positions start at 1, not 0\n",
                  errout.str());
  }

  void testTernary()    // ticket #6182
  {
    check("void test(string str1, const string &val) {\n"
          "    sprintf(str1, \"%s\", val.empty() ? \"I like to eat bananas\" : val.c_str());\n"
          "}\n");
    ASSERT_EQUALS("", errout.str());
  }

  void testUnsignedConst()    // ticket #6321
  {
    check("void test(string str1) {\n"
          "    const unsigned x = 5;\n"
          "    sprintf(str1, \"%u\", x);\n"
          "}\n");
    ASSERT_EQUALS("", errout.str());
  }

  void testAstType()   // ticket #7014
  {
    check("void test() {\n"
          "    printf(\"%c\", \"hello\"[0]);\n"
          "}\n");
    ASSERT_EQUALS("", errout.str());
  }

  void testPrintfTypeAlias1()
  {
    check("using INT = int;\n"
          "INT i;\n"
          "const INT pci;\n"
          "void foo(string str1) {\n"
          "    sprintf(str1, \"%d %p\", i, pci);\n"
          "}\n");
    ASSERT_EQUALS("", errout.str());

    check("using INT = int;\n"
          "INT i;\n"
          "const INT pci;\n"
          "void foo(string str1) {\n"
          "    sprintf(str1, \"%f %f\", i, pci);\n"
          "}\n");
    ASSERT_EQUALS("[scripts/test.ctl:8]: (warning) %f in format string (no. 1) requires 'double' but the argument type is 'signed int'.\n"
                  "[scripts/test.ctl:8]: (warning) %f in format string (no. 2) requires 'double' but the argument type is 'const signed int'.\n", errout.str());
  }
};

void main()
{
  TestIO test;
  test.run();
}
