#pragma once
#include "../GfxAPI/GfxAPI.h"

// Implementation of the Null graphics api. It reports success on all commands, but doesn't actually do anything.
class GfxAPINull : public GfxAPI {
private:
    GfxAPINull() {};
    ~GfxAPINull() {};
    friend class GfxAPI;
};

