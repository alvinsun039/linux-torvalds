// SPDX-License-Identifier: GPL-2.0

#include <linux/clk.h>
#include <linux/console.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/serial_core.h>
#include <linux/serial_reg.h>
#include <linux/slab.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include "gpu/gb_device.h"
#include "gb02_usart.h"

#define GB02MAC2634 200

/*
 * Register offsets
 */

/* TXDATA */
#define GB02MAC2637		0x0
#define GB02MAC2638		31
#define GB02MAC2639		(1 << GB02MAC2638)
#define GB02MAC2640		0
#define GB02MAC2641		(0xff << GB02MAC2640)

/* RXDATA */
#define GB02MAC2642		0x4
#define GB02MAC2643	31
#define GB02MAC2644		(1 << GB02MAC2643)
#define GB02MAC2645		0
#define GB02MAC2646		(0xff << GB02MAC2645)

/* TXCTRL */
#define GB02MAC2647		0x8
#define GB02MAC2648	16
#define GB02MAC2649		(0x7 << GB02MAC2648)
#define GB02MAC2650	1
#define GB02MAC2651		(1 << GB02MAC2650)
#define GB02MAC2652		0
#define GB02MAC2653		(1 << GB02MAC2652)

/* RXCTRL */
#define GB02MAC2654		0xC
#define GB02MAC2655	16
#define GB02MAC2656		(0x7 << GB02MAC2648)
#define GB02MAC2657		0
#define GB02MAC2658		(1 << GB02MAC2657)

/* IE */
#define GB02MAC2659			0x10
#define GB02MAC2660		1
#define GB02MAC2661		(1 << GB02MAC2660)
#define GB02MAC2662		0
#define GB02MAC2663		(1 << GB02MAC2662)

/* IP */
#define GB02MAC2664			0x14
#define GB02MAC2665		1
#define GB02MAC2666		(1 << GB02MAC2665)
#define GB02MAC2667		0
#define GB02MAC2668		(1 << GB02MAC2667)

/* DIV */
#define GB02MAC2669			0x18
#define GB02MAC2670		0
#define GB02MAC2671		(0xffff << GB02_SERIAL_IP_DIV_SHIFT)

/* SETUP */
#define GB02MAC2672		0x1c

/*
 * Config macros
 */

/*
 * GB02MAC2673: maximum number of UARTs on a device that can
 *						  host a serial console
 */
#define GB02MAC2673			2

/*
 * GB02MAC2674: default baud rate that the driver should
 *						   configure itself to use
 */
#define GB02MAC2674		115200

/* GB02_SERIAL_NAME: our driver's name that we pass to the operating system */
#define GB02_SERIAL_NAME			"gb02-mcu-uart"

/* GB02_TTY_PREFIX: tty name prefix for GB02 serial ports */
#define GB02_TTY_PREFIX			"ttyGB"

/* GB02MAC2675: depth of the TX FIFO (in bytes) */
#define GB02MAC2675			8

/* GB02MAC2676: depth of the TX FIFO (in bytes) */
#define GB02MAC2676			8

#if (GB02MAC2675 != GB02MAC2676)
#error Driver does not support configurations with different TX, RX FIFO sizes
#endif

/*
 *
 */

/**
 * GB02STR203 - driver-specific data extension to struct uart_port
 * @port: struct uart_port embedded in this struct
 * @dev: struct device *
 * @ier: shadowed copy of the interrupt enable register
 * @clkin_rate: input clock to the UART IP block.
 * @baud_rate: UART serial line rate (e.g., 115200 baud)
 * @clk_notifier: clock rate change notifier for upstream clock changes
 *
 * Configuration data specific to this GB02 UART.
 */
struct GB02STR203 {
	void __iomem *sysctl_iofunc_base; // bar0
	void __iomem *sysctl_cfg_base;	  // bar4
	struct uart_port	port;
	struct device		*dev;
	unsigned char		ier;
	unsigned long		clkin_rate;
	unsigned long		baud_rate;
	bool task_run;
	struct task_struct *uart_task;
};

