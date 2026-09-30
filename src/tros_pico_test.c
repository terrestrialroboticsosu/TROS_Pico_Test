#include <stdio.h>

#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "pico/stdlib.h"
#include "pico_uart_transports.h"

#include <rcl/error_handling.h>
#include <rcl/rcl.h>
#include <rclc/executor.h>
#include <rclc/rclc.h>
#include <rmw_microros/rmw_microros.h>
#include <std_msgs/msg/int32.h>

#define RIGHT_SIDE_PWM_PIN 11
#define LEFT_SIDE_PWM_PIN 9

rcl_timer_t timer;
rcl_node_t node;
rcl_allocator_t allocator;
rclc_support_t support;
rclc_executor_t executor;
rcl_subscription_t rightSubscriber;
rcl_subscription_t leftSubscriber;
uint right_slice_num = 0;
uint left_slice_num = 0;
int rightData = 0;

rcl_publisher_t publisher;
std_msgs__msg__Int32 pubMsg;
std_msgs__msg__Int32 right_message;
std_msgs__msg__Int32 left_message;

void timer_callback(rcl_timer_t *timer, int64_t last_call_time) {
  (void)timer;
  (void)last_call_time;
  
  pubMsg.data = rightData;
  rcl_publish(&publisher, &pubMsg, NULL);
}

void right_subscription_callback(const void *msgin) {
  const std_msgs__msg__Int32 *msg = (const std_msgs__msg__Int32 *)msgin;
  int percent = msg->data;
  
  // Clamp input between -100 and 100
  if (percent < -100) percent = -100;
  if (percent > 100) percent = 100;
  
  // REMOVED: printf("Received Right: %d%%\n", percent);
  
  int pulse_us = 1500 + (percent * 5);
  
  rightData = percent; 
  pwm_set_chan_level(right_slice_num, PWM_CHAN_B, pulse_us);
}

void left_subscription_callback(const void *msgin) {
  const std_msgs__msg__Int32 *msg = (const std_msgs__msg__Int32 *)msgin;
  int percent = msg->data;
  
  // Clamp input between -100 and 100
  if (percent < -100) percent = -100;
  if (percent > 100) percent = 100;
  
  // REMOVED: printf("Received Left: %d%%\n", percent);
  
  // Convert -100 to 100 into standard RC PWM (1000us to 2000us)
  int pulse_us = 1500 + (percent * 5);
  
  // CHANGE: PWM_CHAN_A -> PWM_CHAN_B
  pwm_set_chan_level(left_slice_num, PWM_CHAN_B, pulse_us); 
}

int main(int argc, const char *const *argv) {
  (void)argc;
  (void)argv;

  rmw_uros_set_custom_transport(
      true, NULL, pico_serial_transport_open, pico_serial_transport_close,
      pico_serial_transport_write, pico_serial_transport_read);

  gpio_set_function(RIGHT_SIDE_PWM_PIN, GPIO_FUNC_PWM);
  gpio_set_function(LEFT_SIDE_PWM_PIN, GPIO_FUNC_PWM);
  
  right_slice_num = pwm_gpio_to_slice_num(RIGHT_SIDE_PWM_PIN);
  left_slice_num = pwm_gpio_to_slice_num(LEFT_SIDE_PWM_PIN);

  // --- 100 Hz RC SERVO PWM SETUP ---
  // 1 tick = 1 microsecond
  pwm_set_clkdiv(right_slice_num, 125.0f);
  pwm_set_clkdiv(left_slice_num, 125.0f);
  
  // 100 Hz = 10ms period. 10,000 ticks = 10ms.
  // We set wrap to 9999 because it counts from 0.
  pwm_set_wrap(right_slice_num, 9999);
  pwm_set_wrap(left_slice_num, 9999);

  // Set Talon SRX to exactly Neutral (1.5ms pulse) on startup
  pwm_set_chan_level(right_slice_num, PWM_CHAN_B, 1500); // GPIO 10 is Chan A
  
  // CHANGE: PWM_CHAN_A -> PWM_CHAN_B
  pwm_set_chan_level(left_slice_num, PWM_CHAN_B, 1500);  // GPIO 9 is Chan B
  
  pwm_set_enabled(right_slice_num, true);
  pwm_set_enabled(left_slice_num, true);

  allocator = rcl_get_default_allocator();

  const int timeout_ms = 1000;
  const uint8_t attempts = 120;

  rcl_ret_t ret = rmw_uros_ping_agent(timeout_ms, attempts);

  if (ret != RCL_RET_OK) {
    return ret;
  }

  rclc_support_init(&support, 0, NULL, &allocator);

  rclc_node_init_default(&node, "pico_node", "", &support);
  rclc_publisher_init_default(&publisher, &node,
                              ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
                              "pico_publisher");
  rclc_subscription_init_default(
      &rightSubscriber, &node, ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
      "right_velocity_percent");
  rclc_subscription_init_default(
      &leftSubscriber, &node, ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
      "left_velocity_percent");

  rclc_timer_init_default(&timer, &support, RCL_MS_TO_NS(1000), timer_callback);
  
  executor = rclc_executor_get_zero_initialized_executor();
  
  // 3 Handles: 1 Timer + 2 Subscriptions
  rclc_executor_init(&executor, &support.context, 3, &allocator);
  
  rclc_executor_add_timer(&executor, &timer);
  rclc_executor_add_subscription(&executor, &rightSubscriber, &right_message,
                                 &right_subscription_callback, ON_NEW_DATA);
  rclc_executor_add_subscription(&executor, &leftSubscriber, &left_message,
                                 &left_subscription_callback, ON_NEW_DATA);

  while (true) {
    rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100));
  }

  return 0;
}