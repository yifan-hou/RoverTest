cmake_minimum_required(VERSION 2.8.3)
#######################
project(RoverExperiment)

add_compile_options(-std=c++11)

# find_package(Boost REQUIRED COMPONENTS system)
find_package(catkin REQUIRED COMPONENTS
  roscpp
  robot_comm
  matvec
)

include_directories(
  include/
  ${catkin_INCLUDE_DIRS}
)

#######################
## Compile Libraries ##
#######################

add_library(RoverDriver SHARED src/RoverMotor.cpp src/TimerLinux.cpp)
target_link_libraries(RoverDriver 
  ${catkin_LIBRARIES}
  libEposCmd.so
)

add_executable(RoverExperiment src/main.cpp)
target_link_libraries(RoverExperiment
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
