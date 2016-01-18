/*
 * hdmiin_audio.c  --  ALSA SoC ROCKCHIP hdmi in audio route driver
 *
 * Driver for rockchip hdmi in audio route framework
 *
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/version.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <asm/dma.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/initval.h>
#include <sound/soc.h>
#include <sound/dmaengine_pcm.h>
#include <asm/io.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>

#include "rk_i2s.h"

/*
 * channel status register
 * 192 frame channel status bits: include 384 subframe bits
 */
#define SPDIF_CHNSR00_ADDR	0xC0
#define SPDIF_CHNSR01_ADDR	0xC4
#define SPDIF_CHNSR02_ADDR	0xC8
#define SPDIF_CHNSR03_ADDR	0xCC
#define SPDIF_CHNSR04_ADDR	0xD0
#define SPDIF_CHNSR05_ADDR	0xD4
#define SPDIF_CHNSR06_ADDR	0xD8
#define SPDIF_CHNSR07_ADDR	0xDC
#define SPDIF_CHNSR08_ADDR	0xE0
#define SPDIF_CHNSR09_ADDR	0xE4
#define SPDIF_CHNSR10_ADDR	0xE8
#define SPDIF_CHNSR11_ADDR	0xEC

/*
 * according to iec958, we only care about
 * the first meaningful 5 bytes(40 bits)
 */
#define CHNSTA_BYTES		(5)
#define BIT_1_LPCM		(0X0<<1)
#define BIT_1_NLPCM		(0x1<<1)

/* sample word length bit 32~35 */
#define CHNS_SAMPLE_WORD_LEN_16 (0x2)
#define CHNS_SAMPLE_WORD_LEN_24	(0xb)

/* sample frequency bit 24~27 */
#define CHNS_SAMPLE_FREQ_22P05K	(0X4)
#define CHNS_SAMPLE_FREQ_44P1K	(0X0)
#define CHNS_SAMPLE_FREQ_88P2K	(0X8)
#define CHNS_SAMPLE_FREQ_176P4K	(0Xc)
#define CHNS_SAMPLE_FREQ_24K	(0X6)
#define CHNS_SAMPLE_FREQ_48K	(0X2)
#define CHNS_SAMPLE_FREQ_96K	(0Xa)
#define CHNS_SAMPLE_FREQ_192K	(0Xe)
#define CHNS_SAMPLE_FREQ_32K	(0X3)
#define CHNS_SAMPLE_FREQ_768K	(0X9)

/* Registers */
#define CFGR			0x00
#define SDBLR			0x04
#define DMACR			0x08
#define INTCR			0x0C
#define INTSR			0x10
#define XFER			0x18
#define SMPDR			0x20

/* transfer configuration register */
#define CFGR_VALID_DATA_16bit		(0x0 << 0)
#define CFGR_VALID_DATA_20bit		(0x1 << 0)
#define CFGR_VALID_DATA_24bit		(0x2 << 0)
#define CFGR_VALID_DATA_MASK		(0x3 << 0)
#define CFGR_HALFWORD_TX_ENABLE		(0x1 << 2)
#define CFGR_HALFWORD_TX_DISABLE	(0x0 << 2)
#define CFGR_HALFWORD_TX_MASK		(0x1 << 2)
#define CFGR_JUSTIFIED_RIGHT		(0x0 << 3)
#define CFGR_JUSTIFIED_LEFT		(0x1 << 3)
#define CFGR_JUSTIFIED_MASK		(0x1 << 3)
#define CFGR_CSE_DISABLE		(0x0 << 6)
#define CFGR_CSE_ENABLE			(0x1 << 6)
#define CFGR_CSE_MASK			(0x1 << 6)
#define CFGR_MCLK_CLR			(0x1 << 7)
#define CFGR_LINEAR_PCM			(0x0 << 8)
#define CFGR_NON_LINEAR_PCM		(0x1 << 8)
#define CFGR_LINEAR_MASK		(0x1 << 8)
#define CFGR_PRE_CHANGE_ENALBLE		(0x1 << 9)
#define CFGR_PRE_CHANGE_DISABLE		(0x0 << 9)
#define CFGR_PRE_CHANGE_MASK		(0x1 << 9)
#define CFGR_CLK_RATE_MASK		(0xFF << 16)

