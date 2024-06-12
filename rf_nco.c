#include <hardware/clocks.h>


#define VREG_VOLTAGE VREG_VOLTAGE_1_20
#define CLK_SYS_HZ (300 * MHZ)
#define INIT_FREQ 94600000


#define LO_BITS_DEPTH 15
#define LO_WORDS (1 << (LO_BITS_DEPTH - 2))

static uint32_t lo_cos[LO_WORDS] __attribute__((__aligned__(1 << LO_BITS_DEPTH)));
static uint32_t lo_sin[LO_WORDS] __attribute__((__aligned__(1 << LO_BITS_DEPTH)));


static void lo_generate(uint32_t *buf, size_t len, double freq, unsigned phase)
{
	static const double base = (UINT_MAX + 1.0) / CLK_SYS_HZ;

	unsigned step = base * freq;
	unsigned accum = phase;

	for (size_t i = 0; i < len; i++) {
		unsigned bits = 0;

		for (int j = 0; j < 32; j++) {
			bits |= accum >> 31;
			bits <<= 1;
			accum += step;
		}

		buf[i] = bits;
	}
}



static void rx_lo_init(double req_freq)
{
	const double step_hz = (double)CLK_SYS_HZ / (4 << LO_BITS_DEPTH);
	double freq = round(req_freq / step_hz) * step_hz;

	lo_generate(lo_cos, LO_WORDS, freq, COS_PHASE);
	lo_generate(lo_sin, LO_WORDS, freq, SIN_PHASE);
}


int main()
{
	vreg_set_voltage(VREG_VOLTAGE);
	set_sys_clock_khz(CLK_SYS_HZ / KHZ, true);
	clock_configure(clk_peri, 0, CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS, CLK_SYS_HZ,
			CLK_SYS_HZ);

	rx_lo_init(INIT_FREQ);
}
