#include <memory>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

#include "nav_msgs/msg/path.hpp"
#include "tf2_msgs/msg/tf_message.hpp"
#include "geometry_msgs/msg/twist.hpp"

#include "tf2/utils.h"

using namespace std::chrono_literals;
using std::placeholders::_1;


struct Point {
    float x;
    float y;
    float yaw = 0;
};


std::vector<Point> path;
bool path_ready = false;
struct Point state;
struct Point reference;
float dt = 0.01;
# define M_PI           3.14159265358979323846  /* pi */

class MinimalSubscriber : public rclcpp::Node
{
  public:
    MinimalSubscriber()
    : Node("controller")
    {
      subscription_path = this->create_subscription<nav_msgs::msg::Path>(
      "path", 1, std::bind(&MinimalSubscriber::getPath_callback, this, _1));
      
      subscription_state = this->create_subscription<tf2_msgs::msg::TFMessage>(
      "tf", 1, std::bind(&MinimalSubscriber::state_callback, this, _1));
      
      publisher_command = this->create_publisher<geometry_msgs::msg::Twist>(
      "cmd_vel", 1);

      controller = this->create_wall_timer(
      10ms, std::bind(&MinimalSubscriber::controller_callback, this));

      RCLCPP_INFO(this->get_logger(), "Node started");
      
    }

  private:
    void getPath_callback(const nav_msgs::msg::Path::SharedPtr msg) const
    {
        //RCLCPP_INFO(this->get_logger(), "I heard: '%s'", msg->data.c_str());

        float x,y;
        for(int i = 0; i < msg->poses.size() ; i++) {
            x = msg->poses[i].pose.position.x;
            y = msg->poses[i].pose.position.y;
            struct Point new_Point;
            new_Point.x = x;
            new_Point.y = y;
            RCLCPP_INFO(this->get_logger(), "received x'%f'", new_Point.x);
            RCLCPP_INFO(this->get_logger(), "received y'%f'", new_Point.y);
            path.push_back(new_Point);
        }

        path_ready = true;
        RCLCPP_INFO(this->get_logger(), "Path received");
      
    }

    void controller_callback()
    {
      /*if(path_ready == true) {
            reference = path.back();

            RCLCPP_INFO(this->get_logger(), "reference x'%f'", reference.x);
            RCLCPP_INFO(this->get_logger(), "reference y'%f'", reference.y);
      */    
            double k1 = 0.5;
            double b = 0.1;

            //input-output lin
            double theta_woffset = state.yaw + M_PI/2.;
            double error_x = 0 - (state.x + b*cos(theta_woffset));
            double error_y = 0 - (state.y + b*sin(theta_woffset));


            //double u1_io = k1*(reference.x - state.x);
            double u1_io = k1*error_x;
            //double u2_io = k1*(reference.y - state.y);
            double u2_io = k1*error_y;
            

            double v = cos(theta_woffset)*u1_io + sin(theta_woffset)*u2_io;
            double w = -sin(theta_woffset)*u1_io/b + cos(theta_woffset)*u2_io/b;

            //RCLCPP_INFO(this->get_logger(), "v: '%f'", v);
            //RCLCPP_INFO(this->get_logger(), "w: '%f'", w);

            auto commanded_vel = geometry_msgs::msg::Twist();
            commanded_vel.linear.x = v;
            commanded_vel.angular.z = w;

            publisher_command->publish(commanded_vel);

     /*       path.pop_back();
            if(path.empty())
                path_ready = false;
      }
    */

    }


    void state_callback(const tf2_msgs::msg::TFMessage::SharedPtr msg) const
    {

        tf2::Quaternion q(
            msg->transforms[0].transform.rotation.x,
            msg->transforms[0].transform.rotation.y,
            msg->transforms[0].transform.rotation.z,
            msg->transforms[0].transform.rotation.w);
        tf2::Matrix3x3 m(q);
        double roll, pitch, yaw;
        m.getRPY(roll, pitch, yaw);

        if(msg->transforms[0].child_frame_id == "base_footprint") {
          state.x = msg->transforms[0].transform.translation.x;
          state.y = msg->transforms[0].transform.translation.y;
          state.yaw = yaw;
        }
        
        RCLCPP_INFO(this->get_logger(), "#####################");
        RCLCPP_INFO(this->get_logger(), "state x'%f'", msg->transforms[0].transform.translation.x);
        RCLCPP_INFO(this->get_logger(), "state y'%f'", state.y);
        RCLCPP_INFO(this->get_logger(), "state theta'%f'", state.yaw);
        
    
    }


    rclcpp::Subscription<nav_msgs::msg::Path>::SharedPtr subscription_path;
    rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr publisher_command;
    rclcpp::Subscription<tf2_msgs::msg::TFMessage>::SharedPtr subscription_state;
    rclcpp::TimerBase::SharedPtr controller;

};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<MinimalSubscriber>());
  rclcpp::shutdown();
  return 0;
}