/*
 * Structure container-of macros
 */

#define port_to_gb02_serial_port(p) (container_of((p), \
							struct GB02STR203, \
							port))

/*
 * Internal functions
 */
static void GB02FUNC1799(struct uart_port *port);

/**
 * GB02FUNC1765() - write to a GB02 serial port register (early)
 * @port: pointer to a struct uart_port record
 * @offs: register address offset from the IP block base address
 * @v: value to write to the register
 *
 * Given a pointer @port to a struct uart_port record, write the value
 * @v to the IP block register address offset @offs.  This function is
 * intended for early console use.
 *
 * Context: Intended to be used only by the earlyconsole code.
 */
static void GB02FUNC1765(u32 v, u16 offs, struct uart_port *port)
{
	writel_relaxed(v, port->membase + offs);
}

/**
 * GB02FUNC1767() - read from a GB02 serial port register (early)
 * @port: pointer to a struct uart_port record
 * @offs: register address offset from the IP block base address
 *
 * Given a pointer @port to a struct uart_port record, read the
 * contents of the IP block register located at offset @offs from the
 * IP block base and return it.  This function is intended for early
 * console use.
 *
 * Context: Intended to be called only by the earlyconsole code or by
 *		  GB02FUNC1770() or GB02FUNC1768() (in this driver)
 *
 * Returns: the register value read from the UART.
 */
static u32 GB02FUNC1767(struct uart_port *port, u16 offs)
{
	return readl_relaxed(port->membase + offs);
}

/**
 * GB02FUNC1768() - write to a GB02 serial port register
 * @v: value to write to the register
 * @offs: register address offset from the IP block base address
 * @gsp: pointer to a struct GB02STR203 record
 *
 * Write the value @v to the IP block register located at offset @offs from the
 * IP block base, given a pointer @gsp to a struct GB02STR203 record.
 *
 * Context: Any context.
 */
static void GB02FUNC1768(u32 v, u16 offs, struct GB02STR203 *gsp)
{
	GB02FUNC1765(v, offs, &gsp->port);
}

/**
 * GB02FUNC1770() - read from a GB02 serial port register
 * @gsp: pointer to a struct GB02STR203 record
 * @offs: register address offset from the IP block base address
 *
 * Read the contents of the IP block register located at offset @offs from the
 * IP block base, given a pointer @gsp to a struct GB02STR203 record.
 *
 * Context: Any context.
 *
 * Returns: the value of the UART register
 */
static u32 GB02FUNC1770(struct GB02STR203 *gsp, u16 offs)
{
	return GB02FUNC1767(&gsp->port, offs);
}

/**
 * GB02FUNC1773() - is the TXFIFO full?
 * @gsp: pointer to a struct GB02STR203
 *
 * Read the transmit FIFO "full" bit, returning a non-zero value if the
 * TX FIFO is full, or zero if space remains.  Intended to be used to prevent
 * writes to the TX FIFO when it's full.
 *
 * Returns: GB02MAC2639 (non-zero) if the transmit FIFO
 * is full, or 0 if space remains.
 */
static int GB02FUNC1773(struct GB02STR203 *gsp)
{
	return GB02FUNC1770(gsp, GB02MAC2637) &
		   GB02MAC2639;
}

/**
 * GB02FUNC1774() - enqueue a byte to transmit onto the TX FIFO
 * @gsp: pointer to a struct GB02STR203
 * @ch: character to transmit
 *
 * Enqueue a byte @ch onto the transmit FIFO, given a pointer @gsp to the
 * struct GB02STR203 * to transmit on.  Caller should first check to
 * ensure that the TXFIFO has space; see GB02FUNC1773().
 *
 * Context: Any context.
 */
