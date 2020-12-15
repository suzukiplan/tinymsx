//
//  EmuBoard.c
//  EmuBoard
//
//  Created by 鈴木　洋司　 on 2018/12/30.
//  Copyright © 2018年 SUZUKIPLAN. All rights reserved.
//

#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "EmuBoard.h"
#include "tinymsx_gw.h"
#include "tinymsx_def.h"
#include "vgsspu_al.h"

/**
 * この内容を書き換えればそれが画面に表示される
 * ※RGB565（16bit color）形式
 */
unsigned short emu_vram[256 * 192];
unsigned char emu_key = 0;
static void* spu;
pthread_mutex_t sound_locker;
static short sound_buffer[65536 * 2];
static volatile unsigned short sound_cursor;

static int emu_initialized = 0;
static void* emu_msx = NULL;

static void sound_callback(void* buffer, size_t size)
{
    while (sound_cursor < size / 2) usleep(100);
    pthread_mutex_lock(&sound_locker);
    memcpy(buffer, sound_buffer, size);
    if (size <= sound_cursor) sound_cursor -= size;
    else sound_cursor = 0;
    pthread_mutex_unlock(&sound_locker);
}

/**
 * 起動時に1回だけ呼び出される
 */
void emu_init(const void* rom, size_t romSize, int type)
{
    puts("emu_init");
    if (emu_initialized) return;
    printf("load rom (size: %lu)\n", romSize);
    emu_msx = tinymsx_create(type, rom, romSize, TINY_MSX_COLOR_MODE_RGB555);
    tinymsx_reset(emu_msx);
    pthread_mutex_init(&sound_locker, NULL);
    spu = vgsspu_start2(44100, 16, 2, 23520, sound_callback);
    emu_initialized = 1;
}

void emu_reload(const void* rom, size_t romSize, int type)
{
    if (!emu_initialized) {
        emu_init(rom, romSize, type);
        return;
    }
    puts("emu_reload");
    if (emu_msx) {
        tinymsx_destroy(emu_msx);
    }
    emu_msx = tinymsx_create(type, rom, romSize, TINY_MSX_COLOR_MODE_RGB555);
    tinymsx_reset(emu_msx);
}

void emu_reset()
{
    tinymsx_reset(emu_msx);
}

/**
 * 画面の更新間隔（1秒間で60回）毎にこの関数がコールバックされる
 * この中で以下の処理を実行する想定:
 * 1. エミュレータのCPU処理を1フレーム分実行
 * 2. エミュレータの画面バッファの内容をvramへコピー
 */
void emu_vsync()
{
    if (!emu_initialized) return;
    tinymsx_tick(emu_msx, 0xFF, 0xFF);
    const void* ptr = tinymsx_display(emu_msx);
    memcpy(emu_vram, ptr, sizeof(emu_vram));
    unsigned short pcm[1024];
    size_t pcmSize = sizeof(pcm);
    memset(pcm, 0, pcmSize);
    pthread_mutex_lock(&sound_locker);
    if (pcmSize + sound_cursor < sizeof(sound_buffer)) {
        memcpy(&sound_buffer[sound_cursor], pcm, pcmSize);
        sound_cursor += pcmSize / 2;
    }
    pthread_mutex_unlock(&sound_locker);
}

unsigned int emu_getScore()
{
    if (!emu_initialized) return 0;
    return 0;
}

int emu_isGameOver()
{
    if (!emu_initialized) return 0;
    return 0;
}

/**
 * 終了時に1回だけ呼び出される
 * この中でエミュレータの初期化処理を実行する想定
 */
void emu_destroy()
{
    puts("emu_destroy");
    if (!emu_initialized) return;
    if (emu_msx) {
        tinymsx_destroy(emu_msx);
        emu_msx = NULL;
    }
    emu_initialized = 0;
}
