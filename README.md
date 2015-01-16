# feather
feather is a simple and lightweight C compiler focusing on performance and reliability 

## Requirements
feather includes a lexer which relies on `flex`, a parser which relies on `bison` and requires a C++ compiler and make. All these tools are normally included on Mac OSX. If they are not, they can be installed with

    brew install flex bison g++ make
