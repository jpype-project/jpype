#!/bin/sh
export PY4A_jpype_DIR=`pwd`
p4a clean-recipe-build jpype --local-recipes native/buildozer/
time p4a apk --requirements=jpype \
    --ndk-dir=$HOME/.buildozer/android/platform/android-ndk-r19c \
    --sdk-dir=$HOME/.buildozer/android/platform/android-sdk \
    --dist-name=build_jpype --arch=armeabi-v7a \
    --debug \
    --name="jpype_test" \
    --version=0.1 \
    --local-recipes native/buildozer/
