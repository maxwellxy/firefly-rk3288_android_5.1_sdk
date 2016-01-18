#ifndef _SF_VPU_VERSION_
#define _SF_VPU_VERSION_

typedef enum RK_CHIP_TYPE {
    NONE,
    RK29,
    RK30,
    RK31,

    RK_CHIP_NUM = 0x100,
} RK_CHIP_TYPE; 

#ifdef __cplusplus
class sf_info
{
public:
    RK_CHIP_TYPE get_chip_type() {return chip_type;}
    int  get_sf_version() {return sf_version;}
    char *get_sf_version_info() {return sf_version_info;}
    static sf_info *getInstance();
    virtual ~sf_info() {};
private:
    static sf_info *singleton;
    sf_info();
    int      sf_version;
    char    *sf_version_info;
    char    *sf_compile_info;
    RK_CHIP_TYPE chip_type;
};

extern "C" {
#endif /* __cplusplus */
RK_CHIP_TYPE get_chip_type();
#ifdef __cplusplus
}
#endif

#endif
