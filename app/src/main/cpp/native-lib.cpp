//
// Created by WangLong on 2019/9/8.
//

#include<jni.h>
#include<stdio.h>
#include <vector>
#include<string.h>
#include <android/log.h>//log输出引入头文件

#include "utils.h"

#include <android/bitmap.h>
#include <opencv2/imgproc/types_c.h>
#include "opencv2/opencv.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"

#define LOG_TAG "FunctionControlJNI"
#define  LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

#define DEFAULT_CARD_WIDTH 800
#define DEFAULT_CARD_HEIGHT 495
#define FIX_CARD_SIZE Size(DEFAULT_CARD_WIDTH, DEFAULT_CARD_HEIGHT)

static const char* const kClassPathName = "soft/wl/function/FunctionControl";

jobject soft_wl_sendCommand(JNIEnv* env, jobject clazz, jint idCmd, jobject request, jobject reply);

void MatToBitmap(JNIEnv *env, Mat& mat, jobject& bitmap);
void BitmapToMat(JNIEnv *env, jobject& bitmap, Mat& mat);
jobject createBitmap(JNIEnv *env, Mat srcData, jobject config);

static const JNINativeMethod method_table[] = {
    {
        "sendCommand",                    "(ILjava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;",
        (void *)soft_wl_sendCommand
    },
};

extern "C" jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    LOGD("JNI_OnLoad");
    JNIEnv* env = NULL;
    jint result = -1;

    if (vm->GetEnv((void**) &env, JNI_VERSION_1_6) != JNI_OK) {
       return result;
    }

    jclass clazz;
    clazz = env->FindClass(kClassPathName);
    if(NULL == clazz) {
        LOGD("JNITest_sendCommand clazz == null");
        return result;
    }
    if (env->RegisterNatives(clazz, method_table, sizeof(method_table) / sizeof(method_table[0])) < 0) {
        LOGD("JNITest RegisterNatives fail");
        return result;
    }
    return JNI_VERSION_1_6;
}

jobject soft_wl_sendCommand(JNIEnv* env, jobject clazz, jint idCmd, jobject request, jobject reply)
{
    Mat src_img ;
    Mat dst_img ;
    BitmapToMat(env, request, src_img);//图片转化成mat

    int num_min_width = src_img.cols * 0.4;
    int num_min_height = src_img.rows  * 0.75;
    int erode_Scale = src_img.cols * 0.1;

    LOGD("num_min_width : %d  num_min_height : %d",num_min_width, num_min_height);

    Mat dst;//归一化
    resize(src_img, dst, FIX_CARD_SIZE);
    //灰度化
    cvtColor(src_img, dst, COLOR_RGB2GRAY);
    //二值化
    threshold(dst, dst, 100, 255, THRESH_BINARY);
    //膨胀处理Size(40, 1)， 40 代表X方向， 1 代表Y方向
    Mat erodeElement = getStructuringElement(MORPH_RECT, Size(erode_Scale, 1));
    erode(dst, dst, erodeElement);
    //轮廓检测
    std::vector<Rect> rects;
    std::vector<std::vector<Point>> contours;
    findContours(dst, contours, RETR_TREE, CHAIN_APPROX_SIMPLE, Point(0, 0));

    LOGD("findContours size : %lu", contours.size());

    //轮廓逻辑处理
    for (int i = 0; i < contours.size(); ++i) {
        Rect rect = boundingRect(contours.at(i));
       //rectangle(dst, rect, Scalar(0, 0, 255));
        //规则判断
        if(rect.width > rect.height * 11 && rect.width < rect.height * 22) {
            rects.push_back(rect);
        }
        LOGD("rect width : %d height : %d  X : %d Y : %d", rect.width, rect.height, rect.x, rect.y);
    }

    LOGD("rects size : %lu", rects.size());

    //获取最终数据
    int lowPoint = 0;
    Rect finalRect;
    for (int i = 0; i < rects.size(); ++i) {
        Rect rect = rects.at(i);
        Point point = rect.tl();
        if(point.y > lowPoint && rect.width > num_min_width
            && rect.y > num_min_height) {
            lowPoint = point.y;
            finalRect = rect;
        }
    }

    LOGD("finalRect width : %d  height : %d ", finalRect.width, finalRect.height);
    LOGD("finalRect X : %d  Y : %d ", finalRect.x, finalRect.y);
    //裁剪
    dst_img = src_img(finalRect);

    //Mat 转 Bitmap
    jobject bitmap = createBitmap(env, dst_img, reply);

    return bitmap;
}

