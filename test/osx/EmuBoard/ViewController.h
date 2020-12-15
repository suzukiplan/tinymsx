//
//  ViewController.h
//  EmuBoard
//
//  Created by 鈴木　洋司　 on 2018/12/30.
//  Copyright © 2018年 SUZUKIPLAN. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface ViewController : NSViewController
-(IBAction)menuReset:(id)sender;
-(IBAction)menuOpenRomFile:(id)sender;
-(IBAction)menuViewSize1x:(id)sender;
-(IBAction)menuViewSize2x:(id)sender;
-(IBAction)menuViewSize3x:(id)sender;
-(IBAction)menuViewSize4x:(id)sender;
-(IBAction)menuQuickSaveToMemory:(id)sender;
-(IBAction)menuQuickLoadFromMemory:(id)sender;
@end

