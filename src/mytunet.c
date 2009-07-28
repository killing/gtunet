#include "mytunet.h"

#include "ethcard.h"
#include "dot1x.h"
#include "tunet.h"
#include "userconfig.h"
#include "util.h"
#include "md5.h"
#include "logs.h"
#include "setting.h"

void mytunet_init()
{
	os_init();
	setting_init();
	logs_init();
	userconfig_init();
	ethcard_init();
	dot1x_init();
	tunet_init();
	
}

void mytunet_cleanup()
{
	dot1x_cleanup();
	tunet_cleanup();
	ethcard_cleanup();
	userconfig_cleanup();
	logs_cleanup();
	setting_cleanup();
	os_cleanup();
}
