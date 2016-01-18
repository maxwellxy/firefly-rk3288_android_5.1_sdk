#ifndef __SCREEMSCALEMANAGER_H__
#define __SCREEMSCALEMANAGER_H__

#define DISPLAY_OVERSCAN_X 		0
#define DISPLAY_OVERSCAN_Y 		1
#define DISPLAY_OVERSCAN_LEFT		2
#define DISPLAY_OVERSCAN_RIGHT		3
#define DISPLAY_OVERSCAN_TOP		4
#define DISPLAY_OVERSCAN_BOTTOM		5
#define DISPLAY_OVERSCAN_ALL		6

class ScreenScaleManager {
	public:
		ScreenScaleManager();
		virtual ~ScreenScaleManager() {}
		void	SSMCtrl(int display, int direction, int scalevalue);
	private:
		int		SSMReadCfg();
		void	InitSysNode();
		char	MainDisplaySysNode[64];
		char	AuxDisplaySysNode[64];
		int		overscan_left;
		int		overscan_right;
		int		overscan_top;
		int		overscan_bottom;
};

#endif /*__SCREEMSCALEMANAGER_H__*/
