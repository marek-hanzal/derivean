#pragma once

namespace derivean {

// Threads worth giving to background work on this machine: its performance cores where the
// processor tells them apart, else all but two of its threads.
unsigned backgroundThreads();

// Puts the calling thread ahead of the pools in the scheduler: the interactive class on macOS, the
// highest priority of a normal process on Windows. Whoever advances the game calls it once: the
// pools may then be as large as the machine, because a frame takes a core from them whenever it
// wants one.
void favourCallingThread();

} // namespace derivean
