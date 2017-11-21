cmake_minimum_required(VERSION 2.8.12)

project(deepin-auto-clean)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt5Core)

add_executable(${PROJECT_NAME} "main.cpp")

target_link_libraries(${PROJECT_NAME} Qt5::Core)

set(CONFIG_PATH /etc/${PROJECT_NAME})

option(DEFINE_MACRO "Option description" ON)
add_definitions(-DCONFIG_PATH=\"${CONFIG_PATH}\")

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
install(FILES ${PROJECT_SOURCE_DIR}/white.txt DESTINATION ${CONFIG_PATH})
install(FILES ${PROJECT_SOURCE_DIR}/black.txt DESTINATION ${CONFIG_PATH})
install(FILES ${PROJECT_SOURCE_DIR}/${PROJECT_NAME}-autostart.desktop DESTINATION /etc/xdg/autostart)
