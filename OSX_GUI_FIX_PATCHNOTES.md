A fix that enables Pymol and the OSX native Aqua windowing system
to play nicely together. By Max Klein, mklein@jhu.edu

Notes:
- Low level Apple libraries (Core Services) strictly enforce a rule
  that only a program's main thread may initialize/interact with Tkinter
  Aqua application windows.

- This patch works around this issue
    - includes code that tests for 3 conditions:
        - An external GUI has been requested during Pymol invocation
        - Pymol is running on OS X
        - The windowing system for external GUI is Aqua

    - When all three conditions are True, the patch code alters the
      normal Pymol kickoff such that the external GUI runs in the main
      thread while the glut GUI, which normally runs on the main thread,
      runs on a child thread instead.

- This particular fix was settled upon for reasons of performance
  and ease of engineering. An alternative fix would be to set up
  a way for both GUIs to share time on the main thread, although
  performance drops would be likely.

- The patch has been extensively tested on my own system (OS X 10.12.5)
  and on the build servers of the homebrew project (Ubuntu 14.04,
  OS X 10.10.5, OS X 10.11.6, OS X 10.12.5). The patch was not found to have
  a noticeable impact on GUI responsiveness/performance.
