// SPDX-License-Identifier: GPL-2.0
/* Copyright (c) 2024 Meta Platforms, Inc. and affiliates. */
#include <test_progs.h>
#include <time.h>

#include <sys/epoll.h>

#include "struct_ops_module.skel.h"
#include "struct_ops_detach.skel.h"

static void check_map_info(struct bpf_map_info *info)
{
	struct bpf_btf_info btf_info;
	char btf_name[256];
	u32 btf_info_len = sizeof(btf_info);
	int err, fd;

	fd = bpf_btf_get_fd_by_id(info->btf_vmlinux_id);
	if (!ASSERT_GE(fd, 0, "get_value_type_btf_obj_fd"))
		return;

	memset(&btf_info, 0, sizeof(btf_info));
	btf_info.name = ptr_to_u64(btf_name);
	btf_info.name_len = sizeof(btf_name);
	err = bpf_btf_get_info_by_fd(fd, &btf_info, &btf_info_len);
	if (!ASSERT_OK(err, "get_value_type_btf_obj_info"))
		goto cleanup;

	if (!ASSERT_EQ(strcmp(btf_name, "bpf_testmod"), 0, "get_value_type_btf_obj_name"))
		goto cleanup;

cleanup:
	close(fd);
}

static void test_struct_ops_load(void)
{
	struct struct_ops_module *skel;
	struct bpf_map_info info = {};
	struct bpf_link *link;
	int err;
	u32 len;

	skel = struct_ops_module__open();
	if (!ASSERT_OK_PTR(skel, "struct_ops_module_open"))
		return;

	skel->struct_ops.testmod_1->data = 13;
	skel->struct_ops.testmod_1->test_2 = skel->progs.test_3;
	/* Since test_2() is not being used, it should be disabled from
	 * auto-loading, or it will fail to load.
	 */
	bpf_program__set_autoload(skel->progs.test_2, false);

	err = struct_ops_module__load(skel);
	if (!ASSERT_OK(err, "struct_ops_module_load"))
		goto cleanup;

	len = sizeof(info);
	err = bpf_map_get_info_by_fd(bpf_map__fd(skel->maps.testmod_1), &info,
				     &len);
	if (!ASSERT_OK(err, "bpf_map_get_info_by_fd"))
		goto cleanup;

	link = bpf_map__attach_struct_ops(skel->maps.testmod_1);
	ASSERT_OK_PTR(link, "attach_test_mod_1");

	/* test_3() will be called from bpf_dummy_reg() in bpf_testmod.c
	 *
	 * In bpf_testmod.c it will pass 4 and 13 (the value of data) to
	 * .test_2.  So, the value of test_2_result should be 20 (4 + 13 +
	 * 3).
	 */
	ASSERT_EQ(skel->bss->test_2_result, 20, "check_shadow_variables");

	bpf_link__destroy(link);

	check_map_info(&info);

cleanup:
	struct_ops_module__destroy(skel);
}

/* Detach a link from a user space program */
static void test_detach_link(void)
{
	struct epoll_event ev, events[2];
	struct struct_ops_detach *skel;
	struct bpf_link *link = NULL;
	int fd, epollfd = -1, nfds;
	int err;

	skel = struct_ops_detach__open_and_load();
	if (!ASSERT_OK_PTR(skel, "struct_ops_detach__open_and_load"))
		return;

	link = bpf_map__attach_struct_ops(skel->maps.testmod_do_detach);
	if (!ASSERT_OK_PTR(link, "attach_struct_ops"))
		goto cleanup;

	fd = bpf_link__fd(link);
	if (!ASSERT_GE(fd, 0, "link_fd"))
		goto cleanup;

	epollfd = epoll_create1(0);
	if (!ASSERT_GE(epollfd, 0, "epoll_create1"))
		goto cleanup;

	ev.events = EPOLLHUP;
	ev.data.fd = fd;
	err = epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);
	if (!ASSERT_OK(err, "epoll_ctl"))
		goto cleanup;

	err = bpf_link__detach(link);
	if (!ASSERT_OK(err, "detach_link"))
		goto cleanup;

	/* Wait for EPOLLHUP */
	nfds = epoll_wait(epollfd, events, 2, 500);
	if (!ASSERT_EQ(nfds, 1, "epoll_wait"))
		goto cleanup;

	if (!ASSERT_EQ(events[0].data.fd, fd, "epoll_wait_fd"))
		goto cleanup;
	if (!ASSERT_TRUE(events[0].events & EPOLLHUP, "events[0].events"))
		goto cleanup;

cleanup:
	if (epollfd >= 0)
		close(epollfd);
	bpf_link__destroy(link);
	struct_ops_detach__destroy(skel);
}

void serial_test_struct_ops_module(void)
{
	if (test__start_subtest("test_struct_ops_load"))
		test_struct_ops_load();
	if (test__start_subtest("test_detach_link"))
		test_detach_link();
}

