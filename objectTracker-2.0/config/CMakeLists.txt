#1.cmake verson，指定cmake版本 
cmake_minimum_required(VERSION 3.2) 
#2.project name，指定项目的名称，一般和项目的文件夹名称对应 

set(LIB_NAME "config") 

PROJECT(${LIB_NAME})

set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")
set(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/lib/)


set(SRC_DIR "./src/")

set(INICONF_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/param_conf/include/")
#3.head file path，头文件目录
INCLUDE_DIRECTORIES(
	${INICONF_INCLUDE_DIRS} 
	)

LINK_DIRECTORIES(${INIPARSER_LIB_DIRS})

aux_source_directory(${SRC_DIR} DIR_SRCS)
#4.source directory，源文件目录
#AUX_SOURCE_DIRECTORY(src DIR_SRCS)

#5.set environment variable，设置环境变量，编译用到的源文件全部都要放到这里，否则编译能够通过，但是执行的时候会出现各种问题，比如"symbol lookup error xxxxx , undefined symbol"
SET(TEST_MATH ${DIR_SRCS}
	)


#6.add executable file添加要编译的可执行文件
add_library(${LIB_NAME} SHARED ${DIR_SRCS} )

#7.add link library，添加可执行文件所需要的库，比如我们用到了libm.so（命名规则：lib+name+.so），就添加该库的名称

set(CMAKE_CXX_FLAGS "-std=c++0x  -lm -pthread")
