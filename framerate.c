#include "tari/framerate.h"

#include "tari/log.h"

// TODO: Refactor into system

Framerate gFramerate;

void setFramerate(Framerate tFramerate) {
  logg("Set framerate");
  logInteger(tFramerate);
  gFramerate = tFramerate;
}

fup Framerate getFramerate()
{
	if (!gFramerate) return 60;

	return gFramerate;
}

double getFramerateFactor() {
  if (!gFramerate) return 1;
  else {
    return 60.0 / gFramerate;
  }
}

double getInverseFramerateFactor() {
  return 1.0 / getFramerateFactor();
}
