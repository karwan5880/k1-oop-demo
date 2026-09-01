cmake_minimum_required(VERSION 3.8)
project(demo1)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(booster_msgs REQUIRED)

add_executable(actions src/actions.cpp)
target_include_directories(actions PRIVATE include)
ament_target_dependencies(actions rclcpp geometry_msgs booster_msgs)

install(TARGETS actions DESTINATION lib/${PROJECT_NAME})
ament_package()
