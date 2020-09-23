#include <jni.h>
#include <string>
#include "opencv2/opencv.hpp"
#include <iostream>
#include <vector>
#include <cmath>
#include <sstream>
#include "android/bitmap.h"
#include "android/log.h"

#define TAG "jni -> WORK" // 这个是自定义的LOG的标识
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG,TAG ,__VA_ARGS__) // 定义LOGD类型
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,TAG ,__VA_ARGS__) // 定义LOGI类型
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,TAG ,__VA_ARGS__) // 定义LOGW类型
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,TAG ,__VA_ARGS__) // 定义LOGE类型
#define LOGF(...) __android_log_print(ANDROID_LOG_FATAL,TAG ,__VA_ARGS__) // 定义LOGF类型


std::string jstring2str(JNIEnv *env, jstring jstr);
void MatToBitmap2(JNIEnv * env, cv::Mat, jobject bitmap, jboolean needPremultiplyAlpha);
jobject create_bitmap(JNIEnv *env, int width, int height);


extern "C"
JNIEXPORT jobjectArray JNICALL
Java_com_example_work_MainActivity_clipImage(JNIEnv *env, jobject thiz, jstring path,
                                             jint left,jint top, jint right, jint bottom) {
    // 第一部分，读取并剪裁图片
    LOGD("-1");
    cv::Mat rgb = cv::imread(jstring2str(env, path));    // 读入图片
    LOGD("-2");
    if (rgb.empty()) {
        LOGD("-3");
        std::cout << "读入图片出错！" << std::endl;
        return nullptr;
    }
    LOGD("-4");
    float resize_scaler = 1;    // resize尺度，命令行第二个参数，根据图片调节
    int width = rgb.cols * resize_scaler;    // resize之后图片高度
    int height = rgb.rows * resize_scaler;    // resize之后图片宽度
    cv::Mat resize_rgb;    // 存储resize之后的图片
    cv::resize(rgb, resize_rgb, cv::Size(width, height));    // resize

    LOGD("-5");
    cv::Point point1(left, top), point2(right, bottom);

    LOGD("-6");
    cv::Mat crop_img = resize_rgb(cv::Rect(point1, point2));    // 裁剪后的图片
    LOGD("-7");
    std::vector<cv::Mat> channel;
    cv::split(crop_img, channel);
    cv::Mat red_image = channel.at(2);    // 提取红色通道

    cv::Mat resize_red_image;
    LOGD("-8");
    cv::resize(red_image, resize_red_image, cv::Size(512, 512));    // 图片大小调整至512*512

    LOGD("-9");
    // 第二部分，对图片进行边缘提取并检测直线
    cv::Mat BW, blur_image;
    cv::GaussianBlur(resize_red_image, blur_image, cv::Size(1, 1), 0, 0, cv::BORDER_DEFAULT);
    cv::Canny(blur_image, BW, 160, 220, 3, false);    // 灰度+边缘检测
    for (int i = 0; i < BW.rows; ++i) {
        for (int j = 0; j < BW.cols; ++j) {
            if (sqrt(pow(i - 256, 2) + pow(j - 256, 2)) > 192)
                BW.at<uchar>(i, j) = 0;    // 过滤表盘边缘像素
        }
    }

    LOGD("-10 start");

    // 第三部分：检测并画出直线
    std::vector<cv::Vec4f> lines;    // 检测到的直线起点和终点
    cv::HoughLinesP(BW, lines, 1, 0.1 * CV_PI / 180, 100, 70, 20);
    int max_len_id = 0;    // 最长的直线对应的下标
    float max_len = 0;    // 最长的直线长度
    for (int i = 0; i < lines.size(); i++) {
        float len = sqrt(pow(lines[i][0] - lines[i][2], 2) + pow(lines[i][1] - lines[i][3], 2));    // 直线长度
        if (len > max_len) {
            max_len_id = i;
            max_len = len;
        }
    }
    LOGD("-10 end");
    if (max_len == 0 || max_len == -1) {
        LOGD("-10 return");
        jclass bitmapCls = env->FindClass("android/graphics/Bitmap");
        return env->NewObjectArray(1, bitmapCls, nullptr);
    }
    LOGD("-11 max_len_id=%d max_len=%d", max_len_id, max_len);
    cv::Mat line_image;    // 画出检测到的直线的图
    LOGD("-11.1");
    std::vector<cv::Mat> red_image_3;    // 为了在直线上用彩色画出直线，需要把原来单通道的灰度图变成三通道的图
    LOGD("-11.2");
//    red_image_3.reserve(3);
    LOGD("-11 - for start");
    for (int i = 0; i < 3; ++i) {
        LOGD("-11 ding--");
        red_image_3.push_back(resize_red_image);
        LOGD("-11 dong~~");
    }
    LOGD("-11 - for end");
    cv::merge(red_image_3, line_image);    // 单通道图变成三通道图
    LOGD("-12 - 1");
    cv::line(line_image, cv::Point(lines[max_len_id][0], lines[max_len_id][1]),
         cv::Point(lines[max_len_id][2], lines[max_len_id][3]), cv::Scalar(255, 0, 0), 2);    // 画出直线
    LOGD("-12 - 2");
    // 第四部分：计算直线与图像中心的夹角并显示出来
    int large_id = sqrt(pow(lines[max_len_id][0] - 256, 2) + pow(lines[max_len_id][1] - 256, 2)) >
                   sqrt(pow(lines[max_len_id][2] - 256, 2) + pow(lines[max_len_id][3] - 256, 2)) ?
                   0 : 1;    // 离图像中心最远的点
    double xa = lines[max_len_id][large_id * 2], ya = lines[max_len_id][large_id * 2 + 1];    // 离图像中心最远的点坐标
    double xb = 256, yb = 256, xc = 256, yc = 256 + 50;    // (xb, yb): 图像中心, (xc, yc): 图像中心下方的点
    std::cout << "A: (" << xa << ", " << ya << ")" << std::endl;
    std::cout << "B: (" << xb << ", " << yb << ")" << std::endl;
    std::cout << "B: (" << xc << ", " << yc << ")" << std::endl;
    double a = sqrt(pow(xc - xb, 2) + pow(yc - yb, 2));    // B, C两点距离
    double b = sqrt(pow(xa - xc, 2) + pow(ya - yc, 2));    // A, C两点距离
    double c = sqrt(pow(xa - xb, 2) + pow(ya - yb, 2));    // A, B两点距离
    double cosB = (pow(a, 2) + pow(c, 2) - pow(b, 2)) / (2 * a*c);    // 角B的余弦值
    double angle_b = acos(cosB) * 180 / 3.14159;    // 角B的度数
    if (xa > xb) angle_b = 360 - angle_b;
    std::cout << "angle: " << angle_b << std::endl;
    int angle_b_int = angle_b + 0.5;
    std::stringstream ss;
    ss << angle_b_int;
    std::string angle_b_string;
    ss >> angle_b_string;
    LOGD("-13");
    cv::putText(line_image, angle_b_string, cv::Point(20, 20), cv::FONT_HERSHEY_COMPLEX, 0.7, cv::Scalar(0, 0, 255), 2, 8);    // 把角度显示在图片上
    LOGD("-14 width=%d heigh=%d", width, height);
    jobject gray_image = create_bitmap(env, line_image.cols, line_image.rows);
    LOGD("-15");
    jobject bw_image = create_bitmap(env, BW.cols, BW.rows);
    LOGD("1");
    MatToBitmap2(env, line_image, gray_image, true);
    LOGD("2");
    MatToBitmap2(env, BW, bw_image, true);
    LOGD("3");
    jclass bitmapCls = env->FindClass("android/graphics/Bitmap");
    LOGD("4");
    jobjectArray result = env->NewObjectArray(2, bitmapCls, nullptr);
    LOGD("5");
    env->SetObjectArrayElement(result, 0, gray_image);
    LOGD("6");
    env->SetObjectArrayElement(result, 1, bw_image);
    return result;
}

