set(project_name ${PROJECT_NAME})

project(${PROJECT_NAME})

set(example_name example_lab4)
set(executable_name Lab4)
add_executable(${executable_name} ${example_name}.cpp)
target_link_libraries(${executable_name} PUBLIC ${project_name})


unset(project_name)
unset(example_name)