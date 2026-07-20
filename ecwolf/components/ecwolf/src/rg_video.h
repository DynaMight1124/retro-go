#pragma once

// Incremental screens must begin with the most recently submitted pixels when
// the Retro-Go backend alternates between two framebuffers.
void RGVideo_BeginIncremental();
void RGVideo_EndIncremental();

// Gameplay timing follows Retro-Go's convention: every emulated tic is
// reported. ECWolf itself renders once after processing any catch-up tics.
void RGVideo_CompleteFrame(unsigned int tics, int busyTime);

class RGVideoIncrementalScope
{
public:
    explicit RGVideoIncrementalScope(bool enabled = true) : enabled(enabled)
    {
        if (enabled)
            RGVideo_BeginIncremental();
    }
    ~RGVideoIncrementalScope()
    {
        if (enabled)
            RGVideo_EndIncremental();
    }

private:
    bool enabled;
};
