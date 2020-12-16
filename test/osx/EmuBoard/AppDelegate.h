//
//  AppDelegate.h
//  EmuBoard
//
//  Created by 鈴木　洋司　 on 2018/12/30.
//  Copyright © 2018年 SUZUKIPLAN. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@class AppDelegate;

@protocol OpenFileDelegate <NSObject>
- (BOOL)application:(AppDelegate*)app didOpenFile:(NSString*)file;
@end

@interface AppDelegate : NSObject <NSApplicationDelegate>
@property (nonatomic, weak) id<OpenFileDelegate> openFileDelegate;
@property (nonatomic, weak) IBOutlet NSMenu* menu;
@property (nonatomic, weak) IBOutlet NSMenuItem* menuQuickLoadFromMemory;
@end