/* transfer start register */
#define XFER_TRAN_STOP			(0x0 << 0)
#define XFER_TRAN_START			(0x1 << 0)
#define XFER_MASK			(0x1 << 0)

/* dma control register */
#define DMACR_TRAN_DMA_DISABLE		(0x0 << 5)
#define DMACR_TRAN_DMA_ENABLE		(0x1 << 5)
#define DMACR_TRAN_DMA_CTL_MASK		(0x1 << 5)
#define DMACR_TRAN_DATA_LEVEL		(0x10)
#define DMACR_TRAN_DATA_LEVEL_MASK	(0x1F)
#define DMACR_TRAN_DMA_MASK		(0x3F)
#define DMA_DATA_LEVEL_16		(0x10)

/* interrupt control register */
#define INTCR_SDBEIE_DISABLE		(0x0 << 4)
#define INTCR_SDBEIE_ENABLE		(0x1 << 4)
#define INTCR_SDBEIE_MASK		(0x1 << 4)

/* size * width: 16*4 = 64 bytes */
#define SPDIF_DMA_BURST_SIZE		(16)

struct rk_i2s_dev {
	struct device *dev;
	struct clk *clk; /* bclk */
	struct clk *mclk; /*mclk output only */
	struct clk *hclk; /*ahb clk */
	struct snd_dmaengine_dai_dma_data capture_dma_data;
	struct snd_dmaengine_dai_dma_data playback_dma_data;
	struct regmap *regmap;
	bool tx_start;
	bool rx_start;
	int xfer_mode; /* 0: i2s, 1: pcm */
	struct delayed_work clk_delayed_work;
};

struct rockchip_spdif_info {
	spinlock_t lock;/*lock parmeter setting.*/
	void __iomem *regs;
	unsigned long clk_rate;
	struct clk *hclk;
	struct clk *clk;
	struct device *dev;
	struct snd_dmaengine_dai_dma_data dma_playback;
	u32 cfgr;
	u32 dmac;
};

extern struct rockchip_spdif_info *g_rk_spdif;
extern struct rk_i2s_dev *g_rk_i2s;
extern int snd_config_hdmi_audio(struct snd_pcm_hw_params *params);

static inline int param_is_mask(int p)
{
    return (p >= SNDRV_PCM_HW_PARAM_FIRST_MASK) &&
        (p <= SNDRV_PCM_HW_PARAM_LAST_MASK);
}

static inline int param_is_interval(int p)
{
    return (p >= SNDRV_PCM_HW_PARAM_FIRST_INTERVAL) &&
        (p <= SNDRV_PCM_HW_PARAM_LAST_INTERVAL);
}

static inline struct snd_interval *param_to_interval(struct snd_pcm_hw_params *p, int n)
{
    return &(p->intervals[n - SNDRV_PCM_HW_PARAM_FIRST_INTERVAL]);
}

static inline struct snd_mask *param_to_mask(struct snd_pcm_hw_params *p, int n)
{
    return &(p->masks[n - SNDRV_PCM_HW_PARAM_FIRST_MASK]);
}

static void param_set_mask(struct snd_pcm_hw_params *p, int n, unsigned int bit)
{
    if (bit >= SNDRV_MASK_MAX)
        return;
    if (param_is_mask(n)) {
        struct snd_mask *m = param_to_mask(p, n);
        m->bits[0] = 0;
        m->bits[1] = 0;
        m->bits[bit >> 5] |= (1 << (bit & 31));
    }
}

static void param_set_min(struct snd_pcm_hw_params *p, int n, unsigned int val)
{
    if (param_is_interval(n)) {
        struct snd_interval *i = param_to_interval(p, n);
        i->min = val;
    }
}

static void param_aset_int(struct snd_pcm_hw_params *p, int n, unsigned int val)
{
    if (param_is_interval(n)) {
        struct snd_interval *i = param_to_interval(p, n);
        i->min = val;
        i->max = val;
        i->integer = 1;
    }
}