static void GB02FUNC1774(struct GB02STR203 *gsp, int ch)
{
	GB02FUNC1768(ch, GB02MAC2637, gsp);
}

/**
 * GB02FUNC1776() - enqueue multiple bytes onto the TX FIFO
 * @gsp: pointer to a struct GB02STR203
 *
 * Transfer up to a TX FIFO size's worth of characters from the Linux serial
 * transmit buffer to the GB02 UART TX FIFO.
 *
 * Context: Any context.  Expects @gsp->port.lock to be held by caller.
 */
static void GB02FUNC1776(struct GB02STR203 *gsp)
{
	struct circ_buf *xmit = &gsp->port.state->xmit;
	int count;

	if (gsp->port.x_char) {
		GB02FUNC1774(gsp, gsp->port.x_char);
		gsp->port.icount.tx++;
		gsp->port.x_char = 0;
		return;
	}
	if (uart_circ_empty(xmit) || uart_tx_stopped(&gsp->port)) {
		GB02FUNC1799(&gsp->port);
		return;
	}
	count = GB02MAC2675;
	do {
		GB02FUNC1774(gsp, xmit->buf[xmit->tail]);
		xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
		gsp->port.icount.tx++;
		if (uart_circ_empty(xmit))
			break;
	} while (--count > 0);

	if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
		uart_write_wakeup(&gsp->port);

	if (uart_circ_empty(xmit))
		GB02FUNC1799(&gsp->port);
}

/**
 * GB02FUNC1781() - enable transmit watermark interrupts
 * @gsp: pointer to a struct GB02STR203
 *
 * Enable interrupt generation when the transmit FIFO watermark is reached
 * on the GB02 UART referred to by @gsp.
 */
static void GB02FUNC1781(struct GB02STR203 *gsp)
{
	if (gsp->ier & GB02MAC2663)
		return;

	gsp->ier |= GB02MAC2663;
	GB02FUNC1768(gsp->ier, GB02MAC2659, gsp);
}

/**
 * GB02FUNC1782() - enable receive watermark interrupts
 * @gsp: pointer to a struct GB02STR203
 *
 * Enable interrupt generation when the receive FIFO watermark is reached
 * on the GB02 UART referred to by @gsp.
 */
static void GB02FUNC1782(struct GB02STR203 *gsp)
{
	if (gsp->ier & GB02MAC2661)
		return;

	gsp->ier |= GB02MAC2661;
	GB02FUNC1768(gsp->ier, GB02MAC2659, gsp);
}

/**
 * GB02FUNC1784() - disable transmit watermark interrupts
 * @gsp: pointer to a struct GB02STR203
 *
 * Disable interrupt generation when the transmit FIFO watermark is reached
 * on the UART referred to by @gsp.
 */
static void GB02FUNC1784(struct GB02STR203 *gsp)
{
	if (!(gsp->ier & GB02MAC2663))
		return;

	gsp->ier &= ~GB02MAC2663;
	GB02FUNC1768(gsp->ier, GB02MAC2659, gsp);
}

/**
 * GB02FUNC1785() - disable receive watermark interrupts
 * @gsp: pointer to a struct GB02STR203
 *
 * Disable interrupt generation when the receive FIFO watermark is reached
 * on the UART referred to by @gsp.
 */
static void GB02FUNC1785(struct GB02STR203 *gsp)
{
	if (!(gsp->ier & GB02MAC2661))
		return;

	gsp->ier &= ~GB02MAC2661;
	GB02FUNC1768(gsp->ier, GB02MAC2659, gsp);
}

/**
 * GB02FUNC1786() - receive a byte from the UART
 * @gsp: pointer to a struct GB02STR203
 * @is_empty: char pointer to return whether the RX FIFO is empty
 *
 * Try to read a byte from the GB02 UART RX FIFO, referenced by
 * @gsp, and to return it.  Also returns the RX FIFO empty bit in
 * the char pointed to by @ch.  The caller must pass the byte back to the
 * Linux serial layer if needed.
 *
 * Returns: the byte read from the UART RX FIFO.
 */
