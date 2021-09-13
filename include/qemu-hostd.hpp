#ifndef __QEMU_HOSTD_HPP_
#define __QEMU_HOSTD_HPP

#include <iostream>
#include <string.h>
#include <signal.h>
#include <json11.hpp>
#include <hiredis/hiredis.h>
#include <hiredis/async.h>
#include <hiredis/adapters/libevent.h>
#include <qemu-hypervisor.hpp>
#include <qemu-link.hpp>
#include <thread>

#define QEMU_DEFAULT_REDIS "10.0.94.254"

void onLaunchMessage(json11::Json json);
void onActivationMessage(redisAsyncContext *c, void *reply, void *privdata);

#endif