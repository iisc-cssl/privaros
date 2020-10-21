

#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include "rclcpp/rclcpp.hpp"

#include "sensor_msgs/msg/image.hpp"

#include "image_tools/options.hpp"


#include <memory>
#include <utility>


#include "rclcpp/rclcpp.hpp"

#include "std_msgs/msg/bool.hpp"

#include "image_tools/options.hpp"

#include "./burger.hpp"




#include <opencv2/core/core.hpp> 

#include <opencv2/highgui/highgui.hpp>

#include <opencv2/imgproc/imgproc.hpp>

#include <stdio.h>
#include <iostream>

using namespace cv;

using namespace std;

std::string
mat_type2encoding(int mat_type)
{
  switch (mat_type) {
    case CV_8UC1:
      return "mono8";
    case CV_8UC3:
      return "bgr8";
    case CV_16SC1:
      return "mono16";
    case CV_8UC4:
      return "rgba8";
    default:
      throw std::runtime_error("Unsupported encoding type");
  }
}



void convert_frame_to_message(
  const cv::Mat & frame, string frame_id, sensor_msgs::msg::Image & msg)
{
  // copy cv information into ros message
  msg.height = frame.rows;
  msg.width = frame.cols;
  msg.encoding = mat_type2encoding(frame.type());
  msg.step = static_cast<sensor_msgs::msg::Image::_step_type>(frame.step);
  size_t size = frame.step * frame.rows;
  msg.data.resize(size);
  memcpy(&msg.data[0], frame.data, size);
  msg.header.frame_id = frame_id;
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

/// Convert the ROS Image message to an OpenCV matrix and display it to the user.
// \param[in] msg The image message to show.
void show_image(
  const sensor_msgs::msg::Image::SharedPtr msg, bool show_camera, rclcpp::Logger logger, auto filterpub)
{
  RCLCPP_INFO(logger, "Received image #%s", msg->header.frame_id.c_str());
  std::cerr << "Received image #" << msg->header.frame_id.c_str() << std::endl;
  auto msgpub = std::make_unique<sensor_msgs::msg::Image>();
  cv::Mat flipped_frame;

    
  // Convert to an OpenCV matrix by assigning the data.
  cv::Mat frame(
    msg->height, msg->width, encoding2mat_type(msg->encoding),
    const_cast<unsigned char *>(msg->data.data()), msg->step);

  if (msg->encoding == "rgb8") {
    cv::cvtColor(frame, frame, cv::COLOR_RGB2BGR);
  }

  cv::Mat cvframe = frame;
  
  cv::Size size(480,480);
  cv::resize(cvframe, cvframe, size); 
  
  blur(cvframe,cvframe,Size(10,10)); 
  
   

  if (!cvframe.empty()) {
      
    // Convert to a ROS image
    convert_frame_to_message(cvframe, msg->header.frame_id, *msgpub);
    
    
    // Publish the image message and increment the frame_id.
    // RCLCPP_INFO(node_logger, "Publishing image #%zd", 0);
    filterpub->publish(std::move(msgpub));
    
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
  std::string topicpub("filterimage");

  // Force flush of the stdout buffer.
  // This ensures a correct sync of all prints
  // even when executed simultaneously within a launch file.
  setvbuf(stdout, NULL, _IONBF, BUFSIZ);

  

  auto node = rclcpp::Node::make_shared("filterapp");

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
  auto filterpub = node->create_publisher<sensor_msgs::msg::Image>(topicpub, qos);

  auto callback = [show_camera, &node, filterpub](const sensor_msgs::msg::Image::SharedPtr msg)
    {
      show_image(msg, show_camera, node->get_logger(), filterpub);
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
