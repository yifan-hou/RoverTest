
Maxon: read current  self
Read FT sensor: topic
Read Realsense, color&point: topic


# Dependency
ROS packages: netft, robot, matvec

# How to run
1. Run realsense ROS package on driver
2. Launch robot_node: ```roslaunch robot_node abb120.launch ```
3. Run this launch file: ```roslaunch RoverTest RoverTest.launch```
