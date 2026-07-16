#ifndef TEST_BOOTSTRAP_H
#define TEST_BOOTSTRAP_H

#include "CmsNanoAod.hxx"

extern nanoaod::Event* g_event;

void cms_nano_aod_config_event(nanoaod::Event* e);
void cms_nano_aod_bootstrap();
int main(int argc, char** argv);
#endif
