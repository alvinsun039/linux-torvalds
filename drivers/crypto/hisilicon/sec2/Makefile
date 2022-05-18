obj-$(CONFIG_CRYPTO_DEV_HISI_SEC2) += hisi_sec2.o
CFLAGS_sec_main.o += $(call cc-option,-mno-outline-atomics)
CFLAGS_sec_crypto.o += $(call cc-option,-mno-outline-atomics)
hisi_sec2-objs = sec_main.o sec_crypto.o
