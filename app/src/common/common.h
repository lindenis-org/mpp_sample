/************************************************************************************************/
/* Copyright (C), 2016-2017, Allwinner Tech. Co., Ltd.                                          */
/************************************************************************************************/
/**
 * @file common.h
 * @brief 为其它模块提供公共服务,如网络服务,时间服务,配置文件操作等,目前由于功能不多,暂时共于一个文件,
          当功能逐渐增多时,可以合并拆分成几个不同的目录结构.
 * @author id: wangguixing
 * @version v0.1
 * @date 2017-04-14
 * @version v0.2
 * @date 2017-05-8
 * @brief add osd create rgb function. wangguixing
 */

#ifndef _COMMON_H_
#define _COMMON_H_

/************************************************************************************************/
/*                                      Include Files                                           */
/************************************************************************************************/
/* None */


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

/************************************************************************************************/
/*                                     Macros & Typedefs                                        */
/************************************************************************************************/
#define MAX_COLOR_SIZE 8

#ifdef INFO_DEBUG
#define DB_PRT(fmt, args...)                                          \
    do {                                                                  \
        printf("[FUN]%s [LINE]%d  " fmt, __FUNCTION__, __LINE__, ##args); \
    } while (0)
#else
#define DB_PRT(fmt, args...)                                          \
    do {} while (0)
#endif

#ifdef ERR_DEBUG
#define ERR_PRT(fmt, args...)                                                                        \
    do {                                                                                                 \
        printf("\033[0;32;31m ERROR! [FUN]%s [LINE]%d  " fmt "\033[0m", __FUNCTION__, __LINE__, ##args); \
    } while (0)
#else
#define ERR_PRT(fmt, args...)                                          \
    do {} while (0)
#endif

typedef enum tag_OSD_RGB_TYPE_E {
    OSD_RGB_555 = 0,
    OSD_RGB_565,
    OSD_RGB_24,
    OSD_RGB_32,
    OSD_RGB_BUTT,
}
OSD_RGB_TYPE_E;

typedef enum tag_FONT_SIZE_TYPE_E {
    FONT_SIZE_16 = 0,
    FONT_SIZE_20,
    FONT_SIZE_24,
    FONT_SIZE_32,
    FONT_SIZE_40,
    FONT_SIZE_48,
    FONT_SIZE_56,
    FONT_SIZE_BUTT,
} FONT_SIZE_TYPE_E;

typedef enum tag_CHAR_CODE_TYPE_E {
    CHAR_CODE_ASCII = 0,
    CHAR_CODE_GB2312,
    CHAR_CODE_UTF8,
    CHAR_CODE_USC2_LIT, /* unicode 2 little endian */
    CHAR_CODE_USC2_BIG, /* unicode 2 big endian */
    CHAR_CODE_BUTT,
} CHAR_CODE_TYPE_E;


/************************************************************************************************/
/*                                    Structure Declarations                                    */
/************************************************************************************************/

typedef struct tag_RGB_PIC_S {
    unsigned int wide;
    unsigned int high;
    OSD_RGB_TYPE_E rgb_type;
    int enable_mosaic;
    int mosaic_size;
    unsigned char background[MAX_COLOR_SIZE]; /* support rgb555, rgb565, rgb24, rgb32 */
    unsigned char mosaic_color[MAX_COLOR_SIZE];

    char *pic_addr;
    int pic_size;
} RGB_PIC_S;

typedef struct tag_PIC_REGION_S {
    int x;
    int y;
    int w;
    int h;
    OSD_RGB_TYPE_E rgb_type;
} PIC_REGION_S;

typedef struct tag_FONT_RGBPIC_S {
    FONT_SIZE_TYPE_E font_type;
    OSD_RGB_TYPE_E   rgb_type;
    int enable_bg;
    unsigned char foreground[MAX_COLOR_SIZE];
    unsigned char background[MAX_COLOR_SIZE];
} FONT_RGBPIC_S;


/************************************************************************************************/
/*                                      Global Variables                                        */
/************************************************************************************************/
/* None */


/************************************************************************************************/
/*                                    Function Declarations                                     */
/************************************************************************************************/

int is_digit_char(char num);
int is_digit_str(char *str);


/**
 * @brief get ip addr from net interface .guixing
 * @param
 * - netdev_name   input
 * - ip                        output
 * @return
 *  - SUCCESS 0
 *  - FAIL   -1
 */
int get_net_dev_ip(const char *netdev_name, char *ip);

/**
 * @brief 加载字体
 * @param font_type 字体大小类型
 * @return
 *  - 成功 0
 *  - 失败 -1
 */
int load_gb2312_file(FONT_SIZE_TYPE_E font_type);

/**
 * @brief 卸载字体。调用该函数后，相关字体操作函数就不能被执行
 * @param
 * @return
 *  - 成功 0
 *  - 失败 -1
 */
int unload_gb2312_font(void);

/**
 * @brief 根据rgb_pic规格创建RGB矩形框
 * @param rgb_pic
 * - wide input 矩形的宽度
 * - high input 矩形的高度
 * - rgb_type      input  rgb的像素格式如，rgb555 rgb888 rgb8888等
 * - enable_mosaic input  是否使能背景马赛克功能
 * - mosaic_size   input  背景马赛克的宽高度
 * - background    input  矩形RGB图片的底色
 * - mosaic_color  input  背景马赛克的颜色
 * - pic_addr  output  创建成功后的图片地址，若失败则为NULL。使用完毕后记得
 * - pic_size  output  创建成功后的图片数据字节长度
 * @return
 *  - 成功 0
 *  - 失败 -1
 */
int create_rectangle_rgb(RGB_PIC_S *rgb_pic);

/**
 * @brief 根据rgb_pic规格创建RGB矩形框
 * @param font_pic
 * - font_type  input 字体大小类型
 * - rgb_type   input RGB格式
 * - enable_bg  input 是否使能背景色
 * - foreground input 字体颜色，即前景色
 * - background input 字体背景色
 * @param rgb_pic
 * - enable_mosaic input 是否使能背景马赛克功能
 * - mosaic_size   input  背景马赛克的宽高度
 * - mosaic_color  input  背景马赛克的颜色
 * - background  output  矩形RGB图片的底色，同font_pic中的background格式一样值
 * - rgb_type    output  rgb的像素格式，同font_pic中的rgb格式一样值
 * - pic_addr    output  创建成功后的图片地址，若失败则为NULL。使用完毕后记得
 * - pic_size    output  创建成功后的图片数据字节长度
 * - wide output 矩形的宽度
 * - high output 矩形的高度
 * @return
 *  - 成功 0
 *  - 失败 -1
 */
int create_font_rectangle(const char *code, const FONT_RGBPIC_S *font_pic, RGB_PIC_S *rgb_pic);

/**
 * @brief 释放rgb_pic图片
 * @param rgb_pic 待释放的rgb图片
 * @return
 *  - 成功 0
 *  - 失败 -1
 */
int release_rgb_picture(RGB_PIC_S *rgb_pic);

/**
 * @brief 将color填充满该图片
 * @param rgb_pic 待填充的RGB图片
 * @param color   所要填充的颜色。注意数据宽度为4字节及以上。
 * @return
 *  - 成功 0
 *  - 失败 -1
 */
int fill_rectangle_rgb(RGB_PIC_S *rgb_pic, const unsigned char *color);

/**
 * @brief 将src_pic图片，按位置信息叠加到dst_pic图片中
 * @param src_pic 源RGB图片
 * @param dst_pic 目标RGB图片
 * @param top     相对目标图片的顶部位置
 * @param left    相对目标图片的左边位置
 * @return
 *  - 成功 0
 *  - 失败 -1
 */
int draw_pic_in_pic(const RGB_PIC_S *src_pic, const RGB_PIC_S *dst_pic, int top, int left);

/**
 * @brief 将字体，按位置信息叠加到dst_pic图片中
 * @param code     字符编码序列
 * @param font_pic 字体RGB格式
 * @param dst_pic  目标RGB图片
 * @param top      相对目标图片的顶部位置
 * @param left     相对目标图片的左边位置
 * @return
 *  - 成功 0
 *  - 失败 -1
 */
int draw_font_in_pic(const char *code, const FONT_RGBPIC_S *font_pic, const RGB_PIC_S *dst_pic, int top, int left);


/**
 * @brief 在格式NV21的YUV图片中画矩形框。
 * @param pNV21      输入为NV21格式
 * @param width      NV21图片宽度
 * @param height     NV21图片高度
 * @param linewidth  矩形框的边宽度
 * @param sx         左上角点坐标X
 * @param sy         左上角点坐标Y
 * @param ex         右下角点坐标X
 * @param ey         右下角点坐标Y
 * @return
 *  - 成功 0
 *  - 失败 -1
 */
void draw_rectangle_nv21(unsigned char *pNV21_y, unsigned char *pNV21_vu, int width, int height, int linewidth, int sx, int sy, int ex, int ey);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* _COMMON_H_ */
