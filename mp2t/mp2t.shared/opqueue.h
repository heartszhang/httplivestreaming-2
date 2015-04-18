#pragma once
#include "mp2t.h"

/*
Within each work queue, the callbacks are serialized. If you call MFPutWorkItem twice with the same work queue, the second callback is not invoked until the first callback has returned.
*/
//MFASYNC_CALLBACK_QUEUE_MULTITHREADED
