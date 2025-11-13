cmake_minimum_required(VERSION 3.8)

project(elog DESCRIPTION "this is elog project")

# 设置变量
SET(ELOG_LIB_NAME elog)

# 添加预定义的宏定义
  
include_directories(
  "."
  "elog"
  )
  

# 设置编译参数
SET(CMAKE_C_FLAGS "-fPIC")

# ##############################################################################
# 查找所有.c文件
# ##############################################################################
# 设置子目录
set(ELOG_SRC_DIR_LIST ".")
list(APPEND 
     ELOG_SRC_DIR_LIST 
	 "plugins/file"
	 "port"
	 "src"
	 )


MESSAGE(STATUS "ELOG_SRC_DIR_LIST=${ELOG_SRC_DIR_LIST}")

# 编译所有源文件
foreach(ELOG_SRC_FILES ${ELOG_SRC_DIR_LIST})
  aux_source_directory(./${ELOG_SRC_FILES} ELOG_SRC_LIST)
endforeach()

MESSAGE(STATUS "ELOG_SRC_LIST=${ELOG_SRC_LIST}")

# ##############################################################################
# 编译目标文件
# ##############################################################################
add_library(${ELOG_LIB_NAME}_shared SHARED ${ELOG_SRC_LIST})
add_library(${ELOG_LIB_NAME}_static STATIC ${ELOG_SRC_LIST})

set_target_properties (${ELOG_LIB_NAME}_shared PROPERTIES OUTPUT_NAME ${ELOG_LIB_NAME})
set_target_properties (${ELOG_LIB_NAME}_static PROPERTIES OUTPUT_NAME ${ELOG_LIB_NAME})

# 设置目标文件复制到的目录
set(COPY_TARGET_DIR "${LIB_LINK_DIR}/thirdlib/lib")
 
# 添加自定义命令复制目标文件
add_custom_command(TARGET ${ELOG_LIB_NAME}_shared POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory ${COPY_TARGET_DIR}
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${ELOG_LIB_NAME}_shared> ${COPY_TARGET_DIR})

add_custom_command(TARGET ${ELOG_LIB_NAME}_static POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory ${COPY_TARGET_DIR}
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${ELOG_LIB_NAME}_static> ${COPY_TARGET_DIR}) 

# 设置源目录和目的目录
set(SOURCE_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/elog")
set(DESTINATION_FOLDER "${LIB_LINK_DIR}/thirdlib/include")
 
# 复制文件和文件夹
file(COPY ${SOURCE_FOLDER}
     DESTINATION ${DESTINATION_FOLDER}
     )