static void param_init(struct snd_pcm_hw_params *p)
{
    int n;

    memset(p, 0, sizeof(*p));
    for (n = SNDRV_PCM_HW_PARAM_FIRST_MASK;
         n <= SNDRV_PCM_HW_PARAM_LAST_MASK; n++) {
            struct snd_mask *m = param_to_mask(p, n);
            m->bits[0] = ~0;
            m->bits[1] = ~0;
    }
    for (n = SNDRV_PCM_HW_PARAM_FIRST_INTERVAL;
         n <= SNDRV_PCM_HW_PARAM_LAST_INTERVAL; n++) {
            struct snd_interval *i = param_to_interval(p, n);
            i->min = 0;
            i->max = ~0;
    }
    p->rmask = ~0U;
    p->cmask = 0;
    p->info = ~0U;
}

static void rk_snd_i2s_rxctrl(int on)
{
	unsigned int val = 0;
	int retry = 10;
	struct rk_i2s_dev *i2s = g_rk_i2s;
	
	if (on) {
		regmap_update_bits(i2s->regmap, I2S_DMACR,
				   I2S_DMACR_RDE_ENABLE, I2S_DMACR_RDE_ENABLE);

		regmap_update_bits(i2s->regmap, I2S_XFER,
				   I2S_XFER_TXS_START | I2S_XFER_RXS_START,
				   I2S_XFER_TXS_START | I2S_XFER_RXS_START);

		i2s->rx_start = true;
	} else {
		i2s->rx_start = false;

		regmap_update_bits(i2s->regmap, I2S_DMACR,
				   I2S_DMACR_RDE_ENABLE, I2S_DMACR_RDE_DISABLE);

		if (!i2s->tx_start) {
			regmap_update_bits(i2s->regmap, I2S_XFER,
					   I2S_XFER_TXS_START |
					   I2S_XFER_RXS_START,
					   I2S_XFER_TXS_STOP |
					   I2S_XFER_RXS_STOP);

			regmap_update_bits(i2s->regmap, I2S_CLR,
					   I2S_CLR_TXC | I2S_CLR_RXC,
					   I2S_CLR_TXC | I2S_CLR_RXC);

			regmap_read(i2s->regmap, I2S_CLR, &val);

			/* Should wait for clear operation to finish */
			while (val) {
				regmap_read(i2s->regmap, I2S_CLR, &val);
				retry--;
				if (!retry) {
					dev_warn(i2s->dev, "fail to clear\n");
					break;
				}
			}
		}
	}

}

static void rk_snd_i2s_txctrl(int on)
{
	unsigned int val = 0;
	int retry = 10;
	struct rk_i2s_dev *i2s = g_rk_i2s;

	if (on) {
		regmap_update_bits(i2s->regmap, I2S_DMACR,
				   I2S_DMACR_TDE_ENABLE, I2S_DMACR_TDE_ENABLE);

		regmap_update_bits(i2s->regmap, I2S_XFER,
				   I2S_XFER_TXS_START | I2S_XFER_RXS_START,
				   I2S_XFER_TXS_START | I2S_XFER_RXS_START);

		i2s->tx_start = true;
	} else {
		i2s->tx_start = false;

		regmap_update_bits(i2s->regmap, I2S_DMACR,
				   I2S_DMACR_TDE_ENABLE, I2S_DMACR_TDE_DISABLE);

		if (!i2s->rx_start) {
			regmap_update_bits(i2s->regmap, I2S_XFER,
					   I2S_XFER_TXS_START |
					   I2S_XFER_RXS_START,
					   I2S_XFER_TXS_STOP |
					   I2S_XFER_RXS_STOP);

			regmap_update_bits(i2s->regmap, I2S_CLR,
					   I2S_CLR_TXC | I2S_CLR_RXC,
					   I2S_CLR_TXC | I2S_CLR_RXC);

			regmap_read(i2s->regmap, I2S_CLR, &val);

			/* Should wait for clear operation to finish */
			while (val) {
				regmap_read(i2s->regmap, I2S_CLR, &val);
				retry--;
				if (!retry) {
					dev_warn(i2s->dev, "fail to clear\n");
					break;
				}
			}
		}
	}

}

