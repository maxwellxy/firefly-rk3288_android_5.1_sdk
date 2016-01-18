#ifdef INV_USE_LEGACY_NAMES
#ifndef MLCONTROL_LEGACY_H
#define MLCONTROL_LEGACY_H

#define MLCtrlGetParams inv_get_control_params
#define MLCtrlSetParams inv_set_control_params
#define MLSetControlSensitivity inv_set_control_sensitivity
#define MLSetControlFunc inv_set_control_func
#define MLGetControlSignal inv_get_control_signal
#define MLGetGridNum inv_get_grid_num
#define MLSetGridThresh inv_get_grid_thresh
#define MLSetGridMax inv_set_grid_max
#define MLSetGridCallback inv_set_grid_callback
#define MLSetControlData inv_set_control_data
#define MLGetControlData inv_get_control_data
#define MLControlUpdate inv_update_control
#define MLEnableControl inv_enable_control
#define MLDisableControl inv_disable_control

#endif //MLCONTROL_LEGACY_H
#endif // 
