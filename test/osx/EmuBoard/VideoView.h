//
//  VideoView.h
//  EmuBoard
//
//  Created by 鈴木　洋司　 on 2018/12/30.
//  Copyright © 2018年 SUZUKIPLAN. All rights reserved.
//

#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

@class VideoView;

@protocol VideoViewDelegate <NSObject>
- (void)videoView:(VideoView*)view
   didChangeScore:(NSInteger)score
       isGameOver:(BOOL)isGameOver;
@end

@interface VideoView : NSView
@property (nonatomic, readwrite) BOOL isFullScreen;
@property (nonatomic, weak, nullable) id<VideoViewDelegate> delegate;
- (id)initWithFrame:(CGRect)frame;
- (void)releaseDisplayLink;
@end

NS_ASSUME_NONNULL_END
