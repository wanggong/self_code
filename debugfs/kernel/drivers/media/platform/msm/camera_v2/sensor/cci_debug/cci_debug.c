//make wanggongzhen@hotmail.com
//addr_type : the reg address type 1 or 2
//data_type : the data type 1 or 2
//i2c_addr   : i2c address , program will shift left 1 , then add read or write flag
//reg_addr  : the register address will read or write
//value       : read i2c or write to i2c

#define pr_fmt(fmt) "%s:%d " fmt, __func__, __LINE__

#include <linux/module.h>
#include "msm_sd.h"
#include "msm_cci.h"
#include <linux/debugfs.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <soc/qcom/camera2.h>
#include <media/v4l2-subdev.h>
#include <media/msmb_camera.h>
#include "msm_camera_i2c.h"
#include "msm_camera_dt_util.h"
#include "msm_camera_io_util.h"


static struct msm_camera_i2c_fn_t debugfs_msm_sensor_cci_func_tbl = {
	.i2c_read = msm_camera_cci_i2c_read,
	.i2c_read_seq = msm_camera_cci_i2c_read_seq,
	.i2c_write = msm_camera_cci_i2c_write,
	.i2c_write_table = msm_camera_cci_i2c_write_table,
	.i2c_write_seq_table = msm_camera_cci_i2c_write_seq_table,
	.i2c_write_table_w_microdelay =
		msm_camera_cci_i2c_write_table_w_microdelay,
	.i2c_util = msm_sensor_cci_i2c_util,
	.i2c_poll =  msm_camera_cci_i2c_poll,
};


static struct msm_camera_i2c_client i2c_client_test; 
static struct msm_camera_cci_client debugfs_cci_client;


static unsigned long i2c_value = 0;
static unsigned long i2c_reg_addr = 0;
static unsigned long addr_type = MSM_CAMERA_I2C_WORD_DATA; 
static unsigned long data_type = MSM_CAMERA_I2C_BYTE_ADDR; 
static unsigned long i2c_addr = 0; 
static enum cci_i2c_master_t cci_master= MASTER_0;

static void init_cci(void)
{
	i2c_client_test.cci_client = &debugfs_cci_client;
	i2c_client_test.addr_type = addr_type;
	i2c_client_test.cci_client->sid = i2c_addr;
	i2c_client_test.cci_client->cci_i2c_master = cci_master ; 

	i2c_client_test.cci_client->retries = 3;
	i2c_client_test.cci_client->id_map = 0;

	i2c_client_test.cci_client->cci_subdev = msm_cci_get_subdev();

	i2c_client_test.i2c_func_tbl = &debugfs_msm_sensor_cci_func_tbl;

	i2c_client_test.i2c_func_tbl->i2c_util(&i2c_client_test, MSM_CCI_INIT);

	
}

static void cci_deinit(void)
{
	i2c_client_test.i2c_func_tbl->i2c_util(&i2c_client_test, MSM_CCI_RELEASE);
}
static int cci_value_set_func(void *data, u64 val)
{
	int rc = -1;
	i2c_value = (u32)val;
	init_cci();
	rc = msm_camera_cci_i2c_write(&i2c_client_test , i2c_reg_addr , (uint16_t)val , data_type);
	if(!rc)
	{
		pr_err("write cci error rc = %d" , rc);
	}
	cci_deinit();
	return 0;
}
static int cci_value_get_func(void *data, u64 *val)
{
	int rc = -1;
	init_cci();
	rc = msm_camera_cci_i2c_read(&i2c_client_test , i2c_reg_addr , (uint16_t *)val , data_type);
	if(!rc)
	{
		pr_err("read cci error rc = %d" , rc);
		*val = 0x57575777;
	}
	cci_deinit();
	return 0;
}


DEFINE_SIMPLE_ATTRIBUTE(fops_i2c_value, cci_value_get_func, cci_value_set_func, "0x%08llx\n");



static void create_cci_debugfs(void)
{
	struct dentry *cci_debugfs = debugfs_create_dir("cci_info", 0);
	debugfs_create_file("value", 0777,cci_debugfs, &i2c_value,&fops_i2c_value);
	debugfs_create_x32("reg_addr", S_IRWXUGO, cci_debugfs,(u32 *)&i2c_reg_addr);
	debugfs_create_x32("addr_type", S_IRWXUGO, cci_debugfs,(u32 *)&addr_type);
	debugfs_create_x32("data_type", S_IRWXUGO, cci_debugfs,(u32 *)&data_type);
	debugfs_create_x32("i2c_addr", S_IRWXUGO, cci_debugfs,(u32 *)&i2c_addr);
}

static int __init cci_debug_init_module(void)
{
	create_cci_debugfs();
	return 0;
}

module_init(cci_debug_init_module);
MODULE_DESCRIPTION("MSM CCI_DEBUG");
MODULE_LICENSE("GPL v2");
