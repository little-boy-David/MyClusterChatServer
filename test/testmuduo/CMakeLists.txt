#头文件搜索路径 对应4
# include_directories()
#库文件搜索路径 对应5
# link_directories()

#设置需要编译的源文件列表 对应3
set(SRC_LIST muduo_server.cpp)

#也可以指定路径将所有文件放入变量名(下面是指定当前路径)：aux_source_directory(. SRC_LIST)

# 设置可执行文件的最终的路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# 根据要编译的文件生成可执行文件 对应1、3
add_executable(server ${SRC_LIST})

#其它命令：生成库：add_library()

# server这个目标程序 需要链接的库 对应6
target_link_libraries(server muduo_net muduo_base pthread)