void MatToBitmap2(JNIEnv * env, cv::Mat mat, jobject bitmap, jboolean needPremultiplyAlpha) {
    AndroidBitmapInfo  info;
    void *pixels = 0;
    cv::Mat &src = mat;

    try {
        LOGD("nMatToBitmap");
        CV_Assert( AndroidBitmap_getInfo(env, bitmap, &info) >= 0 );
        CV_Assert( info.format == ANDROID_BITMAP_FORMAT_RGBA_8888 ||
                   info.format == ANDROID_BITMAP_FORMAT_RGB_565 );
        CV_Assert( src.dims == 2 && info.height == (uint32_t)src.rows && info.width == (uint32_t)src.cols );
        CV_Assert( src.type() == CV_8UC1 || src.type() == CV_8UC3 || src.type() == CV_8UC4 );
        CV_Assert( AndroidBitmap_lockPixels(env, bitmap, &pixels) >= 0 );
        CV_Assert( pixels );
        if( info.format == ANDROID_BITMAP_FORMAT_RGBA_8888 )
        {
            cv::Mat tmp(info.height, info.width, CV_8UC4, pixels);
            if(src.type() == CV_8UC1)
            {
                LOGD("nMatToBitmap: CV_8UC1 -> RGBA_8888");
                cvtColor(src, tmp, cv::COLOR_GRAY2RGBA);
            } else if(src.type() == CV_8UC3){
                LOGD("nMatToBitmap: CV_8UC3 -> RGBA_8888");
                cvtColor(src, tmp, cv::COLOR_RGB2RGBA);
            } else if(src.type() == CV_8UC4){
                LOGD("nMatToBitmap: CV_8UC4 -> RGBA_8888");
                if(needPremultiplyAlpha) cvtColor(src, tmp, cv::COLOR_RGBA2mRGBA);
                else src.copyTo(tmp);
            }
        } else {
            // info.format == ANDROID_BITMAP_FORMAT_RGB_565
            cv::Mat tmp(info.height, info.width, CV_8UC2, pixels);
            if(src.type() == CV_8UC1)
            {
                LOGD("nMatToBitmap: CV_8UC1 -> RGB_565");
                cvtColor(src, tmp, cv::COLOR_GRAY2BGR565);
            } else if(src.type() == CV_8UC3){
                LOGD("nMatToBitmap: CV_8UC3 -> RGB_565");
                cvtColor(src, tmp, cv::COLOR_RGB2BGR565);
            } else if(src.type() == CV_8UC4){
                LOGD("nMatToBitmap: CV_8UC4 -> RGB_565");
                cvtColor(src, tmp, cv::COLOR_RGBA2BGR565);
            }
        }
        AndroidBitmap_unlockPixels(env, bitmap);
        return;
    } catch(const cv::Exception& e) {
        AndroidBitmap_unlockPixels(env, bitmap);
        LOGE("nMatToBitmap caught cv::Exception: %s", e.what());
        jclass je = env->FindClass("java/lang/Exception");
        env->ThrowNew(je, e.what());
        return;
    } catch (...) {
        AndroidBitmap_unlockPixels(env, bitmap);
        LOGE("nMatToBitmap caught unknown exception (...)");
        jclass je = env->FindClass("java/lang/Exception");
        env->ThrowNew(je, "Unknown exception in JNI code {nMatToBitmap}");
        return;
    }
}

