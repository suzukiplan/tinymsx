//
//  VideoView.m
//  EmuBoard
//
//  Created by 鈴木　洋司　 on 2018/12/30.
//  Copyright © 2018年 SUZUKIPLAN. All rights reserved.
//

#import "EmuBoard.h"
#import "VideoLayer.h"
#import "VideoView.h"
#import "constants.h"
#import "tinymsx_def.h"
#include <ctype.h>

static CVReturn MyDisplayLinkCallback(CVDisplayLinkRef displayLink, const CVTimeStamp* now, const CVTimeStamp* outputTime, CVOptionFlags flagsIn, CVOptionFlags* flagsOut, void* context);
extern unsigned char emu_key;

@interface VideoView ()
@property (nonatomic) VideoLayer* videoLayer;
@property CVDisplayLinkRef displayLink;
@property NSInteger score;
@property BOOL isGameOver;
@end

@implementation VideoView

+ (Class)layerClass
{
    return [VideoLayer class];
}

- (id)initWithFrame:(CGRect)frame
{
    if ((self = [super initWithFrame:frame]) != nil) {
        [self setWantsLayer:YES];
        _score = -1;
        _isGameOver = -1 ? YES : NO;
        _videoLayer = [VideoLayer layer];
        [self setLayer:_videoLayer];
        CVDisplayLinkCreateWithActiveCGDisplays(&_displayLink);
        CVDisplayLinkSetOutputCallback(_displayLink, MyDisplayLinkCallback, (__bridge void*)self);
        CVDisplayLinkStart(_displayLink);
    }
    return self;
}

- (void)releaseDisplayLink
{
    if (_displayLink) {
        CVDisplayLinkStop(_displayLink);
        CVDisplayLinkRelease(_displayLink);
        _displayLink = nil;
    }
}

- (void)vsync
{
    emu_vsync();
    [self.videoLayer drawVRAM];
    unsigned int score = emu_getScore();
    BOOL isGameOver = emu_isGameOver() ? YES : NO;
    if (score != _score || isGameOver != _isGameOver) {
        _score = score;
        _isGameOver = isGameOver;
        if (_delegate) {
            [_delegate videoView:self
                  didChangeScore:_score
                      isGameOver:_isGameOver];
        }
    }
}

- (BOOL)acceptsFirstResponder
{
    return YES;
}

- (void)keyDown:(NSEvent*)event
{
    unichar c = [event.charactersIgnoringModifiers characterAtIndex:0];
    NSLog(@"keyDown: %04X", tolower(c));
    switch (tolower(c)) {
        case 0xF703: emu_key |= TINYMSX_JOY_RI; break;
        case 0xF702: emu_key |= TINYMSX_JOY_LE; break;
        case 0xF701: emu_key |= TINYMSX_JOY_DW; break;
        case 0xF700: emu_key |= TINYMSX_JOY_UP; break;
        case 0x000d: emu_key |= TINYMSX_JOY_T2; break;
        case 0x0020: emu_key |= TINYMSX_JOY_T1; break;
        case 0x0078: emu_key |= TINYMSX_JOY_T2; break;
        case 0x007A: emu_key |= TINYMSX_JOY_T1; break;
        case 0x0031: emu_key |= TINYMSX_JOY_S1; break;
        case 0x0032: emu_key |= TINYMSX_JOY_S2; break;
        case 0x0064: emu_printDump(); break;
        case 0x0073: emu_keepRam(); break;
        case 0x0065: emu_compareRam(); break;
        case 0x006C: [self _openSaveData]; break;
    }
}

- (void)keyUp:(NSEvent*)event
{
    unichar c = [event.charactersIgnoringModifiers characterAtIndex:0];
    switch (tolower(c)) {
        case 0xF703: emu_key &= ~TINYMSX_JOY_RI; break;
        case 0xF702: emu_key &= ~TINYMSX_JOY_LE; break;
        case 0xF701: emu_key &= ~TINYMSX_JOY_DW; break;
        case 0xF700: emu_key &= ~TINYMSX_JOY_UP; break;
        case 0x000d: emu_key &= ~TINYMSX_JOY_T2; break;
        case 0x0020: emu_key &= ~TINYMSX_JOY_T1; break;
        case 0x0078: emu_key &= ~TINYMSX_JOY_T2; break;
        case 0x007A: emu_key &= ~TINYMSX_JOY_T1; break;
        case 0x0031: emu_key &= ~TINYMSX_JOY_S1; break;
        case 0x0032: emu_key &= ~TINYMSX_JOY_S2; break;
    }
}

- (void)_openSaveData
{
    NSOpenPanel* panel = [NSOpenPanel openPanel];
    panel.allowsMultipleSelection = NO;
    panel.canChooseDirectories = NO;
    panel.canCreateDirectories = YES;
    panel.canChooseFiles = YES;
    panel.allowedFileTypes = @[ @"bin", @"dat" ];
    [panel beginWithCompletionHandler:^(NSModalResponse result) {
        if (!result) return;
        NSData* data = [NSData dataWithContentsOfURL:panel.URL];
        if (!data) return;
        emu_loadState(data.bytes, data.length);
    }];
}

static CVReturn MyDisplayLinkCallback(CVDisplayLinkRef displayLink, const CVTimeStamp* now, const CVTimeStamp* outputTime, CVOptionFlags flagsIn, CVOptionFlags* flagsOut, void* context)
{
    [(__bridge VideoLayer*)context performSelectorOnMainThread:@selector(vsync) withObject:nil waitUntilDone:NO];
    return kCVReturnSuccess;
}

@end
