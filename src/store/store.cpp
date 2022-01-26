#include "Arduino.h"
#include "result.h"

#include "../configuration.h"

static int stream_fps = STREAM_DEFAULT_FPS;

int store_get_stream_fps() {
  return stream_fps;
}

void store_set_stream_fps(int value) {
  stream_fps = value;
}
