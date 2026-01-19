#ifndef IMPROV_HANDLER_H
#define IMPROV_HANDLER_H

namespace ImprovHandler
{

void begin();
void loop();
void end();
bool isActive();
void busyCallback(const void *);

} // namespace ImprovHandler

#endif // IMPROV_HANDLER_H
