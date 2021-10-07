/*
 * ijkplayer_jni.c
 *
 * Copyright (c) 2013 Bilibili
 * Copyright (c) 2013 Zhang Rui <bbcallen@gmail.com>
 *
 * This file is part of ijkPlayer.
 *
 * ijkPlayer is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * ijkPlayer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with ijkPlayer; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <assert.h>
#include <string.h>
#include <pthread.h>
#include <jni.h>
#include <unistd.h>
#include "j4a/class/java/util/ArrayList.h"
#include "j4a/class/android/os/Bundle.h"
#include "j4a/class/tv/danmaku/ijk/media/player/IjkMediaPlayer.h"
#include "j4a/class/tv/danmaku/ijk/media/player/misc/IMediaDataSource.h"
#include "j4a/class/tv/danmaku/ijk/media/player/misc/IAndroidIO.h"
#include "ijksdl/ijksdl_log.h"
#include "../ff_ffplay.h"
#include "ffmpeg_api_jni.h"
#include "ijkplayer_android_def.h"
#include "ijkplayer_android.h"
#include "ijksdl/android/ijksdl_android_jni.h"
#include "ijksdl/android/ijksdl_codec_android_mediadef.h"
#include "ijkavformat/ijkavformat.h"

#define JNI_MODULE_PACKAGE      "tv/danmaku/ijk/media/player"
#define JNI_CLASS_IJKPLAYER     "tv/danmaku/ijk/media/player/IjkMediaPlayer"
#define JNI_IJK_MEDIA_EXCEPTION "tv/danmaku/ijk/media/player/exceptions/IjkMediaException"

#define IJK_CHECK_MPRET_GOTO(retval, env, label) \
    JNI_CHECK_GOTO((retval != EIJK_INVALID_STATE), env, "java/lang/IllegalStateException", NULL, label); \
    JNI_CHECK_GOTO((retval != EIJK_OUT_OF_MEMORY), env, "java/lang/OutOfMemoryError", NULL, label); \
    JNI_CHECK_GOTO((retval == 0), env, JNI_IJK_MEDIA_EXCEPTION, NULL, label);

static JavaVM* g_jvm;


/**
 * http://www.cs.kent.edu/~ruttan/sysprog/lectures/multi-thread/pthread_mutex_init.html
 *
 * A mutex is a MUTual EXclusion device, and is useful for protecting shared data structures from concurrent modifications,
 * and implementing critical sections and monitors.
 *
 *
 */
typedef struct player_fields_t {
    pthread_mutex_t mutex;
    jclass clazz;
} player_fields_t;
static player_fields_t g_clazz;

static int inject_callback(void *opaque, int type, void *data, size_t data_size);
static bool mediacodec_select_callback(void *opaque, ijkmp_mediacodecinfo_context *mcc);

static IjkMediaPlayer *jni_get_media_player(JNIEnv* env, jobject thiz)
{
    pthread_mutex_lock(&g_clazz.mutex);

    IjkMediaPlayer *mp = (IjkMediaPlayer *) (intptr_t) J4AC_IjkMediaPlayer__mNativeMediaPlayer__get__catchAll(env, thiz);
    if (mp) {
        ijkmp_inc_ref(mp);
    }

    pthread_mutex_unlock(&g_clazz.mutex);
    return mp;
}

static IjkMediaPlayer *jni_set_media_player(JNIEnv* env, jobject thiz, IjkMediaPlayer *mp)
{
    pthread_mutex_lock(&g_clazz.mutex);

    IjkMediaPlayer *old = (IjkMediaPlayer*) (intptr_t) J4AC_IjkMediaPlayer__mNativeMediaPlayer__get__catchAll(env, thiz);
    if (mp) {
        ijkmp_inc_ref(mp);
    }
    J4AC_IjkMediaPlayer__mNativeMediaPlayer__set__catchAll(env, thiz, (intptr_t) mp);

    pthread_mutex_unlock(&g_clazz.mutex);

    // NOTE: ijkmp_dec_ref may block thread
    if (old != NULL ) {
        ijkmp_dec_ref_p(&old);
    }

    return old;
}

static int64_t jni_set_media_data_source(JNIEnv* env, jobject thiz, jobject media_data_source)
{
    int64_t nativeMediaDataSource = 0;

    pthread_mutex_lock(&g_clazz.mutex);

    jobject old = (jobject) (intptr_t) J4AC_IjkMediaPlayer__mNativeMediaDataSource__get__catchAll(env, thiz);
    if (old) {
        J4AC_IMediaDataSource__close__catchAll(env, old);
        J4A_DeleteGlobalRef__p(env, &old);
        J4AC_IjkMediaPlayer__mNativeMediaDataSource__set__catchAll(env, thiz, 0);
    }

    if (media_data_source) {
        jobject global_media_data_source = (*env)->NewGlobalRef(env, media_data_source);
        if (J4A_ExceptionCheck__catchAll(env) || !global_media_data_source)
            goto fail;

        nativeMediaDataSource = (int64_t) (intptr_t) global_media_data_source;
        J4AC_IjkMediaPlayer__mNativeMediaDataSource__set__catchAll(env, thiz, (jlong) nativeMediaDataSource);
    }

fail:
    pthread_mutex_unlock(&g_clazz.mutex);
    return nativeMediaDataSource;
}

static int64_t jni_set_ijkio_androidio(JNIEnv* env, jobject thiz, jobject ijk_io)
{
    int64_t nativeAndroidIO = 0;

    pthread_mutex_lock(&g_clazz.mutex);

    jobject old = (jobject) (intptr_t) J4AC_IjkMediaPlayer__mNativeAndroidIO__get__catchAll(env, thiz);
    if (old) {
        J4AC_IAndroidIO__close__catchAll(env, old);
        J4A_DeleteGlobalRef__p(env, &old);
        J4AC_IjkMediaPlayer__mNativeAndroidIO__set__catchAll(env, thiz, 0);
    }

    if (ijk_io) {
        jobject global_ijkio_androidio = (*env)->NewGlobalRef(env, ijk_io);
        if (J4A_ExceptionCheck__catchAll(env) || !global_ijkio_androidio)
            goto fail;

        nativeAndroidIO = (int64_t) (intptr_t) global_ijkio_androidio;
        J4AC_IjkMediaPlayer__mNativeAndroidIO__set__catchAll(env, thiz, (jlong) nativeAndroidIO);
    }

fail:
    pthread_mutex_unlock(&g_clazz.mutex);
    return nativeAndroidIO;
}

static int message_loop(void *arg);

static void
IjkMediaPlayer_setDataSourceAndHeaders(
    JNIEnv *env, jobject thiz, jstring path,
    jobjectArray keys, jobjectArray values)
{
    MPTRACE("%s\n", __func__);
    int retval = 0;
    const char *c_path = NULL;
    IjkMediaPlayer *mp = jni_get_media_player(env, thiz);
    JNI_CHECK_GOTO(path, env, "java/lang/IllegalArgumentException", "mpjni: setDataSource: null path", LABEL_RETURN);
    JNI_CHECK_GOTO(mp, env, "java/lang/IllegalStateException", "mpjni: setDataSource: null mp", LABEL_RETURN);

    c_path = (*env)->GetStringUTFChars(env, path, NULL );
    JNI_CHECK_GOTO(c_path, env, "java/lang/OutOfMemoryError", "mpjni: setDataSource: path.string oom", LABEL_RETURN);

    ALOGV("setDataSource: path %s", c_path);
    retval = ijkmp_set_data_source(mp, c_path);
    (*env)->ReleaseStringUTFChars(env, path, c_path);

    IJK_CHECK_MPRET_GOTO(retval, env, LABEL_RETURN);

LABEL_RETURN:
    ijkmp_dec_ref_p(&mp);
}

static void
IjkMediaPlayer_setDataSourceFd(JNIEnv *env, jobject thiz, jint fd)
{
    MPTRACE("%s\n", __func__);
    int retval = 0;
    int dupFd = 0;
    char uri[128];
    IjkMediaPlayer *mp = jni_get_media_player(env, thiz);
    JNI_CHECK_GOTO(fd > 0, env, "java/lang/IllegalArgumentException", "mpjni: setDataSourceFd: null fd", LABEL_RETURN);
    JNI_CHECK_GOTO(mp, env, "java/lang/IllegalStateException", "mpjni: setDataSourceFd: null mp", LABEL_RETURN);

    dupFd = dup(fd);

    ALOGV("setDataSourceFd: dup(%d)=%d\n", fd, dupFd);
    snprintf(uri, sizeof(uri), "pipe:%d", dupFd);
    retval = ijkmp_set_data_source(mp, uri);

    IJK_CHECK_MPRET_GOTO(retval, env, LABEL_RETURN);

LABEL_RETURN:
    ijkmp_dec_ref_p(&mp);
}