static void rk_snd_spdif_txctrl(int on)
{
	struct rockchip_spdif_info *spdif = g_rk_spdif;

	void __iomem *regs = spdif->regs;
	u32 dmacr, xfer;
	

	xfer = readl(regs + XFER) & (~XFER_MASK);
	dmacr = readl(regs + DMACR) & (~DMACR_TRAN_DMA_CTL_MASK);

	if (on) {
		xfer |= XFER_TRAN_START;
		dmacr |= DMACR_TRAN_DMA_ENABLE;
		dmacr |= spdif->dmac;
		writel(spdif->cfgr, regs + CFGR);
		writel(dmacr, regs + DMACR);
		writel(xfer, regs + XFER);
	} else {
		xfer &= XFER_TRAN_STOP;
		dmacr &= DMACR_TRAN_DMA_DISABLE;
		writel(xfer, regs + XFER);
		writel(dmacr, regs + DMACR);
		writel(CFGR_MCLK_CLR, regs + CFGR);
	}

	dev_info(spdif->dev, "on: %d, xfer = 0x%x, dmacr = 0x%x\n",
		on, readl(regs + XFER), readl(regs + DMACR));
}

static int rk_hdmiin_audio_trigger(int cmd, int mode)
{
	int ret = 0;

	pr_info("%s\n", __func__);
	switch (cmd)
	{
        case SNDRV_PCM_TRIGGER_START:
			if (HDMIN_NORMAL_MODE == mode)
				rk_snd_i2s_rxctrl(1);
			rk_snd_i2s_txctrl(1);
			rk_snd_spdif_txctrl(1);
            break;

        case SNDRV_PCM_TRIGGER_STOP:
			if (HDMIN_NORMAL_MODE == mode)
				rk_snd_i2s_rxctrl(0);
			rk_snd_i2s_txctrl(0);
			rk_snd_spdif_txctrl(0);
			break;

        default:
            ret = -EINVAL;
            break;
	}

	return ret;
}

