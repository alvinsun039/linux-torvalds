// SPDX-License-Identifier: GPL-2.0-only
/*
* Virtio vcpu stall device driver
*
* This module can be used to detect a stall in any cpu
* in a virtual machine and notify external devices of the stall.
*
* Based on the transmitted information, the device can
* determine which cpu has a stall and resume the VM.
*
*  Copyright (C) Kylin Software. 2023
*/

#include <linux/cpu.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/kernel.h>

#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/nmi.h>
#include <uapi/linux/virtio_ids.h>
#include <linux/virtio_config.h>
#include <linux/param.h>
#include <linux/percpu.h>
#include <linux/slab.h>

#define VCPU_STALL_REG_STATUS		(0x00)
#define VCPU_STALL_REG_LOAD_CNT		(0x04)
#define VCPU_STALL_REG_CURRENT_CNT	(0x08)
#define VCPU_STALL_REG_CLOCK_FREQ_HZ	(0x0C)
#define VCPU_STALL_REG_LEN		(0x10)
#define VCPU_STALL_REG_TIMEOUT_SEC	(0x14)

#define VCPU_STALL_DEFAULT_CLOCK_HZ	(10)
#define VCPU_STALL_MAX_CLOCK_HZ		(100)
#define VCPU_STALL_DEFAULT_TIMEOUT_SEC	(8)
#define VCPU_STALL_MAX_TIMEOUT_SEC	(600)

struct vcpu_stall_detect_config {
	u32 clock_freq_hz;
	u32 stall_timeout_sec;

	enum cpuhp_state hp_online;
};

struct vcpu_stall_priv {
	struct hrtimer vcpu_hrtimer;
	struct virtio_device *vdev;
	u32 cpu_id;
};

struct vcpu_stall {
	struct vcpu_stall_priv *priv;
	struct virtqueue *vq;
	spinlock_t lock;
	struct pet_event {
		u32 cpu_id;
		bool is_initialized;
		u32 ticks;
	} pet_event;
};

static const struct virtio_device_id vcpu_stall_id_table[] = {
	{ VIRTIO_ID_WATCHDOG, VIRTIO_DEV_ANY_ID },
	{ 0, },
};

/* The vcpu stall configuration structure which applies to all the CPUs */
static struct vcpu_stall_detect_config vcpu_stall_config;
static struct vcpu_stall *vcpu_stall;

static struct vcpu_stall_priv __percpu *vcpu_stall_detectors;

static enum hrtimer_restart
vcpu_stall_detect_timer_fn(struct hrtimer *hrtimer)
{
	u32 ticks, ping_timeout_ms;
	struct scatterlist sg;
	int unused, err = 0;

	struct vcpu_stall_priv *vcpu_stall_detector =
		this_cpu_ptr(vcpu_stall->priv);

	/* Reload the stall detector counter register every
	 * `ping_timeout_ms` to prevent the virtual device
	 * from decrementing it to 0. The virtual device decrements this
	 * register at 'clock_freq_hz' frequency.
	 */
	ticks = vcpu_stall_config.clock_freq_hz *
				vcpu_stall_config.stall_timeout_sec;

	spin_lock(&vcpu_stall->lock);
	while (virtqueue_get_buf(vcpu_stall->vq, &unused))
		;
	vcpu_stall->pet_event.ticks = cpu_to_virtio32(vcpu_stall_detector->vdev, ticks);
	vcpu_stall->pet_event.is_initialized = true;
	vcpu_stall->pet_event.cpu_id = vcpu_stall_detector->cpu_id;

	sg_init_one(&sg, &vcpu_stall->pet_event, sizeof(vcpu_stall->pet_event));
	err = virtqueue_add_outbuf(vcpu_stall->vq, &sg, 1, vcpu_stall, GFP_ATOMIC);
	if (!err)
		virtqueue_kick(vcpu_stall->vq);
	else
		pr_err("cpu:%d failed to add outbuf, err:%d\n", vcpu_stall_detector->cpu_id, err);

	spin_unlock(&vcpu_stall->lock);

	ping_timeout_ms = vcpu_stall_config.stall_timeout_sec *
			  MSEC_PER_SEC / 2;
	hrtimer_forward_now(hrtimer,
			    ms_to_ktime(ping_timeout_ms));
	return HRTIMER_RESTART;
}

