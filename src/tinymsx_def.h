
/**
 * SUZUKI PLAN - TinyMSX - constrant definition
 * -----------------------------------------------------------------------------
 * The MIT License (MIT)
 * 
 * Copyright (c) 2020 Yoji Suzuki.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * -----------------------------------------------------------------------------
 */
#ifndef INCLUDE_TINYMSX_DEF_H
#define INCLUDE_TINYMSX_DEF_H

#define TINYMSX_TYPE_SG1000 0
#define TINYMSX_TYPE_MSX1 1

#define TINYMSX_COLOR_MODE_RGB555 0
#define TINYMSX_COLOR_MODE_RGB565 1

#define TINYMSX_JOY_UP 0b00000001
#define TINYMSX_JOY_DW 0b00000010
#define TINYMSX_JOY_LE 0b00000100
#define TINYMSX_JOY_RI 0b00001000
#define TINYMSX_JOY_T1 0b00010000
#define TINYMSX_JOY_T2 0b00100000
#define TINYMSX_JOY_S1 0b01000000 // special key
#define TINYMSX_JOY_S2 0b10000000 // special key

#endif
