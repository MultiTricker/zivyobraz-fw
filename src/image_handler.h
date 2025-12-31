#ifndef IMAGE_HANDLER_H
#define IMAGE_HANDLER_H

#include "http_client.h"

namespace ImageHandler
{
// Read image data from HTTP client (paged mode - for backward compatibility)
void readImageData(HttpClient &http);

// Read image data with direct streaming to display controller
// Returns true if direct streaming was used, false if fell back to paged mode
bool readImageDataDirect(HttpClient &http);

// Check if direct streaming mode is available
bool isDirectStreamingAvailable();
} // namespace ImageHandler

#endif // IMAGE_HANDLER_H
