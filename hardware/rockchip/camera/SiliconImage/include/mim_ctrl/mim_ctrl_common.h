#ifndef __MIM_CTRL_COMMON_H__
#define __MIM_CTRL_COMMON_H__

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Handle to mom ctrl process context.
 *
 */
typedef struct MimCtrlContext_s *MimCtrlContextHandle_t;


/**
 * @brief
 *
 * @note
 *
 */
enum MimCtrl_command_e
{
	MIM_CTRL_CMD_INVALID        = 0,
	MIM_CTRL_CMD_START          = 1,
	MIM_CTRL_CMD_STOP           = 2,
	MIM_CTRL_CMD_SHUTDOWN       = 3,
	MIM_CTRL_CMD_DMA_TRANSFER   = 4,
};

typedef int32_t MimCtrlCmdId_t;

/**
 *  @brief Event signalling callback
 *
 *  Callback for signalling some event which could require application interaction.
 *  The eventId (see @ref MimEventId_t) identifies the event and the content of 
 *  pParam depends on the event ID.
 *
 */
typedef void (* MimCtrlCompletionCb_t)
(
    MimCtrlCmdId_t  CmdId,          /**< The Commad Id of the notifying event */
    RESULT          result,         /**< Result of the executed command */
    const void      *pUserContext   /**< User data pointer that was passed on chain creation */
);


/* @} module_name*/

#ifdef __cplusplus
}
#endif


#endif /* __MIM_CTRL_COMMON_H__ */
