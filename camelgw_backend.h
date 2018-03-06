#include "intu_def.h"



/* triplet invoke on SSF*/
int CAMELGW_call_duration_control(u32 ic_dlg_id, DLG_CB *dlg_ptr, HDR *h, u8 *invokeid_ptr, u8 ReleaseFlag, unsigned int slice_volume);
int CAMELGW_invoke_triplet(u32 ic_dlg_id, DLG_CB *dlg_ptr, HDR *h, u8 ReleaseFlag, unsigned int quota);

/*invoke Apply Charging on SSF */
int CAMELGW_invoke_apply_charging(u32 ic_dlg_id, DLG_CB *dlg_ptr, HDR *h, u8 *invokeid_ptr, u8 ReleaseFlag, unsigned int slice_volume);

//incoke continue and close dialogue by prearranged end
int CAMELGW_invoke_continue(u32 ic_dlg_id, DLG_CB *dlg_ptr, HDR *h);//, u8 *invokeid_ptr);

//invoke continue but didn't close dialogue
int CAMELGW_invoke_continue2(u32 ic_dlg_id, DLG_CB *dlg_ptr, HDR *h);//, u8 *invokeid_ptr); //, u8 dlg_inaprm)
int CAMELGW_invoke_releasecall(u32 ic_dlg_id, DLG_CB *dlg_ptr, HDR *h, u8 *invokeid_ptr);
