/*
 * ijkplayer_internal.h
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

#ifndef IJKPLAYER_ANDROID__IJKPLAYER_INTERNAL_H
#define IJKPLAYER_ANDROID__IJKPLAYER_INTERNAL_H

#include <assert.h>
#include "ijksdl/ijksdl.h"
#include "ff_fferror.h"
#include "ff_ffplay.h"
#include "ijkplayer.h"


struct IjkMediaPlayer {
    volatile int ref_count;
    //互斥锁，用于保证线程安全
    pthread_mutex_t mutex;

    //ffplayer，位于ijkmedia/ijkplayer/ff_ffplay_def.h，他会直接调用ffmpeg的方法了
    FFPlayer *ffplayer;

    //一个函数指针，指向的是谁？指向的其实就是刚才创建的message_loop，即消息循环函数
    int (*msg_loop)(void*);

    //消息机制线程
    SDL_Thread *msg_thread;
    SDL_Thread _msg_thread;

    //播放器状态，例如prepared,resumed,error,completed等
    int mp_state;
    //字符串，就是一个播放url
    char *data_source;
    //弱引用
    void *weak_thiz;

    int restart;
    int restart_from_beginning;
    int seek_req;
    long seek_msec;
};

#endif