static char GB02FUNC1786(struct GB02STR203 *gsp, char *is_empty)
{
	u32 v;
	u8 ch;

	v = GB02FUNC1770(gsp, GB02MAC2642);

	if (!is_empty)
		WARN_ON(1);
	else
		*is_empty = (v & GB02MAC2644) >>
					GB02MAC2643;

	ch = (v & GB02MAC2646) >>
		 GB02MAC2645;

	return ch;
}

/**
 * GB02FUNC1789() - receive multiple bytes from the UART
 * @gsp: pointer to a struct GB02STR203
 *
 * Receive up to an RX FIFO's worth of bytes from the GB02 UART referred
 * to by @gsp and pass them up to the Linux serial layer.
 *
 * Context: Expects gsp->port.lock to be held by caller.
 */
static void GB02FUNC1789(struct GB02STR203 *gsp)
{
	unsigned char ch;
	char is_empty;
	int c;

	for (c = GB02MAC2676; c > 0; --c) {
		ch = GB02FUNC1786(gsp, &is_empty);
		if (is_empty)
			break;

		gsp->port.icount.rx++;
		uart_insert_char(&gsp->port, 0, 0, ch, TTY_NORMAL);
	}

	//spin_unlock(&gsp->port.lock);
	tty_flip_buffer_push(&gsp->port.state->port);
	//spin_lock(&gsp->port.lock);
}

/**
 * GB02FUNC1791() - calculate the divisor setting by the line rate
 * @gsp: pointer to a struct GB02STR203
 *
 * Calculate the appropriate value of the clock divisor for the UART
 * and target line rate referred to by @gsp and write it into the
 * hardware.
 */
static void GB02FUNC1791(struct GB02STR203 *gsp)
{
	u16 div;

	div = DIV_ROUND_UP(gsp->clkin_rate, gsp->baud_rate) - 1;

	GB02FUNC1768(div, GB02MAC2669, gsp);
}

/**
 * GB02FUNC1792() - set the UART "baud rate"
 * @gsp: pointer to a struct GB02STR203
 * @rate: new target bit rate
 *
 * Calculate the UART divisor value for the target bit rate @rate for the
 * GB02 UART described by @gsp and program it into the UART.  There may
 * be some error between the target bit rate and the actual bit rate implemented
 * by the UART due to clock ratio granularity.
 */
static void GB02FUNC1792(struct GB02STR203 *gsp,
								 unsigned int rate)
{
	if (gsp->baud_rate == rate)
		return;

	gsp->baud_rate = rate;
	GB02FUNC1791(gsp);
}

/**
 * GB02FUNC1795() - set the number of stop bits
 * @gsp: pointer to a struct GB02STR203
 * @nstop: 1 or 2 (stop bits)
 *
 * Program the GB02 UART referred to by @gsp to use @nstop stop bits.
 */
static void GB02FUNC1795(struct GB02STR203 *gsp, char nstop)
{
	u32 v;

	if (nstop < 1 || nstop > 2) {
		WARN_ON(1);
		return;
	}

	v = GB02FUNC1770(gsp, GB02MAC2647);
	v &= ~GB02MAC2651;
	v |= (nstop - 1) << GB02MAC2650;
	GB02FUNC1768(v, GB02MAC2647, gsp);
}

/**
 * GB02FUNC1797() - wait for an empty slot on the TX FIFO
 * @gsp: pointer to a struct GB02STR203
 *
 * Delay while the UART TX FIFO referred to by @gsp is marked as full.
 *
 * Context: Any context.
 */
static void __maybe_unused GB02FUNC1797(struct GB02STR203 *gsp)
{
	while (GB02FUNC1773(gsp))
		udelay(1); /* XXX Could probably be more intelligent here */
}

/*
 * Linux serial API functions
 */

