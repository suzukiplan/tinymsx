//
//  ViewController.m
//  EmuBoard
//
//  Created by 鈴木　洋司　 on 2018/12/30.
//  Copyright © 2018年 SUZUKIPLAN. All rights reserved.
//

#import "AppDelegate.h"
#import "ViewController.h"
#import "VideoView.h"
#import "constants.h"
#import "EmuBoard.h"
#import "tinymsx_def.h"

@interface ViewController() <NSWindowDelegate, VideoViewDelegate>
@property (nonatomic, weak) AppDelegate* appDelegate;
@property (nonatomic) VideoView* video;
@property (nonatomic) NSTextField* scoreView;
@property (nonatomic) NSData* rom;
@property (nonatomic) BOOL isFullScreen;
@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
#if 0
    NSString* romFile = [[NSBundle mainBundle] pathForResource:@"battlekid" ofType:@"nes"];
    _rom = [NSData dataWithContentsOfFile:romFile];
    emu_init(_rom.bytes, (size_t)_rom.length, "battlekid");
#endif
    self.view.frame = CGRectMake(0, 0, VRAM_VIEW_WIDTH * 2, VRAM_VIEW_HEIGHT * 2);
    CALayer *layer = [CALayer layer];
    [layer setBackgroundColor:CGColorCreateGenericRGB(0.0, 0.0, 0.2525, 1.0)];
    [self.view setWantsLayer:YES];
    [self.view setLayer:layer];
    _video = [[VideoView alloc] initWithFrame:[self calcVramRect]];
    _video.delegate = self;
    [self.view addSubview:_video];
    _scoreView = [[NSTextField alloc] initWithFrame:NSRectFromCGRect(CGRectMake(0, 0, 0, 0))];
    _scoreView.stringValue = @"SCORE:";
    [_scoreView sizeToFit];
    [self.view addSubview:_scoreView];
    _appDelegate = (AppDelegate*)[NSApplication sharedApplication].delegate;
    NSLog(@"menu: %@", _appDelegate.menu);
    _appDelegate.menu.autoenablesItems = NO;
    //_appDelegate.menu.item
    _appDelegate.menuQuickLoadFromMemory.enabled = NO;
    [self.view.window makeFirstResponder:_video];
}

- (void)viewWillAppear
{
    self.view.window.delegate = self;
}

- (void)windowDidResize:(NSNotification *)notification
{
    _video.frame = [self calcVramRect];
}

- (void)setRepresentedObject:(id)representedObject
{
    [super setRepresentedObject:representedObject];
    // Update the view, if already loaded.
}

- (NSRect)calcVramRect
{
    // 幅を16とした時の高さのアスペクト比を計算
    CGFloat aspectY = VRAM_VIEW_HEIGHT / (VRAM_VIEW_WIDTH / 16.0);
    // window中央にVRAMをaspect-fitで描画
    if (self.view.frame.size.height < self.view.frame.size.width) {
        CGFloat height = self.view.frame.size.height;
        CGFloat width = height / aspectY * 16;
        CGFloat x = (self.view.frame.size.width - width) / 2;
        return NSRectFromCGRect(CGRectMake(x, 0, width, height));
    } else {
        CGFloat width = self.view.frame.size.width;
        CGFloat height = width / 16 * aspectY;
        CGFloat y = (self.view.frame.size.height - height) / 2;
        return NSRectFromCGRect(CGRectMake(0, y, width, height));
    }
}

- (void)videoView:(VideoView *)view
   didChangeScore:(NSInteger)score
       isGameOver:(BOOL)isGameOver
{
    __weak ViewController* weakSelf = self;
    NSString* scoreString;
    if (isGameOver) {
        scoreString = [NSString stringWithFormat:@"SCORE: %ld [GAME OVER]", score];
    } else {
        scoreString = [NSString stringWithFormat:@"SCORE: %ld", score];
    }
    dispatch_async(dispatch_get_main_queue(), ^{
        weakSelf.scoreView.stringValue = scoreString;
        [weakSelf.scoreView sizeToFit];
    });
}

- (void)dealloc
{
    emu_destroy();
}

- (void)menuOpenRomFile:(id)sender
{
    NSLog(@"menuOpenRomFile");
    NSOpenPanel* panel = [NSOpenPanel openPanel];
    panel.allowsMultipleSelection = NO;
    panel.canChooseDirectories = NO;
    panel.canCreateDirectories = YES;
    panel.canChooseFiles = YES;
    panel.allowedFileTypes = @[@"rom", @"sg"];
    __weak ViewController* weakSelf = self;
    [panel beginWithCompletionHandler:^(NSModalResponse result) {
        if (!result) return;
        NSData* data = [NSData dataWithContentsOfURL:panel.URL];
        if (!data) return;
        char gameCode[16];
        memset(gameCode, 0, sizeof(gameCode));
        gameCode[0] = '\0';
        const char* fileName = strrchr(panel.URL.path.UTF8String, '/');
        if (fileName) {
            while ('/' == *fileName || isdigit(*fileName)) fileName++;
            if ('_' == *fileName) fileName++;
            strncpy(gameCode, fileName, 15);
            char* cp = strchr(gameCode, '.');
            if (cp) *cp = '\0';
        }
        NSLog(@"loading game: %s", gameCode);
        emu_reload(data.bytes, data.length, TINY_MSX_TYPE_SG1000);
        [weakSelf.appDelegate.menuQuickLoadFromMemory setEnabled:NO];
        [[NSDocumentController sharedDocumentController] noteNewRecentDocumentURL:panel.URL];
    }];
}

- (void)menuReset:(id)sender
{
    emu_reset();
}

- (void)windowDidEnterFullScreen:(NSNotification *)notification
{
    _isFullScreen = YES;
}

- (void)windowDidExitFullScreen:(NSNotification *)notification
{
    _isFullScreen = NO;
}

- (void)menuViewSize1x:(id)sender
{
    if (_isFullScreen) return;
    [self.view.window setContentSize:NSMakeSize(VRAM_VIEW_WIDTH * 1, VRAM_VIEW_HEIGHT * 1)];
}

- (void)menuViewSize2x:(id)sender
{
    if (_isFullScreen) return;
    [self.view.window setContentSize:NSMakeSize(VRAM_VIEW_WIDTH * 2, VRAM_VIEW_HEIGHT * 2)];
}

- (void)menuViewSize3x:(id)sender
{
    if (_isFullScreen) return;
    [self.view.window setContentSize:NSMakeSize(VRAM_VIEW_WIDTH * 3, VRAM_VIEW_HEIGHT * 3)];
}

- (void)menuViewSize4x:(id)sender
{
    if (_isFullScreen) return;
    [self.view.window setContentSize:NSMakeSize(VRAM_VIEW_WIDTH * 4, VRAM_VIEW_HEIGHT * 4)];
}

- (void)menuQuickSaveToMemory:(id)sender
{
    [_appDelegate.menuQuickLoadFromMemory setEnabled:NO];
}

- (void)menuQuickLoadFromMemory:(id)sender
{
}

@end
