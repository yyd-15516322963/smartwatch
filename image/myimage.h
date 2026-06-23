#ifndef __MYIMAGE_H__
#define __MYIMAGE_H__

extern const unsigned char g_image_date_55x55[6050];
extern const unsigned char g_image_fail_55x55[6050];
extern const unsigned char g_image_humi_55x55[6050];
extern const unsigned char g_image_lock_55x55[6050];
extern const unsigned char g_image_rfid_55x55[6050];
extern const unsigned char g_image_temp_55x55[6050];
extern const unsigned char g_image_time_55x55[6050];
extern const unsigned char g_image_wifi_55x55[6050];
extern const unsigned char g_image_light_55x55[6050];
extern const unsigned char g_image_pulse_55x55[6050];
extern const unsigned char g_image_camera_55x55[6050];
extern const unsigned char g_image_faceid_55x55[6050];
extern const unsigned char g_image_unlock_55x55[6050];
extern const unsigned char g_image_battery_32x32[2048];
extern const unsigned char g_image_success_55x55[6050];
extern const unsigned char g_image_touchid_55x55[6050];
extern const unsigned char g_image_victory_55x55[6050];
extern const unsigned char g_image_warning_55x55[6050];
extern const unsigned char g_image_blueos_200x200[80000];

// 图像信息结构体
typedef struct {
    const char* name;              // 图像名字，方便索引指定图像数据
    const unsigned char* address;  // 图像数组入口地址
    unsigned int width;            // 图像宽度
    unsigned int height;           // 图像高度
    unsigned int size;             // 图像数组大小
} image_info_t;

static const image_info_t g_image_tbl[19] = {
    {"date", g_image_date_55x55, 55, 55, 6050 },//0
    {"fail", g_image_fail_55x55, 55, 55, 6050 },//1
    {"humi", g_image_humi_55x55, 55, 55, 6050 },//2
    {"lock", g_image_lock_55x55, 55, 55, 6050 },//3
    {"rfid", g_image_rfid_55x55, 55, 55, 6050 },//4
    {"temp", g_image_temp_55x55, 55, 55, 6050 },//5
    {"time", g_image_time_55x55, 55, 55, 6050 },//6
    {"wifi", g_image_wifi_55x55, 55, 55, 6050 },//7
    {"light", g_image_light_55x55, 55, 55, 6050 },//8
    {"pulse", g_image_pulse_55x55, 55, 55, 6050 },//9
    {"camera", g_image_camera_55x55, 55, 55, 6050 },//10
    {"faceid", g_image_faceid_55x55, 55, 55, 6050 },//11
    {"unlock", g_image_unlock_55x55, 55, 55, 6050 },//12
    {"battery", g_image_battery_32x32, 32, 32, 512 },//13
    {"success", g_image_success_55x55, 55, 55, 6050 },//14
    {"touchid", g_image_touchid_55x55, 55, 55, 6050 },//15
    {"victory", g_image_victory_55x55, 55, 55, 6050 },//16
    {"warning", g_image_warning_55x55, 55, 55, 6050 },//17
	{"blueos", g_image_blueos_200x200, 200, 200, 80000 },//18
};

#endif //__MYIMAGE_H__
