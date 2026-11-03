## Lightbar与armor分别储存什么信息
### Lightbar
struct Lightbar//一根灯条
{
  std::size_t id;//唯一编号（如果装甲 A 用了编号为 3 的灯条，装甲 B 也用了编号为 3 的灯条，说明这俩抢了，必须删掉一个。如果没有这个 id，程序根本不知道哪两根灯条是同一根。）
  
  Color color;//颜色（红、蓝等）红配红，蓝配蓝。红方机器人和蓝方机器人绝不能配到一起。
  
  cv::Point2f center, top, bottom, top2bottom;//核心位置信息 center灯条中心点 ；top/bottom灯条上下两端；top2bottom是一个方向向量（从顶到底）
  
  std::vector<cv::Point2f> points;//灯条的四个角点（用来在图上画框）
  
接下来是三大几何特征，给check_geometry筛选用

  double angle, angle_error, length, width, ratio;//角度和角度误差（灯条应该差不多是竖直的。如果歪得太离谱（比如斜着长在背景上），程序就会判定误差太大，扔掉。）；长、宽、长宽比
  cv::RotatedRect rotated_rect;//openCV的旋转矩阵类，记录了中心、长度和角度

  Lightbar(const cv::RotatedRect & rotated_rect, std::size_t id);
  
  Lightbar() {};
};
### Armor
struct Armor
{
//1.基础拼装
  Color color;
  Lightbar left, right;     //used to be const
  cv::Point2f center;       // 不是对角线交点，不能作为实际中心！Why？装甲在摄像头中往往是斜着的，这样观察长方形，其对角线交点并不是真实的几何中心
  cv::Point2f center_norm;  // 归一化坐标。大白话解释：就是把绝对坐标变成百分比。x和y除以图像宽高，变成0~1的小数，方便计算。
  std::vector<cv::Point2f> points; // 装甲的四个角点

2.几何筛选
  double ratio;              // 两灯条的中点连线与长灯条的长度之比，用来判断大小装甲
  double side_ratio;         // 长灯条与短灯条的长度之比（若长度相差太多，及说明不是一对灯条）
  double rectangular_error;  // 灯条和中点连线所成夹角与π/2的差值（即是矩形误差，四个点连起来像不像一个矩形）

3.下面是神经网络分类结果
 ArmorType type;       // 结果：大装甲/小装甲 
 ArmorName name;       // 结果：是什么兵种（几号、哨兵等）
  ArmorPriority priority; // 优先级（比如先打英雄还是先打步兵） 
 int class_id;         // 分类器输出的类别数字 
 cv::Mat pattern;      // 截取下来的那块图案（喂给神经网络吃的就是它） 
 double confidence;    // 置信度（神经网络有多大把握）

  bool duplicated;      // 去重标记（true表示它是重复的，等会儿会被删掉）

  Eigen::Vector3d xyz_in_gimbal;  // 单位：m
  Eigen::Vector3d xyz_in_world;   // 单位：m
  Eigen::Vector3d ypr_in_gimbal;  // 单位：rad
  Eigen::Vector3d ypr_in_world;   // 单位：rad
  Eigen::Vector3d ypd_in_world;   // 球坐标系

  double yaw_raw;  // rad

  Armor(const Lightbar & left, const Lightbar & right);
  Armor(
    int class_id, float confidence, const cv::Rect & box, std::vector<cv::Point2f> armor_keypoints);
  Armor(
    int class_id, float confidence, const cv::Rect & box, std::vector<cv::Point2f> armor_keypoints,
    cv::Point2f offset);
  Armor(
    int color_id, int num_id, float confidence, const cv::Rect & box,
    std::vector<cv::Point2f> armor_keypoints);
  Armor(
    int color_id, int num_id, float confidence, const cv::Rect & box,
    std::vector<cv::Point2f> armor_keypoints, cv::Point2f offset);
};
