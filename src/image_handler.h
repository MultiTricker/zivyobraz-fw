#ifndef IMAGE_HANDLER_H
#define IMAGE_HANDLER_H

#include "http_client.h"

namespace ImageHandler
{
// Read image data from HTTP client
void readImageData(HttpClient &http);
} // namespace ImageHandler

#endif // IMAGE_HANDLER_H
