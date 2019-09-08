#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/gpio.h>
#include <asm/exception.h>

#define AT91_AIC5_SSR   0x0
#define AT91_AIC5_INTSEL_MSK  (0x7f << 0)

#define AT91_AIC5_SMR     0x4

#define AT91_AIC5_SVR     0x8
#define AT91_AIC5_IVR     0x10
#define AT91_AIC5_FVR     0x14
#define AT91_AIC5_ISR     0x18

#define AT91_AIC5_IPR0      0x20
#define AT91_AIC5_IPR1      0x24
#define AT91_AIC5_IPR2      0x28
#define AT91_AIC5_IPR3      0x2c
#define AT91_AIC5_IMR     0x30
#define AT91_AIC5_CISR      0x34

#define AT91_AIC5_IECR      0x40
#define AT91_AIC5_IDCR      0x44
#define AT91_AIC5_ICCR      0x48
#define AT91_AIC5_ISCR      0x4c
#define AT91_AIC5_EOICR     0x38
#define AT91_AIC5_SPU     0x3c
#define AT91_AIC5_DCR     0x6c

#define AT91_AIC5_FFER      0x50
#define AT91_AIC5_FFDR      0x54
#define AT91_AIC5_FFSR      0x58

static void __iomem *aic5_base;
static struct gpio_desc *pio_out;

/* Dummy Interrupt handler
 * Will never be called by ARM core IRQ handler
 */
static irqreturn_t dummy_interrupt(int irq, void *data)
{
	return IRQ_NONE;
}

/* Real Interrupt handler
 * Will be called by ARM core FIQ handler
 */
asmlinkage void __exception_irq_entry handle_fiq_as_nmi(struct pt_regs *regs)
{
	u32 irqid;

	/* nop. FIQ handlers for special arch/arm features can be added here. */
	gpiod_set_value(pio_out, 1);
  gpiod_set_value(pio_out, 0);

	/* Clear FIQ in aic5 */
	irqid = readl(aic5_base + AT91_AIC5_ISR);
	readl(aic5_base + AT91_AIC5_FVR);
	writel(0, aic5_base + AT91_AIC5_EOICR);
	//if (irqid != 0)
		//printk(KERN_ERR "%s: Unexpected Interrupt id=%d\n", __func__, irqid);
}

static int aic5_fiq_probe(struct platform_device *pdev)
{
	struct resource   *regs;
	int     irq;
	int     ret;

	regs = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!regs)
		return -ENXIO;

	aic5_base = devm_ioremap_resource(&pdev->dev, regs);
  if (IS_ERR(aic5_base)) {
    return PTR_ERR(aic5_base);
  }

	pio_out = devm_gpiod_get(&pdev->dev, "out", GPIOD_OUT_LOW);
  if (IS_ERR(pio_out))
    return PTR_ERR(pio_out);

	irq = platform_get_irq(pdev, 0);
	if (irq < 0)
		return irq;

	local_fiq_disable();

	ret = devm_request_irq(&pdev->dev, irq, dummy_interrupt,
					0, dev_name(&pdev->dev), NULL);
	if (ret)
		goto out_free_irq;

	local_fiq_enable();

	printk("%s done.\n", __func__);

	return 0;
out_free_irq:
	local_fiq_enable();
	return ret;
}

static int aic5_fiq_remove(struct platform_device *pdev)
{
	/* Nothing to do */
	return 0;
}

#if defined(CONFIG_OF)
static const struct of_device_id aic5_fiq_dt_ids[] = {
	{ .compatible = "atmel,sama5d2-fiq" },
	{ /* sentinel */ }
};

MODULE_DEVICE_TABLE(of, aic5_fiq_dt_ids);
#endif

static struct platform_driver aic5_fiq_driver = {
  .driver   = {
    .name   = "aic5-fiq",
    .of_match_table = of_match_ptr(aic5_fiq_dt_ids),
  },
  .probe    = aic5_fiq_probe,
  .remove   = aic5_fiq_remove,
};
module_platform_driver(aic5_fiq_driver);

MODULE_AUTHOR("Xing Chen <xing.chen@microchip.com>");
MODULE_DESCRIPTION("SAMA5D2 fiq driver");
MODULE_LICENSE("GPL");
