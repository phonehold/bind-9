/*
 * Copyright (C) 2012  Internet Systems Consortium, Inc. ("ISC")
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/* $Id$ */

#ifndef DNS_RRL_H
#define DNS_RRL_H 1

/*
 * Rate limit DNS responses.
 */

#include <isc/lang.h>

#include <dns/fixedname.h>
#include <dns/rdata.h>
#include <dns/types.h>

ISC_LANG_BEGINDECLS


#define DNS_RRL_LOG_FAIL	ISC_LOG_WARNING
#define DNS_RRL_LOG_DROP	ISC_LOG_INFO
#define DNS_RRL_LOG_DEBUG1	ISC_LOG_DEBUG(3)
#define DNS_RRL_LOG_DEBUG2	ISC_LOG_DEBUG(4)
#define DNS_RRL_LOG_DEBUG3	ISC_LOG_DEBUG(9)

#define DNS_RRL_LOG_WS_BUF_LEN	    sizeof("would still")
#define DNS_RRL_LOG_CLIENT_BUF_LEN  (ISC_NETADDR_FORMATSIZE + \
				     sizeof(" for IN ") + sizeof(" "))


typedef struct dns_rrl_hash dns_rrl_hash_t;

/*
 * A rate limit bucket key.
 * This should be small to limit the total size of the database.
 */
typedef isc_uint8_t dns_rrl_kflags_t;
typedef struct dns_rrl_key dns_rrl_key_t;
struct dns_rrl_key {
	isc_uint32_t	    ip[4];	/* IP address */
	unsigned int	    name;	/* hash of DNS name */
	dns_rdatatype_t	    qtype;	/* query type */
	dns_rrl_kflags_t    kflags;
# define DNS_RRL_KFLAG_USED_TCP   0x01
# define DNS_RRL_KFLAG_NXDOMAIN	    0x02
# define DNS_RRL_KFLAG_ERROR	    0x04
# define DNS_RRL_KFLAG_NOT_IN	    0x08
# define DNS_RRL_KFLAG_IPV6	    0x08
};

/*
 * A rate-limit entry.
 * This should be small to limit the total size of the database.
 * With gcc on ARM, the key should have __attribute((__packed__)) to
 *	avoid padding to a multiple of 8 bytes.
 */
typedef struct dns_rrl_entry dns_rrl_entry_t;
typedef ISC_LIST(dns_rrl_entry_t) dns_rrl_bin_t;
struct dns_rrl_entry {
	ISC_LINK(dns_rrl_entry_t) lru;
	ISC_LINK(dns_rrl_entry_t) hlink;
	dns_rrl_bin_t	*bin;
	isc_stdtime_t	last_used;
	isc_int32_t	responses;
# define DNS_RRL_MAX_WINDOW	600
# define DNS_RRL_MAX_RATE	(ISC_INT32_MAX / DNS_RRL_MAX_WINDOW)
	dns_rrl_key_t	key;
	isc_uint8_t	slip_cnt;
	isc_uint8_t	log_secs;
# define DNS_RRL_MAX_LOG_SECS	60
};

/*
 * A hash table of rate-limit entries.
 */
struct dns_rrl_hash {
	isc_stdtime_t	check_time;
	int		length;
	dns_rrl_bin_t	bins[1];
};

/*
 * A block of rate-limit entries.
 */
typedef struct dns_rrl_block dns_rrl_block_t;
struct dns_rrl_block {
	ISC_LINK(dns_rrl_block_t) link;
	int		size;
	dns_rrl_entry_t	entries[1];
};

/*
 * Per-view query rate limit parameters and a pointer to database.
 */
typedef struct dns_rrl dns_rrl_t;
struct dns_rrl {
	isc_mutex_t	lock;
	isc_mem_t	*mctx;

	isc_boolean_t	log_only;
	int		responses_per_second;
	int		errors_per_second;
	int		nxdomains_per_second;
	int		window;
	int		slip;
	double		qps_scale;
	int		max_entries;

	dns_acl_t	*exempt;

	int		num_entries;

	int		qps_responses;
	isc_stdtime_t	qps_time;
	double		qps;
	int		scaled_responses_per_second;
	int		scaled_errors_per_second;
	int		scaled_nxdomains_per_second;
	int		scaled_slip;

	unsigned int	probes;
	unsigned int	searches;

	ISC_LIST(dns_rrl_block_t) blocks;
	ISC_LIST(dns_rrl_entry_t) lru;

	dns_rrl_hash_t	*hash;
	dns_rrl_hash_t	*old_hash;

	int		ipv4_prefixlen;
	isc_uint32_t	ipv4_mask;
	int		ipv6_prefixlen;
	isc_uint32_t	ipv6_mask[4];
};

typedef enum {
	DNS_RRL_RESULT_OK,
	DNS_RRL_RESULT_DROP,
	DNS_RRL_RESULT_SLIP,
} dns_rrl_result_t;

dns_rrl_result_t
dns_rrl(dns_view_t *view, const isc_sockaddr_t *client_addr,
	dns_rdataclass_t rdclass, dns_rdatatype_t qtype,
	dns_name_t *fname, dns_rcode_t rcode, isc_stdtime_t now,
	isc_boolean_t wouldlog, isc_boolean_t is_tcp,
	char *log_ws_buf, int log_ws_buf_len,
	char *log_client_buf, int log_client_buf_len,
	char *fname_buf, int fname_buf_len);

void
dns_rrl_view_destroy(dns_view_t *view);

isc_result_t
dns_rrl_init(dns_rrl_t **rrlp, dns_view_t *view, int min_entries);

ISC_LANG_ENDDECLS

#endif /* DNS_RRL_H */
