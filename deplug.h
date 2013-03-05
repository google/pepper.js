#ifndef DEPLUG_H_
#define DEPLUG_H_

struct DaemonCallback {
  struct DaemonCallback (*func)(void*);
  void* param;
};

typedef struct DaemonCallback (*DaemonFunc)(void*);

extern void LaunchDaemon(DaemonFunc func, void* param);

#endif  // DEPLUG_H_
