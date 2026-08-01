#!/bin/bash

if [ -z "$1" ]; then
  echo "error: missing executable path argument"
  exit 1
fi

EXE=$(echo "$1" | tr -d ':')
if [[ ${EXE::1} != "/" ]]; then
  EXE="/$EXE"
fi
EXE_PATH=$(dirname "$EXE")
MSYS_PATH="/mingw64/bin"

copy_libs() {
  #echo "* copy_libs($1)"
  # fix missing vulkan-1.dll
  cp -n "$MSYS_PATH/vulkan-1.dll" "$EXE_PATH/vulkan-1.dll"
  # now copy required libs
  for lib in $(ldd "$1" | grep 'not found\|/mingw64/bin' | awk '{print $1}'); do
    echo "mingw_copy_libs: $MSYS_PATH/$lib -> $EXE_PATH"
    cp -n "$MSYS_PATH/$lib" "$EXE_PATH/$lib"
    copy_libs "$EXE_PATH/$lib"
  done
}

copy_libs "$EXE"

exit 0
