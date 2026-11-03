#ifndef AUTO_AIM__ARMOR_HPP
#define AUTO_AIM__ARMOR_HPP
// 头文件保护：防止这个文件被重复 include 导致报错

// --- 引入工具包 ---
#include <Eigen/Dense>       // 线性代数库，用来存 3D 坐标和矩阵计算（留给第4组用）
#include <opencv2/opencv.hpp> // 视觉神仙库，画图、找轮廓、切图全靠它
#include <string>            // 存字符串
#include <vector>            // 动态数组，存一堆点或者一堆灯条

namespace auto_aim
{
// ==========================================
// 第一部分：给机器定义“词汇表”（枚举）
// ==========================================

// 颜色种类
enum Color
{
  red,        // 红
  blue,       // 蓝
  extinguish, // 熄灭（没亮灯）
  purple      // 紫色（可能特殊目标用）
};
// 翻译表：把底层数字变成屏幕上打印的英文单词
const std::vector<std::string> COLORS = {"red", "blue", "extinguish", "purple"};

// 装甲尺寸种类
enum ArmorType
{
  big,   // 大装甲板
  small  // 小装甲板
};
const std::vector<std::string> ARMOR_TYPES = {"big", "small"};

// 装甲身份（兵种）
enum ArmorName
{
  one,       // 1号（英雄）
  two,       // 2号（步兵）
  three,     // 3号
  four,      // 4号
  five,      // 5号
  sentry,    // 哨兵
  outpost,   // 前哨站
  base,      // 基地
  not_armor  // 什么都不是（神经网络觉得是反光/背景）
};
// 翻译表：把底层的0, 1, 2变成屏幕上的 "one", "two"
const std::vector<std::string> ARMOR_NAMES = {"one",    "two",     "three", "four",     "five",
                                              "sentry", "outpost", "base",  "not_armor"};

// 优先级（打靶顺序）
enum ArmorPriority
{
  first = 1,
  second,
  third,
  forth,
  fifth
};

// 【核心防呆机制】：合法身份证大全（查表防瞎认）
// 格式：(颜色, 身份, 大小)
// 比如程序算出来个“大号的哨兵”，在这里找不到这个组合，就会被当成误检扔掉。
// 另外基地（base）有大有小；3、4、5号也有大有小。
// clang-format off
const std::vector<std::tuple<Color, ArmorName, ArmorType>> armor_properties = {
  {blue, sentry, small},     {red, sentry, small},     {extinguish, sentry, small},
  {blue, one, small},        {red, one, small},        {extinguish, one, small},
  {blue, two, small},        {red, two, small},        {extinguish, two, small},
  {blue, three, small},      {red, three, small},      {extinguish, three, small},
  {blue, four, small},       {red, four, small},       {extinguish, four, small},
  {blue, five, small},       {red, five, small},       {extinguish, five, small},
  {blue, outpost, small},    {red, outpost, small},    {extinguish, outpost, small},
  {blue, base, big},         {red, base, big},         {extinguish, base, big},      {purple, base, big},       
  {blue, base, small},       {red, base, small},       {extinguish, base, small},    {purple, base, small},    
  {blue, three, big},        {red, three, big},        {extinguish, three, big}, 
  {blue, four, big},         {red, four, big},         {extinguish, four, big},  
  {blue, five, big},         {red, five, big},         {extinguish, five, big}};
// clang-format on

// ==========================================
// 第二部分：Lightbar（灯条）—— 第2组交接的半成品
// ==========================================
struct Lightbar // 一根灯条（发光的筷子）
{
  std::size_t id; // 唯一编号（如果装甲A和装甲B都用了编号为3的灯条，说明这俩抢了，最后去重时必须要删掉一个。如果没有这个id，程序根本不知道哪两根灯条是同一根。）
  Color color;    // 颜色（红、蓝等）。配对铁律：红配红，蓝配蓝。红方和蓝方的灯条绝不能配到一起。
  
  cv::Point2f center, top, bottom, top2bottom; 
  // center: 灯条中心点
  // top/bottom: 灯条上下两端
  // top2bottom: 这是一个方向向量（从顶到底），在 get_pattern 截取装甲图案时非常关键，用来延长灯条框住数字。

  std::vector<cv::Point2f> points; // 灯条的四个角点（平时用来在视频窗口上画黄色的点点）
  
  // 以下三个是“第一道安检”的指标：太歪、太胖、太短的不要
  double angle, angle_error, length, width, ratio; 
  // angle/angle_error: 角度和角度误差（灯条应该差不多是竖直的。如果歪得太离谱，程序就会扔掉）
  // length/width/ratio: 长、宽、长宽比（必须是细长条，太胖的可能是方形反光贴）

