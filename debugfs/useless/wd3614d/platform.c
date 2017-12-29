
#include <linux/init.h>
#include <linux/module.h>
#include <linux/leds.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <asm/irq.h>
#include <linux/of.h>
#include <linux/pwm.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/printk.h>

#define LED_TRIGGER_DEFAULT		"none"

struct xxx_led_flash_data {
	int gpio_en;
	int brightness;
	struct led_classdev cdev;
	struct pinctrl *pinctrl;
	struct pinctrl_state *gpio_state_default;
};


struct sig_set
{
	int value;
	int usec;
};

static struct sig_set sig_high[] = {
	{0,20},
	{1,70},
};

static struct sig_set sig_low[] = {
	{0,70},
	{1,20},
};

static struct sig_set eod[] = {
	{0,20},
	{1,180},
};

static int gpio_signal_output(int gpio_no, int value)
{
	int rc;
	struct sig_set *s = value ? sig_high : sig_low;
	rc = gpio_direction_output(gpio_no, s[0].value);
	if(rc){
		pr_err("%s:%d failed gpio:%d,rc:%d",__func__,__LINE__,gpio_no,rc);
		return rc;
	}
	udelay(s[0].usec);
	rc = gpio_direction_output(gpio_no, s[1].value);
	if(rc){
		pr_err("%s:%d failed gpio:%d,rc:%d",__func__,__LINE__,gpio_no,rc);
		return rc;
	}
	udelay(s[1].usec);
	return rc;
}
static int led_trigger(int gpio_no, int value)
{
	int rc;
	int index;

	rc = gpio_signal_output(gpio_no,1);
	if(rc){
		pr_err("%s:%d failed gpio:%d,rc:%d",__func__,__LINE__,gpio_no,rc);
		goto ERROR;
	}
	udelay(50);
	//output address
	for(index = 0 ; index < 3;index++) {
		rc = gpio_signal_output(gpio_no,0);
		if(rc){
			pr_err("%s:%d failed gpio:%d,rc:%d",__func__,__LINE__,gpio_no,rc);
			goto ERROR;
		}
	}
	//output value
	for(index = 7 ; index >= 0;index--){
		rc = gpio_signal_output(gpio_no,value&(1<<index));
		if(rc){
			pr_err("%s:%d failed gpio:%d,rc:%d",__func__,__LINE__,gpio_no,rc);
			goto ERROR;
		}
	}
	//output eod
	for(index = 0 ; index < 2;index++){
		rc = gpio_direction_output(gpio_no, eod[index].value);
		if(rc){
			pr_err("%s:%d failed gpio:%d,rc:%d",__func__,__LINE__,gpio_no,rc);
		}
		udelay(eod[index].usec);
	}
ERROR:
	return rc;
}


static void xxx_led_brightness_set(struct led_classdev *led_cdev,
				    enum led_brightness value)
{
	struct xxx_led_flash_data *flash_led =
	    container_of(led_cdev, struct xxx_led_flash_data, cdev);

	int brightness = value;
	int flash_mode = 0;
	int flash_value = 0;
	int flash_output = 0;

	brightness = brightness&0x7f;
	if (brightness >= 64) {
		flash_mode = 2;
		flash_value = brightness - 64;
		if(flash_value == 0)
			flash_value = 1;
	} else if(brightness > 0) {
		flash_mode = 3;
		flash_value = brightness;
	}else{
		flash_mode = 0;
		flash_value = 0;
	}
	flash_output = (flash_mode << 6)|(flash_value);
	led_trigger(flash_led->gpio_en,flash_output);
	
	flash_led->brightness = brightness;
	pr_err("%s:%d flash_mode=%d, brightness=%d\n", __func__,
			__LINE__, flash_mode, brightness);
	return;
}

static enum led_brightness xxx_led_brightness_get(struct led_classdev
						   *led_cdev)
{
	struct xxx_led_flash_data *flash_led =
	    container_of(led_cdev, struct xxx_led_flash_data, cdev);
	return flash_led->brightness;
}