static void
IjkMediaPlayer_setDataSourceCallback(JNIEnv *env, jobject thiz, jobject callback)
{
    MPTRACE("%s\n", __func__);
    int retval = 0;
    char uri[128];
    int64_t nativeMediaDataSource = 0;
    IjkMediaPlayer *mp = jni_get_media_player(env, thiz);
    JNI_CHECK_GOTO(callback, env, "java/lang/IllegalArgumentException", "mpjni: setDataSourceCallback: null fd", LABEL_RETURN);
    JNI_CHECK_GOTO(mp, env, "java/lang/IllegalStateException", "mpjni: setDataSourceCallback: null mp", LABEL_RETURN);

    nativeMediaDataSource = jni_set_media_data_source(env, thiz, callback);
    JNI_CHECK_GOTO(nativeMediaDataSource, env, "java/lang/IllegalStateException", "mpjni: jni_set_media_data_source: NewGlobalRef", LABEL_RETURN);

    ALOGV("setDataSourceCallback: %"PRId64"\n", nativeMediaDataSource);
    snprintf(uri, sizeof(uri), "ijkmediadatasource:%"PRId64, nativeMediaDataSource);

    retval = ijkmp_set_data_source(mp, uri);

    IJK_CHECK_MPRET_GOTO(retval, env, LABEL_RETURN);

LABEL_RETURN:
    ijkmp_dec_ref_p(&mp);
}

static void
IjkMediaPlayer_setAndroidIOCallback(JNIEnv *env, jobject thiz, jobject callback) {
    MPTRACE("%s\n", __func__);
    int64_t nativeAndroidIO = 0;

    IjkMediaPlayer *mp = jni_get_media_player(env, thiz);
    JNI_CHECK_GOTO(callback, env, "java/lang/IllegalArgumentException", "mpjni: setAndroidIOCallback: null fd", LABEL_RETURN);
    JNI_CHECK_GOTO(mp, env, "java/lang/IllegalStateException", "mpjni: setAndroidIOCallback: null mp", LABEL_RETURN);

    nativeAndroidIO = jni_set_ijkio_androidio(env, thiz, callback);
    JNI_CHECK_GOTO(nativeAndroidIO, env, "java/lang/IllegalStateException", "mpjni: jni_set_ijkio_androidio: NewGlobalRef", LABEL_RETURN);

    ijkmp_set_option_int(mp, FFP_OPT_CATEGORY_FORMAT, "androidio-inject-callback", nativeAndroidIO);

LABEL_RETURN:
    ijkmp_dec_ref_p(&mp);
}

static void
IjkMediaPlayer_setVideoSurface(JNIEnv *env, jobject thiz, jobject jsurface)
{
    MPTRACE("%s\n", __func__);
    IjkMediaPlayer *mp = jni_get_media_player(env, thiz);
    JNI_CHECK_GOTO(mp, env, NULL, "mpjni: setVideoSurface: null mp", LABEL_RETURN);

    ijkmp_android_set_surface(env, mp, jsurface);

LABEL_RETURN:
    ijkmp_dec_ref_p(&mp);
    return;
}

static void
IjkMediaPlayer_prepareAsync(JNIEnv *env, jobject thiz)
{
    MPTRACE("%s\n", __func__);
    int retval = 0;
    IjkMediaPlayer *mp = jni_get_media_player(env, thiz);
    JNI_CHECK_GOTO(mp, env, "java/lang/IllegalStateException", "mpjni: prepareAsync: null mp", LABEL_RETURN);

    retval = ijkmp_prepare_async(mp);
    IJK_CHECK_MPRET_GOTO(retval, env, LABEL_RETURN);

LABEL_RETURN:
    ijkmp_dec_ref_p(&mp);
}

static void
IjkMediaPlayer_start(JNIEnv *env, jobject thiz)
{
    MPTRACE("%s\n", __func__);
    IjkMediaPlayer *mp = jni_get_media_player(env, thiz);
    JNI_CHECK_GOTO(mp, env, "java/lang/IllegalStateException", "mpjni: start: null mp", LABEL_RETURN);

    ijkmp_start(mp);

LABEL_RETURN:
    ijkmp_dec_ref_p(&mp);
}

static void
IjkMediaPlayer_stop(JNIEnv *env, jobject thiz)
{
    IjkMediaPlayer *mp = jni_get_media_player(env, thiz);
    JNI_CHECK_GOTO(mp, env, "java/lang/IllegalStateException", "mpjni: stop: null mp", LABEL_RETURN);

    ijkmp_stop(mp);

LABEL_RETURN:
    ijkmp_dec_ref_p(&mp);
}

static void
IjkMediaPlayer_pause(JNIEnv *env, jobject thiz)
{
    IjkMediaPlayer *mp = jni_get_media_player(env, thiz);
    JNI_CHECK_GOTO(mp, env, "java/lang/IllegalStateException", "mpjni: pause: null mp", LABEL_RETURN);

    ijkmp_pause(mp);

LABEL_RETURN:
    ijkmp_dec_ref_p(&mp);
}

static void
IjkMediaPlayer_seekTo(JNIEnv *env, jobject thiz, jlong msec)
{
    MPTRACE("%s\n", __func__);
    IjkMediaPlayer *mp = jni_get_media_player(env, thiz);
    JNI_CHECK_GOTO(mp, env, "java/lang/IllegalStateException", "mpjni: seekTo: null mp", LABEL_RETURN);

    ijkmp_seek_to(mp, msec);

LABEL_RETURN:
    ijkmp_dec_ref_p(&mp);
}

static jboolean
IjkMediaPlayer_isPlaying(JNIEnv *env, jobject thiz)
{
    jboolean retval = JNI_FALSE;
    IjkMediaPlayer *mp = jni_get_media_player(env, thiz);
    JNI_CHECK_GOTO(mp, env, NULL, "mpjni: isPlaying: null mp", LABEL_RETURN);

    retval = ijkmp_is_playing(mp) ? JNI_TRUE : JNI_FALSE;

LABEL_RETURN:
    ijkmp_dec_ref_p(&mp);
    return retval;
}

static jlong
IjkMediaPlayer_getCurrentPosition(JNIEnv *env, jobject thiz)
{
    jlong retval = 0;
    IjkMediaPlayer *mp = jni_get_media_player(env, thiz);
    JNI_CHECK_GOTO(mp, env, NULL, "mpjni: getCurrentPosition: null mp", LABEL_RETURN);

    retval = ijkmp_get_current_position(mp);

LABEL_RETURN:
    ijkmp_dec_ref_p(&mp);
    return retval;
}

static jlong
IjkMediaPlayer_getDuration(JNIEnv *env, jobject thiz)
{
    jlong retval = 0;
    IjkMediaPlayer *mp = jni_get_media_player(env, thiz);
    JNI_CHECK_GOTO(mp, env, NULL, "mpjni: getDuration: null mp", LABEL_RETURN);

    retval = ijkmp_get_duration(mp);

LABEL_RETURN:
    ijkmp_dec_ref_p(&mp);
    return retval;
}

static void
IjkMediaPlayer_release(JNIEnv *env, jobject thiz)
{
    MPTRACE("%s\n", __func__);
    IjkMediaPlayer *mp = jni_get_media_player(env, thiz);
    if (!mp)
        return;

    ijkmp_android_set_surface(env, mp, NULL);
    // explicit shutdown mp, in case it is not the last mp-ref here
    ijkmp_shutdown(mp);
    //only delete weak_thiz at release
    jobject weak_thiz = (jobject) ijkmp_set_weak_thiz(mp, NULL );
    (*env)->DeleteGlobalRef(env, weak_thiz);
    jni_set_media_player(env, thiz, NULL);
    jni_set_media_data_source(env, thiz, NULL);

    ijkmp_dec_ref_p(&mp);
}

static void IjkMediaPlayer_native_setup(JNIEnv *env, jobject thiz, jobject weak_this);
static void
IjkMediaPlayer_reset(JNIEnv *env, jobject thiz)
{
    MPTRACE("%s\n", __func__);
    IjkMediaPlayer *mp = jni_get_media_player(env, thiz);
    if (!mp)
        return;

    jobject weak_thiz = (jobject) ijkmp_set_weak_thiz(mp, NULL );

    IjkMediaPlayer_release(env, thiz);
    IjkMediaPlayer_native_setup(env, thiz, weak_thiz);

    ijkmp_dec_ref_p(&mp);
}

static void
IjkMediaPlayer_setLoopCount(JNIEnv *env, jobject thiz, jint loop_count)
{
    MPTRACE("%s\n", __func__);
    IjkMediaPlayer *mp = jni_get_media_player(env, thiz);
    JNI_CHECK_GOTO(mp, env, NULL, "mpjni: setLoopCount: null mp", LABEL_RETURN);

    ijkmp_set_loop(mp, loop_count);

LABEL_RETURN:
    ijkmp_dec_ref_p(&mp);
}

