# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.


FILE(GLOB PROJECT_HEADERS "CppCamera/*.h" "*.h*")
FILE(GLOB PROJECT_SOURCES "*.cpp" )

include_directories(${CMAKE_SOURCE_DIR} external)

add_library( # Sets the name of the library.
             native_camera
             # Sets the library as a shared library.
             SHARED
             # Provides a relative path to your source file(s).
            ${PROJECT_SOURCES} ${PROJECT_HEADERS} ${OpenCV_INCLUDE_DIRS})



if(${ANDROID_ABI} MATCHES "armeabi-v7a")
    FILE(GLOB EXTERNAL_LIBS "lib32/*.so")
else()
    FILE(GLOB EXTERNAL_LIBS "lib64/*.so")
endif()


target_link_libraries( native_camera ${EXTERNAL_LIBS} android log m dl GLESv2 EGL )