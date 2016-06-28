# Backport tool

A tool to transform C++11 language elements to C++03 compatible form.

## Usage

There are two different ways for transformation:
  * If only one file is transformed it is enough to specify the path of the file.
  * If more files or a whole project is transformed, a `compile_command.json` file must be created which specifies the compilation information about all files. In this case the following argument can be used:
    * Command line option `-p` can be used to specify the directory where the `compile_commands.json` file is stored.
    * Command line option `-include` can be used to specify the files being transformed.
        * If there is only one file to transform, then the `-include` option should contain the path of the file.
        * If there are multiple files to transform, then the `-include` option should contain the path of the directory. All source files in the directory (which are also listed in the `compile_commands.json`) will be transformed.
    * It is possible to use `-include-from` instead of `-include` which should contain the path of a text file. This text file lists all the source files that we want to transform.


### Commands line arguments
  * `-summary`: During transformation, the tool will show a summary about the number of transforms performed by each step.
  * `-no-db`: Turns off the database used for incremental transformation.
  * `-in-place`: The tool overwrites the original files instead of copying and modifying files in a temporary directory.
  * __Select transformations__: You can select specific transformations which will be performed. If you don't use this argument, the tool will perform all the transformations that are implemented. (*Note: Some transformations expect that other ones are already finished. So if you are trying to run some transformations individually, you can get unexpected results.*)
    * `-modify-auto`: Transforms template functions with `auto` return type.
    * `-instantiate-template`: Specializing templates containing template-dependent `auto` expressions.
    * `-replace-lambda`: Transforms lambda expressions.
    * `-replace-member-init`: Transforms member field initializators.
    * `-replace-for-range`: Transforms for-range style `for` loops.
    * `-deduce-auto`: Deduces variables with `auto` type.
    * `-remove-auto`: Removes those template-dependent `auto` expressions that are in a class that hasn't been instantiated and that are in a method that has not been called.
  * __Serializing transformations__:
    * `-serialize-replacements`: The tool writes transformations to a file instead of replacing them in the source files. It can be used only when at least one transform is selected to be performed.
    * `-serialize-dir`: Specifies the directory, where the files (that contain the serialized transformations) will be stored.

## Getting Started
### Dependencies
  * LLVM Release 3.6
  * Clang Release 3.6
  * Clang extra tools 3.6
  * cmake 3

### Supported platforms
  * Linux
  * Windows

### Building the source code

1. Checkout LLVM 3.6 from repository
        
        svn co http://llvm.org/svn/llvm-project/llvm/branches/release_36/ llvm
        
2. Checkout Clang 3.6 from repository
        
        cd llvm/tools
        svn co http://llvm.org/svn/llvm-project/cfe/branches/release_36/ clang

3. Checkout Clang extra tools from repository

        cd clang/tools
        svn co http://llvm.org/svn/llvm-project/clang-tools-extra/branches/release_36/ extra

4. Checkout backport tool into extra directory

        cd extra
        git clone https://github.com/sed-szeged/cppbackport.git
        cp -R cppbackport/backport backport
        
5. Edit CMakeLists.txt in extra directory.

        add_subdirectory(cppbackport)

6. Applying patch files

        cd ../..
        patch -p0 < tools/extra/cppbackport/ClangPatch/unknownattrib.patch
        patch -p0 < tools/extra/cppbackport/ClangPatch/ignoremsasm.patch
        patch -p0 < tools/extra/cppbackport/ClangPatch/template-template-parameter-namespace-print-fix.patch
        patch -p0 < tools/extra/cppbackport/ClangPatch/ASTMatchFinder.cpp.patch

        cd lib/Sema
        patch -p0 < ../../tools/extra/cppbackport/ClangPatch/llvm.tools.clang.lib.Sema.SemaTemplateInstantiateDecl.cpp.patch
        patch -p0 < ../../tools/extra/cppbackport/ClangPatch/llvm.tools.clang.lib.Sema.Sema.cpp.patch

7. Create build directory and run cmake.

        cmake <..>/llvm -DCMAKE_BUILD_TYPE=Release
        make

    
### Authors and Contributors

Backport tool is created and maintained by the [Department of Software Engineering](http://www.sed.inf.u-szeged.hu), [University of Szeged](http://www.u-szeged.hu). 

#### Contribution
If you would like to contribute to the project, create an issue at our GitHub page or send a pull request. Every contribution is welcome!