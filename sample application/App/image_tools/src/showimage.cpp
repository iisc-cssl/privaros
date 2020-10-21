// Copyright 2015 Open Source Robotics Foundation, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include "rclcpp/rclcpp.hpp"

#include "sensor_msgs/msg/image.hpp"

#include "image_tools/options.hpp"


#include <sys/time.h>
#include <time.h>
#include <stdlib.h>


long nanosec(struct timeval t) { /* Calculate nanoseconds in a timeval structure */
  
  return((t.tv_sec*1000000+t.tv_usec)*1000);
}
/// Convert a sensor_msgs::Image encoding type (stored as a string) to an OpenCV encoding type.
/**
 * \param[in] encoding A string representing the encoding type.
 * \return The OpenCV encoding type.
 */
int
encoding2mat_type(const std::string & encoding)
{
  if (encoding == "mono8") {
    return CV_8UC1;
  } else if (encoding == "bgr8") {
    return CV_8UC3;
  } else if (encoding == "mono16") {
    return CV_16SC1;
  } else if (encoding == "rgba8") {
    return CV_8UC4;
  } else if (encoding == "bgra8") {
    return CV_8UC4;
  } else if (encoding == "32FC1") {
    return CV_32FC1;
  } else if (encoding == "rgb8") {
    return CV_8UC3;
  } else {
    throw std::runtime_error("Unsupported encoding type");
  }
}

int count = 0;
/// Convert the ROS Image message to an OpenCV matrix and display it to the user.
// \param[in] msg The image message to show.
void show_image(
  const sensor_msgs::msg::Image::SharedPtr msg, bool show_camera, rclcpp::Logger logger)
{
  struct timeval now, sender;
  gettimeofday(&now, NULL); 

  std::string data = msg->header.frame_id.c_str();
  std::string delimiter = "$";
  std::string seconds = data.substr(data.find(":")+1, data.find(delimiter));
  std::string u_seconds = data.substr(data.find(delimiter) + 1);

  sender.tv_sec = stol(seconds);
  sender.tv_usec = stold(u_seconds);

  RCLCPP_INFO(logger, "Received image #%s", msg->header.frame_id.c_str());
  
  RCLCPP_INFO(logger, "I heard: [%s] seconds and [%s] micro seconds, count = [%d]", seconds.c_str(), u_seconds.c_str(), count);
  RCLCPP_INFO(logger, "Difference: %lf\n", (nanosec(now) - nanosec(sender)) / 1000000.0);
  count += 1;
  std::cerr << "Received image #" << msg->header.frame_id.c_str() << std::endl;

  if (show_camera) {
    // Convert to an OpenCV matrix by assigning the data.
    cv::Mat frame(
      msg->height, msg->width, encoding2mat_type(msg->encoding),
      const_cast<unsigned char *>(msg->data.data()), msg->step);
    
    if (msg->encoding == "rgb8") {
      cv::cvtColor(frame, frame, cv::COLOR_RGB2BGR);
    }

    cv::Mat cvframe = frame;

    // Show the image in a window called "showimage".
    cv::imshow("showimage", cvframe);
    // Draw the screen and wait for 1 millisecond.
    cv::waitKey(1);
  }
}

int main(int argc, char * argv[])
{
  // Pass command line arguments to rclcpp.
  rclcpp::init(argc, argv);

  // Initialize default demo parameters
  size_t depth = rmw_qos_profile_default.depth;
  rmw_qos_reliability_policy_t reliability_policy = rmw_qos_profile_default.reliability;
  rmw_qos_history_policy_t history_policy = rmw_qos_profile_default.history;
  bool show_camera = true;
  std::string topic("imageraw");

  // Force flush of the stdout buffer.
  // This ensures a correct sync of all prints
  // even when executed simultaneously within a launch file.
  setvbuf(stdout, NULL, _IONBF, BUFSIZ);

  // Configure demo parameters with command line options.
  if (!parse_command_options(
      argc, argv, &depth, &reliability_policy, &history_policy, &show_camera, nullptr, nullptr,
      nullptr, nullptr, &topic))
  {
    return 0;
  }

  if (show_camera) {
    // Initialize an OpenCV named window called "showimage".
    cv::namedWindow("showimage", cv::WINDOW_AUTOSIZE);
    cv::waitKey(1);
  }

  // Initialize a ROS node.
  auto node = rclcpp::Node::make_shared("showimage");

  // Set quality of service profile based on command line options.
  auto qos = rclcpp::QoS(
    rclcpp::QoSInitialization(
      // The history policy determines how messages are saved until taken by
      // the reader.
      // KEEP_ALL saves all messages until they are taken.
      // KEEP_LAST enforces a limit on the number of messages that are saved,
      // specified by the "depth" parameter.
      history_policy,
      // Depth represents how many messages to store in history when the
      // history policy is KEEP_LAST.
      depth
  ));

  // The reliability policy can be reliable, meaning that the underlying transport layer will try
  // ensure that every message gets received in order, or best effort, meaning that the transport
  // makes no guarantees about the order or reliability of delivery.
  qos.reliability(reliability_policy);

  auto callback = [show_camera, &node](const sensor_msgs::msg::Image::SharedPtr msg)
    {
      show_image(msg, show_camera, node->get_logger());
    };

  std::cerr << "Subscribing to topic '" << topic << "'" << std::endl;
  RCLCPP_INFO(node->get_logger(), "Subscribing to topic '%s'", topic.c_str());
  // Initialize a subscriber that will receive the ROS Image message to be displayed.
  auto sub = node->create_subscription<sensor_msgs::msg::Image>(
    topic, qos, callback);

  rclcpp::spin(node);

  rclcpp::shutdown();

  return 0;
}
