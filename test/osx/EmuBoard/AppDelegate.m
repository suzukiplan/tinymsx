//
//  AppDelegate.m
//  EmuBoard
//
//  Created by 鈴木　洋司　 on 2018/12/30.
//  Copyright © 2018年 SUZUKIPLAN. All rights reserved.
//

#import "AppDelegate.h"

@interface AppDelegate ()

@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification*)aNotification
{
}

- (void)applicationWillTerminate:(NSNotification*)aNotification
{
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)theApplication
{
    // Windowを閉じたらアプリを終了させる
    return YES;
}

- (BOOL)application:(NSApplication*)sender openFile:(NSString*)filename
{
    if (_openFileDelegate) {
        return [_openFileDelegate application:self didOpenFile:filename];
    } else {
        return NO;
    }
}

@end