static int rk_hdmiin_audio_hw_params(struct snd_pcm_hw_params *params)
{
	unsigned int val = 0;
	struct rk_i2s_dev *i2s = g_rk_i2s;
	struct rockchip_spdif_info *spdif = g_rk_spdif;
	void __iomem *regs = spdif->regs;
	u32 cfgr, dmac, intcr, chnregval;
	char chnsta[CHNSTA_BYTES];

	pr_info("%s: fmt: %d: sr: %d\n", __func__, params_format(params), params_rate(params));

	/* i2s cfg */
	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S8:
		val |= I2S_TXCR_VDW(8);
		break;
	case SNDRV_PCM_FORMAT_S16_LE:
		val |= I2S_TXCR_VDW(16);
		break;
	case SNDRV_PCM_FORMAT_S20_3LE:
		val |= I2S_TXCR_VDW(20);
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
		val |= I2S_TXCR_VDW(24);
		break;
	default:
		return -EINVAL;
	}

	regmap_update_bits(i2s->regmap, I2S_TXCR, I2S_TXCR_VDW_MASK, val);
	regmap_update_bits(i2s->regmap, I2S_RXCR, I2S_RXCR_VDW_MASK, val);
	regmap_update_bits(i2s->regmap, I2S_DMACR, I2S_DMACR_TDL_MASK,
			   I2S_DMACR_TDL(16));
	regmap_update_bits(i2s->regmap, I2S_DMACR, I2S_DMACR_RDL_MASK,
			   I2S_DMACR_RDL(16));

	/* spdif cfg */
	clk_set_rate(spdif->clk, 128 * params_rate(params));

	cfgr = readl(regs + CFGR);

	cfgr &= ~CFGR_VALID_DATA_MASK;
	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		cfgr |= CFGR_VALID_DATA_16bit;
		break;
	case SNDRV_PCM_FORMAT_S20_3LE:
		cfgr |= CFGR_VALID_DATA_20bit;
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
		cfgr |= CFGR_VALID_DATA_24bit;
		break;
	default:
		return -EINVAL;
	}

	cfgr &= ~CFGR_HALFWORD_TX_MASK;
	cfgr |= CFGR_HALFWORD_TX_ENABLE;

	/* no need divder, let set_syclk care about this */
	cfgr &= ~CFGR_CLK_RATE_MASK;
	cfgr |= (0x0<<16);

	cfgr &= ~CFGR_JUSTIFIED_MASK;
	cfgr |= CFGR_JUSTIFIED_RIGHT;

	cfgr &= ~CFGR_CSE_MASK;
	cfgr |= CFGR_CSE_ENABLE;

	cfgr &= ~CFGR_LINEAR_MASK;
	cfgr |= CFGR_LINEAR_PCM;

	cfgr &= ~CFGR_PRE_CHANGE_MASK;
	cfgr |= CFGR_PRE_CHANGE_ENALBLE;

	spdif->cfgr = cfgr;
	writel(cfgr, regs + CFGR);

	intcr = readl(regs + INTCR) & (~INTCR_SDBEIE_MASK);
	intcr |= INTCR_SDBEIE_DISABLE;
	writel(intcr, regs + INTCR);

	dmac = readl(regs + DMACR) & (~DMACR_TRAN_DATA_LEVEL_MASK);
	dmac |= DMA_DATA_LEVEL_16;
	spdif->dmac = dmac;
	writel(dmac, regs + DMACR);

	/* channel status bit */
	memset(chnsta, 0x0, CHNSTA_BYTES);
	switch (params_rate(params)) {
	case 44100:
		val = CHNS_SAMPLE_FREQ_44P1K;
		break;
	case 48000:
		val = CHNS_SAMPLE_FREQ_48K;
		break;
	case 88200:
		val = CHNS_SAMPLE_FREQ_88P2K;
		break;
	case 96000:
		val = CHNS_SAMPLE_FREQ_96K;
		break;
	case 176400:
		val = CHNS_SAMPLE_FREQ_176P4K;
		break;
	case 192000:
		val = CHNS_SAMPLE_FREQ_192K;
		break;
	default:
		val = CHNS_SAMPLE_FREQ_44P1K;
		break;
	}

	chnsta[0] |= BIT_1_LPCM;
	chnsta[3] |= val;
	chnsta[4] |= ((~val)<<4 | CHNS_SAMPLE_WORD_LEN_16);

	chnregval = (chnsta[4] << 16) | (chnsta[4]);
	writel(chnregval, regs + SPDIF_CHNSR02_ADDR);

	chnregval = (chnsta[3] << 24) | (chnsta[3] << 8);
	writel(chnregval, regs + SPDIF_CHNSR01_ADDR);

	chnregval = (chnsta[1] << 24) | (chnsta[0] << 16) |
				(chnsta[1] << 8) | (chnsta[0]);
	writel(chnregval, regs + SPDIF_CHNSR00_ADDR);

	return 0;
}

static int audio_route_active = 0;
extern void rk1000_audio_cfg(int loopback);
extern void es8323_codec_set_reg(int loopback);
static int hdmin_mode = HDMIN_NORMAL_MODE;
static DEFINE_MUTEX(hdmin_lock);

static int snd_stop_hdmi_in_audio_route_l(void)
{
	if (!audio_route_active)
		return 0;
//	pr_info("%s\n", __func__);

	snd_dmaengine_hdmiin_audio_pcm_trigger(SNDRV_PCM_TRIGGER_STOP, hdmin_mode);
	rk_hdmiin_audio_trigger(SNDRV_PCM_TRIGGER_STOP, hdmin_mode);
    es8323_codec_set_reg(0);
	audio_route_active--;

	return 0;
}

int snd_stop_hdmi_in_audio_route(void)
{
	mutex_lock(&hdmin_lock);
	snd_stop_hdmi_in_audio_route_l();
	mutex_unlock(&hdmin_lock);

	return 0;
}
EXPORT_SYMBOL(snd_stop_hdmi_in_audio_route);

