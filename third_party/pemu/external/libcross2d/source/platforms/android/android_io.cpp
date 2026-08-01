//
// Created by cpasjuste on 07/10/2022.
//

#include <SDL.h>
#include <jni.h>
#include "cross2d/c2d.h"

c2d::AndroidIo::AndroidIo() : POSIXIo() {
    // get romfs and data paths from java
    auto env = (JNIEnv *) SDL_AndroidGetJNIEnv();
    auto activity = (jobject) SDL_AndroidGetActivity();
    jclass clazz(env->GetObjectClass(activity));

    static jmethodID midGetDataPath = env->GetStaticMethodID(
            clazz, "getDataPath", "()Ljava/lang/String;");
    static jmethodID midGetRomFsPath = env->GetStaticMethodID(
            clazz, "getRomFsPath", "()Ljava/lang/String;");

    auto dataPath = (jstring) env->CallStaticObjectMethod(clazz, midGetDataPath);
    if (dataPath) {
        const char *utf = env->GetStringUTFChars(dataPath, nullptr);
        if (utf) {
            m_dataPath = utf;
            m_dataPath += "/";
            env->ReleaseStringUTFChars(dataPath, utf);
        }
        env->DeleteLocalRef(dataPath);
    } else {
        m_dataPath = std::string(SDL_AndroidGetInternalStoragePath()) + "/data/";
    }

    auto romFsPath = (jstring) env->CallStaticObjectMethod(clazz, midGetRomFsPath);
    if (romFsPath) {
        const char *utf = env->GetStringUTFChars(romFsPath, nullptr);
        if (utf) {
            m_romFsPath = utf;
            m_romFsPath += "/";
            env->ReleaseStringUTFChars(romFsPath, utf);
        }
        env->DeleteLocalRef(romFsPath);
    } else {
        m_romFsPath = std::string(SDL_AndroidGetInternalStoragePath()) + "/romfs/";
    }

    env->DeleteLocalRef(activity);
    env->DeleteLocalRef(clazz);

    printf("AndroidIo::AndroidIo(): data path: %s\n", m_dataPath.c_str());
    printf("AndroidIo::AndroidIo(): romfs path: %s\n", m_romFsPath.c_str());
}

std::string c2d::AndroidIo::getDataPath() {
    return m_dataPath;
}

std::string c2d::AndroidIo::getRomFsPath() {
    return m_romFsPath;
}
