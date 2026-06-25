#ifndef __MYIMAGE_H__
#define __MYIMAGE_H__

extern const unsigned char g_image_date_55x55[6050];
extern const unsigned char g_image_humi_55x55[6050];
extern const unsigned char g_image_temp_55x55[6050];
extern const unsigned char g_image_time_55x55[6050];
extern const unsigned char g_image_wifi_55x55[6050];
extern const unsigned char g_image_light_55x55[6050];
extern const unsigned char g_image_pulse_55x55[6050];
extern const unsigned char g_image_camera_55x55[6050];
extern const unsigned char g_image_faceid_55x55[6050];
extern const unsigned char g_image_battery_32x32[2048];
extern const unsigned char g_image_touchid_55x55[6050];
extern const unsigned char g_image_blueos_200x200[80000];
extern const unsigned char g_image_pic2_240x280[134400];

// 图像信息结构�?
typedef struct {
    const char* name;              // 图像名字，方便索引指定图像数�?
    const unsigned char* address;  // 图像数组入口地址
    unsigned int width;            // 图像宽度
    unsigned int height;           // 图像高度
    unsigned int size;             // 图像数组大小
} image_info_t;

static const image_info_t g_image_tbl[13] = {
    {"date", g_image_date_55x55, 55, 55, 6050 },//0
    {"humi", g_image_humi_55x55, 55, 55, 6050 },//1
    {"temp", g_image_temp_55x55, 55, 55, 6050 },//2
    {"time", g_image_time_55x55, 55, 55, 6050 },//3
    {"wifi", g_image_wifi_55x55, 55, 55, 6050 },//4
    {"light", g_image_light_55x55, 55, 55, 6050 },//5
    {"pulse", g_image_pulse_55x55, 55, 55, 6050 },//6
    {"camera", g_image_camera_55x55, 55, 55, 6050 },//7
    {"faceid", g_image_faceid_55x55, 55, 55, 6050 },//8
    {"battery", g_image_battery_32x32, 32, 32, 512 },//9
    {"touchid", g_image_touchid_55x55, 55, 55, 6050 },//10
	{"blueos", g_image_blueos_200x200, 200, 200, 80000 },//11
    {"pic2", g_image_pic2_240x280, 240, 280, 134400 },//12
};

#endif //__MYIMAGE_H__
