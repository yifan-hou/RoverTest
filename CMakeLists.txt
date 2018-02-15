cmake_minimum_required(VERSION 2.8.3)
#######################
project(Epos)
set (CMAKE_CXX_FLAGS "--std=gnu++11 ${CMAKE_CXX_FLAGS}")

# find_package(Boost REQUIRED COMPONENTS system)


include_directories(
  include/
)

#######################
## Compile Libraries ##
#######################

add_library(RoverDriver SHARED src/RoverTest.cpp)
target_link_libraries(RoverDriver 
  libEposCmd.so
)

add_executable(unit_test src/test.cpp)
target_link_libraries(unit_test
  RoverDriver
)

add_executable(closeEpos src/closeEpos.cpp)
target_link_libraries(closeEpos
  RoverDriver
)

#######################
## Install Libraries ##
#######################

# # 
# #   Installation for ROS. Comment out if not using ROS.
# # 
# catkin_package(
#   INCLUDE_DIRS include ${PROJECT_BINARY_DIR}/
#   LIBRARIES ${PROJECT_NAME}
#   CATKIN_DEPENDS roscpp
#   )

# install(DIRECTORY include ${PROJECT_BINARY_DIR}/
#     DESTINATION ${CATKIN_PACKAGE_INCLUDE_DESTINATION}
#     FILES_MATCHING PATTERN "*.h"
#     )


# # 
# #   Uncomment this part if used as an independent library.
# #   Install headers/libs to /usr/local. 
# # 
# # headers
# install(DIRECTORY include ${PROJECT_BINARY_DIR}/
#     DESTINATION /usr/local/include/${PROJECT_NAME}
#     FILES_MATCHING PATTERN "*.h"
#     )

# # binaries
# install(TARGETS ${PROJECT_NAME} egm_proto practical_socket
#     DESTINATION /usr/local/lib/${PROJECT_NAME}
#     )
