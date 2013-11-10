#ifndef COBALTIRC_CRASHCATCHER_H
#define COBALTIRC_CRASHCATCHER_H

#ifdef __unix__

void init_crash_catcher();

#else // ifdef __unix__
#define init_crash_catcher()
#endif // ifdef __unix__
#endif // ifndef COBALTIRC_CRASHCATCHER_H