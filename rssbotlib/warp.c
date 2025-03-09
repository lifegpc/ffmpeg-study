#include "warp.h"
#include "libavcodec/version.h"

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(59, 24, 100)
#define OLD_CHANNEL_LAYOUT 1
#else
#define NEW_CHANNEL_LAYOUT 1
#endif

int get_codecpar_channels(AVCodecParameters* par) {
    #if NEW_CHANNEL_LAYOUT
        return par->ch_layout.nb_channels;
    #else
        return par->channels;
    #endif
}
