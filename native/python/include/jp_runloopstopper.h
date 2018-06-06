#import <Cocoa/Cocoa.h>

@interface JPRunLoopStopper : NSObject
{
    BOOL shouldStop;
}

+ currentRunLoopStopper;

- init;
- (BOOL) shouldRun;
+ (void) addRunLoopStopper:(JPRunLoopStopper *) runLoopStopper 
         toRunLoop:(NSRunLoop *) runLoop;
+ (void) removeRunLoopStopperFromRunLoop:(NSRunLoop *) runLoop;
- (void) stop;
- (void) performStop:(id) sender;

@end
