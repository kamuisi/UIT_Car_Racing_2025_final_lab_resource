add_library(camera STATIC Camera.cpp)
add_library(servo_oled STATIC Servo.cpp SH1106.cpp)
add_library(model STATIC Model.cpp)
add_library(motor STATIC Motor.cpp)

target_link_libraries(camera PRIVATE ${OpenCV_LIBS})
target_link_libraries(servo_oled PRIVATE i2c)
target_link_libraries(model PRIVATE torchtrt PUBLIC ${TORCH_LIBRARIES} ${OpenCV_LIBS})