static jint
IjkMediaPlayer_getLoopCount(JNIEnv *env, jobject thiz)
{
    jint loop_count = 1;
    MPTRACE("%s\n", __func__);
    IjkMediaPlayer *mp = jni_get_media_player(env, thiz);
    JNI_CHECK_GOTO(mp, env, NULL, "mpjni: getLoopCount: null mp", LABEL_RETURN);

    loop_count = ijkmp_get_loop(mp);

LABEL_RETURN:
    ijkmp_dec_ref_p(&mp);
    return loop_count;
}

static jfloat
ijkMediaPlayer_getPropertyFloat(JNIEnv *env, jobject thiz, jint id, jfloat default_value)
{
    jfloat value = default_value;
    IjkMediaPlayer *mp = jni_get_media_player(env, thiz);
    JNI_CHECK_GOTO(mp, env, NULL, "mpjni: getPropertyFloat: null mp", LABEL_RETURN);

    value = ijkmp_get_property_float(mp, id, default_value);

LABEL_RETURN:
    ijkmp_dec_ref_p(&mp);
    return value;
}

static void
ijkMediaPlayer_setPropertyFloat(JNIEnv *env, jobject thiz, jint id, jfloat value)
{
    IjkMediaPlayer *mp = jni_get_media_player(env, thiz);
    JNI_CHECK_GOTO(mp, env, NULL, "mpjni: setPropertyFloat: null mp", LABEL_RETURN);

    ijkmp_set_property_float(mp, id, value);

LABEL_RETURN:
    ijkmp_dec_ref_p(&mp);
    return;
}

static jlong
ijkMediaPlayer_getPropertyLong(JNIEnv *env, jobject thiz, jint id, jlong default_value)
{
    jlong value = default_value;
    IjkMediaPlayer *mp = jni_get_media_player(env, thiz);
    JNI_CHECK_GOTO(mp, env, NULL, "mpjni: getPropertyLong: null mp", LABEL_RETURN);

    value = ijkmp_get_property_int64(mp, id, default_value);

LABEL_RETURN:
    ijkmp_dec_ref_p(&mp);
    return value;
}

static void
ijkMediaPlayer_setPropertyLong(JNIEnv *env, jobject thiz, jint id, jlong value)
{
    IjkMediaPlayer *mp = jni_get_media_player(env, thiz);
    JNI_CHECK_GOTO(mp, env, NULL, "mpjni: setPropertyLong: null mp", LABEL_RETURN);

    ijkmp_set_property_int64(mp, id, value);

LABEL_RETURN:
    ijkmp_dec_ref_p(&mp);
    return;
}

static void
ijkMediaPlayer_setStreamSelected(JNIEnv *env, jobject thiz, jint stream, jboolean selected)
{
    IjkMediaPlayer *mp = jni_get_media_player(env, thiz);
    int ret = 0;
    JNI_CHECK_GOTO(mp, env, NULL, "mpjni: setStreamSelected: null mp", LABEL_RETURN);

    ret = ijkmp_set_stream_selected(mp, stream, selected);
    if (ret < 0) {
        ALOGE("failed to %s %d", selected ? "select" : "deselect", stream);
        goto LABEL_RETURN;
    }

LABEL_RETURN:
    ijkmp_dec_ref_p(&mp);
    return;
}

static void
IjkMediaPlayer_setVolume(JNIEnv *env, jobject thiz, jfloat leftVolume, jfloat rightVolume)
{
    MPTRACE("%s\n", __func__);
    IjkMediaPlayer *mp = jni_get_media_player(env, thiz);
    JNI_CHECK_GOTO(mp, env, NULL, "mpjni: setVolume: null mp", LABEL_RETURN);

    ijkmp_android_set_volume(env, mp, leftVolume, rightVolume);

LABEL_RETURN:
    ijkmp_dec_ref_p(&mp);
}

static jint
IjkMediaPlayer_getAudioSessionId(JNIEnv *env, jobject thiz)
{
    jint audio_session_id = 0;
    MPTRACE("%s\n", __func__);
    IjkMediaPlayer *mp = jni_get_media_player(env, thiz);
    JNI_CHECK_GOTO(mp, env, NULL, "mpjni: getAudioSessionId: null mp", LABEL_RETURN);

    audio_session_id = ijkmp_android_get_audio_session_id(env, mp);

LABEL_RETURN:
    ijkmp_dec_ref_p(&mp);
    return audio_session_id;
}

static void
IjkMediaPlayer_setOption(JNIEnv *env, jobject thiz, jint category, jobject name, jobject value)
{
    MPTRACE("%s\n", __func__);
    IjkMediaPlayer *mp = jni_get_media_player(env, thiz);
    const char *c_name = NULL;
    const char *c_value = NULL;
    JNI_CHECK_GOTO(mp, env, "java/lang/IllegalStateException", "mpjni: setOption: null mp", LABEL_RETURN);

    if (!name) {
        goto LABEL_RETURN;
    }

    c_name = (*env)->GetStringUTFChars(env, name, NULL );
    JNI_CHECK_GOTO(c_name, env, "java/lang/OutOfMemoryError", "mpjni: setOption: name.string oom", LABEL_RETURN);
    JNI_CHECK_GOTO(mp, env, NULL, "mpjni: IjkMediaPlayer_setOption: null name", LABEL_RETURN);

    if (value) {
        c_value = (*env)->GetStringUTFChars(env, value, NULL );
        JNI_CHECK_GOTO(c_name, env, "java/lang/OutOfMemoryError", "mpjni: setOption: name.string oom", LABEL_RETURN);
    }

    ijkmp_set_option(mp, category, c_name, c_value);

LABEL_RETURN:
    if (c_name)
        (*env)->ReleaseStringUTFChars(env, name, c_name);
    if (c_value)
        (*env)->ReleaseStringUTFChars(env, value, c_value);
    ijkmp_dec_ref_p(&mp);
}

static void
IjkMediaPlayer_setOptionLong(JNIEnv *env, jobject thiz, jint category, jobject name, jlong value)
{
    MPTRACE("%s\n", __func__);
    IjkMediaPlayer *mp = jni_get_media_player(env, thiz);
    const char *c_name = NULL;
    JNI_CHECK_GOTO(mp, env, "java/lang/IllegalStateException", "mpjni: setOptionLong: null mp", LABEL_RETURN);

    c_name = (*env)->GetStringUTFChars(env, name, NULL );
    JNI_CHECK_GOTO(c_name, env, "java/lang/OutOfMemoryError", "mpjni: setOptionLong: name.string oom", LABEL_RETURN);

    ijkmp_set_option_int(mp, category, c_name, value);

LABEL_RETURN:
    if (c_name)
        (*env)->ReleaseStringUTFChars(env, name, c_name);
    ijkmp_dec_ref_p(&mp);
}

static jstring
IjkMediaPlayer_getColorFormatName(JNIEnv *env, jclass clazz, jint mediaCodecColorFormat)
{
    const char *codec_name = SDL_AMediaCodec_getColorFormatName(mediaCodecColorFormat);
    if (!codec_name)
        return NULL ;

    return (*env)->NewStringUTF(env, codec_name);
}

static jstring
IjkMediaPlayer_getVideoCodecInfo(JNIEnv *env, jobject thiz)
{
    MPTRACE("%s\n", __func__);
    jstring jcodec_info = NULL;
    int ret = 0;
    char *codec_info = NULL;
    IjkMediaPlayer *mp = jni_get_media_player(env, thiz);
    JNI_CHECK_GOTO(mp, env, "java/lang/IllegalStateException", "mpjni: getVideoCodecInfo: null mp", LABEL_RETURN);

    ret = ijkmp_get_video_codec_info(mp, &codec_info);
    if (ret < 0 || !codec_info)
        goto LABEL_RETURN;

    jcodec_info = (*env)->NewStringUTF(env, codec_info);
LABEL_RETURN:
    if (codec_info)
        free(codec_info);

    ijkmp_dec_ref_p(&mp);
    return jcodec_info;
}

static jstring
IjkMediaPlayer_getAudioCodecInfo(JNIEnv *env, jobject thiz)
{
    MPTRACE("%s\n", __func__);
    jstring jcodec_info = NULL;
    int ret = 0;
    char *codec_info = NULL;
    IjkMediaPlayer *mp = jni_get_media_player(env, thiz);
    JNI_CHECK_GOTO(mp, env, "java/lang/IllegalStateException", "mpjni: getAudioCodecInfo: null mp", LABEL_RETURN);

    ret = ijkmp_get_audio_codec_info(mp, &codec_info);
    if (ret < 0 || !codec_info)
        goto LABEL_RETURN;

    jcodec_info = (*env)->NewStringUTF(env, codec_info);
LABEL_RETURN:
    if (codec_info)
        free(codec_info);

    ijkmp_dec_ref_p(&mp);
    return jcodec_info;
}