void BitmapToMat2(JNIEnv *env, jobject& bitmap, Mat& mat, jboolean needUnPremultiplyAlpha) {
    AndroidBitmapInfo info;
    void *pixels = 0;
    Mat &dst = mat;

    try {
        LOGD("nBitmapToMat");
        CV_Assert(AndroidBitmap_getInfo(env, bitmap, &info) >= 0);
        CV_Assert(info.format == ANDROID_BITMAP_FORMAT_RGBA_8888 ||
                  info.format == ANDROID_BITMAP_FORMAT_RGB_565);
        CV_Assert(AndroidBitmap_lockPixels(env, bitmap, &pixels) >= 0);
        CV_Assert(pixels);
        dst.create(info.height, info.width, CV_8UC4);
        if (info.format == ANDROID_BITMAP_FORMAT_RGBA_8888) {
            LOGD("nBitmapToMat: RGBA_8888 -> CV_8UC4");
            Mat tmp(info.height, info.width, CV_8UC4, pixels);
            if (needUnPremultiplyAlpha) cvtColor(tmp, dst, COLOR_mRGBA2RGBA);
            else tmp.copyTo(dst);
        } else {
            // info.format == ANDROID_BITMAP_FORMAT_RGB_565
            LOGD("nBitmapToMat: RGB_565 -> CV_8UC4");
            Mat tmp(info.height, info.width, CV_8UC2, pixels);
            cvtColor(tmp, dst, COLOR_BGR5652RGBA);
        }
        AndroidBitmap_unlockPixels(env, bitmap);
        return;
    } catch (const cv::Exception &e) {
        AndroidBitmap_unlockPixels(env, bitmap);
        LOGD("nBitmapToMat catched cv::Exception: %s", e.what());
        jclass je = env->FindClass("org/opencv/core/CvException");
        if (!je) je = env->FindClass("java/lang/Exception");
        env->ThrowNew(je, e.what());
        return;
    } catch (...) {
        AndroidBitmap_unlockPixels(env, bitmap);
        LOGD("nBitmapToMat catched unknown exception (...)");
        jclass je = env->FindClass("java/lang/Exception");
        env->ThrowNew(je, "Unknown exception in JNI code {nBitmapToMat}");
        return;
    }
}

void BitmapToMat(JNIEnv *env, jobject& bitmap, Mat& mat) {
    BitmapToMat2(env, bitmap, mat, false);
}

void MatToBitmap2(JNIEnv *env, Mat& mat, jobject& bitmap, jboolean needPremultiplyAlpha) {
    AndroidBitmapInfo info;
    void *pixels = 0;
    Mat &src = mat;

    try {
        LOGD("nMatToBitmap");
        CV_Assert(AndroidBitmap_getInfo(env, bitmap, &info) >= 0);
        CV_Assert(info.format == ANDROID_BITMAP_FORMAT_RGBA_8888 ||
                  info.format == ANDROID_BITMAP_FORMAT_RGB_565);
        CV_Assert(src.dims == 2 && info.height == (uint32_t) src.rows &&
                  info.width == (uint32_t) src.cols);
        CV_Assert(src.type() == CV_8UC1 || src.type() == CV_8UC3 || src.type() == CV_8UC4);
        CV_Assert(AndroidBitmap_lockPixels(env, bitmap, &pixels) >= 0);
        CV_Assert(pixels);
        if (info.format == ANDROID_BITMAP_FORMAT_RGBA_8888) {
            LOGD("MatToBitmap2 ANDROID_BITMAP_FORMAT_RGBA_8888");
            Mat tmp(info.height, info.width, CV_8UC4, pixels);
            if (src.type() == CV_8UC1) {
                LOGD("nMatToBitmap: CV_8UC1 -> RGBA_8888");
                cvtColor(src, tmp, COLOR_GRAY2RGBA);
            } else if (src.type() == CV_8UC3) {
                LOGD("nMatToBitmap: CV_8UC3 -> RGBA_8888");
                cvtColor(src, tmp, COLOR_RGB2RGBA);
            } else if (src.type() == CV_8UC4) {
                LOGD("nMatToBitmap: CV_8UC4 -> RGBA_8888");
                if (needPremultiplyAlpha)
                    cvtColor(src, tmp, COLOR_RGBA2mRGBA);
                else
                    src.copyTo(tmp);
            }
        } else {
            LOGD("MatToBitmap2 ANDROID_BITMAP_FORMAT_Other");
            Mat tmp(info.height, info.width, CV_8UC2, pixels);
            if (src.type() == CV_8UC1) {
                LOGD("nMatToBitmap: CV_8UC1 -> RGB_565");
                cvtColor(src, tmp, COLOR_GRAY2BGR565);
            } else if (src.type() == CV_8UC3) {
                LOGD("nMatToBitmap: CV_8UC3 -> RGB_565");
                cvtColor(src, tmp, COLOR_RGB2BGR565);
            } else if (src.type() == CV_8UC4) {
                LOGD("nMatToBitmap: CV_8UC4 -> RGB_565");
                cvtColor(src, tmp, COLOR_RGBA2BGR565);
            }
        }
        AndroidBitmap_unlockPixels(env, bitmap);
        return;
    } catch (const cv::Exception &e) {
        AndroidBitmap_unlockPixels(env, bitmap);
        LOGD("nMatToBitmap catched cv::Exception: %s", e.what());
        jclass je = env->FindClass("org/opencv/core/CvException");
        if (!je) je = env->FindClass("java/lang/Exception");
        env->ThrowNew(je, e.what());
        return;
    } catch (...) {
        AndroidBitmap_unlockPixels(env, bitmap);
        LOGD("nMatToBitmap catched unknown exception (...)");
        jclass je = env->FindClass("java/lang/Exception");
        env->ThrowNew(je, "Unknown exception in JNI code {nMatToBitmap}");
        return;
    }
}

void MatToBitmap(JNIEnv *env, Mat& mat, jobject& bitmap) {
    MatToBitmap2(env, mat, bitmap, false);
}

jobject createBitmap(JNIEnv *env, Mat srcData, jobject config) {
    jclass clazz = env->FindClass("android/graphics/Bitmap");
    jmethodID jmethodID1 = env->GetStaticMethodID(clazz, "createBitmap", "(IILandroid/graphics/Bitmap$Config;)Landroid/graphics/Bitmap;");
    jobject bitmap = env->CallStaticObjectMethod(clazz, jmethodID1, srcData.cols, srcData.rows, config);
    MatToBitmap(env, srcData, bitmap);
    return bitmap;
}