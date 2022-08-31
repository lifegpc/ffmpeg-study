#include "warp.h"

int get_codecpar_channels(AVCodecParameters* par) {
    #ifdef FF_API_OLD_CHANNEL_LAYOUT
        return par->ch_layout.nb_channels;
    #else
        return par->channels;
    #endif
}
