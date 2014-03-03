#pragma once

#include <errno.h>

#define errno2error(errno) (-(100 + (errno)))

enum dnssec_error {
	DNSSEC_EOK = 0,

	/* Directly mapped error codes. */
	DNSSEC_ENOMEM = errno2error(ENOMEM),
	DNSSEC_EINVAL = errno2error(EINVAL),
//	DNSSEC_ENOTSUP = errno2error(ENOTSUP),
//	DNSSEC_EBUSY = errno2error(EBUSY),
//	DNSSEC_EAGAIN = errno2error(EAGAIN),
//	DNSSEC_EACCES = errno2error(EACCES),
//	DNSSEC_ECONNREFUSED = errno2error(ECONNREFUSED),
//	DNSSEC_EISCONN = errno2error(EISCONN),
//	DNSSEC_EADDRINUSE = errno2error(EADDRINUSE),
//	DNSSEC_ENOENT = errno2error(ENOENT),
//	DNSSEC_ERANGE = errno2error(ERANGE),

	DNSSEC_ERROR = -1000,
};

#undef errno2error