#ifndef _DISPLAYMANAGER_H__
#define _DISPLAYMANAGER_H__

#define BUFFER_LENGTH	256
#define NAME_LENGTH		48
#define MODE_LENGTH		36
#define TYPE_LENGTH		12
#define MAX_NODE		10

enum {
	MAIN_DISPLAY = 0,
	AUX_DISPLAY
};

struct displaynode {
	char path[BUFFER_LENGTH];
	char mode[MODE_LENGTH];
	char name[NAME_LENGTH];
	int type;
	int property;
	int enable;
	int connect;
	struct displaynode *prev;
	struct displaynode *next;
};

class DisplayManager {
	public:
		DisplayManager();
		virtual ~DisplayManager() {}
		
		void 	getIfaceInfo(SocketClient *cli, int display);
		void 	getCurIface(SocketClient *cli, int display);
		int 	enableIface(int display, char* iface, int enable);
		
		void 	getModeList(SocketClient *cli, int display, char* iface);
		void 	getCurMode(SocketClient *cli, int display, char* iface);
		int 	setMode(int display, char* iface, char *mode);
		void 	setHDMIEnable(int display);
		void 	setHDMIDisable(int display);
		void 	selectNextIface(int display);
		void 	init(void);
		void	switchFramebuffer(int display, int xres, int yres);
		void	get3DModes(SocketClient *cli, int display, char* iface);
		void	get3DMode(SocketClient *cli, int display, char* iface);
		int		set3DMode(int display, char* iface, char *mode);
		void 	saveConfig(void);
	private:
		struct displaynode *main_display_list;
		struct displaynode *aux_display_list;
		int		powerup;
		int 	readConfig(void);
        int   	readUbootConfig(char *hdmi_mode, char *tve_mode);
		int 	readSysfs(void);
		int 	operateIfaceEnable(struct displaynode *node, int operate);
		int		readIfaceConnect(struct displaynode *node);
		int		operateIfaceMode(struct displaynode *node, int type, char *mode);
		void	display_list_add(struct displaynode *node);
		const char*	type2string(int type);
		int 	string2type(const char *str);
		void	led_ctrl(struct displaynode *node);
		void	updatesinkaudioinfo(struct displaynode *node);
};

#endif
