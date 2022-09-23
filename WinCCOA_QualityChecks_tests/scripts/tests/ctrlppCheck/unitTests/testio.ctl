#uses "classes/QualityGates/Tools/CppCheck/TestFixture"

// #include "checkio.h"
// #include "platform.h"
// #include "settings.h"
// #include "testsuite.h"
// #include "tokenize.h"


struct TestIO : TestFixture {

    void run() {
        TEST_CASE(coutCerrMisusage);

        TEST_CASE(wrongMode_simple);
        TEST_CASE(wrongMode_complex);
        TEST_CASE(useClosedFile);
        TEST_CASE(fileIOwithoutPositioning);
        TEST_CASE(seekOnAppendedFile);
        TEST_CASE(fflushOnInputStream);

        TEST_CASE(testScanf1); // Scanf without field limiters
        TEST_CASE(testScanf2);
        TEST_CASE(testScanf3); // #3494
        TEST_CASE(testScanf4); // #ticket 2553

        TEST_CASE(testScanfArgument);
        TEST_CASE(testPrintfArgumentVariables);
        TEST_CASE(testPosixPrintfScanfParameterPosition);  // #4900

        TEST_CASE(testMicrosoftPrintfArgument); // ticket #4902
        TEST_CASE(testMicrosoftScanfArgument);

        TEST_CASE(testTernary); // ticket #6182
        TEST_CASE(testUnsignedConst); // ticket #6132

        TEST_CASE(testAstType); // #7014
        TEST_CASE(testPrintf0WithSuffix); // ticket #7069
        TEST_CASE(testReturnValueTypeStdLib);

        TEST_CASE(testPrintfTypeAlias1);
        TEST_CASE(testPrintfAuto); // #8992
    }

    void coutCerrMisusage() {
        check(
            "void foo() {\n"
            "  std::cout << std::cout;\n"
            "}");
        ASSERT_EQUALS("[test.cpp:2]: (error) Invalid usage of output stream: '<< std::cout'.\n", errout.str());

        check(
            "void foo() {\n"
            "  std::cout << (std::cout);\n"
            "}");
        ASSERT_EQUALS("[test.cpp:2]: (error) Invalid usage of output stream: '<< std::cout'.\n", errout.str());

        check(
            "void foo() {\n"
            "  std::cout << \"xyz\" << std::cout;\n"
            "}");
        ASSERT_EQUALS("[test.cpp:2]: (error) Invalid usage of output stream: '<< std::cout'.\n", errout.str());

        check(
            "void foo(int i) {\n"
            "  std::cout << i << std::cerr;\n"
            "}");
        ASSERT_EQUALS("[test.cpp:2]: (error) Invalid usage of output stream: '<< std::cerr'.\n", errout.str());

        check(
            "void foo() {\n"
            "  std::cout << \"xyz\";\n"
            "  std::cout << \"xyz\";\n"
            "}");
        ASSERT_EQUALS("", errout.str());

        check(
            "void foo() {\n"
            "  std::cout << std::cout.good();\n"
            "}");
        ASSERT_EQUALS("", errout.str());

        check(
            "void foo() {\n"
            "    unknownObject << std::cout;\n"
            "}");
        ASSERT_EQUALS("", errout.str());

        check(
            "void foo() {\n"
            "  MACRO(std::cout <<, << std::cout)\n"
            "}");
        ASSERT_EQUALS("", errout.str());
    }