static int xxx_led_flash_probe(struct platform_device *pdev)
{
	int rc = 0;
	const char *temp_str;
	struct xxx_led_flash_data *flash_led = NULL;
	struct device_node *node = pdev->dev.of_node;

	flash_led = devm_kzalloc(&pdev->dev, sizeof(struct xxx_led_flash_data),
				 GFP_KERNEL);
	if (flash_led == NULL) {
		dev_err(&pdev->dev, "%s:%d Unable to allocate memory\n",
			__func__, __LINE__);
		return -ENOMEM;
	}

	flash_led->cdev.default_trigger = LED_TRIGGER_DEFAULT;
	rc = of_property_read_string(node, "linux,default-trigger", &temp_str);
	if (!rc)
		flash_led->cdev.default_trigger = temp_str;

	flash_led->gpio_en = of_get_named_gpio(node, "gpio_en", 0);
	if (flash_led->gpio_en < 0) {
		dev_err(&pdev->dev,
				"%s:%d Looking up %s property in node %s failed. rc =  %d\n",
				__func__, __LINE__, "gpio_en", node->full_name, flash_led->gpio_en);
		goto error;
	} else {
		rc = gpio_request(flash_led->gpio_en, "CAMERA_FLASH_EN");
		if (rc) {
			dev_err(&pdev->dev,
					"%s: Failed to request gpio %d,rc = %d\n",
					__func__, flash_led->gpio_en, rc);

			goto error;
		}
		gpio_direction_output(flash_led->gpio_en, 0);
	}


	rc = of_property_read_string(node, "linux,name", &flash_led->cdev.name);
	if (rc) {
		dev_err(&pdev->dev, "%s: Failed to read linux name. rc = %d\n",
			__func__, rc);
		goto error_reg;
	}

	platform_set_drvdata(pdev, flash_led);
	flash_led->cdev.max_brightness = 63;
	flash_led->cdev.brightness_set = xxx_led_brightness_set;
	flash_led->cdev.brightness_get = xxx_led_brightness_get;

	rc = led_classdev_register(&pdev->dev, &flash_led->cdev);
	if (rc) {
		dev_err(&pdev->dev, "%s: Failed to register led dev. rc = %d\n",
			__func__, rc);
		goto error_reg;
	}
	pr_err("%s: probe successfully!\n", __func__);
	return 0;

error_reg:	
	gpio_free(flash_led->gpio_en);
error:
	if (IS_ERR(flash_led->pinctrl))
		devm_pinctrl_put(flash_led->pinctrl);
	dev_err(&pdev->dev, "%s probe failed!\n", __func__);
	devm_kfree(&pdev->dev, flash_led);
	return rc;
}

static int xxx_led_flash_remove(struct platform_device *pdev)
{
	struct xxx_led_flash_data *flash_led =
	    (struct xxx_led_flash_data *)platform_get_drvdata(pdev);

	gpio_free(flash_led->gpio_en);
	if (IS_ERR(flash_led->pinctrl))
		devm_pinctrl_put(flash_led->pinctrl);
	led_classdev_unregister(&flash_led->cdev);
	devm_kfree(&pdev->dev, flash_led);
	return 0;
}

static struct of_device_id xxx_led_flash_match_table[] = {
	{ .compatible = "xxx,xxx", },
	{}
};

static struct platform_driver xxx_led_flash_driver = {
	.probe = xxx_led_flash_probe,
	.remove = xxx_led_flash_remove,
	.driver = {
		.name = "xxx_led",
		.owner = THIS_MODULE,
		.of_match_table = xxx_led_flash_match_table,
	},
};

static int __init xxx_led_flash_init(void)
{
	return platform_driver_register(&xxx_led_flash_driver);
}

static void __exit xxx_led_flash_exit(void)
{
	return platform_driver_unregister(&xxx_led_flash_driver);
}

late_initcall(xxx_led_flash_init);
module_exit(xxx_led_flash_exit);

MODULE_DESCRIPTION("Camera Flash Driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("leds:leds-flash");