inline static void fillMetaInternal(JNIEnv *env, jobject jbundle, IjkMediaMeta *meta, const char *key, const char *default_value)
{
    const char *value = ijkmeta_get_string_l(meta, key);
    if (value == NULL )
        value = default_value;

    J4AC_Bundle__putString__withCString__catchAll(env, jbundle, key, value);
}

static jobject
IjkMediaPlayer_getMediaMeta(JNIEnv *env, jobject thiz)
{
    MPTRACE("%s\n", __func__);
    bool is_locked = false;
    jobject jret_bundle = NULL;
    jobject jlocal_bundle = NULL;
    jobject jstream_bundle = NULL;
    jobject jarray_list = NULL;
    IjkMediaMeta *meta = NULL;
    IjkMediaPlayer *mp = jni_get_media_player(env, thiz);
    JNI_CHECK_GOTO(mp, env, "java/lang/IllegalStateException", "mpjni: getMediaMeta: null mp", LABEL_RETURN);

    meta = ijkmp_get_meta_l(mp);
    if (!meta)
        goto LABEL_RETURN;

    ijkmeta_lock(meta);
    is_locked = true;

    jlocal_bundle = J4AC_Bundle__Bundle(env);
    if (J4A_ExceptionCheck__throwAny(env)) {
        goto LABEL_RETURN;
    }

    fillMetaInternal(env, jlocal_bundle, meta, IJKM_KEY_FORMAT, NULL );
    fillMetaInternal(env, jlocal_bundle, meta, IJKM_KEY_DURATION_US, NULL );
    fillMetaInternal(env, jlocal_bundle, meta, IJKM_KEY_START_US, NULL );
    fillMetaInternal(env, jlocal_bundle, meta, IJKM_KEY_BITRATE, NULL );

    fillMetaInternal(env, jlocal_bundle, meta, IJKM_KEY_VIDEO_STREAM, "-1");
    fillMetaInternal(env, jlocal_bundle, meta, IJKM_KEY_AUDIO_STREAM, "-1");
    fillMetaInternal(env, jlocal_bundle, meta, IJKM_KEY_TIMEDTEXT_STREAM, "-1");

    jarray_list = J4AC_ArrayList__ArrayList(env);
    if (J4A_ExceptionCheck__throwAny(env)) {
        goto LABEL_RETURN;
    }

    size_t count = ijkmeta_get_children_count_l(meta);
    for (size_t i = 0; i < count; ++i) {
        IjkMediaMeta *streamRawMeta = ijkmeta_get_child_l(meta, i);
        if (streamRawMeta) {
            jstream_bundle = J4AC_Bundle__Bundle(env);
            if (J4A_ExceptionCheck__throwAny(env)) {
                goto LABEL_RETURN;
            }

            fillMetaInternal(env, jstream_bundle, streamRawMeta, IJKM_KEY_TYPE,     IJKM_VAL_TYPE__UNKNOWN);
            fillMetaInternal(env, jstream_bundle, streamRawMeta, IJKM_KEY_LANGUAGE, NULL);
            const char *type = ijkmeta_get_string_l(streamRawMeta, IJKM_KEY_TYPE);
            if (type) {
                fillMetaInternal(env, jstream_bundle, streamRawMeta, IJKM_KEY_CODEC_NAME, NULL );
                fillMetaInternal(env, jstream_bundle, streamRawMeta, IJKM_KEY_CODEC_PROFILE, NULL );
                fillMetaInternal(env, jstream_bundle, streamRawMeta, IJKM_KEY_CODEC_LEVEL, NULL );
                fillMetaInternal(env, jstream_bundle, streamRawMeta, IJKM_KEY_CODEC_LONG_NAME, NULL );
                fillMetaInternal(env, jstream_bundle, streamRawMeta, IJKM_KEY_CODEC_PIXEL_FORMAT, NULL );
                fillMetaInternal(env, jstream_bundle, streamRawMeta, IJKM_KEY_BITRATE, NULL );
                fillMetaInternal(env, jstream_bundle, streamRawMeta, IJKM_KEY_CODEC_PROFILE_ID, NULL );

                if (0 == strcmp(type, IJKM_VAL_TYPE__VIDEO)) {
                    fillMetaInternal(env, jstream_bundle, streamRawMeta, IJKM_KEY_WIDTH, NULL );
                    fillMetaInternal(env, jstream_bundle, streamRawMeta, IJKM_KEY_HEIGHT, NULL );
                    fillMetaInternal(env, jstream_bundle, streamRawMeta, IJKM_KEY_FPS_NUM, NULL );
                    fillMetaInternal(env, jstream_bundle, streamRawMeta, IJKM_KEY_FPS_DEN, NULL );
                    fillMetaInternal(env, jstream_bundle, streamRawMeta, IJKM_KEY_TBR_NUM, NULL );
                    fillMetaInternal(env, jstream_bundle, streamRawMeta, IJKM_KEY_TBR_DEN, NULL );
                    fillMetaInternal(env, jstream_bundle, streamRawMeta, IJKM_KEY_SAR_NUM, NULL );
                    fillMetaInternal(env, jstream_bundle, streamRawMeta, IJKM_KEY_SAR_DEN, NULL );
                } else if (0 == strcmp(type, IJKM_VAL_TYPE__AUDIO)) {
                    fillMetaInternal(env, jstream_bundle, streamRawMeta, IJKM_KEY_SAMPLE_RATE, NULL );
                    fillMetaInternal(env, jstream_bundle, streamRawMeta, IJKM_KEY_CHANNEL_LAYOUT, NULL );
                }
                J4AC_ArrayList__add(env, jarray_list, jstream_bundle);
                if (J4A_ExceptionCheck__throwAny(env)) {
                    goto LABEL_RETURN;
                }
            }

            SDL_JNI_DeleteLocalRefP(env, &jstream_bundle);
        }
    }

    J4AC_Bundle__putParcelableArrayList__withCString__catchAll(env, jlocal_bundle, IJKM_KEY_STREAMS, jarray_list);
    jret_bundle = jlocal_bundle;
    jlocal_bundle = NULL;
LABEL_RETURN:
    if (is_locked && meta)
        ijkmeta_unlock(meta);

    SDL_JNI_DeleteLocalRefP(env, &jstream_bundle);
    SDL_JNI_DeleteLocalRefP(env, &jlocal_bundle);
    SDL_JNI_DeleteLocalRefP(env, &jarray_list);

    ijkmp_dec_ref_p(&mp);
    return jret_bundle;
}

static void
IjkMediaPlayer_native_init(JNIEnv *env)
{

    /**
     * (C++11) The predefined identifier __func__ is implicitly defined as a string that contains the unqualified and unadorned name of
     * the enclosing function. __func__ is mandated by the C++ standard and is not a Microsoft extension.
     *
     * https://docs.microsoft.com/en-us/cpp/cpp/func?view=msvc-160
     */
    MPTRACE("---999---%s\n", __func__);
}

/**
 *
 * https://stackoverflow.com/questions/56533288/why-is-typedef-struct-x-x-allowed
 *
 ???
typedef struct point point;
struct point {
    int x;
    int y;
};

The line typedef struct point point; does two things:
It creates a forward declaration of struct point
It creates a type alias for struct point called point.

 */


static void
IjkMediaPlayer_native_setup(JNIEnv *env, jobject thiz, jobject weak_this)
{
    MPTRACE("666  %s\n", __func__);
    //创建IjkMediaPlayer，并传入message_loop()方法作为参数

    // message_loop这个函数作为参数被传递，在这里依然还没有调用，即没有开启循环读队列。
    // 猜想message_loop函数一定是被传递下去，在某个地方被调用了，并且需要在一个独立的线程中调用，
    // 就像模拟Android的HandlerThread类做的那样，一个独立的线程中开启了循环。
    IjkMediaPlayer *mp = ijkmp_android_create(message_loop);


    JNI_CHECK_GOTO(mp, env, "java/lang/OutOfMemoryError", "mpjni: native_setup: ijkmp_create() failed", LABEL_RETURN);

    jni_set_media_player(env, thiz, mp);
    ijkmp_set_weak_thiz(mp, (*env)->NewGlobalRef(env, weak_this));
    ijkmp_set_inject_opaque(mp, ijkmp_get_weak_thiz(mp));
    ijkmp_set_ijkio_inject_opaque(mp, ijkmp_get_weak_thiz(mp));
    ijkmp_android_set_mediacodec_select_callback(mp, mediacodec_select_callback, ijkmp_get_weak_thiz(mp));

LABEL_RETURN:
    ijkmp_dec_ref_p(&mp);
}

static void
IjkMediaPlayer_native_finalize(JNIEnv *env, jobject thiz, jobject name, jobject value)
{
    MPTRACE("%s\n", __func__);
    IjkMediaPlayer_release(env, thiz);
}

