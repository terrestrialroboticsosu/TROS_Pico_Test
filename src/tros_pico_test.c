#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"
#include <stdio.h>

#include <rcl/error_handling.h>
#include <rcl/rcl.h>
#include <rclc/executor.h>
#include <rclc/rclc.h>
#include <rmw_microros/rmw_microros.h>
#include <std_msgs/msg/float64.h>
#include <std_msgs/msg/int32.h>

#include "pico/stdlib.h"
#include "pico_uart_transports.h"
#include "std_msgs/msg/float64.h"

const uint PWM_PIN = 10;

rcl_timer_t timer;
rcl_node_t node;
rcl_allocator_t allocator;
rclc_support_t support;
rclc_executor_t executor;
rcl_subscription_t subscriber;
uint slice_num = 0;
int toSet = 0;

rcl_publisher_t publisher;
std_msgs__msg__Int32 pubMsg;
std_msgs__msg__Int32 motor_output_message;

void timer_callback(rcl_timer_t *timer, int64_t last_call_time) {
  rcl_ret_t ret = rcl_publish(&publisher, &pubMsg, NULL);
  pubMsg.data = toSet;
}

void subscription_callback(const void *msgin) {
  const std_msgs__msg__Int32 *msg = (const std_msgs__msg__Int32 *)msgin;
  printf("Received: %d\n", msg->data);
  toSet = msg->data * 80; // 100 -> 8000
  pwm_set_chan_level(slice_num, PWM_CHAN_B, toSet);
}

// 125 MHz

int main(int argc, const char *const *argv) {
  rmw_uros_set_custom_transport(
      true, NULL, pico_serial_transport_open, pico_serial_transport_close,
      pico_serial_transport_write, pico_serial_transport_read);

  // gpio_init(LED_PIN);
  // gpio_set_dir(LED_PIN, GPIO_OUT);
  gpio_set_function(10, GPIO_FUNC_PWM);
  gpio_set_function(11, GPIO_FUNC_PWM);
  slice_num = pwm_gpio_to_slice_num(PWM_PIN);

  pwm_set_wrap(slice_num, 8000);

  pwm_set_chan_level(slice_num, PWM_CHAN_B, 2000);

  allocator = rcl_get_default_allocator();

  // Wait for agent successful ping for 2 minutes.
  const int timeout_ms = 1000;
  const uint8_t attempts = 120;

  rcl_ret_t ret = rmw_uros_ping_agent(timeout_ms, attempts);

  if (ret != RCL_RET_OK) {
    // Unreachable agent, exiting program.
    return ret;
  }

  rclc_support_init(&support, 0, NULL, &allocator);

  rclc_node_init_default(&node, "pico_node", "", &support);
  rclc_publisher_init_default(&publisher, &node,
                              ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
                              "pico_publisher");
  rclc_subscription_init_default(
      &subscriber, &node, ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
      "motor_output_percent");

  rclc_timer_init_default(&timer, &support, RCL_MS_TO_NS(1000), timer_callback);
  executor = rclc_executor_get_zero_initialized_executor();
  rclc_executor_init(&executor, &support.context, 2, &allocator);
  rclc_executor_add_timer(&executor, &timer);
  rclc_executor_add_subscription(&executor, &subscriber, &motor_output_message,
                                 &subscription_callback, ON_NEW_DATA);
  while (true) {
    rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100));
  }
  return 0;
  // rcl_allocator_t allocator = rcl_get_default_allocator();
  // rclc_support_t support;
  //
  // // create init_options
  // RCCHECK(rclc_support_init(&support, argc, argv, &allocator));
  //
  // // create node
  // rcl_node_t node;
  // RCCHECK(rclc_node_init_default(&node, "tros_pwm_test_pico", "", &support));
  //
  // // create publisher
  // RCCHECK(rclc_publisher_init_default(
  //     &publisher, &node, ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
  //     "int32_publisher"));
  //
  // // create subscriber
  // RCCHECK(rclc_subscription_init_default(
  //     &subscriber, &node, ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
  //     "motor_output_percent"));
  //
  // // create timer,
  // rcl_timer_t timer;
  // const unsigned int timer_timeout = 1000;
  // RCCHECK(rclc_timer_init_default(&timer, &support,
  // RCL_MS_TO_NS(timer_timeout),
  //                                 timer_callback));
  //
  // // create executor
  // rclc_executor_t executor = rclc_executor_get_zero_initialized_executor();
  // RCCHECK(rclc_executor_init(&executor, &support.context, 2, &allocator));
  // RCCHECK(rclc_executor_add_timer(&executor, &timer));
  // RCCHECK(rclc_executor_add_subscription(&executor, &subscriber, &recv_msg,
  //                                        &subscription_callback,
  //                                        ON_NEW_DATA));
  //
  // send_msg.data = 0;
  //
  // while (true) {
  //   rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100));
  // }
  //
  // RCCHECK(rcl_subscription_fini(&subscriber, &node));
  // RCCHECK(rcl_publisher_fini(&publisher, &node));
  // RCCHECK(rcl_node_fini(&node));
}