static void GB02FUNC1799(struct uart_port *port)
{
	struct GB02STR203 *gsp = port_to_gb02_serial_port(port);

	GB02FUNC1784(gsp);
}

static void GB02FUNC1801(struct uart_port *port)
{
	struct GB02STR203 *gsp = port_to_gb02_serial_port(port);

	GB02FUNC1785(gsp);
}

static void GB02FUNC1803(struct uart_port *port)
{
	struct GB02STR203 *gsp = port_to_gb02_serial_port(port);

	GB02FUNC1781(gsp);
	if (!gsp->task_run) {
		wake_up_process(gsp->uart_task);
		gsp->task_run = true;
	}
}

static int GB02FUNC1806(void *dev_id)
{
	struct GB02STR203 *gsp = dev_id;
	u32 ip, rx_num;

	while (!kthread_should_stop()) {
		ip = GB02FUNC1770(gsp, GB02MAC2664);
		rx_num = GB02FUNC1770(gsp, 0x4c);
		if (ip & GB02MAC2666)
			GB02FUNC1789(gsp);
		if (ip & GB02MAC2668)
			GB02FUNC1776(gsp);
		if (!gsp->task_run) {
			__set_current_state(TASK_INTERRUPTIBLE);
			schedule();
		}
	}

	return 0;
}

static unsigned int GB02FUNC1807(struct uart_port *port)
{
	return TIOCSER_TEMT;
}

static unsigned int GB02FUNC1808(struct uart_port *port)
{
	return TIOCM_CAR | TIOCM_CTS | TIOCM_DSR;
}

static void GB02FUNC1809(struct uart_port *port, unsigned int mctrl)
{
	/* IP block does not support these signals */
}

static void GB02FUNC1810(struct uart_port *port, int break_state)
{
	/* IP block does not support sending a break */
}

static int GB02FUNC1811(struct uart_port *port)
{
	struct GB02STR203 *gsp = port_to_gb02_serial_port(port);

	GB02FUNC1782(gsp);
	if (!gsp->task_run) {
		wake_up_process(gsp->uart_task);
		gsp->task_run = true;
	}

	return 0;
}

static void GB02FUNC1813(struct uart_port *port)
{
	struct GB02STR203 *gsp = port_to_gb02_serial_port(port);

	GB02FUNC1785(gsp);
	GB02FUNC1784(gsp);
	gsp->task_run = false;
}

static void GB02FUNC1814(struct uart_port *port,
									struct ktermios *termios,
									struct ktermios *old)
{
	struct GB02STR203 *gsp = port_to_gb02_serial_port(port);
	unsigned long flags;
	u32 v, old_v;
	int rate;
	char nstop;

	if ((termios->c_cflag & CSIZE) != CS8)
		dev_err_once(gsp->port.dev, "only 8-bit words supported\n");
	if (termios->c_iflag & (INPCK | PARMRK))
		dev_err_once(gsp->port.dev, "parity checking not supported\n");
	if (termios->c_iflag & BRKINT)
		dev_err_once(gsp->port.dev, "BREAK detection not supported\n");

	/* Set number of stop bits */
	nstop = (termios->c_cflag & CSTOPB) ? 2 : 1;
	GB02FUNC1795(gsp, nstop);

	/* Set line rate */
	rate = uart_get_baud_rate(port, termios, old, 0, gsp->clkin_rate / 16);
	GB02FUNC1792(gsp, rate);

	spin_lock_irqsave(&gsp->port.lock, flags);

	/* Update the per-port timeout */
	uart_update_timeout(port, termios->c_cflag, rate);

	gsp->port.read_status_mask = 0;

	/* Ignore all characters if CREAD is not set */
	v = GB02FUNC1770(gsp, GB02MAC2654);
	old_v = v;
	if ((termios->c_cflag & CREAD) == 0)
		v &= GB02MAC2658;
	else
		v |= GB02MAC2658;
	if (v != old_v)
		GB02FUNC1768(v, GB02MAC2654, gsp);