// NOTE: support to be called from read_thread
static int
inject_callback(void *opaque, int what, void *data, size_t data_size)
{
    JNIEnv     *env     = NULL;
    jobject     jbundle = NULL;
    int         ret     = -1;
    SDL_JNI_SetupThreadEnv(&env);

    jobject weak_thiz = (jobject) opaque;
    if (weak_thiz == NULL )
        goto fail;
    switch (what) {
        case AVAPP_CTRL_WILL_HTTP_OPEN:
        case AVAPP_CTRL_WILL_LIVE_OPEN:
        case AVAPP_CTRL_WILL_CONCAT_SEGMENT_OPEN: {
            AVAppIOControl *real_data = (AVAppIOControl *)data;
            real_data->is_handled = 0;

            jbundle = J4AC_Bundle__Bundle__catchAll(env);
            if (!jbundle) {
                ALOGE("%s: J4AC_Bundle__Bundle__catchAll failed for case %d\n", __func__, what);
                goto fail;
            }
            J4AC_Bundle__putString__withCString__catchAll(env, jbundle, "url", real_data->url);
            J4AC_Bundle__putInt__withCString__catchAll(env, jbundle, "segment_index", real_data->segment_index);
            J4AC_Bundle__putInt__withCString__catchAll(env, jbundle, "retry_counter", real_data->retry_counter);
            real_data->is_handled = J4AC_IjkMediaPlayer__onNativeInvoke(env, weak_thiz, what, jbundle);
            if (J4A_ExceptionCheck__catchAll(env)) {
                goto fail;
            }

            J4AC_Bundle__getString__withCString__asCBuffer(env, jbundle, "url", real_data->url, sizeof(real_data->url));
            if (J4A_ExceptionCheck__catchAll(env)) {
                goto fail;
            }
            ret = 0;
            break;
        }
        case AVAPP_EVENT_WILL_HTTP_OPEN:
        case AVAPP_EVENT_DID_HTTP_OPEN:
        case AVAPP_EVENT_WILL_HTTP_SEEK:
        case AVAPP_EVENT_DID_HTTP_SEEK: {
            AVAppHttpEvent *real_data = (AVAppHttpEvent *) data;
            jbundle = J4AC_Bundle__Bundle__catchAll(env);
            if (!jbundle) {
                ALOGE("%s: J4AC_Bundle__Bundle__catchAll failed for case %d\n", __func__, what);
                goto fail;
            }
            J4AC_Bundle__putString__withCString__catchAll(env, jbundle, "url", real_data->url);
            J4AC_Bundle__putLong__withCString__catchAll(env, jbundle, "offset", real_data->offset);
            J4AC_Bundle__putInt__withCString__catchAll(env, jbundle, "error", real_data->error);
            J4AC_Bundle__putInt__withCString__catchAll(env, jbundle, "http_code", real_data->http_code);
            J4AC_Bundle__putLong__withCString__catchAll(env, jbundle, "file_size", real_data->filesize);
            J4AC_IjkMediaPlayer__onNativeInvoke(env, weak_thiz, what, jbundle);
            if (J4A_ExceptionCheck__catchAll(env))
                goto fail;
            ret = 0;
            break;
        }
        case AVAPP_CTRL_DID_TCP_OPEN:
        case AVAPP_CTRL_WILL_TCP_OPEN: {
            AVAppTcpIOControl *real_data = (AVAppTcpIOControl *)data;
            jbundle = J4AC_Bundle__Bundle__catchAll(env);
            if (!jbundle) {
                ALOGE("%s: J4AC_Bundle__Bundle__catchAll failed for case %d\n", __func__, what);
                goto fail;
            }
            J4AC_Bundle__putInt__withCString__catchAll(env, jbundle, "error", real_data->error);
            J4AC_Bundle__putInt__withCString__catchAll(env, jbundle, "family", real_data->family);
            J4AC_Bundle__putString__withCString__catchAll(env, jbundle, "ip", real_data->ip);
            J4AC_Bundle__putInt__withCString__catchAll(env, jbundle, "port", real_data->port);
            J4AC_Bundle__putInt__withCString__catchAll(env, jbundle, "fd", real_data->fd);
            J4AC_IjkMediaPlayer__onNativeInvoke(env, weak_thiz, what, jbundle);
            if (J4A_ExceptionCheck__catchAll(env))
                goto fail;
            ret = 0;
            break;
        }
        default: {
            ret = 0;
        }
    }
fail:
    SDL_JNI_DeleteLocalRefP(env, &jbundle);
    return ret;
}

static bool mediacodec_select_callback(void *opaque, ijkmp_mediacodecinfo_context *mcc)
{
    JNIEnv *env = NULL;
    jobject weak_this = (jobject) opaque;
    const char *found_codec_name = NULL;

    if (JNI_OK != SDL_JNI_SetupThreadEnv(&env)) {
        ALOGE("%s: SetupThreadEnv failed\n", __func__);
        return -1;
    }

    found_codec_name = J4AC_IjkMediaPlayer__onSelectCodec__withCString__asCBuffer(env, weak_this, mcc->mime_type, mcc->profile, mcc->level, mcc->codec_name, sizeof(mcc->codec_name));
    if (J4A_ExceptionCheck__catchAll(env) || !found_codec_name) {
        ALOGE("%s: onSelectCodec failed\n", __func__);
        goto fail;
    }

fail:
    return found_codec_name;
}

//post_event（）方法会将事件从c层抛到java层的：
inline static void post_event(JNIEnv *env, jobject weak_this, int what, int arg1, int arg2)
{
    // MPTRACE("post_event(%p, %p, %d, %d, %d)", (void*)env, (void*) weak_this, what, arg1, arg2);
    J4AC_IjkMediaPlayer__postEventFromNative(env, weak_this, what, arg1, arg2, NULL);

    // same as below ...
//    J4AC_tv_danmaku_ijk_media_player_IjkMediaPlayer__postEventFromNative(env, weak_this, what, arg1, arg2, NULL);

    // MPTRACE("post_event()=void");
}

inline static void post_event2(JNIEnv *env, jobject weak_this, int what, int arg1, int arg2, jobject obj)
{
    // MPTRACE("post_event2(%p, %p, %d, %d, %d, %p)", (void*)env, (void*) weak_this, what, arg1, arg2, (void*)obj);
    J4AC_IjkMediaPlayer__postEventFromNative(env, weak_this, what, arg1, arg2, obj);
    // MPTRACE("post_event2()=void");
}