jobject create_bitmap(JNIEnv *env, int width, int height) {
    //Bitmap.Config config = Bitmap.Config.valueOf(String configName);
    LOGD("-14 -1");
//    jclass bitmapConfig = env->FindClass("android/graphics/Bitmap$Config");
//    jfieldID rgba8888FieldID = env->GetStaticFieldID(bitmapConfig, "ARGB_8888",
//                                                     "Landroid/graphics/Bitmap$Config;");
//    jobject rgba8888Obj = env->GetStaticObjectField(bitmapConfig, rgba8888FieldID);
//    jclass bitmapClass = env->FindClass("android/graphics/Bitmap");
//    jmethodID createBitmapMethodID = env->GetStaticMethodID(bitmapClass,"createBitmap", "(IILandroid/graphics/Bitmap$Config;)Landroid/graphics/Bitmap;");
//    jobject newBitmap = env->CallStaticObjectMethod(bitmapClass, createBitmapMethodID, width, height, rgba8888Obj);
    jstring configName = env->NewStringUTF("ARGB_8888");
    LOGD("-14 -2");
    jclass bitmapConfigClass = env->FindClass("android/graphics/Bitmap$Config");
    LOGD("-14 -3");
    jmethodID valueOfBitmapConfigFunction = env->GetStaticMethodID(bitmapConfigClass, "valueOf",
                                                                   "(Ljava/lang/Class;Ljava/lang/String;)Ljava/lang/Enum;");
    LOGD("-14 -4");
    jobject bitmapConfig = env->CallStaticObjectMethod(bitmapConfigClass, valueOfBitmapConfigFunction,bitmapConfigClass, configName);
    LOGD("-14 -5");

    // Bitmap newBitmap = Bitmap.createBitmap(int width,int height,Bitmap.Config config);
    jclass bitmap = env->FindClass("android/graphics/Bitmap");
    LOGD("-14 -6");
    jmethodID createBitmapFunction = env->GetStaticMethodID(bitmap, "createBitmap", "(IILandroid/graphics/Bitmap$Config;)Landroid/graphics/Bitmap;");
    LOGD("-14 -7");
    jobject newBitmap = env->CallStaticObjectMethod(bitmap, createBitmapFunction, width, height, bitmapConfig);
    LOGD("-14 -8");

    return newBitmap;
}



std::string jstring2str(JNIEnv *env, jstring jstr) {
    char* rtn = nullptr;
    jclass clsstring = env->FindClass("java/lang/String");
    jstring strencode = env->NewStringUTF("GB2312");
    jmethodID mid = env->GetMethodID(clsstring, "getBytes", "(Ljava/lang/String;)[B");
    jbyteArray barr = (jbyteArray) env->CallObjectMethod(jstr, mid, strencode);
    jsize alen = env->GetArrayLength(barr);
    jbyte *ba = env->GetByteArrayElements(barr, JNI_FALSE);
    if (alen > 0) {
        rtn = (char *) malloc(alen + 1);
        memcpy(rtn, ba, alen);
        rtn[alen] = 0;
    }
    env->ReleaseByteArrayElements(barr, ba, 0);
    std::string stemp(rtn);
    free(rtn);
    return stemp;
}