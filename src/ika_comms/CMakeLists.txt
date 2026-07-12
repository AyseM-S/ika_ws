cmake_minimum_required(VERSION 3.8)
project(ika_comms)
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(nlohmann_json REQUIRED)

include_directories(include)

add_executable(route_bridge_node src/route_bridge_node.cpp)
ament_target_dependencies(route_bridge_node rclcpp nav_msgs geometry_msgs)
target_link_libraries(route_bridge_node nlohmann_json::nlohmann_json)

add_executable(test_route_sender src/test_route_sender.cpp)
target_link_libraries(test_route_sender nlohmann_json::nlohmann_json)

add_executable(test_route_sender_short src/test_route_sender_short.cpp)
target_link_libraries(test_route_sender_short nlohmann_json::nlohmann_json)

add_executable(test_route_sender_rejected src/test_route_sender_rejected.cpp)
target_link_libraries(test_route_sender_rejected nlohmann_json::nlohmann_json)

install(TARGETS route_bridge_node test_route_sender test_route_sender_short test_route_sender_rejected DESTINATION lib/${PROJECT_NAME})

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()
ament_package()