	spin_unlock_irqrestore(&gsp->port.lock, flags);
}

static void GB02FUNC1816(struct uart_port *port)
{
}

static int GB02FUNC1817(struct uart_port *port)
{
	return 0;
}

static void GB02FUNC1818(struct uart_port *port, int flags)
{
	struct GB02STR203 *gsp = port_to_gb02_serial_port(port);

	gsp->port.type = GB02MAC2634;
}

static int GB02FUNC1819(struct uart_port *port,
								   struct serial_struct *ser)
{
	return -EINVAL;
}

static const char *GB02FUNC1820(struct uart_port *port)
{
	return port->type == GB02MAC2634 ? "GB02 UART/USART" : NULL;
}

#ifdef CONFIG_CONSOLE_POLL
static int GB02FUNC1821(struct uart_port *port)
{
	struct GB02STR203 *gsp = port_to_gb02_serial_port(port);
	char is_empty, ch;

	ch = GB02FUNC1786(gsp, &is_empty);
	if (is_empty)
		return NO_POLL_CHAR;

	return ch;
}

static void GB02FUNC1822(struct uart_port *port,
									  unsigned char c)
{
	struct GB02STR203 *gsp = port_to_gb02_serial_port(port);

	GB02FUNC1797(gsp);
	GB02FUNC1774(gsp, c);
}
#endif /* CONFIG_CONSOLE_POLL */

/*
 * Early console support
 */

#ifdef CONFIG_SERIAL_EARLYCON
static void GB02FUNC1823(struct uart_port *port, int c)
{
	while (GB02FUNC1767(port, GB02MAC2637) &
		   GB02MAC2639)
		cpu_relax();

	GB02FUNC1765(c, GB02MAC2637, port);
}

static void GB02FUNC1826(struct console *con, const char *s,
									unsigned int n)
{
	struct earlycon_device *dev = con->data;
	struct uart_port *port = &dev->port;

	uart_console_write(port, s, n, GB02FUNC1823);
}

static int __init GB02FUNC1827(struct earlycon_device *dev,
		const char *options)
{
	struct uart_port *port = &dev->port;

	if (!port->membase)
		return -ENODEV;

	dev->con->write = GB02FUNC1826;

	return 0;
}

OF_EARLYCON_DECLARE(gb02, "gb02,uart0", GB02FUNC1827);
#endif /* CONFIG_SERIAL_EARLYCON */

/*
 * Linux console interface
 */

#ifdef CONFIG_SERIAL_GB02_CONSOLE

static struct GB02STR203 *gb02_serial_console_ports[GB02MAC2673];

static void GB02FUNC1829(struct uart_port *port, int ch)
{
	struct GB02STR203 *gsp = port_to_gb02_serial_port(port);

	GB02FUNC1797(gsp);
	GB02FUNC1774(gsp, ch);
}

static void GB02FUNC1830(struct console *co, const char *s,
									  unsigned int count)
{
	struct GB02STR203 *gsp = gb02_serial_console_ports[co->index];
	unsigned long flags;
	unsigned int ier;
	int locked = 1;

	if (!gsp)
		return;

	local_irq_save(flags);
	if (gsp->port.sysrq)
		locked = 0;
	else if (oops_in_progress)
		locked = spin_trylock(&gsp->port.lock);
	else
		spin_lock(&gsp->port.lock);

	ier = GB02FUNC1770(gsp, GB02MAC2659);
	GB02FUNC1768(0, GB02MAC2659, gsp);

	uart_console_write(&gsp->port, s, count, GB02FUNC1829);

	GB02FUNC1768(ier, GB02MAC2659, gsp);

	if (locked)
		spin_unlock(&gsp->port.lock);
	local_irq_restore(flags);
}

