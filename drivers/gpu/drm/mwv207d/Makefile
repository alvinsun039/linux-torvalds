
ccflags-y += -I$(src)

mwv207d-y += \
	mwv207d_drv.o \
	mwv207d_monitor.o \
	mwv207d_irq.o \
	mwv207d_db.o \
	mwv207d_ctx.o \
	mwv207d_submit.o \
	mwv207d_sched.o \
	mwv207d_pipe_2d.o \
	mwv207d_pipe_3d.o \
	mwv207d_pipe_codec.o \
	mwv207d_gem.o \
	mwv207d_bo.o \
	mwv207d_ttm.o \
	mwv207d_hw.o \
	mwv207d_vm.o \
	mwv207d_vbios.o \
	mwv207d_debugfs.o \
	mwv207d_info.o \
	mwv207d_gart.o

mwv207d-y += \
	dc/mwv207d_kms.o \
	dc/mwv207d_vkms.o \
	dc/mwv207d_fb.o \
	dc/mwv207d_va.o \
	dc/mwv207d_audio.o \
	dc/mwv207d_edp.o \
	dc/mwv207d_hdmi.o \
	dc/mwv207d_vga.o \
	dc/mwv207d_dvo.o \
	dc/mwv207d_vi.o \
	dc/mwv207d_i2c.o

mwv207d-y += \
	selftest/selftest.o

obj-$(CONFIG_DRM_MWV207D) := mwv207d.o

