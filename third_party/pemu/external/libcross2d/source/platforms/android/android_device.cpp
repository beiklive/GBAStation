//
// Created by cpasjuste on 04/02/2021.
//

#include <jni.h>
#include "cross2d/c2d.h"

using namespace c2d;

std::string ANDROIDDevice::getName() {
    // get device name from java
    std::string name;
    auto env = (JNIEnv *) SDL_AndroidGetJNIEnv();
    auto activity = (jobject) SDL_AndroidGetActivity();
    jclass clazz(env->GetObjectClass(activity));

    static jmethodID midGetDeviceName = env->GetStaticMethodID(
            clazz, "getDeviceName", "()Ljava/lang/String;");

    auto string = (jstring) env->CallStaticObjectMethod(clazz, midGetDeviceName);
    if (string) {
        const char *utf = env->GetStringUTFChars(string, nullptr);
        if (utf) {
            name = utf;
            env->ReleaseStringUTFChars(string, utf);
        }
        env->DeleteLocalRef(string);
    }

    env->DeleteLocalRef(activity);
    env->DeleteLocalRef(clazz);

    printf("ANDROIDDevice::getName: %s\n", name.c_str());

    return name;
}
