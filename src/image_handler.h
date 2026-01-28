#ifndef IMAGE_HANDLER_H
#define IMAGE_HANDLER_H

#include "http_client.h"

namespace ImageHandler
{

// Result of image streaming operation
enum class ImageStreamingResult
{
  Success,
  FallbackToPaged,
  FatalError
};

// Read image data from HTTP client (paged mode - for backward compatibility)
void readImageData(HttpClient &http);

// Read image data with direct streaming to display controller
// Returns ImageStreamingResult to indicate success, fallback, or fatal error
ImageStreamingResult readImageDataDirect(HttpClient &http);

// Check if direct streaming mode is available
bool isDirectStreamingAvailable();
} // namespace ImageHandler

#endif // IMAGE_HANDLER_H
