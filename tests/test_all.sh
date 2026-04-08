#!/bin/bash
cd /home/roote/webserver
mkdir -p build_tests tests/upload

echo "Compiling tests..."
g++ -Wall -Wextra -Werror -std=c++98 \
  srcs/http/MethodHandler.cpp \
  srcs/http/RequestParser.cpp \
  srcs/http/HTTPRequest.cpp \
  srcs/http/ResponseBuilder.cpp \
  srcs/http/FileUpload.cpp \
  srcs/http/ChunkedDecoder.cpp \
  tests/MockCGIHandler.cpp \
  tests/ultimate_test.cpp \
  -o build_tests/ultimate_test

if [ $? -eq 0 ]; then
  echo "Compilation successful. Running tests..."
  ./build_tests/ultimate_test
else
  echo "Compilation failed."
fi