  cv::RotatedRect rotated_rect; // OpenCV的旋转矩形类（记录了中心、宽高、旋转角度，底层数据）

  Lightbar(const cv::RotatedRect & rotated_rect, std::size_t id); // 正经构造函数：从矩形里提取长宽高等信息
  Lightbar() {}; // 空壳子构造函数：防止在别处声明空对象时编译器报错
};

// ==========================================
// 第三部分：Armor（装甲）—— 第3组的最终产品（大礼盒）
// ==========================================
struct Armor
{
  // --- 1. 基础拼装：两根筷子夹一块板 ---
  Color color;
  Lightbar left, right;     // 左灯条、右灯条
  cv::Point2f center;       // 装甲中心点（注释警告：不是对角线交点！Why？因为摄像头看装甲往往是斜着的，透视变形会导致对角线交点偏移，不能作为真实几何中心使用）
  cv::Point2f center_norm;  // 归一化坐标。大白话解释：把绝对像素坐标变成百分比。x和y除以图像宽高，变成0~1的小数，换分辨率也不怕。
  std::vector<cv::Point2f> points; // 装甲的四个角点（交给第4组算 3D 坐标的核心数据）

  // --- 2. 第二道安检：几何指标 ---
  double ratio;              // 两灯条的中点连线，与长灯条的长度之比（判断大小装甲的关键）
  double side_ratio;         // 长灯条与短灯条的长度之比（左右两边长度差太多，说明不是一对）
  double rectangular_error;  // 矩形误差（灯条和中点连线所成夹角与π/2的差值，也就是四个点连起来到底像不像个矩形）

  // --- 3. 神经网络的判决书（分类结果） ---
  ArmorType type;            // 结果：大装甲/小装甲
  ArmorName name;            // 结果：是什么兵种（几号、哨兵、基地、或者 not_armor 不是装甲）
  ArmorPriority priority;    // 优先级（比如先打英雄还是先打步兵）
  int class_id;              // 分类器输出的底层类别数字
  cv::Rect box;              // 神经网络直接给出的包围框（YOLO流派会用）
  cv::Mat pattern;           // 截取下来的那块小图案（喂给神经网络吃的就是它）
  double confidence;         // 置信度（神经网络有多大把握，太低的话 check_name 会把它扔掉）
  
  // --- 4. 去重标记 ---
  bool duplicated;           // 去重开关（true 表示它抢了别人的灯条，最后会被 remove_if 删掉）

  // --- 5. 留给第4组（位姿解算）和第5组（跟踪）填的坑 ---
  Eigen::Vector3d xyz_in_gimbal;  // 目标相对于云台的三维坐标 单位：m
  Eigen::Vector3d xyz_in_world;   // 目标在整个比赛场地（世界坐标系）里的绝对坐标 单位：m
  Eigen::Vector3d ypr_in_gimbal;  // 偏航、俯仰、翻滚：目标相对于云台的倾斜角度。比如对面在爬坡，装甲就是向上仰的。 单位：rad
  Eigen::Vector3d ypr_in_world;   // 目标在世界坐标系里的绝对朝向。 单位：rad
  Eigen::Vector3d ypd_in_world;   // 用“偏航角(Yaw)、俯仰角(Pitch)、距离(Distance)”来表示目标在哪里。 球坐标系
  
  double yaw_raw;  // rad

  // --- 6. 多种构造函数：为了兼容不同流派 ---
  // 第1种：传统视觉流派（我们组学的，靠两根灯条配对算出来）
  Armor(const Lightbar & left, const Lightbar & right);
  
  // 后面4种：深度学习流派（YOLO直接输出框和关键点，不需要找灯条。为了兼容未来可能换算法而留的接口）
  Armor(int class_id, float confidence, const cv::Rect & box, std::vector<cv::Point2f> armor_keypoints);
  Armor(int class_id, float confidence, const cv::Rect & box, std::vector<cv::Point2f> armor_keypoints, cv::Point2f offset);
  Armor(int color_id, int num_id, float confidence, const cv::Rect & box, std::vector<cv::Point2f> armor_keypoints);
  Armor(int color_id, int num_id, float confidence, const cv::Rect & box, std::vector<cv::Point2f> armor_keypoints, cv::Point2f offset);
};

}  // namespace auto_aim

#endif  // AUTO_AIM__ARMOR_HPP
