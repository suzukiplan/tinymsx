//
//  AppDelegate.h
//  EmuBoard
//
//  Created by 鈴木　洋司　 on 2018/12/30.
//  Copyright © 2018年 SUZUKIPLAN. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface AppDelegate : NSObject <NSApplicationDelegate>
@property (nonatomic, weak) IBOutlet NSMenu* menu;
@property (nonatomic, weak) IBOutlet NSMenuItem* menuQuickLoadFromMemory;
@end

