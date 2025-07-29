/* SPDX-License-Identifier: GPL-2.0 */
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>

/*** constants ***/
#define SYSFS_PWM_DIR "/sys/class/pwm"
#define GB02MAC2043 64

/*** PWM functions ***/
/* PWM export */
static int GB02FUNC1556(unsigned int pwm, unsigned char pwmno);
/* PWM unexport */
static int GB02FUNC1559(unsigned int pwm, unsigned char pwmno);
/* PWM configuration */
static int pwm_config(unsigned int pwm, unsigned char pwmno, unsigned int period, unsigned int duty_cycle);
/* PWM enable */
static int pwm_enable(unsigned int pwm, unsigned char pwmno, unsigned char enable);
/* PWM disable */
