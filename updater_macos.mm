#include "updater_macos.hpp"

#ifdef WITH_SPARKLE
#import <Sparkle/Sparkle.h>

static SPUStandardUpdaterController *g_controller = nil;

void updater_macos_init() {
    // startingUpdater:YES makes Sparkle run a background check on launch
    // (respecting its own cooldown and user preference).
    g_controller = [[SPUStandardUpdaterController alloc]
        initWithStartingUpdater:YES
        updaterDelegate:nil
        userDriverDelegate:nil];
}

void updater_macos_check() {
    [g_controller.updater checkForUpdates];
}
#endif