static int __init GB02FUNC1831(struct console *co, char *options)
{
	struct GB02STR203 *gsp;
	int baud = GB02MAC2674;
	int bits = 8;
	int parity = 'n';
	int flow = 'n';

	if (co->index < 0 || co->index >= GB02MAC2673)
		return -ENODEV;

	gsp = gb02_serial_console_ports[co->index];
	if (!gsp)
		return -ENODEV;

	if (options)
		uart_parse_options(options, &baud, &parity, &bits, &flow);

	return uart_set_options(&gsp->port, co, baud, parity, bits, flow);
}

static struct uart_driver gb02_serial_uart_driver;

static struct console gb02_serial_console = {
	.name		= GB02_TTY_PREFIX,
	.write		= GB02FUNC1830,
	.device		= uart_console_device,
	.setup		= GB02FUNC1831,
	.flags		= CON_PRINTBUFFER,
	.index		= -1,
	.data		= &gb02_serial_uart_driver,
};

static int __init GB02FUNC1832(void)
{
	register_console(&gb02_serial_console);
	return 0;
}

console_initcall(GB02FUNC1832);

static void GB02FUNC1833(struct GB02STR203 *gsp)
{
	gb02_serial_console_ports[gsp->port.line] = gsp;
}

static void GB02FUNC1834(struct GB02STR203 *gsp)
{
	gb02_serial_console_ports[gsp->port.line] = 0;
}

#define GB02MAC2716	(&gb02_serial_console)

#else

#define GB02MAC2716	NULL

static void GB02FUNC1833(struct GB02STR203 *gsp)
{}
static void GB02FUNC1834(struct GB02STR203 *gsp)
{}

#endif

static unsigned long GB02FUNC1835(struct GB02STR203 *gsp)
{
	unsigned long periph_freq_div, periph_freq;

	periph_freq_div = GB02FUNC530(gsp->sysctl_iofunc_base, GB02MAC850) & 0x7;
	periph_freq = GB02MAC757 * 1000 / (periph_freq_div + 1);
	return periph_freq;
}

static void GB02FUNC1837(struct GB02STR203 *gsp)
{
	GB02FUNC528(GB02MAC740, gsp->sysctl_cfg_base, GB02MAC734);
}

static const struct uart_ops gb02_serial_uops = {
	.tx_empty	= GB02FUNC1807,
	.set_mctrl	= GB02FUNC1809,
	.get_mctrl	= GB02FUNC1808,
	.stop_tx	= GB02FUNC1799,
	.start_tx	= GB02FUNC1803,
	.stop_rx	= GB02FUNC1801,
	.break_ctl	= GB02FUNC1810,
	.startup	= GB02FUNC1811,
	.shutdown	= GB02FUNC1813,
	.set_termios	= GB02FUNC1814,
	.type		= GB02FUNC1820,
	.release_port	= GB02FUNC1816,
	.request_port	= GB02FUNC1817,
	.config_port	= GB02FUNC1818,
	.verify_port	= GB02FUNC1819,
#ifdef CONFIG_CONSOLE_POLL
	.poll_get_char	= GB02FUNC1821,
	.poll_put_char	= GB02FUNC1822,
#endif
};

static struct uart_driver gb02_serial_uart_driver = {
	.owner		= THIS_MODULE,
	.driver_name	= GB02_SERIAL_NAME,
	.dev_name	= GB02_TTY_PREFIX,
	.nr		= GB02MAC2673,
	.cons		= GB02MAC2716,
};

