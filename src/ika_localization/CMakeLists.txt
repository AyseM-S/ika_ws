cmake_minimum_required(VERSION 3.8)
project(ika_localization)
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()
# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(std_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)

add_executable(wheel_odom_node src/wheel_odom_node.cpp)
ament_target_dependencies(wheel_odom_node rclcpp std_msgs nav_msgs)

add_executable(fake_sensors src/fake_sensors.cpp)
ament_target_dependencies(fake_sensors rclcpp std_msgs sensor_msgs geometry_msgs)

install(TARGETS
  wheel_odom_node
  fake_sensors
  DESTINATION lib/${PROJECT_NAME})

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()
ament_package()
