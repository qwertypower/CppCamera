cmake_minimum_required(VERSION 3.4.1)


set(LIBNFT_PROJ_NAME CppCamera)
project(CppCamera)


# OpenCV
if(ANDROID)
	if(${ANDROID_ABI} MATCHES "armeabi-v7a")
		set(OpenCV_DIR ./external/android/OpenCV-3.4.6/arm32/sdk/native/jni)
	elseif(${ANDROID_ABI} MATCHES "arm64-v8a")
		set(OpenCV_DIR ./external/android/OpenCV-3.4.6/arm64/sdk/native/jni)
		add_definitions(-D__ARM_NEON__)
	endif()
endif()
if(WIN32 OR ( ((UNIX OR APPLE) AND NOT IOS) AND NOT ANDROID))
	add_definitions(-DUSE_SSE4)
	add_definitions(-DNEON2SSE_DISABLE_PERFORMANCE_WARNING)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
	set(OpenCV_STATIC 1)
endif()
if(IOS)
	set(OpenCV_DIR ./external/ios/OpenCV-3.4.9/share/OpenCV)
	add_definitions(-D__ARM_NEON__)
endif()


find_package(OpenCV REQUIRED COMPONENTS core imgproc)
include_directories(${OpenCV_INCLUDE_DIRS})
include_directories("./external/")



FILE(GLOB PROJECT_HEADERS "cppcamera/*.h")
FILE(GLOB PROJECT_SOURCES "cppcamera/*.cpp" "cppcamera/*.mm")

add_definitions(-DSHARED_BUILD)


if(APPLE AND NOT IOS)
	add_library(${LIBNFT_PROJ_NAME} MODULE ${PROJECT_SOURCES} ${PROJECT_HEADERS} ${OpenCV_INCLUDE_DIRS})
else()
	add_library(${LIBNFT_PROJ_NAME} SHARED ${PROJECT_SOURCES} ${PROJECT_HEADERS} ${OpenCV_INCLUDE_DIRS})
endif()



set(CMAKE_CXX_STANDARD 14)
set(CMAKE_C_STANDARD 11)
set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Installation Directory")

# Windows or Linux
if(WIN32 OR ( (UNIX AND NOT APPLE) AND NOT ANDROID ))
	target_link_libraries( ${LIBNFT_PROJ_NAME} ${OpenCV_LIBS})
endif()

# MacOS or iOS
if(APPLE)
	if(IOS)
		set_target_properties(${LIBNFT_PROJ_NAME} PROPERTIES
			XCODE_ATTRIBUTE_ENABLE_BITCODE "NO"
			FRAMEWORK TRUE
			FRAMEWORK_VERSION A
			MACOSX_FRAMEWORK_IDENTIFIER com.Julivi.nft
		)
		target_link_libraries( ${LIBNFT_PROJ_NAME} ${OpenCV_LIBS} "-framework Accelerate -framework AVFoundation -framework CoreImage -framework CoreMedia -framework CoreVideo -framework OpenGLES")
	else()
		set_target_properties(${LIBNFT_PROJ_NAME} PROPERTIES
			BUNDLE TRUE
			MACOSX_BUNDLE_INFO_STRING "libnft library"
			MACOSX_BUNDLE_LONG_VERSION_STRING 1.0.0
			MACOSX_BUNDLE_BUNDLE_NAME nft
			MACOSX_BUNDLE_GUI_IDENTIFIER com.Julivi.nft
			MACOSX_BUNDLE_SHORT_VERSION_STRING 1.0.0
			MACOSX_BUNDLE_BUNDLE_VERSION 1
		)
		target_link_libraries( ${LIBNFT_PROJ_NAME} ${OpenCV_LIBS})
	endif()
endif()

# Android
if(ANDROID)
	target_link_libraries( ${LIBNFT_PROJ_NAME} ${OpenCV_LIBS} android log m dl GLESv2 EGL )
endif()