static int GB02FUNC1839(struct GB02STR39 *gb_dev, struct GB02STR72 *peri_info, unsigned int device_id)
{
	struct GB02STR203 *gsp;
	struct GB02STR70 *pcie_info;
	int mcu_peri_bar_id, peri_base_bar_id;
	int id, ret = 0;

	pcie_info = gb_dev->gb_pcie;
	id = device_id;
	if (id > GB02MAC2673) {
		dev_err(gb_dev->dev, "too many UARTs (%d)\n", id);
		return -EINVAL;
	}

	gsp = devm_kzalloc(gb_dev->dev, sizeof(*gsp), GFP_KERNEL);
	if (!gsp)
		return -ENOMEM;
	mcu_peri_bar_id = GB02FUNC474(pcie_info->GB02STR153);
	peri_base_bar_id = GB02FUNC477(pcie_info->GB02STR153);
	gsp->sysctl_iofunc_base = gb_dev->gb_pcie->pci_bars[mcu_peri_bar_id].mmio; // bar0
	gsp->sysctl_cfg_base = gb_dev->gb_pcie->pci_bars[peri_base_bar_id].mmio; //bar4
	GB02FUNC1837(gsp);
	gsp->port.dev = gb_dev->dev;
	gsp->port.type = GB02MAC2634;
	gsp->port.iotype = UPIO_MEM;
	gsp->port.fifosize = GB02MAC2675;
	gsp->port.ops = &gb02_serial_uops;
	gsp->port.line = id;
	gsp->port.membase = gsp->sysctl_iofunc_base + GB02MAC748 + GB02MAC795 + GB02MAC804 * id;
	gsp->dev = gb_dev->dev;
	gsp->task_run = false;

	gsp->uart_task = kthread_create(GB02FUNC1806, gsp, "gb02_uart%d", id);
	if (IS_ERR(gsp->uart_task)) {
		dev_err(gb_dev->dev, "gb02 uart_task creat failed\n");
		goto init_out;
	}

	/* Set up clock divider */
	gsp->clkin_rate = GB02FUNC1835(gsp);
	gsp->baud_rate = GB02MAC2674;
	gsp->port.uartclk = gsp->clkin_rate;
	peri_info->priv = gsp;
	GB02FUNC1791(gsp);

	/* Enable transmits and set the watermark level to 1 */
	GB02FUNC1768((1 << GB02MAC2648) |
			   GB02MAC2653,
			   GB02MAC2647, gsp);

	/* Enable receives and set the watermark level to 0 */
	GB02FUNC1768((0 << GB02MAC2655) |
			   GB02MAC2658,
			   GB02MAC2654, gsp);

	/* TODO: adjust using driver data for UART_SETUP reg */
	/* No partity check, 8 bit len, cts/rts disable, dma disable */
	GB02FUNC1768(0x3<<4, GB02MAC2672, gsp);

	GB02FUNC1833(gsp);

	ret = uart_add_one_port(&gb02_serial_uart_driver, &gsp->port);
	if (ret != 0) {
		dev_err(gb_dev->dev, "could not add uart: %d\n", ret);
		goto init_out;
	}
	wake_up_process(gsp->uart_task);//TODO

	return 0;

init_out:
	GB02FUNC1834(gsp);
	return ret;
}

static void GB02FUNC1840(struct GB02STR72 *peri_info)
{
	struct GB02STR203 *gsp = (struct GB02STR203 *)peri_info->priv;

	kthread_stop(gsp->uart_task);
	GB02FUNC1834(gsp);
	uart_remove_one_port(&gb02_serial_uart_driver, &gsp->port);
	uart_unregister_driver(&gb02_serial_uart_driver);
}

int GB02FUNC1841(struct GB02STR39 *gb_dev, struct GB02STR72 *peri_info)
{
	static u8 uart_complete;
	int device_id = 0;
	int ret;

	device_id = peri_info->mcu_peripherals_device_id;
	//uart driver only register once
	if (!uart_complete) {
		ret = uart_register_driver(&gb02_serial_uart_driver);
		if (ret)
			return ret;
	}

	uart_complete = 1;
	ret = GB02FUNC1839(gb_dev, peri_info, device_id);

	return ret;
}

void GB02FUNC1842(struct GB02STR72 *peri_info)
{
	uart_unregister_driver(&gb02_serial_uart_driver);
	GB02FUNC1840(peri_info);
}
