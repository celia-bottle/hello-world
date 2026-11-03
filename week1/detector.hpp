#ifndef AUTO_AIM__DETECTOR_HPP
#define AUTO_AIM__DETECTOR_HPP

#include <list>
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

#include "armor.hpp"
#include "classifier.hpp"

namespace auto_aim
{

class Detector
{
public:
  Detector(const std::string & config_path, bool debug = true);//读配置，设置要不要弹窗显示（debug）

  std::list<Armor> detect(const cv::Mat & bgr_img, int frame_count = -1);//核心业务：全图检测

  bool detect(Armor & armor, const cv::Mat & bgr_img);//局部精修（如果你已经有个大概位置，让它在这个位置附近重新找准四个角点）
  friend class YOLOV8;

private:
//核心工具：神经网络分类器（黑盒）
  Classifier classifier_;

  double threshold_;// 二值化阈值
  double max_angle_error_;   // 灯条最大倾斜角
  double min_lightbar_ratio_, max_lightbar_ratio_; // 灯条长宽比
  double min_lightbar_length_; // 灯条最短长度
  double min_armor_ratio_, max_armor_ratio_;  // 装甲整体宽高比
  double max_side_ratio_;// 左右灯条长度差限制
  double min_confidence_;  // 神经网络的最低置信度要求
  double max_rectangular_error_;  // 矩形误差限制

 //  辅助开关
  bool debug_;          // 是否显示画面
  std::string save_path_; // 存储异常图案的路径（patterns文件夹）

  // 利用PCA回归角点，参考自https://github.com/CSU-FYT-Vision/FYT2024_vision
  void lightbar_points_corrector(Lightbar & lightbar, const cv::Mat & gray_img) const;

 // 四大安检门（返回 bool，false 就淘汰）
  bool check_geometry(const Lightbar & lightbar) const; // 查灯条几何
  bool check_geometry(const Armor & armor) const;       // 查装甲几何
  bool check_name(const Armor & armor) const;           // 查身份（是不是 not_armor，置信度够不够）
  bool check_type(const Armor & armor) const;           // 查大小（大装甲/小装甲和名字对不对得上）

//  四大加工厂（干活，返回值）
  Color get_color(const cv::Mat & bgr_img, const std::vector<cv::Point> & contour) const; // 判红蓝
  cv::Mat get_pattern(const cv::Mat & bgr_img, const Armor & armor) const;                // 裁图片
  ArmorType get_type(const Armor & armor);                                                // 算大小
  cv::Point2f get_center_norm(const cv::Mat & bgr_img, const cv::Point2f & center) const; // 算归一化坐标


  void save(const Armor & armor) const;// 把有疑问的图存到本地
  void show_result(
    const cv::Mat & binary_img, const cv::Mat & bgr_img, const std::list<Lightbar> & lightbars,
    const std::list<Armor> & armors, int frame_count) const;   // 在屏幕上画图
};
};

}  // namespace auto_aim

#endif  // AUTO_AIM__DETECTOR_HPP
