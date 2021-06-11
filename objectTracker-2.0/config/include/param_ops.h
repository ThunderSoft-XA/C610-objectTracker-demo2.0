#ifndef __PARAM_H__
#define __PARAM_H__
#include "../../include/publicattr.h"

using namespace gstcamera;


#ifdef __cplusplus
extern "C"
{
#endif
typedef struct __stream_conf
{
	char gst_dic[16];
	int gst_id;
	char gst_name[16];
	char gst_sink[16];
	char decode[5];//h264 h265
	char enable[4]; //on off  tempoary don`t use
	int width;
	int height;
	int framerate;
	char path[1024];
	char format[8];
	AIType ai_type;   //0 = None, 1 = objectTracker
	GstType gst_type;
} StreamConf;

int param_init(void);
void param_deinit(void);
int param_set_int(const char *section, const char *key, int val);
const char *param_get_string(const char *section, const char *key, const char *notfound);
int param_get_int(const char *section, const char *key, int notfound);

int param_set_string(const char *section, const char *key, char *val);
int gst_param_load(char *fileName, StreamConf* pstStreamConf);
#ifdef __cplusplus
}
#endif
#endif
