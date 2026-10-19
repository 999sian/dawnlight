#pragma once

// iOS uses package-time gateways, not runtime code patching. Dusklight v2.0.3
// prepares game functions but not the private host UI/audio/combo functions.
#ifndef DAWNLIGHT_IOS
#if defined(__APPLE__)
#include <TargetConditionals.h>
#define DAWNLIGHT_IOS TARGET_OS_IPHONE
#else
#define DAWNLIGHT_IOS 0
#endif
#endif