//message_loop_n函数中取消息，并用post_event调用jni方法把事件发送到java层
static void message_loop_n(JNIEnv *env, IjkMediaPlayer *mp)
{
    jobject weak_thiz = (jobject) ijkmp_get_weak_thiz(mp);
    JNI_CHECK_GOTO(weak_thiz, env, NULL, "mpjni: message_loop_n: null weak_thiz", LABEL_RETURN);

    while (1) {
        AVMessage msg;

        //取消息队列的消息，如果没有消息就阻塞，直到有消息被发到消息队列。
        int retval = ijkmp_get_msg(mp, &msg, 1);
        if (retval < 0)
            break;

        // block-get should never return 0
        assert(retval > 0);

        switch (msg.what) {
        case FFP_MSG_FLUSH:
            //调用post_event，把事件发送到java层。
            MPTRACE("FFP_MSG_FLUSH:\n");
            post_event(env, weak_thiz, MEDIA_NOP, 0, 0);
            break;
        case FFP_MSG_ERROR:
            MPTRACE("FFP_MSG_ERROR: %d\n", msg.arg1);
            post_event(env, weak_thiz, MEDIA_ERROR, MEDIA_ERROR_IJK_PLAYER, msg.arg1);
            break;
        case FFP_MSG_PREPARED:
            MPTRACE("FFP_MSG_PREPARED:\n");
            post_event(env, weak_thiz, MEDIA_PREPARED, 0, 0);
            break;
        case FFP_MSG_COMPLETED:
            MPTRACE("FFP_MSG_COMPLETED:\n");
            post_event(env, weak_thiz, MEDIA_PLAYBACK_COMPLETE, 0, 0);
            break;
        case FFP_MSG_VIDEO_SIZE_CHANGED:
            MPTRACE("FFP_MSG_VIDEO_SIZE_CHANGED: %d, %d\n", msg.arg1, msg.arg2);
            post_event(env, weak_thiz, MEDIA_SET_VIDEO_SIZE, msg.arg1, msg.arg2);
            break;
        case FFP_MSG_SAR_CHANGED:
            MPTRACE("FFP_MSG_SAR_CHANGED: %d, %d\n", msg.arg1, msg.arg2);
            post_event(env, weak_thiz, MEDIA_SET_VIDEO_SAR, msg.arg1, msg.arg2);
            break;
        case FFP_MSG_VIDEO_RENDERING_START:
            MPTRACE("FFP_MSG_VIDEO_RENDERING_START:\n");
            post_event(env, weak_thiz, MEDIA_INFO, MEDIA_INFO_VIDEO_RENDERING_START, 0);
            break;
        case FFP_MSG_AUDIO_RENDERING_START:
            MPTRACE("FFP_MSG_AUDIO_RENDERING_START:\n");
            post_event(env, weak_thiz, MEDIA_INFO, MEDIA_INFO_AUDIO_RENDERING_START, 0);
            break;
        case FFP_MSG_VIDEO_ROTATION_CHANGED:
            MPTRACE("FFP_MSG_VIDEO_ROTATION_CHANGED: %d\n", msg.arg1);
            post_event(env, weak_thiz, MEDIA_INFO, MEDIA_INFO_VIDEO_ROTATION_CHANGED, msg.arg1);
            break;
        case FFP_MSG_AUDIO_DECODED_START:
            MPTRACE("FFP_MSG_AUDIO_DECODED_START:\n");
            post_event(env, weak_thiz, MEDIA_INFO, MEDIA_INFO_AUDIO_DECODED_START, 0);
            break;
        case FFP_MSG_VIDEO_DECODED_START:
            MPTRACE("FFP_MSG_VIDEO_DECODED_START:\n");
            post_event(env, weak_thiz, MEDIA_INFO, MEDIA_INFO_VIDEO_DECODED_START, 0);
            break;
        case FFP_MSG_OPEN_INPUT:
            MPTRACE("FFP_MSG_OPEN_INPUT:\n");
            post_event(env, weak_thiz, MEDIA_INFO, MEDIA_INFO_OPEN_INPUT, 0);
            break;
        case FFP_MSG_FIND_STREAM_INFO:
            MPTRACE("FFP_MSG_FIND_STREAM_INFO:\n");
            post_event(env, weak_thiz, MEDIA_INFO, MEDIA_INFO_FIND_STREAM_INFO, 0);
            break;
        case FFP_MSG_COMPONENT_OPEN:
            MPTRACE("FFP_MSG_COMPONENT_OPEN:\n");
            post_event(env, weak_thiz, MEDIA_INFO, MEDIA_INFO_COMPONENT_OPEN, 0);
            break;
        case FFP_MSG_BUFFERING_START:
            MPTRACE("FFP_MSG_BUFFERING_START:\n");
            post_event(env, weak_thiz, MEDIA_INFO, MEDIA_INFO_BUFFERING_START, msg.arg1);
            break;
        case FFP_MSG_BUFFERING_END:
            MPTRACE("FFP_MSG_BUFFERING_END:\n");
            post_event(env, weak_thiz, MEDIA_INFO, MEDIA_INFO_BUFFERING_END, msg.arg1);
            break;
        case FFP_MSG_BUFFERING_UPDATE:
            // MPTRACE("FFP_MSG_BUFFERING_UPDATE: %d, %d", msg.arg1, msg.arg2);
            post_event(env, weak_thiz, MEDIA_BUFFERING_UPDATE, msg.arg1, msg.arg2);
            break;
        case FFP_MSG_BUFFERING_BYTES_UPDATE:
            break;
        case FFP_MSG_BUFFERING_TIME_UPDATE:
            break;
        case FFP_MSG_SEEK_COMPLETE:
            MPTRACE("FFP_MSG_SEEK_COMPLETE:\n");
            post_event(env, weak_thiz, MEDIA_SEEK_COMPLETE, 0, 0);
            break;
        case FFP_MSG_ACCURATE_SEEK_COMPLETE:
            MPTRACE("FFP_MSG_ACCURATE_SEEK_COMPLETE:\n");
            post_event(env, weak_thiz, MEDIA_INFO, MEDIA_INFO_MEDIA_ACCURATE_SEEK_COMPLETE, msg.arg1);
            break;
        case FFP_MSG_PLAYBACK_STATE_CHANGED:
            break;
        case FFP_MSG_TIMED_TEXT:
            if (msg.obj) {
                jstring text = (*env)->NewStringUTF(env, (char *)msg.obj);
                post_event2(env, weak_thiz, MEDIA_TIMED_TEXT, 0, 0, text);
                J4A_DeleteLocalRef__p(env, &text);
            }
            else {
                post_event2(env, weak_thiz, MEDIA_TIMED_TEXT, 0, 0, NULL);
            }
            break;
        case FFP_MSG_GET_IMG_STATE:
            if (msg.obj) {
                jstring file_name = (*env)->NewStringUTF(env, (char *)msg.obj);
                post_event2(env, weak_thiz, MEDIA_GET_IMG_STATE, msg.arg1, msg.arg2, file_name);
                J4A_DeleteLocalRef__p(env, &file_name);
            }
            else {
                post_event2(env, weak_thiz, MEDIA_GET_IMG_STATE, msg.arg1, msg.arg2, NULL);
            }
            break;
        case FFP_MSG_VIDEO_SEEK_RENDERING_START:
            MPTRACE("FFP_MSG_VIDEO_SEEK_RENDERING_START:\n");
            post_event(env, weak_thiz, MEDIA_INFO, MEDIA_INFO_VIDEO_SEEK_RENDERING_START, msg.arg1);
            break;
        case FFP_MSG_AUDIO_SEEK_RENDERING_START:
            MPTRACE("FFP_MSG_AUDIO_SEEK_RENDERING_START:\n");
            post_event(env, weak_thiz, MEDIA_INFO, MEDIA_INFO_AUDIO_SEEK_RENDERING_START, msg.arg1);
            break;
        default:
            ALOGE("unknown FFP_MSG_xxx(%d)\n", msg.what);
            break;
        }
        msg_free_res(&msg);
    }

LABEL_RETURN:
    ;
}


// message_loop()方法创建了类似android的looper的消息机制
static int message_loop(void *arg)
{
    MPTRACE("%s\n", __func__);

    JNIEnv *env = NULL;
    if (JNI_OK != SDL_JNI_SetupThreadEnv(&env)) {
        ALOGE("%s: SetupThreadEnv failed\n", __func__);
        return -1;
    }

    IjkMediaPlayer *mp = (IjkMediaPlayer*) arg;
    JNI_CHECK_GOTO(mp, env, NULL, "mpjni: native_message_loop: null mp", LABEL_RETURN);

    //开启类似Android的looper的消息机制。
    message_loop_n(env, mp);

LABEL_RETURN:
    ijkmp_dec_ref_p(&mp);

    MPTRACE("message_loop exit");
    return 0;
}

// ----------------------------------------------------------------------------
void monstartup(const char *libname);
void moncleanup(void);

static void
IjkMediaPlayer_native_profileBegin(JNIEnv *env, jclass clazz, jstring libName)
{
    MPTRACE("%s\n", __func__);

    const char *c_lib_name = NULL;
    static int s_monstartup = 0;

    if (!libName)
        return;

    if (s_monstartup) {
        ALOGW("monstartup already called\b");
        return;
    }

    c_lib_name = (*env)->GetStringUTFChars(env, libName, NULL );
    JNI_CHECK_GOTO(c_lib_name, env, "java/lang/OutOfMemoryError", "mpjni: monstartup: libName.string oom", LABEL_RETURN);

    s_monstartup = 1;
    monstartup(c_lib_name);
    ALOGD("monstartup: %s\n", c_lib_name);

LABEL_RETURN:
    if (c_lib_name)
        (*env)->ReleaseStringUTFChars(env, libName, c_lib_name);
}

static void
IjkMediaPlayer_native_profileEnd(JNIEnv *env, jclass clazz)
{
    MPTRACE("%s\n", __func__);
    static int s_moncleanup = 0;

    if (s_moncleanup) {
        ALOGW("moncleanu already called\b");
        return;
    }

    s_moncleanup = 1;
    moncleanup();
    ALOGD("moncleanup\n");
}

static void
IjkMediaPlayer_native_setLogLevel(JNIEnv *env, jclass clazz, jint level)
{
    MPTRACE("%s(%d)\n", __func__, level);
    ijkmp_global_set_log_level(level);
    ALOGD("moncleanup\n");
}

static void
IjkMediaPlayer_setFrameAtTime(JNIEnv *env, jobject thiz, jstring path, jlong start_time, jlong end_time, jint num, jint definition) {
    IjkMediaPlayer *mp = jni_get_media_player(env, thiz);
    const char *c_path = NULL;
    JNI_CHECK_GOTO(path, env, "java/lang/IllegalArgumentException", "mpjni: setFrameAtTime: null path", LABEL_RETURN);
    JNI_CHECK_GOTO(mp, env, "java/lang/IllegalStateException", "mpjni: setFrameAtTime: null mp", LABEL_RETURN);

    c_path = (*env)->GetStringUTFChars(env, path, NULL );
    JNI_CHECK_GOTO(c_path, env, "java/lang/OutOfMemoryError", "mpjni: setFrameAtTime: path.string oom", LABEL_RETURN);

    ALOGV("setFrameAtTime: path %s", c_path);
    ijkmp_set_frame_at_time(mp, c_path, start_time, end_time, num, definition);
    (*env)->ReleaseStringUTFChars(env, path, c_path);

LABEL_RETURN:
    ijkmp_dec_ref_p(&mp);
    return;
}





// ----------------------------------------------------------------------------