static int snd_start_hdmi_in_audio_route_l(int mode)
{
	struct snd_pcm_hw_params params;

	if (audio_route_active)
		return 0;

	hdmin_mode = mode;
	audio_route_active++;
	//rk1000_audio_cfg(1);
    es8323_codec_set_reg(1);
    param_init(&params);
    param_set_mask(&params, SNDRV_PCM_HW_PARAM_FORMAT, SNDRV_PCM_FORMAT_S16_LE);
    param_set_mask(&params, SNDRV_PCM_HW_PARAM_SUBFORMAT, SNDRV_PCM_SUBFORMAT_STD);
    param_set_min(&params, SNDRV_PCM_HW_PARAM_PERIOD_SIZE, 1024);
    param_aset_int(&params, SNDRV_PCM_HW_PARAM_SAMPLE_BITS, 16);
    param_aset_int(&params, SNDRV_PCM_HW_PARAM_FRAME_BITS, 16 * 2);
    param_aset_int(&params, SNDRV_PCM_HW_PARAM_CHANNELS, 2);
    param_aset_int(&params, SNDRV_PCM_HW_PARAM_PERIODS, 2);
    param_aset_int(&params, SNDRV_PCM_HW_PARAM_RATE, 44100);

	snd_dmaengine_hdmiin_audio_pcm_open();              //dma set
	rk_hdmiin_audio_hw_params(&params);                 //config hdmi in audio
	snd_config_hdmi_audio(&params);                     //config hdmi out audio
	dmaengine_hdmiin_audio_pcm_hw_params(mode);
	snd_dmaengine_hdmiin_audio_pcm_trigger(SNDRV_PCM_TRIGGER_START, mode);
	rk_hdmiin_audio_trigger(SNDRV_PCM_TRIGGER_START, mode);

	return 0;
}

int snd_start_hdmi_in_audio_route(void)
{
	mutex_lock(&hdmin_lock);
	snd_start_hdmi_in_audio_route_l(HDMIN_NORMAL_MODE);
	mutex_unlock(&hdmin_lock);

	return 0;
}
EXPORT_SYMBOL(snd_start_hdmi_in_audio_route);

int snd_hdmiin_capture_mode(bool en)
{
	int mode;
    printk("%s %d  %d \n",__FUNCTION__,__LINE__,en);

	mutex_lock(&hdmin_lock);
	if (!audio_route_active) {
		mutex_unlock(&hdmin_lock);
		return 0;
	}

	if (en)
		mode = HDMIN_CAPTURE_MODE;
	else
		mode = HDMIN_NORMAL_MODE;

	snd_stop_hdmi_in_audio_route_l();
	snd_start_hdmi_in_audio_route_l(mode);
	mutex_unlock(&hdmin_lock);

	return 0;
}
EXPORT_SYMBOL(snd_hdmiin_capture_mode);

int snd_get_hdmiin_audio_pcm_slave_config(struct dma_slave_config *slave_config, enum dma_chan_device_id id)
{
	int ret = 0;
	struct rk_i2s_dev *i2s = g_rk_i2s;
	struct rockchip_spdif_info *spdif = g_rk_spdif;

	switch(id) {
	case I2S_CAPTURE:
		slave_config->direction = DMA_DEV_TO_MEM;
		slave_config->src_addr_width = i2s->capture_dma_data.addr_width;
		slave_config->src_addr = i2s->capture_dma_data.addr;
		slave_config->src_maxburst = i2s->capture_dma_data.maxburst;
		slave_config->device_fc = false;
		slave_config->slave_id = i2s->capture_dma_data.slave_id;
		break;

	case I2S_PLAYBACK:
		slave_config->direction = DMA_MEM_TO_DEV;
		slave_config->dst_addr_width = i2s->playback_dma_data.addr_width;
		slave_config->dst_addr = i2s->playback_dma_data.addr;
		slave_config->dst_maxburst = i2s->playback_dma_data.maxburst;
		slave_config->device_fc = false;
		slave_config->slave_id = i2s->playback_dma_data.slave_id;
		break;

	case SPDIF_PLAYBACK:
		slave_config->direction = DMA_MEM_TO_DEV;
		slave_config->dst_addr_width = spdif->dma_playback.addr_width;
		slave_config->dst_addr = spdif->dma_playback.addr;
		slave_config->dst_maxburst = spdif->dma_playback.maxburst;
		slave_config->device_fc = false;
		slave_config->slave_id = 0;
		break;

	default:
		pr_err("%s: invalid params\n", __func__);
		break;
	}

	return ret;
}
