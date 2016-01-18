#ifndef __HDCP_H__
#define __HDCP_H__
/* HDCP sysfs node */
#define HDCP_FIRMWARE_LOADING		"/sys/class/firmware/hdcp/loading"
#define HDCP_FIRMWARE_DATA		"/sys/class/firmware/hdcp/data"
#define HDCP_ENABLE			"/sys/class/misc/hdcp/enable"
#define HDCP_TRYTIMES			"/sys/class/misc/hdcp/trytimes"

/* HDCP Key file path */
#define KEY_FILE "/system/vendor/firmware/hdcp.keys"
/* HDCP SRM file path */
#define SRM_FILE "/system/vendor/firmware/hdcp.srm"

/* HDCP key size */
#define HDCP_KEY_SIZE	308
/* HDCP seed size*/
#define HDCP_SEED_SIZE 2
/* HDCP authentication size */
#define HDCP_AUTH_RETRY_TIMES	20

//#define HDCP_KEY_READ_FROM_FILE

#define HDCP_KEY_READ_FROM_IDB
#define HDCP_KEY_IDB_OFFSET		62

// If key is encrypted, set true. default is false.
#define HDCP_KEY_ENCRYPTED		0

extern void Hdcp_init(void);
extern void Hdcp_enable(void);

#define HDCP2_FW_NAME		"/system/vendor/firmware/hdcp2.fw"
#define HDCP2_MODULE		"/system/lib/modules/hdcp2.ko"

#ifdef __cplusplus
extern "C" {
#endif 

#ifdef SUPPORT_HDCP2

int rk_hdmi_hdcp2_init(char *fw_name, char *srm_name);
int rk_hdmi_hdcp2_start(void);
void rk_hdmi_hdcp2_exit(void);

#else
static int rk_hdmi_hdcp2_init(char *fw_name, char *srm_name)
{
	return 0;
}

static int rk_hdmi_hdcp2_start(void)
{
	return 0;
}

static void rk_hdmi_hdcp2_exit(void)
{
	
}
#endif

#ifdef __cplusplus
}
#endif

#endif /*__HDCP_H__*/