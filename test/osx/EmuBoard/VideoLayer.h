//
//  VideoLayer.h
//  EmuBoard
//
//  Created by 鈴木　洋司　 on 2018/12/30.
//  Copyright © 2018年 SUZUKIPLAN. All rights reserved.
//

#import <QuartzCore/QuartzCore.h>

NS_ASSUME_NONNULL_BEGIN

@interface VideoLayer : CALayer
- (void)drawVRAM;
@end

NS_ASSUME_NONNULL_END
