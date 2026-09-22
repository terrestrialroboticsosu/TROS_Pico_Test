# tros_pico_test
Required environment variables:
- `PICO_SDK_PATH`: path to the pico sdk as set up [here](https://github.com/micro-ROS/micro_ros_raspberrypi_pico_sdk/tree/jazzy)
- `MICRO_ROS_PICO_DIR`: path to [this thing](https://github.com/micro-ROS/micro_ros_raspberrypi_pico_sdk/tree/jazzy)

This project requires a full ros2 jazzy environment. God help you.

## Using

Build:
```sh
mkdir build
cmake -B build
cd build
make
```

Deploy to a pico in write mode:
```sh
cp tros_pico_test.uf2 /run/media/$USER/RPI-RP2
```

Get the agent up and running:
```sh
ros2 run micro_ros_agent micro_ros_agent serial --dev /dev/ttyACM0 -b 115200
```

Publish values from 0-100 for motor driving:
```sh
ros2 topic pub /motor_output_percent std_msgs/msg/Int32 "data: 64"
```

Show what's being (theoretically) sent to the motor:
```sh
ros2 topic echo /pico_publisher
```
