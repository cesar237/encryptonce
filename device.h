/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2015-2019 Jason A. Donenfeld <Jason@zx2c4.com>. All Rights Reserved.
 */

#ifndef _WG_DEVICE_H
#define _WG_DEVICE_H

#include "noise.h"
#include "allowedips.h"
#include "peerlookup.h"
#include "cookie.h"

#include <linux/types.h>
#include <linux/netdevice.h>
#include <linux/workqueue.h>
#include <linux/mutex.h>
#include <linux/net.h>
#include <linux/ptr_ring.h>

struct wg_device;

struct multicore_worker {
	void *ptr;
	struct work_struct work;
};

extern int wg_batch_size;
extern int wg_nr_rings;

#ifdef DEFINE_WG_PARAMS

/* This section will only be compiled once in a specific source file */
int wg_batch_size = 4;
module_param(wg_batch_size, int, 0644);
MODULE_PARM_DESC(wg_batch_size, "Batch size for wireguard operations");

int wg_nr_rings = num_online_cpus();
module_param(wg_nr_rings, int, 0644);
MODULE_PARM_DESC(wg_nr_rings, "Number of ring buffers");

#endif

struct crypt_queue {
	struct ptr_ring ring[NR_RINGS];
	struct multicore_worker __percpu *worker;
	int last_cpu;
};

struct prev_queue {
	struct sk_buff *head, *tail, *peeked;
	struct { struct sk_buff *next, *prev; } empty; // Match first 2 members of struct sk_buff.
	atomic_t count;
};

struct wg_device {
	struct net_device *dev;
	struct crypt_queue encrypt_queue, decrypt_queue, handshake_queue;
	struct sock __rcu *sock4, *sock6;
	struct net __rcu *creating_net;
	struct noise_static_identity static_identity;
	struct workqueue_struct *packet_crypt_wq,*handshake_receive_wq, *handshake_send_wq;
	struct cookie_checker cookie_checker;
	struct pubkey_hashtable *peer_hashtable;
	struct index_hashtable *index_hashtable;
	struct allowedips peer_allowedips;
	struct mutex device_update_lock, socket_update_lock;
	struct list_head device_list, peer_list;
	atomic_t handshake_queue_len;
	unsigned int num_peers, device_update_gen;
	u32 fwmark;
	u16 incoming_port;
};

int wg_device_init(void);
void wg_device_uninit(void);

#endif /* _WG_DEVICE_H */