    void wrongMode_simple() {
        // Read mode
        check("void foo(FILE*& f) {\n"
              "    f = fopen(name, \"r\");\n"
              "    fread(buffer, 5, 6, f);\n"
              "    rewind(f);\n"
              "    fwrite(buffer, 5, 6, f);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:5]: (error) Write operation on a file that was opened only for reading.\n", errout.str());

        check("void foo(FILE*& f) {\n"
              "    f = fopen(name, \"r+\");\n"
              "    fwrite(buffer, 5, 6, f);\n"
              "}");
        ASSERT_EQUALS("", errout.str());


        // Write mode
        check("void foo(FILE*& f) {\n"
              "    f = fopen(name, \"w\");\n"
              "    fwrite(buffer, 5, 6, f);\n"
              "    rewind(f);\n"
              "    fread(buffer, 5, 6, f);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:5]: (error) Read operation on a file that was opened only for writing.\n", errout.str());

        check("void foo(FILE*& f) {\n"
              "    f = fopen(name, \"w+\");\n"
              "    fread(buffer, 5, 6, f);\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("void foo(FILE*& f) {\n"
              "    f = tmpfile();\n"
              "    fread(buffer, 5, 6, f);\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        // Append mode
        check("void foo(FILE*& f) {\n"
              "    f = fopen(name, \"a\");\n"
              "    fwrite(buffer, 5, 6, f);\n"
              "    rewind(f);\n"
              "    fread(buffer, 5, 6, f);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:4]: (warning) Repositioning operation performed on a file opened in append mode has no effect.\n"
                      "[test.cpp:5]: (error) Read operation on a file that was opened only for writing.\n", errout.str());

        check("void foo(FILE*& f) {\n"
              "    f = fopen(name, \"a+\");\n"
              "    fread(buffer, 5, 6, f);\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        // Variable declared locally
        check("void foo() {\n"
              "    FILE* f = fopen(name, \"r\");\n"
              "    fwrite(buffer, 5, 6, f);\n"
              "    fclose(f);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:3]: (error) Write operation on a file that was opened only for reading.\n", errout.str());

        // Call unknown function
        check("void foo(FILE*& f) {\n"
              "    f = fopen(name, \"a\");\n"
              "    fwrite(buffer, 5, 6, f);\n"
              "    bar(f);\n"
              "    fread(buffer, 5, 6, f);\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        // freopen and tmpfile
        check("void foo(FILE*& f) {\n"
              "    f = freopen(name, \"r\", f);\n"
              "    fwrite(buffer, 5, 6, f);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:3]: (error) Write operation on a file that was opened only for reading.\n", errout.str());

        // Crash tests
        check("void foo(FILE*& f) {\n"
              "    f = fopen(name, mode);\n" // No assertion failure (#3830)
              "    fwrite(buffer, 5, 6, f);\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("void fopen(std::string const &filepath, std::string const &mode);"); // #3832
    }

    void wrongMode_complex() {
        check("void foo(FILE* f) {\n"
              "    if(a) f = fopen(name, \"w\");\n"
              "    else  f = fopen(name, \"r\");\n"
              "    if(a) fwrite(buffer, 5, 6, f);\n"
              "    else  fread(buffer, 5, 6, f);\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("void foo() {\n"
              "    FILE* f;\n"
              "    if(a) f = fopen(name, \"w\");\n"
              "    else  f = fopen(name, \"r\");\n"
              "    if(a) fwrite(buffer, 5, 6, f);\n"
              "    else  fread(buffer, 5, 6, f);\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("void foo() {\n"
              "    FILE* f = fopen(name, \"w\");\n"
              "    if(a) fwrite(buffer, 5, 6, f);\n"
              "    else  fread(buffer, 5, 6, f);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:4]: (error) Read operation on a file that was opened only for writing.\n", errout.str());
    }

    void useClosedFile() {
        check("void foo(FILE*& f) {\n"
              "    fclose(f);\n"
              "    fwrite(buffer, 5, 6, f);\n"
              "    clearerr(f);\n"
              "    fread(buffer, 5, 6, f);\n"
              "    ungetc('a', f);\n"
              "    ungetwc(L'a', f);\n"
              "    rewind(f);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:3]: (error) Used file that is not opened.\n"
                      "[test.cpp:4]: (error) Used file that is not opened.\n"
                      "[test.cpp:5]: (error) Used file that is not opened.\n"
                      "[test.cpp:6]: (error) Used file that is not opened.\n"
                      "[test.cpp:7]: (error) Used file that is not opened.\n"
                      "[test.cpp:8]: (error) Used file that is not opened.\n", errout.str());

        check("void foo(FILE*& f) {\n"
              "    if(!ferror(f)) {\n"
              "        fclose(f);\n"
              "        return;"
              "    }\n"
              "    fwrite(buffer, 5, 6, f);\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("void foo(FILE*& f) {\n"
              "    fclose(f);\n"
              "    f = fopen(name, \"r\");\n"
              "    fread(buffer, 5, 6, f);\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("void foo(FILE*& f) {\n"
              "    f = fopen(name, \"r\");\n"
              "    f = g;\n"
              "    fwrite(buffer, 5, 6, f);\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("void foo() {\n"
              "    FILE* f;\n"
              "    fwrite(buffer, 5, 6, f);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:3]: (error) Used file that is not opened.\n", errout.str());

        check("void foo() {\n"
              "    FILE* f(stdout);\n"
              "    fwrite(buffer, 5, 6, f);\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("void foo() {\n" // #3965
              "    FILE* f[3];\n"
              "    f[0] = fopen(name, mode);\n"
              "    fclose(f[0]);\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        // #4368: multiple functions
        check("static FILE *fp = NULL;\n"
              "\n"
              "void close()\n"
              "{\n"
              "  fclose(fp);\n"
              "}\n"
              "\n"
              "void dump()\n"
              "{\n"
              "  if (fp == NULL) return;\n"
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

        check("static FILE *fp = NULL;\n"
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
        ASSERT_EQUALS("[test.cpp:11]: (error) Used file that is not opened.\n", errout.str());

        // #4466
        check("void chdcd_parse_nero(FILE *infile) {\n"
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

        check("void chdcd_parse_nero(FILE *infile) {\n"
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
        check("void foo() {\n"
              "    struct {FILE *f1; FILE *f2;} a;\n"
              "    a.f1 = fopen(name,mode);\n"
              "    a.f2 = fopen(name,mode);\n"
              "    fclose(a.f1);\n"
              "    fclose(a.f2);\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        // #1473
        check("void foo() {\n"
              "    FILE *a = fopen(\"aa\", \"r\");\n"
              "    while (fclose(a)) {}\n"
              "}");
        ASSERT_EQUALS("[test.cpp:3]: (error) Used file that is not opened.\n", errout.str());

        // #6823
        check("void foo() {\n"
              "    FILE f[2];\n"
              "    f[0] = fopen(\"1\", \"w\");\n"
              "    f[1] = fopen(\"2\", \"w\");\n"
              "    fclose(f[0]);\n"
              "    fclose(f[1]);\n"
              "}");
        ASSERT_EQUALS("", errout.str());
    }

    void fileIOwithoutPositioning() {
        check("void foo(FILE* f) {\n"
              "    fwrite(buffer, 5, 6, f);\n"
              "    fread(buffer, 5, 6, f);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:3]: (error) Read and write operations without a call to a positioning function (fseek, fsetpos or rewind) or fflush in between result in undefined behaviour.\n", errout.str());

        check("void foo(FILE* f) {\n"
              "    fread(buffer, 5, 6, f);\n"
              "    fwrite(buffer, 5, 6, f);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:3]: (error) Read and write operations without a call to a positioning function (fseek, fsetpos or rewind) or fflush in between result in undefined behaviour.\n", errout.str());

        check("void foo(FILE* f, bool read) {\n"
              "    if(read)\n"
              "        fread(buffer, 5, 6, f);\n"
              "    else\n"
              "        fwrite(buffer, 5, 6, f);\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("void foo(FILE* f) {\n"
              "    fread(buffer, 5, 6, f);\n"
              "    fflush(f);\n"
              "    fwrite(buffer, 5, 6, f);\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("void foo(FILE* f) {\n"
              "    fread(buffer, 5, 6, f);\n"
              "    rewind(f);\n"
              "    fwrite(buffer, 5, 6, f);\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("void foo(FILE* f) {\n"
              "    fread(buffer, 5, 6, f);\n"
              "    fsetpos(f, pos);\n"
              "    fwrite(buffer, 5, 6, f);\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("void foo(FILE* f) {\n"
              "    fread(buffer, 5, 6, f);\n"
              "    fseek(f, 0, SEEK_SET);\n"
              "    fwrite(buffer, 5, 6, f);\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("void foo(FILE* f) {\n"
              "    fread(buffer, 5, 6, f);\n"
              "    long pos = ftell(f);\n"
              "    fwrite(buffer, 5, 6, f);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:4]: (error) Read and write operations without a call to a positioning function (fseek, fsetpos or rewind) or fflush in between result in undefined behaviour.\n", errout.str());

        // #6452 - member functions
        check("class FileStream {\n"
              "    void insert(const ByteVector &data, ulong start);\n"
              "    void seek(long offset, Position p);\n"
              "    FileStreamPrivate *d;\n"
              "};\n"
              "void FileStream::insert(const ByteVector &data, ulong start) {\n"
              "    int bytesRead = fread(aboutToOverwrite.data(), 1, bufferLength, d->file);\n"
              "    seek(writePosition);\n"
              "    fwrite(buffer.data(), sizeof(char), buffer.size(), d->file);\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("class FileStream {\n"
              "    void insert(const ByteVector &data, ulong start);\n"
              "    FileStreamPrivate *d;\n"
              "};\n"
              "void FileStream::insert(const ByteVector &data, ulong start) {\n"
              "    int bytesRead = fread(aboutToOverwrite.data(), 1, bufferLength, d->file);\n"
              "    unknown(writePosition);\n"
              "    fwrite(buffer.data(), sizeof(char), buffer.size(), d->file);\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("class FileStream {\n"
              "    void insert(const ByteVector &data, ulong start);\n"
              "    FileStreamPrivate *d;\n"
              "};\n"
              "void known(int);\n"
              "void FileStream::insert(const ByteVector &data, ulong start) {\n"
              "    int bytesRead = fread(aboutToOverwrite.data(), 1, bufferLength, d->file);\n"
              "    known(writePosition);\n"
              "    fwrite(buffer.data(), sizeof(char), buffer.size(), d->file);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:9]: (error) Read and write operations without a call to a positioning function (fseek, fsetpos or rewind) or fflush in between result in undefined behaviour.\n", errout.str());

        check("class FileStream {\n"
              "    void insert(const ByteVector &data, ulong start);\n"
              "    FileStreamPrivate *d;\n"
              "};\n"
              "void known(int);\n"
              "void FileStream::insert(const ByteVector &data, ulong start) {\n"
              "    int bytesRead = fread(X::data(), 1, bufferLength, d->file);\n"
              "    known(writePosition);\n"
              "    fwrite(X::data(), sizeof(char), buffer.size(), d->file);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:9]: (error) Read and write operations without a call to a positioning function (fseek, fsetpos or rewind) or fflush in between result in undefined behaviour.\n", errout.str());
    }

    void seekOnAppendedFile() {
        check("void foo() {\n"
              "    FILE* f = fopen(\"\", \"a+\");\n"
              "    fseek(f, 0, SEEK_SET);\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("void foo() {\n"
              "    FILE* f = fopen(\"\", \"w\");\n"
              "    fseek(f, 0, SEEK_SET);\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("void foo() {\n"
              "    FILE* f = fopen(\"\", \"a\");\n"
              "    fseek(f, 0, SEEK_SET);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:3]: (warning) Repositioning operation performed on a file opened in append mode has no effect.\n", errout.str());

        check("void foo() {\n"
              "    FILE* f = fopen(\"\", \"a\");\n"
              "    fflush(f);\n"
              "}");
        ASSERT_EQUALS("", errout.str()); // #5578

        check("void foo() {\n"
              "    FILE* f = fopen(\"\", \"a\");\n"
              "    fclose(f);\n"
              "    f = fopen(\"\", \"r\");\n"
              "    fseek(f, 0, SEEK_SET);\n"
              "}");
        ASSERT_EQUALS("", errout.str()); // #6566
    }

    void fflushOnInputStream() {
        check("void foo()\n"
              "{\n"
              "    fflush(stdin);\n"
              "}", false, true);
        ASSERT_EQUALS("[test.cpp:3]: (portability) fflush() called on input stream 'stdin' may result in undefined behaviour on non-linux systems.\n", errout.str());

        check("void foo()\n"
              "{\n"
              "    fflush(stdout);\n"
              "}", false, true);
        ASSERT_EQUALS("", errout.str());

        check("void foo(FILE*& f) {\n"
              "    f = fopen(path, \"r\");\n"
              "    fflush(f);\n"
              "}", false, true);
        ASSERT_EQUALS("[test.cpp:3]: (portability) fflush() called on input stream 'f' may result in undefined behaviour on non-linux systems.\n", errout.str());

        check("void foo(FILE*& f) {\n"
              "    f = fopen(path, \"w\");\n"
              "    fflush(f);\n"
              "}", false, true);
        ASSERT_EQUALS("", errout.str());

        check("void foo(FILE*& f) {\n"
              "    fflush(f);\n"
              "}", false, true);
        ASSERT_EQUALS("", errout.str());
    }

    void testScanf1() {
        check("void foo() {\n"
              "    int a, b;\n"
              "    FILE *file = fopen(\"test\", \"r\");\n"
              "    a = fscanf(file, \"aa %s\", bar);\n"
              "    b = scanf(\"aa %S\", bar);\n"
              "    b = scanf(\"aa %ls\", bar);\n"
              "    sscanf(foo, \"%[^~]\", bar);\n"
              "    scanf(\"%dx%s\", &b, bar);\n"
              "    fclose(file);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:4]: (warning) fscanf() without field width limits can crash with huge input data.\n"
                      "[test.cpp:5]: (warning) scanf() without field width limits can crash with huge input data.\n"
                      "[test.cpp:6]: (warning) scanf() without field width limits can crash with huge input data.\n"
                      "[test.cpp:7]: (warning) sscanf() without field width limits can crash with huge input data.\n"
                      "[test.cpp:8]: (warning) scanf() without field width limits can crash with huge input data.\n", errout.str());
    }

    void testScanf2() {
        check("void foo() {\n"
              "    scanf(\"%5s\", bar);\n" // Width specifier given
              "    scanf(\"%5[^~]\", bar);\n" // Width specifier given
              "    scanf(\"aa%%s\", bar);\n" // No %s
              "    scanf(\"aa%d\", &a);\n" // No %s
              "    scanf(\"aa%ld\", &a);\n" // No %s
              "    scanf(\"%*[^~]\");\n" // Ignore input
              "}");
        ASSERT_EQUALS("[test.cpp:4]: (warning) scanf format string requires 0 parameters but 1 is given.\n", errout.str());
    }

    void testScanf3() { // ticket #3494
        check("void f() {\n"
              "  char str[8];\n"
              "  scanf(\"%7c\", str);\n"
              "  scanf(\"%8c\", str);\n"
              "  scanf(\"%9c\", str);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:5]: (error) Width 9 given in format string (no. 1) is larger than destination buffer 'str[8]', use %8c to prevent overflowing it.\n", errout.str());
    }

    void testScanf4() { // ticket #2553
        check("void f()\n"
              "{\n"
              "  char str [8];\n"
              "  scanf (\"%70s\",str);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:4]: (error) Width 70 given in format string (no. 1) is larger than destination buffer 'str[8]', use %7s to prevent overflowing it.\n", errout.str());
    }

    void testScanfArgument() {
        check("void foo() {\n"
              "    scanf(\"%1d\", &foo);\n"
              "    sscanf(bar, \"%1d\", &foo);\n"
              "    scanf(\"%1u%1u\", &foo, bar());\n"
              "    scanf(\"%*1x %1x %29s\", &count, KeyName);\n" // #3373
              "    fscanf(f, \"%7ms\", &ref);\n" // #3461
              "    sscanf(ip_port, \"%*[^:]:%4d\", &port);\n" // #3468
              "}");
        ASSERT_EQUALS("", errout.str());

        check("void foo() {\n"
              "    scanf(\"\", &foo);\n"
              "    scanf(\"%1d\", &foo, &bar);\n"
              "    fscanf(bar, \"%1d\", &foo, &bar);\n"
              "    scanf(\"%*1x %1x %29s\", &count, KeyName, foo);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:2]: (warning) scanf format string requires 0 parameters but 1 is given.\n"
                      "[test.cpp:3]: (warning) scanf format string requires 1 parameter but 2 are given.\n"
                      "[test.cpp:4]: (warning) fscanf format string requires 1 parameter but 2 are given.\n"
                      "[test.cpp:5]: (warning) scanf format string requires 2 parameters but 3 are given.\n", errout.str());

        check("void foo() {\n"
              "    scanf(\"%1d\");\n"
              "    scanf(\"%1u%1u\", bar());\n"
              "    sscanf(bar, \"%1d%1d\", &foo);\n"
              "    scanf(\"%*1x %1x %29s\", &count);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:2]: (error) scanf format string requires 1 parameter but only 0 are given.\n"
                      "[test.cpp:3]: (error) scanf format string requires 2 parameters but only 1 is given.\n"
                      "[test.cpp:4]: (error) sscanf format string requires 2 parameters but only 1 is given.\n"
                      "[test.cpp:5]: (error) scanf format string requires 2 parameters but only 1 is given.\n", errout.str());

        check("void foo() {\n"
              "    char input[10];\n"
              "    char output[5];\n"
              "    sscanf(input, \"%3s\", output);\n"
              "    sscanf(input, \"%4s\", output);\n"
              "    sscanf(input, \"%5s\", output);\n"
              "}", false);
        ASSERT_EQUALS("[test.cpp:6]: (error) Width 5 given in format string (no. 1) is larger than destination buffer 'output[5]', use %4s to prevent overflowing it.\n", errout.str());

        check("void foo() {\n"
              "    char input[10];\n"
              "    char output[5];\n"
              "    sscanf(input, \"%s\", output);\n"
              "    sscanf(input, \"%3s\", output);\n"
              "    sscanf(input, \"%4s\", output);\n"
              "    sscanf(input, \"%5s\", output);\n"
              "}", true);
        ASSERT_EQUALS("[test.cpp:5]: (warning, inconclusive) Width 3 given in format string (no. 1) is smaller than destination buffer 'output[5]'.\n"
                      "[test.cpp:7]: (error) Width 5 given in format string (no. 1) is larger than destination buffer 'output[5]', use %4s to prevent overflowing it.\n"
                      "[test.cpp:4]: (warning) sscanf() without field width limits can crash with huge input data.\n", errout.str());

        check("void foo() {\n"
              "    const size_t BUFLENGTH(2048);\n"
              "    typedef char bufT[BUFLENGTH];\n"
              "    bufT line= {0};\n"
              "    bufT projectId= {0};\n"
              "    const int scanrc=sscanf(line, \"Project(\\\"{%36s}\\\")\", projectId);\n"
              "    sscanf(input, \"%5s\", output);\n"
              "}", true);
        ASSERT_EQUALS("", errout.str());

        check("void foo(unsigned int i) {\n"
              "  scanf(\"%h\", &i);\n"
              "  scanf(\"%hh\", &i);\n"
              "  scanf(\"%l\", &i);\n"
              "  scanf(\"%ll\", &i);\n"
              "  scanf(\"%j\", &i);\n"
              "  scanf(\"%z\", &i);\n"
              "  scanf(\"%t\", &i);\n"
              "  scanf(\"%L\", &i);\n"
              "  scanf(\"%I\", &i);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:2]: (warning) 'h' in format string (no. 1) is a length modifier and cannot be used without a conversion specifier.\n"
                      "[test.cpp:3]: (warning) 'hh' in format string (no. 1) is a length modifier and cannot be used without a conversion specifier.\n"
                      "[test.cpp:4]: (warning) 'l' in format string (no. 1) is a length modifier and cannot be used without a conversion specifier.\n"
                      "[test.cpp:5]: (warning) 'll' in format string (no. 1) is a length modifier and cannot be used without a conversion specifier.\n"
                      "[test.cpp:6]: (warning) 'j' in format string (no. 1) is a length modifier and cannot be used without a conversion specifier.\n"
                      "[test.cpp:7]: (warning) 'z' in format string (no. 1) is a length modifier and cannot be used without a conversion specifier.\n"
                      "[test.cpp:8]: (warning) 't' in format string (no. 1) is a length modifier and cannot be used without a conversion specifier.\n"
                      "[test.cpp:9]: (warning) 'L' in format string (no. 1) is a length modifier and cannot be used without a conversion specifier.\n"
                      "[test.cpp:10]: (warning) 'I' in format string (no. 1) is a length modifier and cannot be used without a conversion specifier.\n", errout.str());


        check("void foo() {\n"
              "    scanf(\"%u\", \"s3\");\n"
              "}", true);
        ASSERT_EQUALS("[test.cpp:2]: (warning) %u in format string (no. 1) requires 'unsigned int *' but the argument type is 'const char *'.\n", errout.str());

        check("void foo(long l) {\n"
              "    scanf(\"%u\", l);\n"
              "}", true);
        ASSERT_EQUALS("[test.cpp:2]: (warning) %u in format string (no. 1) requires 'unsigned int *' but the argument type is 'signed long'.\n", errout.str());

        check("void foo() {\n"
              "    scanf(\"%Ld\", \"s3\");\n"
              "}", true);
        ASSERT_EQUALS("[test.cpp:2]: (warning) %Ld in format string (no. 1) requires 'long long *' but the argument type is 'const char *'.\n", errout.str());

        check("void foo(int i) {\n"
              "    scanf(\"%Ld\", i);\n"
              "}", true);
        ASSERT_EQUALS("[test.cpp:2]: (warning) %Ld in format string (no. 1) requires 'long long *' but the argument type is 'signed int'.\n", errout.str());

        check("void foo() {\n"
              "    scanf(\"%d\", \"s3\");\n"
              "}", true);
        ASSERT_EQUALS("[test.cpp:2]: (warning) %d in format string (no. 1) requires 'int *' but the argument type is 'const char *'.\n", errout.str());

        check("void foo(long l) {\n"
              "    scanf(\"%d\", l);\n"
              "}", true);
        ASSERT_EQUALS("[test.cpp:2]: (warning) %d in format string (no. 1) requires 'int *' but the argument type is 'signed long'.\n", errout.str());

        check("void foo() {\n"
              "    scanf(\"%x\", \"s3\");\n"
              "}", true);
        ASSERT_EQUALS("[test.cpp:2]: (warning) %x in format string (no. 1) requires 'unsigned int *' but the argument type is 'const char *'.\n", errout.str());

        check("void foo(long l) {\n"
              "    scanf(\"%x\", l);\n"
              "}", true);
        ASSERT_EQUALS("[test.cpp:2]: (warning) %x in format string (no. 1) requires 'unsigned int *' but the argument type is 'signed long'.\n", errout.str());

        check("void foo() {\n"
              "    scanf(\"%f\", \"s3\");\n"
              "}", true);
        ASSERT_EQUALS("[test.cpp:2]: (warning) %f in format string (no. 1) requires 'float *' but the argument type is 'const char *'.\n", errout.str());

        check("void foo(float f) {\n"
              "    scanf(\"%f\", f);\n"
              "}", true);
        ASSERT_EQUALS("[test.cpp:2]: (warning) %f in format string (no. 1) requires 'float *' but the argument type is 'float'.\n", errout.str());

        check("void foo() {\n"
              "    scanf(\"%n\", \"s3\");\n"
              "}", true);
        ASSERT_EQUALS("[test.cpp:2]: (warning) %n in format string (no. 1) requires 'int *' but the argument type is 'const char *'.\n", errout.str());

        check("void foo(long l) {\n"
              "    scanf(\"%n\", l);\n"
              "}", true);
        ASSERT_EQUALS("[test.cpp:2]: (warning) %n in format string (no. 1) requires 'int *' but the argument type is 'signed long'.\n", errout.str());

        check("void g() {\n" // #5104
              "    myvector<int> v1(1);\n"
              "    scanf(\"%d\",&v1[0]);\n"
              "    myvector<unsigned int> v2(1);\n"
              "    scanf(\"%u\",&v2[0]);\n"
              "    myvector<unsigned int> v3(1);\n"
              "    scanf(\"%x\",&v3[0]);\n"
              "    myvector<double> v4(1);\n"
              "    scanf(\"%lf\",&v4[0]);\n"
              "    myvector<char *> v5(1);\n"
              "    scanf(\"%10s\",v5[0]);\n"
              "}\n");
        ASSERT_EQUALS("", errout.str());

        {
            check("void g() {\n"
                  "    const char c[]=\"42\";\n"
                  "    scanf(\"%s\", c);\n"
                  "}\n");
            ASSERT_EQUALS("[test.cpp:3]: (warning) %s in format string (no. 1) requires a 'char *' but the argument type is 'const char *'.\n"
                          "[test.cpp:3]: (warning) scanf() without field width limits can crash with huge input data.\n", errout.str());
        }
    }

    void testPrintfArgument() {
        check("void foo() {\n"
              "    printf(\"%i\");\n"
              "    printf(\"%i%s\", 123);\n"
              "    printf(\"%i%s%d\", 0, bar());\n"
              "    printf(\"%i%%%s%d\", 0, bar());\n"
              "    printf(\"%idfd%%dfa%s%d\", 0, bar());\n"
              "    fprintf(stderr,\"%u%s\");\n"
              "    snprintf(str,10,\"%u%s\");\n"
              "    sprintf(string1, \"%-*.*s\", 32, string2);\n" // #3364
              "    snprintf(a, 9, \"%s%d\", \"11223344\");\n" // #3655
              "}");
        ASSERT_EQUALS("[test.cpp:2]: (error) printf format string requires 1 parameter but only 0 are given.\n"
                      "[test.cpp:3]: (error) printf format string requires 2 parameters but only 1 is given.\n"
                      "[test.cpp:4]: (error) printf format string requires 3 parameters but only 2 are given.\n"
                      "[test.cpp:5]: (error) printf format string requires 3 parameters but only 2 are given.\n"
                      "[test.cpp:6]: (error) printf format string requires 3 parameters but only 2 are given.\n"
                      "[test.cpp:7]: (error) fprintf format string requires 2 parameters but only 0 are given.\n"
                      "[test.cpp:8]: (error) snprintf format string requires 2 parameters but only 0 are given.\n"
                      "[test.cpp:9]: (error) sprintf format string requires 3 parameters but only 2 are given.\n"
                      "[test.cpp:10]: (error) snprintf format string requires 2 parameters but only 1 is given.\n", errout.str());

        check("void foo(char *str) {\n"
              "    printf(\"\", 0);\n"
              "    printf(\"%i\", 123, bar());\n"
              "    printf(\"%i%s\", 0, bar(), 43123);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:2]: (warning) printf format string requires 0 parameters but 1 is given.\n"
                      "[test.cpp:3]: (warning) printf format string requires 1 parameter but 2 are given.\n"
                      "[test.cpp:4]: (warning) printf format string requires 2 parameters but 3 are given.\n", errout.str());

        check("void foo() {\n" // swprintf exists as MSVC extension and as standard function: #4790
              "    swprintf(string1, L\"%i\", 32, string2);\n" // MSVC implementation
              "    swprintf(string1, L\"%s%s\", L\"a\", string2);\n" // MSVC implementation
              "    swprintf(string1, 6, L\"%i\", 32, string2);\n" // Standard implementation
              "    swprintf(string1, 6, L\"%i%s\", 32, string2);\n" // Standard implementation
              "}");
        ASSERT_EQUALS("[test.cpp:2]: (warning) swprintf format string requires 1 parameter but 2 are given.\n"
                      "[test.cpp:4]: (warning) swprintf format string requires 1 parameter but 2 are given.\n", errout.str());

        check("void foo(char *str) {\n"
              "    printf(\"%i\", 0);\n"
              "    printf(\"%i%s\", 123, bar());\n"
              "    printf(\"%i%s%d\", 0, bar(), 43123);\n"
              "    printf(\"%i%%%s%d\", 0, bar(), 43123);\n"
              "    printf(\"%idfd%%dfa%s%d\", 0, bar(), 43123);\n"
              "    printf(\"%\"PRId64\"\", 123);\n"
              "    fprintf(stderr,\"%\"PRId64\"\", 123);\n"
              "    snprintf(str,10,\"%\"PRId64\"\", 123);\n"
              "    fprintf(stderr, \"error: %m\");\n" // #3339
              "    printf(\"string: %.*s\", len, string);\n" // #3311
              "    fprintf(stderr, \"%*cText.\", indent, ' ');\n" // #3313
              "    sprintf(string1, \"%*\", 32);\n" // #3364
              "}");
        ASSERT_EQUALS("", errout.str());

        check("void foo(char* s, const char* s2, std::string s3, int i) {\n"
              "    printf(\"%s%s\", s, s2);\n"
              "    printf(\"%s\", i);\n"
              "    printf(\"%i%s\", i, i);\n"
              "    printf(\"%s\", s3);\n"
              "    printf(\"%s\", \"s4\");\n"
              "    printf(\"%u\", s);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:3]: (warning) %s in format string (no. 1) requires 'char *' but the argument type is 'signed int'.\n"
                      "[test.cpp:4]: (warning) %s in format string (no. 2) requires 'char *' but the argument type is 'signed int'.\n"
                      "[test.cpp:5]: (warning) %s in format string (no. 1) requires 'char *' but the argument type is 'std::string'.\n"
                      "[test.cpp:7]: (warning) %u in format string (no. 1) requires 'unsigned int' but the argument type is 'char *'.\n", errout.str());

        check("void foo(char* s, const char* s2, std::string s3, int i) {\n"
              "    printf(\"%jd\", s);\n"
              "    printf(\"%ji\", s);\n"
              "    printf(\"%ju\", s2);\n"
              "    printf(\"%jo\", s3);\n"
              "    printf(\"%jx\", i);\n"
              "    printf(\"%jX\", i);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:2]: (warning) %jd in format string (no. 1) requires 'intmax_t' but the argument type is 'char *'.\n"
                      "[test.cpp:3]: (warning) %ji in format string (no. 1) requires 'intmax_t' but the argument type is 'char *'.\n"
                      "[test.cpp:4]: (warning) %ju in format string (no. 1) requires 'uintmax_t' but the argument type is 'const char *'.\n"
                      "[test.cpp:5]: (warning) %jo in format string (no. 1) requires 'uintmax_t' but the argument type is 'std::string'.\n"
                      "[test.cpp:6]: (warning) %jx in format string (no. 1) requires 'uintmax_t' but the argument type is 'signed int'.\n"
                      "[test.cpp:7]: (warning) %jX in format string (no. 1) requires 'uintmax_t' but the argument type is 'signed int'.\n", errout.str());

        check("void foo(uintmax_t uim, std::string s3, unsigned int ui, int i) {\n"
              "    printf(\"%ju\", uim);\n"
              "    printf(\"%ju\", ui);\n"
              "    printf(\"%jd\", ui);\n"
              "    printf(\"%jd\", s3);\n"
              "    printf(\"%jd\", i);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:3]: (warning) %ju in format string (no. 1) requires 'uintmax_t' but the argument type is 'unsigned int'.\n"
                      "[test.cpp:4]: (warning) %jd in format string (no. 1) requires 'intmax_t' but the argument type is 'unsigned int'.\n"
                      "[test.cpp:5]: (warning) %jd in format string (no. 1) requires 'intmax_t' but the argument type is 'std::string'.\n"
                      "[test.cpp:6]: (warning) %jd in format string (no. 1) requires 'intmax_t' but the argument type is 'signed int'.\n", errout.str());

        check("void foo(const int* cpi, const int ci, int i, int* pi, std::string s) {\n"
              "    printf(\"%n\", cpi);\n"
              "    printf(\"%n\", ci);\n"
              "    printf(\"%n\", i);\n"
              "    printf(\"%n\", pi);\n"
              "    printf(\"%n\", s);\n"
              "    printf(\"%n\", \"s4\");\n"
              "}");
        ASSERT_EQUALS("[test.cpp:3]: (warning) %n in format string (no. 1) requires 'int *' but the argument type is 'signed int'.\n"
                      "[test.cpp:4]: (warning) %n in format string (no. 1) requires 'int *' but the argument type is 'signed int'.\n"
                      "[test.cpp:6]: (warning) %n in format string (no. 1) requires 'int *' but the argument type is 'std::string'.\n"
                      "[test.cpp:7]: (warning) %n in format string (no. 1) requires 'int *' but the argument type is 'const char *'.\n", errout.str());

        check("class foo {};\n"
              "void foo(const int* cpi, foo f, bar b, bar* bp, double d, int i, unsigned int u) {\n"
              "    printf(\"%X\", f);\n"
              "    printf(\"%c\", \"s4\");\n"
              "    printf(\"%o\", d);\n"
              "    printf(\"%x\", cpi);\n"
              "    printf(\"%o\", b);\n"
              "    printf(\"%X\", bp);\n"
              "    printf(\"%X\", u);\n"
              "    printf(\"%X\", i);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:3]: (warning) %X in format string (no. 1) requires 'unsigned int' but the argument type is 'foo'.\n"
                      "[test.cpp:4]: (warning) %c in format string (no. 1) requires 'unsigned int' but the argument type is 'const char *'.\n"
                      "[test.cpp:5]: (warning) %o in format string (no. 1) requires 'unsigned int' but the argument type is 'double'.\n"
                      "[test.cpp:6]: (warning) %x in format string (no. 1) requires 'unsigned int' but the argument type is 'const signed int *'.\n"
                      "[test.cpp:8]: (warning) %X in format string (no. 1) requires 'unsigned int' but the argument type is 'bar *'.\n", errout.str());

        check("class foo {};\n"
              "void foo(const char* cpc, char* pc) {\n"
              "    printf(\"%x\", cpc);\n"
              "    printf(\"%x\", pc);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:3]: (warning) %x in format string (no. 1) requires 'unsigned int' but the argument type is 'const char *'.\n"
                      "[test.cpp:4]: (warning) %x in format string (no. 1) requires 'unsigned int' but the argument type is 'char *'.\n", errout.str());

        check("class foo {};\n"
              "void foo(const int* cpi, foo f, bar b, bar* bp, double d, unsigned int u, unsigned char uc) {\n"
              "    printf(\"%i\", f);\n"
              "    printf(\"%d\", \"s4\");\n"
              "    printf(\"%d\", d);\n"
              "    printf(\"%d\", u);\n"
              "    printf(\"%d\", cpi);\n"
              "    printf(\"%i\", b);\n"
              "    printf(\"%i\", bp);\n"
              "    printf(\"%i\", uc);\n" // char is smaller than int, so there shouldn't be a problem
              "}");
        ASSERT_EQUALS("[test.cpp:3]: (warning) %i in format string (no. 1) requires 'int' but the argument type is 'foo'.\n"
                      "[test.cpp:4]: (warning) %d in format string (no. 1) requires 'int' but the argument type is 'const char *'.\n"
                      "[test.cpp:5]: (warning) %d in format string (no. 1) requires 'int' but the argument type is 'double'.\n"
                      "[test.cpp:6]: (warning) %d in format string (no. 1) requires 'int' but the argument type is 'unsigned int'.\n"
                      "[test.cpp:7]: (warning) %d in format string (no. 1) requires 'int' but the argument type is 'const signed int *'.\n"
                      "[test.cpp:9]: (warning) %i in format string (no. 1) requires 'int' but the argument type is 'bar *'.\n", errout.str());

        check("class foo {};\n"
              "void foo(const int* cpi, foo f, bar b, bar* bp, double d, int i, bool bo) {\n"
              "    printf(\"%u\", f);\n"
              "    printf(\"%u\", \"s4\");\n"
              "    printf(\"%u\", d);\n"
              "    printf(\"%u\", i);\n"
              "    printf(\"%u\", cpi);\n"
              "    printf(\"%u\", b);\n"
              "    printf(\"%u\", bp);\n"
              "    printf(\"%u\", bo);\n" // bool shouldn't have a negative sign
              "}");
        ASSERT_EQUALS("[test.cpp:3]: (warning) %u in format string (no. 1) requires 'unsigned int' but the argument type is 'foo'.\n"
                      "[test.cpp:4]: (warning) %u in format string (no. 1) requires 'unsigned int' but the argument type is 'const char *'.\n"
                      "[test.cpp:5]: (warning) %u in format string (no. 1) requires 'unsigned int' but the argument type is 'double'.\n"
                      "[test.cpp:6]: (warning) %u in format string (no. 1) requires 'unsigned int' but the argument type is 'signed int'.\n"
                      "[test.cpp:7]: (warning) %u in format string (no. 1) requires 'unsigned int' but the argument type is 'const signed int *'.\n"
                      "[test.cpp:9]: (warning) %u in format string (no. 1) requires 'unsigned int' but the argument type is 'bar *'.\n", errout.str());

        check("class foo {};\n"
              "void foo(const int* cpi, foo f, bar b, bar* bp, char c) {\n"
              "    printf(\"%p\", f);\n"
              "    printf(\"%p\", c);\n"
              "    printf(\"%p\", bp);\n"
              "    printf(\"%p\", cpi);\n"
              "    printf(\"%p\", b);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:3]: (warning) %p in format string (no. 1) requires an address but the argument type is 'foo'.\n"
                      "[test.cpp:4]: (warning) %p in format string (no. 1) requires an address but the argument type is 'char'.\n", errout.str());

        check("class foo {};\n"
              "void foo(char* pc, const char* cpc) {\n"
              "    printf(\"%p\", pc);\n"
              "    printf(\"%p\", cpc);\n"
              "    printf(\"%p\", \"s4\");\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("class foo {};\n"
              "void foo(const int* cpi, foo f, bar b, bar* bp, double d) {\n"
              "    printf(\"%e\", f);\n"
              "    printf(\"%E\", \"s4\");\n"
              "    printf(\"%f\", cpi);\n"
              "    printf(\"%G\", bp);\n"
              "    printf(\"%f\", d);\n"
              "    printf(\"%f\", b);\n"
              "    printf(\"%f\", (float)cpi);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:3]: (warning) %e in format string (no. 1) requires 'double' but the argument type is 'foo'.\n"
                      "[test.cpp:4]: (warning) %E in format string (no. 1) requires 'double' but the argument type is 'const char *'.\n"
                      "[test.cpp:5]: (warning) %f in format string (no. 1) requires 'double' but the argument type is 'const signed int *'.\n"
                      "[test.cpp:6]: (warning) %G in format string (no. 1) requires 'double' but the argument type is 'bar *'.\n", errout.str());

        check("class foo {};\n"
              "void foo(const char* cpc, char* pc) {\n"
              "    printf(\"%e\", cpc);\n"
              "    printf(\"%E\", pc);\n"
              "    printf(\"%f\", cpc);\n"
              "    printf(\"%G\", pc);\n"
              "    printf(\"%f\", pc);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:3]: (warning) %e in format string (no. 1) requires 'double' but the argument type is 'const char *'.\n"
                      "[test.cpp:4]: (warning) %E in format string (no. 1) requires 'double' but the argument type is 'char *'.\n"
                      "[test.cpp:5]: (warning) %f in format string (no. 1) requires 'double' but the argument type is 'const char *'.\n"
                      "[test.cpp:6]: (warning) %G in format string (no. 1) requires 'double' but the argument type is 'char *'.\n"
                      "[test.cpp:7]: (warning) %f in format string (no. 1) requires 'double' but the argument type is 'char *'.\n", errout.str());
                      
        check("class foo;\n"
              "void foo(foo f) {\n"
              "    printf(\"%u\", f);\n"
              "    printf(\"%f\", f);\n"
              "    printf(\"%p\", f);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:3]: (warning) %u in format string (no. 1) requires 'unsigned int' but the argument type is 'foo'.\n"
                      "[test.cpp:4]: (warning) %f in format string (no. 1) requires 'double' but the argument type is 'foo'.\n"
                      "[test.cpp:5]: (warning) %p in format string (no. 1) requires an address but the argument type is 'foo'.\n", errout.str());

        // Ticket #4189 (Improve check (printf("%l") not detected)) tests (according to C99 7.19.6.1.7)
        // False positive tests
        check("void foo(signed char sc, unsigned char uc, short int si, unsigned short int usi) {\n"
              "  printf(\"%hhx %hhd\", sc, uc);\n"
              "  printf(\"%hd %hu\", si, usi);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:2]: (warning) %hhx in format string (no. 1) requires 'unsigned char' but the argument type is 'signed char'.\n"
                      "[test.cpp:2]: (warning) %hhd in format string (no. 2) requires 'char' but the argument type is 'unsigned char'.\n", errout.str());

        check("void foo(long long int lli, unsigned long long int ulli, long int li, unsigned long int uli) {\n"
              "  printf(\"%llo %llx\", lli, ulli);\n"
              "  printf(\"%ld %lu\", li, uli);\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("void foo(intmax_t im, uintmax_t uim, size_t s, ptrdiff_t p, long double ld, std::size_t ss, std::ptrdiff_t sp) {\n"
              "  printf(\"%jd %jo\", im, uim);\n"
              "  printf(\"%zx\", s);\n"
              "  printf(\"%ti\", p);\n"
              "  printf(\"%Lf\", ld);\n"
              "  printf(\"%zx\", ss);\n"
              "  printf(\"%ti\", sp);\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        // Unrecognized (and non-existent in standard library) specifiers.
        // Perhaps should emit warnings
        check("void foo(intmax_t im, uintmax_t uim, size_t s, ptrdiff_t p, long double ld, std::size_t ss, std::ptrdiff_t sp) {\n"
              "  printf(\"%jb %jw\", im, uim);\n"
              "  printf(\"%zr\", s);\n"
              "  printf(\"%tm\", p);\n"
              "  printf(\"%La\", ld);\n"
              "  printf(\"%zv\", ss);\n"
              "  printf(\"%tp\", sp);\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("void foo(long long l, ptrdiff_t p, std::ptrdiff_t sp) {\n"
              "  printf(\"%td\", p);\n"
              "  printf(\"%td\", sp);\n"
              "  printf(\"%td\", l);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:4]: (warning) %td in format string (no. 1) requires 'ptrdiff_t' but the argument type is 'signed long long'.\n", errout.str());

        check("void foo(int i, long double ld) {\n"
              "  printf(\"%zx %zu\", i, ld);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:2]: (warning) %zx in format string (no. 1) requires 'size_t' but the argument type is 'signed int'.\n"
                      "[test.cpp:2]: (warning) %zu in format string (no. 2) requires 'size_t' but the argument type is 'long double'.\n", errout.str());

        check("void foo(unsigned int ui, long double ld) {\n"
              "  printf(\"%zu %zx\", ui, ld);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:2]: (warning) %zu in format string (no. 1) requires 'size_t' but the argument type is 'unsigned int'.\n"
                      "[test.cpp:2]: (warning) %zx in format string (no. 2) requires 'size_t' but the argument type is 'long double'.\n", errout.str());

        check("void foo(int i, long double ld) {\n"
              "  printf(\"%tx %tu\", i, ld);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:2]: (warning) %tx in format string (no. 1) requires 'unsigned ptrdiff_t' but the argument type is 'signed int'.\n"
                      "[test.cpp:2]: (warning) %tu in format string (no. 2) requires 'unsigned ptrdiff_t' but the argument type is 'long double'.\n", errout.str());

        // False negative test
        check("void foo(unsigned int i) {\n"
              "  printf(\"%h\", i);\n"
              "  printf(\"%hh\", i);\n"
              "  printf(\"%l\", i);\n"
              "  printf(\"%ll\", i);\n"
              "  printf(\"%j\", i);\n"
              "  printf(\"%z\", i);\n"
              "  printf(\"%t\", i);\n"
              "  printf(\"%L\", i);\n"
              "  printf(\"%I\", i);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:2]: (warning) 'h' in format string (no. 1) is a length modifier and cannot be used without a conversion specifier.\n"
                      "[test.cpp:3]: (warning) 'hh' in format string (no. 1) is a length modifier and cannot be used without a conversion specifier.\n"
                      "[test.cpp:4]: (warning) 'l' in format string (no. 1) is a length modifier and cannot be used without a conversion specifier.\n"
                      "[test.cpp:5]: (warning) 'll' in format string (no. 1) is a length modifier and cannot be used without a conversion specifier.\n"
                      "[test.cpp:6]: (warning) 'j' in format string (no. 1) is a length modifier and cannot be used without a conversion specifier.\n"
                      "[test.cpp:7]: (warning) 'z' in format string (no. 1) is a length modifier and cannot be used without a conversion specifier.\n"
                      "[test.cpp:8]: (warning) 't' in format string (no. 1) is a length modifier and cannot be used without a conversion specifier.\n"
                      "[test.cpp:9]: (warning) 'L' in format string (no. 1) is a length modifier and cannot be used without a conversion specifier.\n"
                      "[test.cpp:10]: (warning) 'I' in format string (no. 1) is a length modifier and cannot be used without a conversion specifier.\n", errout.str());

        check("void foo(unsigned int i) {\n"
              "  printf(\"%hd\", i);\n"
              "  printf(\"%hhd\", i);\n"
              "  printf(\"%ld\", i);\n"
              "  printf(\"%lld\", i);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:2]: (warning) %hd in format string (no. 1) requires 'short' but the argument type is 'unsigned int'.\n"
                      "[test.cpp:3]: (warning) %hhd in format string (no. 1) requires 'char' but the argument type is 'unsigned int'.\n"
                      "[test.cpp:4]: (warning) %ld in format string (no. 1) requires 'long' but the argument type is 'unsigned int'.\n"
                      "[test.cpp:5]: (warning) %lld in format string (no. 1) requires 'long long' but the argument type is 'unsigned int'.\n", errout.str());

        check("void foo(unsigned int i) {\n"
              "  printf(\"%ld\", i);\n"
              "  printf(\"%lld\", i);\n"
              "  printf(\"%lu\", i);\n"
              "  printf(\"%llu\", i);\n"
              "  printf(\"%lx\", i);\n"
              "  printf(\"%llx\", i);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:2]: (warning) %ld in format string (no. 1) requires 'long' but the argument type is 'unsigned int'.\n"
                      "[test.cpp:3]: (warning) %lld in format string (no. 1) requires 'long long' but the argument type is 'unsigned int'.\n"
                      "[test.cpp:4]: (warning) %lu in format string (no. 1) requires 'unsigned long' but the argument type is 'unsigned int'.\n"
                      "[test.cpp:5]: (warning) %llu in format string (no. 1) requires 'unsigned long long' but the argument type is 'unsigned int'.\n"
                      "[test.cpp:6]: (warning) %lx in format string (no. 1) requires 'unsigned long' but the argument type is 'unsigned int'.\n"
                      "[test.cpp:7]: (warning) %llx in format string (no. 1) requires 'unsigned long long' but the argument type is 'unsigned int'.\n", errout.str());

        check("void foo(int i, intmax_t im, ptrdiff_t p) {\n"
              "  printf(\"%lld\", i);\n"
              "  printf(\"%lld\", im);\n"
              "  printf(\"%lld\", p);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:2]: (warning) %lld in format string (no. 1) requires 'long long' but the argument type is 'signed int'.\n", errout.str());


        check("class Foo {\n"
              "    double d;\n"
              "    struct Bar {\n"
              "        int i;\n"
              "    } bar[2];\n"
              "    struct Baz {\n"
              "        int i;\n"
              "    } baz;\n"
              "};\n"
              "int a[10];\n"
              "Foo f[10];\n"
              "void foo(const Foo* foo) {\n"
              "    printf(\"%d %f %f %d %f %f\",\n"
              "        foo->d, foo->bar[0].i, a[0],\n"
              "        f[0].d, f[0].baz.i, f[0].bar[0].i);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:13]: (warning) %d in format string (no. 1) requires 'int' but the argument type is 'double'.\n"
                      "[test.cpp:13]: (warning) %f in format string (no. 2) requires 'double' but the argument type is 'signed int'.\n"
                      "[test.cpp:13]: (warning) %f in format string (no. 3) requires 'double' but the argument type is 'int'.\n"
                      "[test.cpp:13]: (warning) %d in format string (no. 4) requires 'int' but the argument type is 'double'.\n"
                      "[test.cpp:13]: (warning) %f in format string (no. 5) requires 'double' but the argument type is 'int'.\n"
                      "[test.cpp:13]: (warning) %f in format string (no. 6) requires 'double' but the argument type is 'int'.\n", errout.str());

        check("short f() { return 0; }\n"
              "void foo() { printf(\"%d %u %lu %I64u %I64d %f %Lf %p\", f(), f(), f(), f(), f(), f(), f(), f()); }");
        ASSERT_EQUALS("[test.cpp:2]: (warning) %u in format string (no. 2) requires 'unsigned int' but the argument type is 'signed short'.\n"
                      "[test.cpp:2]: (warning) %lu in format string (no. 3) requires 'unsigned long' but the argument type is 'signed short'.\n"
                      "[test.cpp:2]: (warning) %I64u in format string (no. 4) requires 'unsigned __int64' but the argument type is 'signed short'.\n"
                      "[test.cpp:2]: (warning) %I64d in format string (no. 5) requires '__int64' but the argument type is 'signed short'.\n"
                      "[test.cpp:2]: (warning) %f in format string (no. 6) requires 'double' but the argument type is 'signed short'.\n"
                      "[test.cpp:2]: (warning) %Lf in format string (no. 7) requires 'long double' but the argument type is 'signed short'.\n"
                      "[test.cpp:2]: (warning) %p in format string (no. 8) requires an address but the argument type is 'signed short'.\n", errout.str());

        check("unsigned short f() { return 0; }\n"
              "void foo() { printf(\"%u %d %ld %I64d %I64u %f %Lf %p\", f(), f(), f(), f(), f(), f(), f(), f()); }");
        ASSERT_EQUALS("[test.cpp:2]: (warning) %ld in format string (no. 3) requires 'long' but the argument type is 'unsigned short'.\n"
                      "[test.cpp:2]: (warning) %I64d in format string (no. 4) requires '__int64' but the argument type is 'unsigned short'.\n"
                      "[test.cpp:2]: (warning) %I64u in format string (no. 5) requires 'unsigned __int64' but the argument type is 'unsigned short'.\n"
                      "[test.cpp:2]: (warning) %f in format string (no. 6) requires 'double' but the argument type is 'unsigned short'.\n"
                      "[test.cpp:2]: (warning) %Lf in format string (no. 7) requires 'long double' but the argument type is 'unsigned short'.\n"
                      "[test.cpp:2]: (warning) %p in format string (no. 8) requires an address but the argument type is 'unsigned short'.\n", errout.str());

        check("int f() { return 0; }\n"
              "void foo() { printf(\"%d %u %lu %I64u %I64d %f %Lf %p\", f(), f(), f(), f(), f(), f(), f(), f()); }");
        ASSERT_EQUALS("[test.cpp:2]: (warning) %u in format string (no. 2) requires 'unsigned int' but the argument type is 'signed int'.\n"
                      "[test.cpp:2]: (warning) %lu in format string (no. 3) requires 'unsigned long' but the argument type is 'signed int'.\n"
                      "[test.cpp:2]: (warning) %I64u in format string (no. 4) requires 'unsigned __int64' but the argument type is 'signed int'.\n"
                      "[test.cpp:2]: (warning) %I64d in format string (no. 5) requires '__int64' but the argument type is 'signed int'.\n"
                      "[test.cpp:2]: (warning) %f in format string (no. 6) requires 'double' but the argument type is 'signed int'.\n"
                      "[test.cpp:2]: (warning) %Lf in format string (no. 7) requires 'long double' but the argument type is 'signed int'.\n"
                      "[test.cpp:2]: (warning) %p in format string (no. 8) requires an address but the argument type is 'signed int'.\n", errout.str());

        check("unsigned int f() { return 0; }\n"
              "void foo() { printf(\"%u %d %ld %I64d %I64u %f %Lf %p\", f(), f(), f(), f(), f(), f(), f(), f()); }");
        ASSERT_EQUALS("[test.cpp:2]: (warning) %d in format string (no. 2) requires 'int' but the argument type is 'unsigned int'.\n"
                      "[test.cpp:2]: (warning) %ld in format string (no. 3) requires 'long' but the argument type is 'unsigned int'.\n"
                      "[test.cpp:2]: (warning) %I64d in format string (no. 4) requires '__int64' but the argument type is 'unsigned int'.\n"
                      "[test.cpp:2]: (warning) %I64u in format string (no. 5) requires 'unsigned __int64' but the argument type is 'unsigned int'.\n"
                      "[test.cpp:2]: (warning) %f in format string (no. 6) requires 'double' but the argument type is 'unsigned int'.\n"
                      "[test.cpp:2]: (warning) %Lf in format string (no. 7) requires 'long double' but the argument type is 'unsigned int'.\n"
                      "[test.cpp:2]: (warning) %p in format string (no. 8) requires an address but the argument type is 'unsigned int'.\n", errout.str());

        check("long f() { return 0; }\n"
              "void foo() { printf(\"%ld %u %lu %I64u %I64d %f %Lf %p\", f(), f(), f(), f(), f(), f(), f(), f()); }");
        ASSERT_EQUALS("[test.cpp:2]: (warning) %u in format string (no. 2) requires 'unsigned int' but the argument type is 'signed long'.\n"
                      "[test.cpp:2]: (warning) %lu in format string (no. 3) requires 'unsigned long' but the argument type is 'signed long'.\n"
                      "[test.cpp:2]: (warning) %I64u in format string (no. 4) requires 'unsigned __int64' but the argument type is 'signed long'.\n"
                      "[test.cpp:2]: (warning) %I64d in format string (no. 5) requires '__int64' but the argument type is 'signed long'.\n"
                      "[test.cpp:2]: (warning) %f in format string (no. 6) requires 'double' but the argument type is 'signed long'.\n"
                      "[test.cpp:2]: (warning) %Lf in format string (no. 7) requires 'long double' but the argument type is 'signed long'.\n"
                      "[test.cpp:2]: (warning) %p in format string (no. 8) requires an address but the argument type is 'signed long'.\n", errout.str());

        check("unsigned long f() { return 0; }\n"
              "void foo() { printf(\"%lu %d %ld %I64d %I64u %f %Lf %p\", f(), f(), f(), f(), f(), f(), f(), f()); }");
        ASSERT_EQUALS("[test.cpp:2]: (warning) %d in format string (no. 2) requires 'int' but the argument type is 'unsigned long'.\n"
                      "[test.cpp:2]: (warning) %ld in format string (no. 3) requires 'long' but the argument type is 'unsigned long'.\n"
                      "[test.cpp:2]: (warning) %I64d in format string (no. 4) requires '__int64' but the argument type is 'unsigned long'.\n"
                      "[test.cpp:2]: (warning) %I64u in format string (no. 5) requires 'unsigned __int64' but the argument type is 'unsigned long'.\n"
                      "[test.cpp:2]: (warning) %f in format string (no. 6) requires 'double' but the argument type is 'unsigned long'.\n"
                      "[test.cpp:2]: (warning) %Lf in format string (no. 7) requires 'long double' but the argument type is 'unsigned long'.\n"
                      "[test.cpp:2]: (warning) %p in format string (no. 8) requires an address but the argument type is 'unsigned long'.\n", errout.str());

        check("long long f() { return 0; }\n"
              "void foo() { printf(\"%lld %u %lu %I64u %I64d %f %Lf %p\", f(), f(), f(), f(), f(), f(), f(), f()); }");
        ASSERT_EQUALS("[test.cpp:2]: (warning) %u in format string (no. 2) requires 'unsigned int' but the argument type is 'signed long long'.\n"
                      "[test.cpp:2]: (warning) %lu in format string (no. 3) requires 'unsigned long' but the argument type is 'signed long long'.\n"
                      "[test.cpp:2]: (warning) %I64u in format string (no. 4) requires 'unsigned __int64' but the argument type is 'signed long long'.\n"
                      "[test.cpp:2]: (warning) %f in format string (no. 6) requires 'double' but the argument type is 'signed long long'.\n"
                      "[test.cpp:2]: (warning) %Lf in format string (no. 7) requires 'long double' but the argument type is 'signed long long'.\n"
                      "[test.cpp:2]: (warning) %p in format string (no. 8) requires an address but the argument type is 'signed long long'.\n", errout.str());

        check("unsigned long long f() { return 0; }\n"
              "void foo() { printf(\"%llu %d %ld %I64d %I64u %f %Lf %p\", f(), f(), f(), f(), f(), f(), f(), f()); }");
        ASSERT_EQUALS("[test.cpp:2]: (warning) %d in format string (no. 2) requires 'int' but the argument type is 'unsigned long long'.\n"
                      "[test.cpp:2]: (warning) %ld in format string (no. 3) requires 'long' but the argument type is 'unsigned long long'.\n"
                      "[test.cpp:2]: (warning) %I64d in format string (no. 4) requires '__int64' but the argument type is 'unsigned long long'.\n"
                      "[test.cpp:2]: (warning) %f in format string (no. 6) requires 'double' but the argument type is 'unsigned long long'.\n"
                      "[test.cpp:2]: (warning) %Lf in format string (no. 7) requires 'long double' but the argument type is 'unsigned long long'.\n"
                      "[test.cpp:2]: (warning) %p in format string (no. 8) requires an address but the argument type is 'unsigned long long'.\n", errout.str());

        check("float f() { return 0; }\n"
              "void foo() { printf(\"%f %d %ld %u %lu %I64d %I64u %Lf %p\", f(), f(), f(), f(), f(), f(), f(), f(), f()); }");
        ASSERT_EQUALS("[test.cpp:2]: (warning) %d in format string (no. 2) requires 'int' but the argument type is 'float'.\n"
                      "[test.cpp:2]: (warning) %ld in format string (no. 3) requires 'long' but the argument type is 'float'.\n"
                      "[test.cpp:2]: (warning) %u in format string (no. 4) requires 'unsigned int' but the argument type is 'float'.\n"
                      "[test.cpp:2]: (warning) %lu in format string (no. 5) requires 'unsigned long' but the argument type is 'float'.\n"
                      "[test.cpp:2]: (warning) %I64d in format string (no. 6) requires '__int64' but the argument type is 'float'.\n"
                      "[test.cpp:2]: (warning) %I64u in format string (no. 7) requires 'unsigned __int64' but the argument type is 'float'.\n"
                      "[test.cpp:2]: (warning) %Lf in format string (no. 8) requires 'long double' but the argument type is 'float'.\n"
                      "[test.cpp:2]: (warning) %p in format string (no. 9) requires an address but the argument type is 'float'.\n", errout.str());

        check("double f() { return 0; }\n"
              "void foo() { printf(\"%f %d %ld %u %lu %I64d %I64u %Lf %p\", f(), f(), f(), f(), f(), f(), f(), f(), f()); }");
        ASSERT_EQUALS("[test.cpp:2]: (warning) %d in format string (no. 2) requires 'int' but the argument type is 'double'.\n"
                      "[test.cpp:2]: (warning) %ld in format string (no. 3) requires 'long' but the argument type is 'double'.\n"
                      "[test.cpp:2]: (warning) %u in format string (no. 4) requires 'unsigned int' but the argument type is 'double'.\n"
                      "[test.cpp:2]: (warning) %lu in format string (no. 5) requires 'unsigned long' but the argument type is 'double'.\n"
                      "[test.cpp:2]: (warning) %I64d in format string (no. 6) requires '__int64' but the argument type is 'double'.\n"
                      "[test.cpp:2]: (warning) %I64u in format string (no. 7) requires 'unsigned __int64' but the argument type is 'double'.\n"
                      "[test.cpp:2]: (warning) %Lf in format string (no. 8) requires 'long double' but the argument type is 'double'.\n"
                      "[test.cpp:2]: (warning) %p in format string (no. 9) requires an address but the argument type is 'double'.\n", errout.str());

        check("long double f() { return 0; }\n"
              "void foo() { printf(\"%Lf %d %ld %u %lu %I64d %I64u %f %p\", f(), f(), f(), f(), f(), f(), f(), f(), f()); }");
        ASSERT_EQUALS("[test.cpp:2]: (warning) %d in format string (no. 2) requires 'int' but the argument type is 'long double'.\n"
                      "[test.cpp:2]: (warning) %ld in format string (no. 3) requires 'long' but the argument type is 'long double'.\n"
                      "[test.cpp:2]: (warning) %u in format string (no. 4) requires 'unsigned int' but the argument type is 'long double'.\n"
                      "[test.cpp:2]: (warning) %lu in format string (no. 5) requires 'unsigned long' but the argument type is 'long double'.\n"
                      "[test.cpp:2]: (warning) %I64d in format string (no. 6) requires '__int64' but the argument type is 'long double'.\n"
                      "[test.cpp:2]: (warning) %I64u in format string (no. 7) requires 'unsigned __int64' but the argument type is 'long double'.\n"
                      "[test.cpp:2]: (warning) %f in format string (no. 8) requires 'double' but the argument type is 'long double'.\n"
                      "[test.cpp:2]: (warning) %p in format string (no. 9) requires an address but the argument type is 'long double'.\n", errout.str());

        check("int f() { return 0; }\n"
              "void foo() { printf(\"%I64d %I64u %I64x %d\", f(), f(), f(), f()); }");
        ASSERT_EQUALS("[test.cpp:2]: (warning) %I64d in format string (no. 1) requires '__int64' but the argument type is 'signed int'.\n"
                      "[test.cpp:2]: (warning) %I64u in format string (no. 2) requires 'unsigned __int64' but the argument type is 'signed int'.\n"
                      "[test.cpp:2]: (warning) %I64x in format string (no. 3) requires 'unsigned __int64' but the argument type is 'signed int'.\n", errout.str());

        check("long long f() { return 0; }\n"
              "void foo() { printf(\"%I32d %I32u %I32x %lld\", f(), f(), f(), f()); }");
        ASSERT_EQUALS("[test.cpp:2]: (warning) %I32d in format string (no. 1) requires '__int32' but the argument type is 'signed long long'.\n"
                      "[test.cpp:2]: (warning) %I32u in format string (no. 2) requires 'unsigned __int32' but the argument type is 'signed long long'.\n"
                      "[test.cpp:2]: (warning) %I32x in format string (no. 3) requires 'unsigned __int32' but the argument type is 'signed long long'.\n", errout.str());

        check("unsigned long long f() { return 0; }\n"
              "void foo() { printf(\"%I32d %I32u %I32x %llx\", f(), f(), f(), f()); }");
        ASSERT_EQUALS("[test.cpp:2]: (warning) %I32d in format string (no. 1) requires '__int32' but the argument type is 'unsigned long long'.\n"
                      "[test.cpp:2]: (warning) %I32u in format string (no. 2) requires 'unsigned __int32' but the argument type is 'unsigned long long'.\n"
                      "[test.cpp:2]: (warning) %I32x in format string (no. 3) requires 'unsigned __int32' but the argument type is 'unsigned long long'.\n", errout.str());

        check("signed char f() { return 0; }\n"
              "void foo() { printf(\"%Id %Iu %Ix %hhi\", f(), f(), f(), f()); }");
        ASSERT_EQUALS("[test.cpp:2]: (warning) %Id in format string (no. 1) requires 'ptrdiff_t' but the argument type is 'signed char'.\n"
                      "[test.cpp:2]: (warning) %Iu in format string (no. 2) requires 'size_t' but the argument type is 'signed char'.\n"
                      "[test.cpp:2]: (warning) %Ix in format string (no. 3) requires 'size_t' but the argument type is 'signed char'.\n", errout.str());

        check("unsigned char f() { return 0; }\n"
              "void foo() { printf(\"%Id %Iu %Ix %hho\", f(), f(), f(), f()); }");
        ASSERT_EQUALS("[test.cpp:2]: (warning) %Id in format string (no. 1) requires 'ptrdiff_t' but the argument type is 'unsigned char'.\n"
                      "[test.cpp:2]: (warning) %Iu in format string (no. 2) requires 'size_t' but the argument type is 'unsigned char'.\n"
                      "[test.cpp:2]: (warning) %Ix in format string (no. 3) requires 'size_t' but the argument type is 'unsigned char'.\n", errout.str());

        check("namespace bar { int f() { return 0; } }\n"
              "void foo() { printf(\"%d %u %lu %f %Lf %p\", bar::f(), bar::f(), bar::f(), bar::f(), bar::f(), bar::f()); }");
        ASSERT_EQUALS("[test.cpp:2]: (warning) %u in format string (no. 2) requires 'unsigned int' but the argument type is 'signed int'.\n"
                      "[test.cpp:2]: (warning) %lu in format string (no. 3) requires 'unsigned long' but the argument type is 'signed int'.\n"
                      "[test.cpp:2]: (warning) %f in format string (no. 4) requires 'double' but the argument type is 'signed int'.\n"
                      "[test.cpp:2]: (warning) %Lf in format string (no. 5) requires 'long double' but the argument type is 'signed int'.\n"
                      "[test.cpp:2]: (warning) %p in format string (no. 6) requires an address but the argument type is 'signed int'.\n", errout.str());

        check("struct Fred { int i; } f;\n"
              "void foo() { printf(\"%d %u %lu %f %Lf %p\", f.i, f.i, f.i, f.i, f.i, f.i); }");
        ASSERT_EQUALS("[test.cpp:2]: (warning) %u in format string (no. 2) requires 'unsigned int' but the argument type is 'signed int'.\n"
                      "[test.cpp:2]: (warning) %lu in format string (no. 3) requires 'unsigned long' but the argument type is 'signed int'.\n"
                      "[test.cpp:2]: (warning) %f in format string (no. 4) requires 'double' but the argument type is 'signed int'.\n"
                      "[test.cpp:2]: (warning) %Lf in format string (no. 5) requires 'long double' but the argument type is 'signed int'.\n"
                      "[test.cpp:2]: (warning) %p in format string (no. 6) requires an address but the argument type is 'signed int'.\n", errout.str());

        check("struct Fred { unsigned int u; } f;\n"
              "void foo() { printf(\"%u %d %ld %f %Lf %p\", f.u, f.u, f.u, f.u, f.u, f.u); }");
        ASSERT_EQUALS("[test.cpp:2]: (warning) %d in format string (no. 2) requires 'int' but the argument type is 'unsigned int'.\n"
                      "[test.cpp:2]: (warning) %ld in format string (no. 3) requires 'long' but the argument type is 'unsigned int'.\n"
                      "[test.cpp:2]: (warning) %f in format string (no. 4) requires 'double' but the argument type is 'unsigned int'.\n"
                      "[test.cpp:2]: (warning) %Lf in format string (no. 5) requires 'long double' but the argument type is 'unsigned int'.\n"
                      "[test.cpp:2]: (warning) %p in format string (no. 6) requires an address but the argument type is 'unsigned int'.\n", errout.str());

        check("struct Fred { unsigned int ui() { return 0; } } f;\n"
              "void foo() { printf(\"%u %d %ld %f %Lf %p\", f.ui(), f.ui(), f.ui(), f.ui(), f.ui(), f.ui()); }");
        ASSERT_EQUALS("[test.cpp:2]: (warning) %d in format string (no. 2) requires 'int' but the argument type is 'unsigned int'.\n"
                      "[test.cpp:2]: (warning) %ld in format string (no. 3) requires 'long' but the argument type is 'unsigned int'.\n"
                      "[test.cpp:2]: (warning) %f in format string (no. 4) requires 'double' but the argument type is 'unsigned int'.\n"
                      "[test.cpp:2]: (warning) %Lf in format string (no. 5) requires 'long double' but the argument type is 'unsigned int'.\n"
                      "[test.cpp:2]: (warning) %p in format string (no. 6) requires an address but the argument type is 'unsigned int'.\n", errout.str());

        // #4975
        check("void f(int len, int newline) {\n"
              "    printf(\"%s\", newline ? a : str + len);\n"
              "    printf(\"%s\", newline + newline);\n"
              "}\n");
        ASSERT_EQUALS("[test.cpp:3]: (warning) %s in format string (no. 1) requires 'char *' but the argument type is 'signed int'.\n", errout.str());

        check("struct Fred { int i; } f;\n"
              "struct Fred & bar() { };\n"
              "void foo() { printf(\"%d %u %lu %f %Lf %p\", bar().i, bar().i, bar().i, bar().i, bar().i, bar().i); }");
        ASSERT_EQUALS("[test.cpp:3]: (warning) %u in format string (no. 2) requires 'unsigned int' but the argument type is 'signed int'.\n"
                      "[test.cpp:3]: (warning) %lu in format string (no. 3) requires 'unsigned long' but the argument type is 'signed int'.\n"
                      "[test.cpp:3]: (warning) %f in format string (no. 4) requires 'double' but the argument type is 'signed int'.\n"
                      "[test.cpp:3]: (warning) %Lf in format string (no. 5) requires 'long double' but the argument type is 'signed int'.\n"
                      "[test.cpp:3]: (warning) %p in format string (no. 6) requires an address but the argument type is 'signed int'.\n", errout.str());

        check("struct Fred { int i; } f;\n"
              "const struct Fred & bar() { };\n"
              "void foo() { printf(\"%d %u %lu %f %Lf %p\", bar().i, bar().i, bar().i, bar().i, bar().i, bar().i); }");
        ASSERT_EQUALS("[test.cpp:3]: (warning) %u in format string (no. 2) requires 'unsigned int' but the argument type is 'signed int'.\n"
                      "[test.cpp:3]: (warning) %lu in format string (no. 3) requires 'unsigned long' but the argument type is 'signed int'.\n"
                      "[test.cpp:3]: (warning) %f in format string (no. 4) requires 'double' but the argument type is 'signed int'.\n"
                      "[test.cpp:3]: (warning) %Lf in format string (no. 5) requires 'long double' but the argument type is 'signed int'.\n"
                      "[test.cpp:3]: (warning) %p in format string (no. 6) requires an address but the argument type is 'signed int'.\n", errout.str());

        check("struct Fred { int i; } f;\n"
              "static const struct Fred & bar() { };\n"
              "void foo() { printf(\"%d %u %lu %f %Lf %p\", bar().i, bar().i, bar().i, bar().i, bar().i, bar().i); }");
        ASSERT_EQUALS("[test.cpp:3]: (warning) %u in format string (no. 2) requires 'unsigned int' but the argument type is 'signed int'.\n"
                      "[test.cpp:3]: (warning) %lu in format string (no. 3) requires 'unsigned long' but the argument type is 'signed int'.\n"
                      "[test.cpp:3]: (warning) %f in format string (no. 4) requires 'double' but the argument type is 'signed int'.\n"
                      "[test.cpp:3]: (warning) %Lf in format string (no. 5) requires 'long double' but the argument type is 'signed int'.\n"
                      "[test.cpp:3]: (warning) %p in format string (no. 6) requires an address but the argument type is 'signed int'.\n", errout.str());

        check("struct Fred { int i; } f[2];\n"
              "struct Fred * bar() { return f; };\n"
              "void foo() { printf(\"%d %u %lu %f %Lf %p\", bar()[0].i, bar()[0].i, bar()[0].i, bar()[0].i, bar()[0].i, bar()[0].i); }");
        ASSERT_EQUALS("[test.cpp:3]: (warning) %u in format string (no. 2) requires 'unsigned int' but the argument type is 'signed int'.\n"
                      "[test.cpp:3]: (warning) %lu in format string (no. 3) requires 'unsigned long' but the argument type is 'signed int'.\n"
                      "[test.cpp:3]: (warning) %f in format string (no. 4) requires 'double' but the argument type is 'signed int'.\n"
                      "[test.cpp:3]: (warning) %Lf in format string (no. 5) requires 'long double' but the argument type is 'signed int'.\n"
                      "[test.cpp:3]: (warning) %p in format string (no. 6) requires an address but the argument type is 'signed int'.\n", errout.str());

        check("struct Fred { int i; } f[2];\n"
              "const struct Fred * bar() { return f; };\n"
              "void foo() { printf(\"%d %u %lu %f %Lf %p\", bar()[0].i, bar()[0].i, bar()[0].i, bar()[0].i, bar()[0].i, bar()[0].i); }");
        ASSERT_EQUALS("[test.cpp:3]: (warning) %u in format string (no. 2) requires 'unsigned int' but the argument type is 'signed int'.\n"
                      "[test.cpp:3]: (warning) %lu in format string (no. 3) requires 'unsigned long' but the argument type is 'signed int'.\n"
                      "[test.cpp:3]: (warning) %f in format string (no. 4) requires 'double' but the argument type is 'signed int'.\n"
                      "[test.cpp:3]: (warning) %Lf in format string (no. 5) requires 'long double' but the argument type is 'signed int'.\n"
                      "[test.cpp:3]: (warning) %p in format string (no. 6) requires an address but the argument type is 'signed int'.\n", errout.str());

        check("struct Fred { int i; } f[2];\n"
              "static const struct Fred * bar() { return f; };\n"
              "void foo() { printf(\"%d %u %lu %f %Lf %p\", bar()[0].i, bar()[0].i, bar()[0].i, bar()[0].i, bar()[0].i, bar()[0].i); }");
        ASSERT_EQUALS("[test.cpp:3]: (warning) %u in format string (no. 2) requires 'unsigned int' but the argument type is 'signed int'.\n"
                      "[test.cpp:3]: (warning) %lu in format string (no. 3) requires 'unsigned long' but the argument type is 'signed int'.\n"
                      "[test.cpp:3]: (warning) %f in format string (no. 4) requires 'double' but the argument type is 'signed int'.\n"
                      "[test.cpp:3]: (warning) %Lf in format string (no. 5) requires 'long double' but the argument type is 'signed int'.\n"
                      "[test.cpp:3]: (warning) %p in format string (no. 6) requires an address but the argument type is 'signed int'.\n", errout.str());

        check("struct Fred { int32_t i; } f;\n"
              "struct Fred & bar() { };\n"
              "void foo() { printf(\"%d %ld %u %lu %f %Lf\", bar().i, bar().i, bar().i, bar().i, bar().i, bar().i); }");
        ASSERT_EQUALS("[test.cpp:3]: (warning) %ld in format string (no. 2) requires 'long' but the argument type is 'signed int'.\n"
                      "[test.cpp:3]: (warning) %u in format string (no. 3) requires 'unsigned int' but the argument type is 'signed int'.\n"
                      "[test.cpp:3]: (warning) %lu in format string (no. 4) requires 'unsigned long' but the argument type is 'signed int'.\n"
                      "[test.cpp:3]: (warning) %f in format string (no. 5) requires 'double' but the argument type is 'signed int'.\n"
                      "[test.cpp:3]: (warning) %Lf in format string (no. 6) requires 'long double' but the argument type is 'signed int'.\n",
                      errout.str());

        // #4984
        check("void f(double *x) {\n"
              "    printf(\"%f\", x[0]);\n"
              "}\n");
        ASSERT_EQUALS("", errout.str());

        check("int array[10];\n"
              "int * foo() { return array; }\n"
              "void f() {\n"
              "    printf(\"%f\", foo()[0]);\n"
              "}\n");
        ASSERT_EQUALS("[test.cpp:4]: (warning) %f in format string (no. 1) requires 'double' but the argument type is 'signed int'.\n", errout.str());

        check("struct Base { int length() { } };\n"
              "struct Derived : public Base { };\n"
              "void foo(Derived * d) {\n"
              "    printf(\"%f\", d.length());\n"
              "}\n");
        ASSERT_EQUALS("[test.cpp:4]: (warning) %f in format string (no. 1) requires 'double' but the argument type is 'signed int'.\n", errout.str());

        check("std::vector<int> v;\n"
              "void foo() {\n"
              "    printf(\"%d %u %f\", v[0], v[0], v[0]);\n"
              "}\n");
        ASSERT_EQUALS("[test.cpp:3]: (warning) %u in format string (no. 2) requires 'unsigned int' but the argument type is 'signed int'.\n"
                      "[test.cpp:3]: (warning) %f in format string (no. 3) requires 'double' but the argument type is 'signed int'.\n", errout.str());

        // #4999 (crash)
        check("int bar(int a);\n"
              "void foo() {\n"
              "    printf(\"%d\", bar(0));\n"
              "}\n");
        ASSERT_EQUALS("", errout.str());

        check("void foo() {\n"
              "    printf(\"%f %d\", static_cast<int>(1.0f), reinterpret_cast<const void *>(0));\n"
              "}\n");
        ASSERT_EQUALS("[test.cpp:2]: (warning) %f in format string (no. 1) requires 'double' but the argument type is 'signed int'.\n"
                      "[test.cpp:2]: (warning) %d in format string (no. 2) requires 'int' but the argument type is 'const void *'.\n", errout.str());

        check("void foo() {\n"
              "    UNKNOWN * u;\n"
              "    printf(\"%d %x %u %f\", u[i], u[i], u[i], u[i]);\n"
              "}\n");
        ASSERT_EQUALS("", errout.str());

        check("void foo() {\n"
              "    long * l;\n"
              "    printf(\"%d %x %u %f\", l[i], l[i], l[i], l[i]);\n"
              "}\n");
        ASSERT_EQUALS("[test.cpp:3]: (warning) %d in format string (no. 1) requires 'int' but the argument type is 'signed long'.\n"
                      "[test.cpp:3]: (warning) %x in format string (no. 2) requires 'unsigned int' but the argument type is 'signed long'.\n"
                      "[test.cpp:3]: (warning) %u in format string (no. 3) requires 'unsigned int' but the argument type is 'signed long'.\n"
                      "[test.cpp:3]: (warning) %f in format string (no. 4) requires 'double' but the argument type is 'signed long'.\n", errout.str());

        check("void f() {\n" // #5104
              "    myvector<unsigned short> v1(1,0);\n"
              "    printf(\"%d\",v1[0]);\n"
              "    myvector<int> v2(1,0);\n"
              "    printf(\"%d\",v2[0]);\n"
              "    myvector<unsigned int> v3(1,0);\n"
              "    printf(\"%u\",v3[0]);\n"
              "    myvector<unsigned int> v4(1,0);\n"
              "    printf(\"%x\",v4[0]);\n"
              "    myvector<double> v5(1,0);\n"
              "    printf(\"%f\",v5[0]);\n"
              "    myvector<bool> v6(1,0);\n"
              "    printf(\"%u\",v6[0]);\n"
              "    myvector<char *> v7(1,0);\n"
              "    printf(\"%s\",v7[0]);\n"
              "}\n");
        ASSERT_EQUALS("", errout.str());

        check("std::vector<char> v;\n" // #5151
              "void foo() {\n"
              "   printf(\"%c %u %f\", v.at(32), v.at(32), v.at(32));\n"
              "}\n");
        ASSERT_EQUALS("[test.cpp:3]: (warning) %u in format string (no. 2) requires 'unsigned int' but the argument type is 'char'.\n"
                      "[test.cpp:3]: (warning) %f in format string (no. 3) requires 'double' but the argument type is 'char'.\n", errout.str());

        // #5195 (segmentation fault)
        check("void T::a(const std::vector<double>& vx) {\n"
              "    printf(\"%f\", vx.at(0));\n"
              "}\n");
        ASSERT_EQUALS("", errout.str());

        // #5486
        check("void foo() {\n"
              "    ssize_t test = 0;\n"
              "    printf(\"%zd\", test);\n"
              "}\n");
        ASSERT_EQUALS("", errout.str());

        // #6009
        check("extern std::string StringByReturnValue();\n"
              "extern int         IntByReturnValue();\n"
              "void MyFunction() {\n"
              "    printf( \"%s - %s\", StringByReturnValue(), IntByReturnValue() );\n"
              "}\n");
        ASSERT_EQUALS("[test.cpp:4]: (warning) %s in format string (no. 1) requires 'char *' but the argument type is 'std::string'.\n"
                      "[test.cpp:4]: (warning) %s in format string (no. 2) requires 'char *' but the argument type is 'signed int'.\n", errout.str());

        check("template <class T, size_t S>\n"
              "struct Array {\n"
              "    T data[S];\n"
              "    T & operator [] (size_t i) { return data[i]; }\n"
              "};\n"
              "void foo() {\n"
              "    Array<int, 10> array1;\n"
              "    Array<float, 10> array2;\n"
              "    printf(\"%u %u\", array1[0], array2[0]);\n"
              "}\n");
        ASSERT_EQUALS("[test.cpp:9]: (warning) %u in format string (no. 1) requires 'unsigned int' but the argument type is 'int'.\n"
                      "[test.cpp:9]: (warning) %u in format string (no. 2) requires 'unsigned int' but the argument type is 'float'.\n", errout.str());

        // Ticket #7445
        check("struct S { unsigned short x; } s = {0};\n"
              "void foo() {\n"
              "    printf(\"%d\", s.x);\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        // Ticket #7601
        check("void foo(int i, unsigned int ui, long long ll, unsigned long long ull) {\n"
              "    printf(\"%Ld %Lu %Ld %Lu\", i, ui, ll, ull);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:2]: (warning) %Ld in format string (no. 1) requires 'long long' but the argument type is 'signed int'.\n"
                      "[test.cpp:2]: (warning) %Lu in format string (no. 2) requires 'unsigned long long' but the argument type is 'unsigned int'.\n", errout.str());

        check("void foo(char c, unsigned char uc, short s, unsigned short us, int i, unsigned int ui, long l, unsigned long ul) {\n"
              "    printf(\"%hhd %hhd %hhd %hhd %hhd %hhd %hhd %hhd\", c, uc, s, us, i, ui, l, ul);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:2]: (warning) %hhd in format string (no. 2) requires 'char' but the argument type is 'unsigned char'.\n"
                      "[test.cpp:2]: (warning) %hhd in format string (no. 3) requires 'char' but the argument type is 'signed short'.\n"
                      "[test.cpp:2]: (warning) %hhd in format string (no. 4) requires 'char' but the argument type is 'unsigned short'.\n"
                      "[test.cpp:2]: (warning) %hhd in format string (no. 5) requires 'char' but the argument type is 'signed int'.\n"
                      "[test.cpp:2]: (warning) %hhd in format string (no. 6) requires 'char' but the argument type is 'unsigned int'.\n"
                      "[test.cpp:2]: (warning) %hhd in format string (no. 7) requires 'char' but the argument type is 'signed long'.\n"
                      "[test.cpp:2]: (warning) %hhd in format string (no. 8) requires 'char' but the argument type is 'unsigned long'.\n", errout.str());

        check("void foo(char c, unsigned char uc, short s, unsigned short us, int i, unsigned int ui, long l, unsigned long ul) {\n"
              "    printf(\"%hhu %hhu %hhu %hhu %hhu %hhu %hhu %hhu\", c, uc, s, us, i, ui, l, ul);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:2]: (warning) %hhu in format string (no. 1) requires 'unsigned char' but the argument type is 'char'.\n"
                      "[test.cpp:2]: (warning) %hhu in format string (no. 3) requires 'unsigned char' but the argument type is 'signed short'.\n"
                      "[test.cpp:2]: (warning) %hhu in format string (no. 4) requires 'unsigned char' but the argument type is 'unsigned short'.\n"
                      "[test.cpp:2]: (warning) %hhu in format string (no. 5) requires 'unsigned char' but the argument type is 'signed int'.\n"
                      "[test.cpp:2]: (warning) %hhu in format string (no. 6) requires 'unsigned char' but the argument type is 'unsigned int'.\n"
                      "[test.cpp:2]: (warning) %hhu in format string (no. 7) requires 'unsigned char' but the argument type is 'signed long'.\n"
                      "[test.cpp:2]: (warning) %hhu in format string (no. 8) requires 'unsigned char' but the argument type is 'unsigned long'.\n", errout.str());

        check("void foo(char c, unsigned char uc, short s, unsigned short us, int i, unsigned int ui, long l, unsigned long ul) {\n"
              "    printf(\"%hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx\", c, uc, s, us, i, ui, l, ul);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:2]: (warning) %hhx in format string (no. 1) requires 'unsigned char' but the argument type is 'char'.\n"
                      "[test.cpp:2]: (warning) %hhx in format string (no. 3) requires 'unsigned char' but the argument type is 'signed short'.\n"
                      "[test.cpp:2]: (warning) %hhx in format string (no. 4) requires 'unsigned char' but the argument type is 'unsigned short'.\n"
                      "[test.cpp:2]: (warning) %hhx in format string (no. 5) requires 'unsigned char' but the argument type is 'signed int'.\n"
                      "[test.cpp:2]: (warning) %hhx in format string (no. 6) requires 'unsigned char' but the argument type is 'unsigned int'.\n"
                      "[test.cpp:2]: (warning) %hhx in format string (no. 7) requires 'unsigned char' but the argument type is 'signed long'.\n"
                      "[test.cpp:2]: (warning) %hhx in format string (no. 8) requires 'unsigned char' but the argument type is 'unsigned long'.\n", errout.str());

        check("void foo(char c, unsigned char uc, short s, unsigned short us, int i, unsigned int ui, long l, unsigned long ul) {\n"
              "    printf(\"%hd %hd %hd %hd %hd %hd %hd %hd\", c, uc, s, us, i, ui, l, ul);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:2]: (warning) %hd in format string (no. 1) requires 'short' but the argument type is 'char'.\n"
                      "[test.cpp:2]: (warning) %hd in format string (no. 2) requires 'short' but the argument type is 'unsigned char'.\n"
                      "[test.cpp:2]: (warning) %hd in format string (no. 4) requires 'short' but the argument type is 'unsigned short'.\n"
                      "[test.cpp:2]: (warning) %hd in format string (no. 5) requires 'short' but the argument type is 'signed int'.\n"
                      "[test.cpp:2]: (warning) %hd in format string (no. 6) requires 'short' but the argument type is 'unsigned int'.\n"
                      "[test.cpp:2]: (warning) %hd in format string (no. 7) requires 'short' but the argument type is 'signed long'.\n"
                      "[test.cpp:2]: (warning) %hd in format string (no. 8) requires 'short' but the argument type is 'unsigned long'.\n", errout.str());

        check("void foo(char c, unsigned char uc, short s, unsigned short us, int i, unsigned int ui, long l, unsigned long ul) {\n"
              "    printf(\"%hu %hu %hu %hu %hu %hu %hu %hu\", c, uc, s, us, i, ui, l, ul);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:2]: (warning) %hu in format string (no. 1) requires 'unsigned short' but the argument type is 'char'.\n"
                      "[test.cpp:2]: (warning) %hu in format string (no. 2) requires 'unsigned short' but the argument type is 'unsigned char'.\n"
                      "[test.cpp:2]: (warning) %hu in format string (no. 3) requires 'unsigned short' but the argument type is 'signed short'.\n"
                      "[test.cpp:2]: (warning) %hu in format string (no. 5) requires 'unsigned short' but the argument type is 'signed int'.\n"
                      "[test.cpp:2]: (warning) %hu in format string (no. 6) requires 'unsigned short' but the argument type is 'unsigned int'.\n"
                      "[test.cpp:2]: (warning) %hu in format string (no. 7) requires 'unsigned short' but the argument type is 'signed long'.\n"
                      "[test.cpp:2]: (warning) %hu in format string (no. 8) requires 'unsigned short' but the argument type is 'unsigned long'.\n", errout.str());

        check("void foo(char c, unsigned char uc, short s, unsigned short us, int i, unsigned int ui, long l, unsigned long ul) {\n"
              "    printf(\"%hx %hx %hx %hx %hx %hx %hx %hx\", c, uc, s, us, i, ui, l, ul);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:2]: (warning) %hx in format string (no. 1) requires 'unsigned short' but the argument type is 'char'.\n"
                      "[test.cpp:2]: (warning) %hx in format string (no. 2) requires 'unsigned short' but the argument type is 'unsigned char'.\n"
                      "[test.cpp:2]: (warning) %hx in format string (no. 3) requires 'unsigned short' but the argument type is 'signed short'.\n"
                      "[test.cpp:2]: (warning) %hx in format string (no. 5) requires 'unsigned short' but the argument type is 'signed int'.\n"
                      "[test.cpp:2]: (warning) %hx in format string (no. 6) requires 'unsigned short' but the argument type is 'unsigned int'.\n"
                      "[test.cpp:2]: (warning) %hx in format string (no. 7) requires 'unsigned short' but the argument type is 'signed long'.\n"
                      "[test.cpp:2]: (warning) %hx in format string (no. 8) requires 'unsigned short' but the argument type is 'unsigned long'.\n", errout.str());

        // #7837 - Use ValueType for function call
        check("struct S {\n"
              "  double (* f)(double);\n"
              "};\n"
              "\n"
              "void foo(struct S x) {\n"
              "  printf(\"%f\", x.f(4.0));\n"
              "}");
        ASSERT_EQUALS("", errout.str());
    }

    void testPosixPrintfScanfParameterPosition() { // #4900  - No support for parameters in format strings
        check("void foo() {"
              "  int bar;"
              "  printf(\"%1$d\", 1);"
              "  printf(\"%1$d, %d, %1$d\", 1, 2);"
              "  scanf(\"%1$d\", &bar);"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("void foo() {\n"
              "  int bar;\n"
              "  printf(\"%1$d\");\n"
              "  printf(\"%1$d, %d, %4$d\", 1, 2, 3);\n"
              "  scanf(\"%2$d\", &bar);\n"
              "  printf(\"%0$f\", 0.0);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:3]: (error) printf format string requires 1 parameter but only 0 are given.\n"
                      "[test.cpp:4]: (warning) printf: referencing parameter 4 while 3 arguments given\n"
                      "[test.cpp:5]: (warning) scanf: referencing parameter 2 while 1 arguments given\n"
                      "[test.cpp:6]: (warning) printf: parameter positions start at 1, not 0\n"
                      "", errout.str());
    }


    void testMicrosoftPrintfArgument() {
        check("void foo() {\n"
              "    size_t s;\n"
              "    int i;\n"
              "    printf(\"%I\", s);\n"
              "    printf(\"%I6\", s);\n"
              "    printf(\"%I6x\", s);\n"
              "    printf(\"%I16\", s);\n"
              "    printf(\"%I16x\", s);\n"
              "    printf(\"%I32\", s);\n"
              "    printf(\"%I64\", s);\n"
              "    printf(\"%I%i\", s, i);\n"
              "    printf(\"%I6%i\", s, i);\n"
              "    printf(\"%I6x%i\", s, i);\n"
              "    printf(\"%I16%i\", s, i);\n"
              "    printf(\"%I16x%i\", s, i);\n"
              "    printf(\"%I32%i\", s, i);\n"
              "    printf(\"%I64%i\", s, i);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:4]: (warning) 'I' in format string (no. 1) is a length modifier and cannot be used without a conversion specifier.\n"
                      "[test.cpp:5]: (warning) 'I' in format string (no. 1) is a length modifier and cannot be used without a conversion specifier.\n"
                      "[test.cpp:6]: (warning) 'I' in format string (no. 1) is a length modifier and cannot be used without a conversion specifier.\n"
                      "[test.cpp:7]: (warning) 'I' in format string (no. 1) is a length modifier and cannot be used without a conversion specifier.\n"
                      "[test.cpp:8]: (warning) 'I' in format string (no. 1) is a length modifier and cannot be used without a conversion specifier.\n"
                      "[test.cpp:9]: (warning) 'I32' in format string (no. 1) is a length modifier and cannot be used without a conversion specifier.\n"
                      "[test.cpp:10]: (warning) 'I64' in format string (no. 1) is a length modifier and cannot be used without a conversion specifier.\n"
                      "[test.cpp:11]: (warning) 'I' in format string (no. 1) is a length modifier and cannot be used without a conversion specifier.\n"
                      "[test.cpp:12]: (warning) 'I' in format string (no. 1) is a length modifier and cannot be used without a conversion specifier.\n"
                      "[test.cpp:13]: (warning) 'I' in format string (no. 1) is a length modifier and cannot be used without a conversion specifier.\n"
                      "[test.cpp:14]: (warning) 'I' in format string (no. 1) is a length modifier and cannot be used without a conversion specifier.\n"
                      "[test.cpp:15]: (warning) 'I' in format string (no. 1) is a length modifier and cannot be used without a conversion specifier.\n"
                      "[test.cpp:16]: (warning) 'I32' in format string (no. 1) is a length modifier and cannot be used without a conversion specifier.\n"
                      "[test.cpp:17]: (warning) 'I64' in format string (no. 1) is a length modifier and cannot be used without a conversion specifier.\n", errout.str());

    }

    void testMicrosoftScanfArgument() {

        check("void foo() {\n"
              "    size_t s;\n"
              "    int i;\n"
              "    scanf(\"%I\", &s);\n"
              "    scanf(\"%I6\", &s);\n"
              "    scanf(\"%I6x\", &s);\n"
              "    scanf(\"%I16\", &s);\n"
              "    scanf(\"%I16x\", &s);\n"
              "    scanf(\"%I32\", &s);\n"
              "    scanf(\"%I64\", &s);\n"
              "    scanf(\"%I%i\", &s, &i);\n"
              "    scanf(\"%I6%i\", &s, &i);\n"
              "    scanf(\"%I6x%i\", &s, &i);\n"
              "    scanf(\"%I16%i\", &s, &i);\n"
              "    scanf(\"%I16x%i\", &s, &i);\n"
              "    scanf(\"%I32%i\", &s, &i);\n"
              "    scanf(\"%I64%i\", &s, &i);\n"
              "}");
        ASSERT_EQUALS("[test.cpp:4]: (warning) 'I' in format string (no. 1) is a length modifier and cannot be used without a conversion specifier.\n"
                      "[test.cpp:5]: (warning) 'I' in format string (no. 1) is a length modifier and cannot be used without a conversion specifier.\n"
                      "[test.cpp:6]: (warning) 'I' in format string (no. 1) is a length modifier and cannot be used without a conversion specifier.\n"
                      "[test.cpp:7]: (warning) 'I' in format string (no. 1) is a length modifier and cannot be used without a conversion specifier.\n"
                      "[test.cpp:8]: (warning) 'I' in format string (no. 1) is a length modifier and cannot be used without a conversion specifier.\n"
                      "[test.cpp:9]: (warning) 'I32' in format string (no. 1) is a length modifier and cannot be used without a conversion specifier.\n"
                      "[test.cpp:10]: (warning) 'I64' in format string (no. 1) is a length modifier and cannot be used without a conversion specifier.\n"
                      "[test.cpp:11]: (warning) 'I' in format string (no. 1) is a length modifier and cannot be used without a conversion specifier.\n"
                      "[test.cpp:12]: (warning) 'I' in format string (no. 1) is a length modifier and cannot be used without a conversion specifier.\n"
                      "[test.cpp:13]: (warning) 'I' in format string (no. 1) is a length modifier and cannot be used without a conversion specifier.\n"
                      "[test.cpp:14]: (warning) 'I' in format string (no. 1) is a length modifier and cannot be used without a conversion specifier.\n"
                      "[test.cpp:15]: (warning) 'I' in format string (no. 1) is a length modifier and cannot be used without a conversion specifier.\n"
                      "[test.cpp:16]: (warning) 'I32' in format string (no. 1) is a length modifier and cannot be used without a conversion specifier.\n"
                      "[test.cpp:17]: (warning) 'I64' in format string (no. 1) is a length modifier and cannot be used without a conversion specifier.\n", errout.str());
    }


    void testTernary() {  // ticket #6182
        check("void test(const std::string &val) {\n"
              "    printf(\"%s\", val.empty() ? \"I like to eat bananas\" : val.c_str());\n"
              "}\n");
        ASSERT_EQUALS("", errout.str());
    }

    void testUnsignedConst() {  // ticket #6321
        check("void test() {\n"
              "    unsigned const x = 5;\n"
              "    printf(\"%u\", x);\n"
              "}\n");
        ASSERT_EQUALS("", errout.str());
    }

    void testAstType() { // ticket #7014
        check("void test() {\n"
              "    printf(\"%c\", \"hello\"[0]);\n"
              "}\n");
        ASSERT_EQUALS("", errout.str());

        check("void test() {\n"
              "    printf(\"%lld\", (long long)1);\n"
              "}\n");
        ASSERT_EQUALS("", errout.str());

        check("void test() {\n"
              "    printf(\"%i\", (short *)x);\n"
              "}\n");
        ASSERT_EQUALS("[test.cpp:2]: (warning) %i in format string (no. 1) requires 'int' but the argument type is 'signed short *'.\n", errout.str());

        check("int (*fp)();\n" // #7178 - function pointer call
              "void test() {\n"
              "    printf(\"%i\", fp());\n"
              "}\n");
        ASSERT_EQUALS("", errout.str());
    }

    void testPrintf0WithSuffix() { // ticket #7069
        check("void foo() {\n"
              "    printf(\"%u %lu %llu\", 0U, 0UL, 0ULL);\n"
              "    printf(\"%u %lu %llu\", 0u, 0ul, 0ull);\n"
              "}\n");
        ASSERT_EQUALS("", errout.str());
    }

    void testReturnValueTypeStdLib() {
        check("void f() {\n"
              "   const char *s = \"0\";\n"
              "   printf(\"%ld%lld\", atol(s), atoll(s));\n"
              "}");
        ASSERT_EQUALS("", errout.str());
    }

    void testPrintfTypeAlias1() {
        check("using INT = int;\n\n"
              "using PINT = INT *;\n"
              "using PCINT = const PINT;\n"
              "INT i;\n"
              "PINT pi;\n"
              "PCINT pci;"
              "void foo() {\n"
              "    printf(\"%d %p %p\", i, pi, pci);\n"
              "};");
        ASSERT_EQUALS("", errout.str());

        check("using INT = int;\n\n"
              "using PINT = INT *;\n"
              "using PCINT = const PINT;\n"
              "INT i;\n"
              "PINT pi;\n"
              "PCINT pci;"
              "void foo() {\n"
              "    printf(\"%f %f %f\", i, pi, pci);\n"
              "};");
        ASSERT_EQUALS("[test.cpp:8]: (warning) %f in format string (no. 1) requires 'double' but the argument type is 'signed int'.\n"
                      "[test.cpp:8]: (warning) %f in format string (no. 2) requires 'double' but the argument type is 'signed int *'.\n"
                      "[test.cpp:8]: (warning) %f in format string (no. 3) requires 'double' but the argument type is 'const signed int *'.\n", errout.str());
    }

    void testPrintfAuto() { // #8992
        check("void f() {\n"
              "    auto s = sizeof(int);\n"
              "    printf(\"%zu\", s);\n"
              "    printf(\"%f\", s);\n"
              "}\n", false, true);
        ASSERT_EQUALS("[test.cpp:4]: (portability) %f in format string (no. 1) requires 'double' but the argument type is 'size_t {aka unsigned long}'.\n", errout.str());
    }
};

void main()
{
  TestIO test;
  test.run();
}