static JNINativeMethod g_methods[] = {
    {
        "_setDataSource",
        "(Ljava/lang/String;[Ljava/lang/String;[Ljava/lang/String;)V",
        (void *) IjkMediaPlayer_setDataSourceAndHeaders
    },
    { "_setDataSourceFd",       "(I)V",     (void *) IjkMediaPlayer_setDataSourceFd },
    { "_setDataSource",         "(Ltv/danmaku/ijk/media/player/misc/IMediaDataSource;)V", (void *)IjkMediaPlayer_setDataSourceCallback },
    { "_setAndroidIOCallback",  "(Ltv/danmaku/ijk/media/player/misc/IAndroidIO;)V", (void *)IjkMediaPlayer_setAndroidIOCallback },

    { "_setVideoSurface",       "(Landroid/view/Surface;)V", (void *) IjkMediaPlayer_setVideoSurface },
    { "_prepareAsync",          "()V",      (void *) IjkMediaPlayer_prepareAsync },
    { "_start",                 "()V",      (void *) IjkMediaPlayer_start },
    { "_stop",                  "()V",      (void *) IjkMediaPlayer_stop },
    { "seekTo",                 "(J)V",     (void *) IjkMediaPlayer_seekTo },
    { "_pause",                 "()V",      (void *) IjkMediaPlayer_pause },
    { "isPlaying",              "()Z",      (void *) IjkMediaPlayer_isPlaying },
    { "getCurrentPosition",     "()J",      (void *) IjkMediaPlayer_getCurrentPosition },
    { "getDuration",            "()J",      (void *) IjkMediaPlayer_getDuration },
    { "_release",               "()V",      (void *) IjkMediaPlayer_release },
    { "_reset",                 "()V",      (void *) IjkMediaPlayer_reset },
    { "setVolume",              "(FF)V",    (void *) IjkMediaPlayer_setVolume },
    { "getAudioSessionId",      "()I",      (void *) IjkMediaPlayer_getAudioSessionId },
    { "native_init",            "()V",      (void *) IjkMediaPlayer_native_init },
    { "native_setup",           "(Ljava/lang/Object;)V", (void *) IjkMediaPlayer_native_setup },
    { "native_finalize",        "()V",      (void *) IjkMediaPlayer_native_finalize },

    { "_setOption",             "(ILjava/lang/String;Ljava/lang/String;)V", (void *) IjkMediaPlayer_setOption },
    { "_setOption",             "(ILjava/lang/String;J)V",                  (void *) IjkMediaPlayer_setOptionLong },

    { "_getColorFormatName",    "(I)Ljava/lang/String;",    (void *) IjkMediaPlayer_getColorFormatName },
    { "_getVideoCodecInfo",     "()Ljava/lang/String;",     (void *) IjkMediaPlayer_getVideoCodecInfo },
    { "_getAudioCodecInfo",     "()Ljava/lang/String;",     (void *) IjkMediaPlayer_getAudioCodecInfo },
    { "_getMediaMeta",          "()Landroid/os/Bundle;",    (void *) IjkMediaPlayer_getMediaMeta },
    { "_setLoopCount",          "(I)V",                     (void *) IjkMediaPlayer_setLoopCount },
    { "_getLoopCount",          "()I",                      (void *) IjkMediaPlayer_getLoopCount },
    { "_getPropertyFloat",      "(IF)F",                    (void *) ijkMediaPlayer_getPropertyFloat },
    { "_setPropertyFloat",      "(IF)V",                    (void *) ijkMediaPlayer_setPropertyFloat },
    { "_getPropertyLong",       "(IJ)J",                    (void *) ijkMediaPlayer_getPropertyLong },
    { "_setPropertyLong",       "(IJ)V",                    (void *) ijkMediaPlayer_setPropertyLong },
    { "_setStreamSelected",     "(IZ)V",                    (void *) ijkMediaPlayer_setStreamSelected },

    { "native_profileBegin",    "(Ljava/lang/String;)V",    (void *) IjkMediaPlayer_native_profileBegin },
    { "native_profileEnd",      "()V",                      (void *) IjkMediaPlayer_native_profileEnd },

    { "native_setLogLevel",     "(I)V",                     (void *) IjkMediaPlayer_native_setLogLevel },
    { "_setFrameAtTime",        "(Ljava/lang/String;JJII)V", (void *) IjkMediaPlayer_setFrameAtTime },
};

JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved)
{
    JNIEnv* env = NULL;

    g_jvm = vm;
    if ((*vm)->GetEnv(vm, (void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        return -1;
    }
    assert(env != NULL);

    pthread_mutex_init(&g_clazz.mutex, NULL );

    // FindClass returns LocalReference
    IJK_FIND_JAVA_CLASS(env, g_clazz.clazz, JNI_CLASS_IJKPLAYER);
    /**
     *
     *
    IJK_FIND_JAVA_CLASS  -->

    jclass clazz = (*env)->FindClass(env, JNI_CLASS_IJKPLAYER);
    g_clazz.clazz = (*env)->NewGlobalRef(env, clazz);
    (*env)->DeleteLocalRef(env, clazz);
     *
     */


    (*env)->RegisterNatives(env, g_clazz.clazz, g_methods, NELEM(g_methods) );
    /**
     *
     * https://stackoverflow.com/questions/1010645/what-does-the-registernatives-method-do
     *
     * map  native functions in java to c functions ...
     *
     */

    //播放器全局初始化，注册ffmpeg的解码器，解封装器，加载外部库如openssl等
    ijkmp_global_init();


    ijkmp_global_set_inject_callback(inject_callback);

    /**
     *  #include "ijkplayer_android.h"
     *  ---->
     *  ijkplayer.h
     */

    FFmpegApi_global_init(env);
    SDL_JNI_OnLoad(vm,reserved);

    return JNI_VERSION_1_4;
}

JNIEXPORT void JNI_OnUnload(JavaVM *jvm, void *reserved)
{
    ijkmp_global_uninit();

    pthread_mutex_destroy(&g_clazz.mutex);
}

/**

入口
 https://www.jianshu.com/nb/40971763

----------------------------------------------

 理解ijkplayer（二）项目结构分析
https://www.jianshu.com/p/b5a2584e03f1

 我们可以看到规则，每当java层调用jni方法例如：
java调用的jni方法	 对应Ijkplayer内部的方法
XXX	                 IjkMediaPlayer_XXX

----

ijkplayer_jni.c的每一个Jni映射方法几乎都是这样，拿到一个IjkMediaPlayer对象，然后跳转到ijkplayer.c中的函数。

ijkplayer_jni.c是通过
#include "ijkplayer_android.h"     而ijkplayer_android.h中，又
#include "../ijkplayer.h"    所以，具备了调用 ijkpalyer.h的能力的

 方法对应
 ijkplayer_jin.c	           ijkplayer.h / ijkpalyer.c
IjkMediaPlayer_prepareAsync	   ijkmp_prepare_async
IjkMediaPlayer_start	       ijkmp_start

----

 ijkplayer.c中的代码会调用到ff_ffplay.c中的代码，以及类似的ff_ffxxx.c中的代码：

即以ff开头的文件中的函数:
 cpp/ijkmedia/ijkplayer/ff_ffplay.c
 等

----------------------------------------------

 理解ijkplayer（三）从Java层开始初始化

https://www.jianshu.com/p/0501be9cf4bf

native_init()
native_setup()  ---> ijkmp_android_create  (创建IjkMediaPlayer、FFPlayer / 设置硬解软解 / )
_setDataSource()
_setVideoSurface


 ----------------------------------------------

理解ijkplayer（四）拉流

https://www.jianshu.com/p/f633da0db4dd


_prepareAsync()


  ijkmedia/ijkplayer/android/ijkplayer_jni.c  ---> IjkMediaPlayer_prepareAsync
-->
 ijkmedia/ijkplayer/ijkplayer.c  ---> ijkmp_prepare_async  ---> ijkmp_prepare_async_l
-->
 ijkmedia/ijkplayer/ff_ffplay.c  ---> ffp_prepare_async_l  ---> stream_open   ---> read_thread  --->  stream_component_open
                                                           ---> ffpipeline_open_audio_output   打开音频输出设备



  --- stream_open：
创建VideoState对象，并初始化他的一些默认属性。
初始化视频、音频、字幕的解码后的帧队列。
初始化视频、音频、字幕的解码前的包队列。
初始化播放器音量。
创建视频渲染线程。
创建视频数据读取线程（从网络读取或者从文件读取，io操作）。
初始化解码器。（ffmpeg应该会在内部创建解码线程）。
因此，在openstream()方法中完成了最主要的3个线程的创建。



 创建视频渲染/读取线程-->
 static int read_thread(void *arg)


 小结：

总结一下：视频读取线程大致做的事情就是：

1 ffmpeg进行协议探测，封装格式探测等网络请求，为创建解码器做准备。
2 创建video，audio，subtitle解码器并开启相应的解码线程。
3 for循环不断地调用av_read_frame()去从ffmpeg内部维护的网络包缓存去取出下载好的AVPacket，并放入相应的队列中，供稍后解码线程取出解码。


 ----------------------------------------------

 理解ijkplayer（五）解码、播放

 https://www.jianshu.com/p/1e10507f18b6


 stream_component_open

找到解码器
初始化解码器
分别启动audio_thread，video_thread和subtitle_thread这3条解码线程，内部开始不断解码。

字幕解码线程  subtitle_thread

 从字幕的解码流程中可以看出解码的大致逻辑为：
1 循环地调用decoder_decode_frame（），在这个方法里面对视频，音频和字幕3种流用switch语句来分别处理解码。当然，在音频解码audio_thread和视频解码video_thread中同样会调用这个方法的。
2 解码前，先从PacketQueue读取包数据，这个数据从哪里来？从read_thread()函数中调用的ffmpeg的函数：av_read_frame(ic, pkt);来的。
3 解码时，先塞给解码器pkt数据，再从解码器中读出解码好的frame数据。
4 再把frame数据入队FrameQueue，留给稍后的渲染器来从FrameQueue中读取


 **/



/**
 *
 * ffplay read线程分析
 *
 * https://zhuanlan.zhihu.com/p/43672062
 *
**/


/**
 *
 * ffplay 分析概述
 *
 * https://zhuanlan.zhihu.com/p/44694286
 *
 *
 * ----
 * ffplay packet queue分析    https://zhuanlan.zhihu.com/p/43295650
 *
 * ffplay用PacketQueue保存解封装后的数据，即保存AVPacket。
 *
 *  cpp/ijkmedia/ijkplayer/ff_ffplay_def.h :    MyAVPacketList
 *  cpp/ijkmedia/ijkplayer/ff_ffplay_def.h :    PacketQueue
 *

PacketQueue操作提供以下方法： ( cpp/ijkmedia/ijkplayer/ff_ffplay.c )
packet_queue_init：初始化
packet_queue_destroy：销毁
packet_queue_start：启用
packet_queue_abort：中止
packet_queue_get：获取一个节点
packet_queue_put：存入一个节点
packet_queue_put_nullpacket：存入一个空节点
packet_queue_flush：清除队列内所有的节点

 *
 * ----
 *  frame queue分析
 * https://zhuanlan.zhihu.com/p/43564980
 *
 *
 * ffplay用frame queue保存解码后的数据。
 *
 *  结构体Frame用于保存一帧视频画面、音频或者字幕：
 *
 *
 *  接着设计了一个FrameQueue用于表示整个帧队列：
 *
 *  cpp/ijkmedia/ijkplayer/ff_ffplay_def.h :    Frame
 *  cpp/ijkmedia/ijkplayer/ff_ffplay_def.h :    FrameQueue
 *
 *  下面看看FrameQueue提供的函数： ( cpp/ijkmedia/ijkplayer/ff_ffplay.c )
 *
 *  初始化函数:frame_queue_init
 *  反初始化:frame_queue_destory
 *   FrameQueue的“写”分两步，先调用frame_queue_peek_writable获取一个可写节点，
            在对节点操作结束后，调用frame_queue_push告知FrameQueue“存入”该节点。
 *
 *   FrameQueue的读也分两步。frame_queue_peek_readable和frame_queue_next。



 ------
 ffplay read线程分析
https://zhuanlan.zhihu.com/p/43672062

static int read_thread(void *arg)
   ( in  cpp/ijkmedia/ijkplayer/ff_ffplay.c )


准备阶段

准备阶段，主要包括以下步骤：
avformat_open_input
avformat_find_stream_info
av_find_best_stream
stream_component_open

 stream_component_open --->
 avcodec_find_decoder找到所需解码器（AVCodec）
 decoder_init
 decoder_start:  “启动”了PacketQueue，并创建了一个名为"decoder"的线程专门用于解码，具体的解码流程由传入参数fn  ( audio_thread / video_thread / subtitle_thread  )  决定。



主循环读数据，主要为：

 特殊流程的处理:


 常规流程的处理
  av_read_frame读取一个包(AVPacket)
  返回值处理
  pkt_in_play_range计算
  packet_queue_put放入各自队列，或者丢弃




 ------
ffplay解码线程分析
 https://zhuanlan.zhihu.com/p/43948483


 ffplay的解码线程独立于读线程，并且每种类型的流(AVStream)都有其各自的解码线程，
 如video_thread用于解码video stream，audio_thread用于解码audio stream，subtitle_thread用于解码subtitle stream。



        PacketQueue     FrameQueue     Clock       解码线程
 视频    videoq          pictq          vidclk     video_thread
 音频    audioq          sampq          audclk     audio_thread
 字母    subtitleq       subpq                     subtitle_thread



video_thread
 软解 （ ???  如何走到这里 ??? ）--> ffplay_video_thread
 1  调用get_video_frame解码一帧图像
 2  “计算”时长和pts
 3  调用queue_picture放入FrameQueue


 get_video_frame--->decoder_decode_frame解码



 ------
 https://zhuanlan.zhihu.com/p/44122324
 ffplay video显示线程分析


video_refresh的主体流程分为3个步骤：

计算上一帧应显示的时长，判断是否继续显示上一帧
估算当前帧应显示的时长，判断是否要丢帧
调用video_display进行显示



**/


/**
 *
 *
  * ( in  cpp/ijkmedia/ijkplayer/ff_ffplay.c )
  *
  * video_refresh_thread --->
 *
-----
 stream_open 中创建   ( in  cpp/ijkmedia/ijkplayer/ff_ffplay.c )

    //创建视频渲染线程
    is->video_refresh_tid = SDL_CreateThreadEx(&is->_video_refresh_tid, video_refresh_thread, ffp, "ff_vout");



 *
video_refresh  : called to display each frame
 --->
 video_display2
---->
video_image_display2
---->
 SDL_VoutDisplayYUVOverlay
 ---> 调用SDL_Vout下的 (ffp->vout)
 SDL_Vout --> display_overlay    (初始化中设置  ijkmp_android_create --> SDL_VoutAndroid_CreateForAndroidSurface()  )


 -->  cpp/ijkmedia/ijksdl/android/ijksdl_vout_android_nativewindow.c
 func_display_overlay_l
--->  cpp/ijkmedia/ijksdl/ijksdl_egl.c
 IJK_EGL_display


------------------> func_display_overlay_l 4  -----> 使用android_nativewindow播放



**/


/**
 *
 * --
 * https://zhuanlan.zhihu.com/p/44139512
 * ffplay audio输出线程分析-----------------------------------------------------------------------------

stream_component_open --> audio_open    : audio_open是ffplay封装的函数，会优先尝试请求参数能否打开输出设备，尝试失败后会自动查找最佳的参数重新尝试。
                                          在audio_open函数内，注册sdl_audio_callback函数为音频输出的回调函数。那么，主要的音频输出的逻辑就在sdl_audio_callback函数内了。



 ---------------------


 ff_aout_android -->音频输出线程

 cpp/ijkmedia/ijksdl/android/ijksdl_aout_android_audiotrack.c
 -->aout_thread
 -->aout_thread_n

    while (!opaque->abort_request) {
    ....

    }

 -->
 SDL_AudioCallback callback  --->   audio_open中注册的回调




 *
 *
 */


/**
 *
 * https://zhuanlan.zhihu.com/p/44615185
 *
 * ffplay音视频同步
 *
 *
 *  pts是presentation timestamp的缩写
 *  它的单位由timebase决定。timebase的类型是结构体AVRational（用于表示分数）：

typedef struct AVRational{
    int num; ///< Numerator
    int den; ///< Denominator
} AVRational;



将pts转化为秒，一般做法是：pts * av_q2d(timebase)




 Clock:

cpp/ijkmedia/ijkplayer/ff_ffplay_def.h




----------------
   ffplay音视频同步分析——视频同步音频
  https://zhuanlan.zhihu.com/p/44615401

 ffplay中将视频同步到音频的主要方案是，如果视频播放过快，则重复播放上一帧，以等待音频；如果视频播放过慢，则丢帧追赶音频。

这一部分的逻辑实现在视频输出函数video_refresh中







 *
**/






/**
 *

davidww-sdl-SDL_RunThread: [5837] ff_msg_loop
davidww-sdl-SDL_RunThread: [5839] ff_read  流媒体读取线程
davidww-sdl-SDL_RunThread: [5838] ff_vout  视频输出线程
davidww-sdl-SDL_RunThread: [5844] ff_aout_android  音频输出线程
davidww-sdl-SDL_RunThread: [5845] ff_audio_dec
davidww-sdl-SDL_RunThread: [5849] ff_video_dec  音频解码线程

 --——> 音频输出线程 如果使用 opensles
 davidww-sdl-SDL_RunThread: [6854] ff_aout_opensles

**/