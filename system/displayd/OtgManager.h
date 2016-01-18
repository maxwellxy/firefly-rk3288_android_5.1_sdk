#ifndef __OTGMANAGER_H__
#define __OTGMANAGER_H__

class OtgManager {
	public:
		OtgManager();
		virtual ~OtgManager() {}
	private:
		int		OtgReadCfg();
		void	OtgCtrl(int otgstatus); 
};

#endif /*__OTGMANAGER_H__*/
