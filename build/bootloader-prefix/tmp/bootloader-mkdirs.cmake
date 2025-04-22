# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/sodir/esp/v5.3.1/esp-idf/components/bootloader/subproject"
  "/home/sodir/cameraProject/build/bootloader"
  "/home/sodir/cameraProject/build/bootloader-prefix"
  "/home/sodir/cameraProject/build/bootloader-prefix/tmp"
  "/home/sodir/cameraProject/build/bootloader-prefix/src/bootloader-stamp"
  "/home/sodir/cameraProject/build/bootloader-prefix/src"
  "/home/sodir/cameraProject/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/sodir/cameraProject/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/sodir/cameraProject/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
