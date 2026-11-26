#pragma once

namespace derivean {

// Threads worth giving to background work on this machine: its performance cores where the
// processor tells them apart, else all but two of its threads.
unsigned backgroundThreads();

// Puts the calling thread in the scheduler's interactive class. Whoever advances the game calls it
// once: the pools may then be as large as the machine, because a frame takes a core from them
// whenever it wants one. Only macOS has such a class; elsewhere the call does nothing.
void favourCallingThread();

} // namespace derivean