static int start_stall_detector_cpu(unsigned int cpu)
{
	u32 ticks, ping_timeout_ms;
	struct scatterlist sg;
	struct hrtimer *vcpu_hrtimer;
	int err = 0;

	struct vcpu_stall_priv *vcpu_stall_detector =
		this_cpu_ptr(vcpu_stall->priv);

	vcpu_stall_detector->cpu_id = cpu;

	vcpu_hrtimer = &vcpu_stall_detector->vcpu_hrtimer;

	/* Compute the number of ticks required for the stall detector
	 * counter register based on the internal clock frequency and the
	 * timeout value given from the device tree.
	 */
	ticks = vcpu_stall_config.clock_freq_hz *
		vcpu_stall_config.stall_timeout_sec;
	vcpu_stall->pet_event.ticks = cpu_to_virtio32(vcpu_stall_detector->vdev, ticks);

	/* Pet the stall detector at half of its expiration timeout
	 * to prevent spurious resets.
	 */
	ping_timeout_ms = vcpu_stall_config.stall_timeout_sec *
			  MSEC_PER_SEC / 2;

	hrtimer_init(vcpu_hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	vcpu_hrtimer->function = vcpu_stall_detect_timer_fn;

	vcpu_stall->pet_event.is_initialized = true;

	spin_lock(&vcpu_stall->lock);
	vcpu_stall->pet_event.cpu_id = cpu;
	sg_init_one(&sg, &vcpu_stall->pet_event, sizeof(vcpu_stall->pet_event));
	err = virtqueue_add_outbuf(vcpu_stall->vq, &sg, 1, vcpu_stall, GFP_ATOMIC);
	if (!err)
		virtqueue_kick(vcpu_stall->vq);

	spin_unlock(&vcpu_stall->lock);

	hrtimer_start(vcpu_hrtimer, ms_to_ktime(ping_timeout_ms),
		      HRTIMER_MODE_REL_PINNED);
	return err;
}

static int stop_stall_detector_cpu(unsigned int cpu)
{
	int err = 0;
	struct scatterlist sg;

	struct vcpu_stall_priv *vcpu_stall_detector =
		per_cpu_ptr(vcpu_stall_detectors, cpu);

	/* Disable the stall detector for the current CPU */
	hrtimer_cancel(&vcpu_stall_detector->vcpu_hrtimer);
	vcpu_stall->pet_event.is_initialized = false;
	vcpu_stall->pet_event.cpu_id = cpu;

	spin_lock(&vcpu_stall->lock);
	sg_init_one(&sg, &vcpu_stall->pet_event, sizeof(vcpu_stall->pet_event));
	err = virtqueue_add_outbuf(vcpu_stall->vq, &sg, 1, vcpu_stall, GFP_ATOMIC);
	if (!err)
		virtqueue_kick(vcpu_stall->vq);

	spin_unlock(&vcpu_stall->lock);

	return err;
}

static int vcpu_stall_detect_probe(struct virtio_device *vdev)
{
	int ret, cpu;
	u32 clock_freq_hz = VCPU_STALL_DEFAULT_CLOCK_HZ;
	u32 stall_timeout_sec = VCPU_STALL_DEFAULT_TIMEOUT_SEC;

	vcpu_stall = kzalloc(sizeof(struct vcpu_stall), GFP_KERNEL);
	if (!vcpu_stall) {
		ret = -ENOMEM;
		goto err;
	}
	vdev->priv = vcpu_stall;

	vcpu_stall->priv = devm_alloc_percpu(&vdev->dev,
					     typeof(struct vcpu_stall_priv));
	if (!vcpu_stall->priv) {
		ret = -ENOMEM;
		goto failed_priv;
	}

	for_each_possible_cpu(cpu) {
		struct vcpu_stall_priv *priv;

		priv = per_cpu_ptr(vcpu_stall->priv, cpu);
		priv->vdev = vdev;
	}

	ret = virtio_cread_feature(vdev, VCPU_STALL_REG_CLOCK_FREQ_HZ,
				   struct vcpu_stall_detect_config, clock_freq_hz,
				   &clock_freq_hz);
	if (ret || !clock_freq_hz) {
		if (!(clock_freq_hz > 0 &&
		      clock_freq_hz < VCPU_STALL_MAX_CLOCK_HZ)) {
			dev_warn(&vdev->dev, "clk out of range\n");
			clock_freq_hz = VCPU_STALL_DEFAULT_CLOCK_HZ;
		}
	}
	ret = virtio_cread_feature(vdev, VCPU_STALL_REG_TIMEOUT_SEC,
				   struct vcpu_stall_detect_config, stall_timeout_sec,
				   &stall_timeout_sec);
	if (ret || !stall_timeout_sec) {
		if (!(stall_timeout_sec > 0 &&
		      stall_timeout_sec < VCPU_STALL_MAX_TIMEOUT_SEC)) {
			dev_warn(&vdev->dev, "stall timeout out of range\n");
			stall_timeout_sec = VCPU_STALL_DEFAULT_TIMEOUT_SEC;
		}
	}

	vcpu_stall_config = (struct vcpu_stall_detect_config) {
		.clock_freq_hz		= clock_freq_hz,
		.stall_timeout_sec	= stall_timeout_sec
	};

	/* find virtqueue for guest to send pet event to host */
	vcpu_stall->vq = virtio_find_single_vq(vdev, NULL, "pet-event");
	if (IS_ERR(vcpu_stall->vq)) {
		dev_err(&vdev->dev, "failed to find vq\n");
		goto failed_priv;
	}

	spin_lock_init(&vcpu_stall->lock);

	ret = cpuhp_setup_state(CPUHP_AP_ONLINE_DYN,
				"virt/vcpu_stall_detector:online",
				start_stall_detector_cpu,
				stop_stall_detector_cpu);
	if (ret < 0) {
		dev_err(&vdev->dev, "failed to install cpu hotplug\n");
		goto failed_priv;
	}

	vcpu_stall_config.hp_online = ret;
	return 0;

failed_priv:
	kfree(vcpu_stall);
err:
	return ret;
}

static void vcpu_stall_detect_remove(struct virtio_device *vdev)
{
	int cpu;

	cpuhp_remove_state(vcpu_stall_config.hp_online);

	for_each_possible_cpu(cpu)
		stop_stall_detector_cpu(cpu);
}

static unsigned int features_legacy[] = {
	VCPU_STALL_REG_STATUS, VCPU_STALL_REG_LOAD_CNT, VCPU_STALL_REG_CURRENT_CNT,
	VCPU_STALL_REG_CLOCK_FREQ_HZ, VCPU_STALL_REG_LEN, VCPU_STALL_REG_TIMEOUT_SEC
};

static unsigned int features[] = {
	VCPU_STALL_REG_STATUS, VCPU_STALL_REG_LOAD_CNT, VCPU_STALL_REG_CURRENT_CNT,
	VCPU_STALL_REG_CLOCK_FREQ_HZ, VCPU_STALL_REG_LEN, VCPU_STALL_REG_TIMEOUT_SEC
};

static struct virtio_driver vcpu_stall_detect_driver = {
	.feature_table	= features,
	.feature_table_size = ARRAY_SIZE(features),
	.feature_table_legacy	= features_legacy,
	.feature_table_size_legacy	= ARRAY_SIZE(features_legacy),
	.driver.name	= KBUILD_MODNAME,
	.driver.owner	= THIS_MODULE,
	.id_table =	vcpu_stall_id_table,
	.probe  = vcpu_stall_detect_probe,
	.remove = vcpu_stall_detect_remove,
};

module_virtio_driver(vcpu_stall_detect_driver);

MODULE_LICENSE("GPL");
MODULE_DEVICE_TABLE(virtio, vcpu_stall_id_table);
MODULE_AUTHOR("zhanghao1 <zhanghao1@kylinos.cn>");
MODULE_DESCRIPTION("VCPU stall detector");
