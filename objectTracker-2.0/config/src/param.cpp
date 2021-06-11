#include "config/include/iniparser.h"
#include "config/include/file_ops.h"
#include "config/include/param_ops.h"
#include "errno.h"


static char mfileName[1024]={0};
static dictionary *conf_dic;

static FILE *fp_dic = NULL;

int is_file_exist(char *file_path)
{
	if (!file_path)
		return -1;

	if (access(file_path, 0) != -1)
		return 0;

	return -1;
}


/*bref; sync
 * 		employee_time= 
 * 		visitor_time=
 * 		illegal_time= 
 * 		statistic
 * 		recognized_cnt= */
int param_init(void)
{
	int ret = is_file_exist(mfileName);
	if(ret != 0)
	{
		printf("%s: %s not exsit \n",__func__,mfileName);
	return -1;
	}
	conf_dic = iniparser_load(mfileName);
	if (!conf_dic) {
		printf("failed to load app config file:%s", mfileName);
		return -1;
	}


	return 0;
}

void param_deinit(void)
{
	if (conf_dic)
		iniparser_freedict(conf_dic);
	if(fp_dic)
    	fclose(fp_dic);
}


int param_set_int(const char *section, const char *key, int val)
{
	char buf[32];
	int notfound = -1;
	int ret = 0;

	if (!conf_dic)
		return notfound;

	snprintf(buf, sizeof(buf), "%s:%s", section, key);
	char int_buf[32];
	snprintf(int_buf, sizeof(int_buf), "%d",val);

	printf("%s: buf %s val %d ",__func__,buf,val);

	ret = iniparser_set(conf_dic, (const char *)buf, (const char *)int_buf);

//写入文件
     fp_dic = fopen(mfileName, "w");
    if( fp_dic == NULL ) {
        printf("stone:fopen error!\n");
		return -1;
    }
    iniparser_dump_ini(conf_dic,fp_dic);
	fclose(fp_dic);

	return ret;
}

int param_set_string(const char *section, const char *key, char *val)
{
	char buf[32];
	int notfound = -1;
	int ret = 0;

	if (!conf_dic)
		return notfound;

	snprintf(buf, sizeof(buf), "%s:%s", section, key);
	printf("%s: buf %s val %s ",__func__,buf,val);

	ret = iniparser_set(conf_dic, (const char *)buf, (const char *)val);
//写入文件
     fp_dic = fopen(mfileName, "w");
    if( fp_dic == NULL ) {
        printf("stone:fopen error!\n");
		return -1;
    }

    iniparser_dump_ini(conf_dic,fp_dic);
	fclose(fp_dic);

	return ret;
}

const char *param_get_string(const char *section, const char *key, const char *notfound)
{
	char buf[32];
	char *str = NULL;

	if (!conf_dic)
		return notfound;

	snprintf(buf, sizeof(buf), "%s:%s", section, key);

	str =  (char *)iniparser_getstring(conf_dic, buf, notfound);
	return (const char *)str;
}

int param_get_int(const char *section, const char *key, int notfound)
{
	char buf[32];
	int ret = 0;

	if (!conf_dic)
		return notfound;

	snprintf(buf, sizeof(buf), "%s:%s", section, key);

	ret = iniparser_getint(conf_dic, buf, notfound);

	return ret;
}

int gst_param_load(char *fileName, StreamConf* pstCameraConf)
{
	sprintf(mfileName,"%s",fileName);
	
	int ret = param_init();
	if(ret)
		return -1;

	sprintf(pstCameraConf->gst_name, "%s",param_get_string(pstCameraConf->gst_dic,"gstname","gst_zero"));
	sprintf(pstCameraConf->gst_sink, "%s",param_get_string(pstCameraConf->gst_dic,"sinkname","gst_sink"));
	sprintf(pstCameraConf->decode, "%s",param_get_string(pstCameraConf->gst_dic,"decode","NULL"));
	sprintf(pstCameraConf->format, "%s",param_get_string(pstCameraConf->gst_dic,"format","NV12"));
	sprintf(pstCameraConf->enable, "%s",param_get_string(pstCameraConf->gst_dic,"enable","off"));
	sprintf(pstCameraConf->path, "%s",param_get_string(pstCameraConf->gst_dic,"path","NULL"));

	pstCameraConf->gst_type = (GstType)param_get_int(pstCameraConf->gst_dic,"gsttype",0);
	pstCameraConf->ai_type = (AIType)param_get_int(pstCameraConf->gst_dic,"AIType",0);
	pstCameraConf->framerate = param_get_int(pstCameraConf->gst_dic,"framerate",30);
	pstCameraConf->height = param_get_int(pstCameraConf->gst_dic,"height",1080);
	pstCameraConf->width = param_get_int(pstCameraConf->gst_dic,"width",1920);
	pstCameraConf->gst_id = param_get_int(pstCameraConf->gst_dic,"gstid",0);
	

	param_deinit();
	printf("%s\n",__FUNCTION__);
	printf("gstID: %d\n",pstCameraConf->gst_id);
	printf("gstName: %s\n",pstCameraConf->gst_name);
	printf("sinkName: %s\n",pstCameraConf->gst_sink);
	printf("gsttype: %d\n",pstCameraConf->gst_type);
	printf("AIType: %d\n",pstCameraConf->ai_type);
	printf("enable: %s\n",pstCameraConf->enable);
	printf("path: %s\n",pstCameraConf->path);
	printf("decode: %s\n",pstCameraConf->decode);
	printf("framerate: %d\n",pstCameraConf->framerate);
	printf("height: %d\n",pstCameraConf->height);
	printf("width: %d\n",pstCameraConf->width);
	/*
	printf("w_x: %d\n",pstCameraConf->w_x);
	printf("w_y: %d\n",pstCameraConf->w_y);
	printf("w_h: %d\n",pstCameraConf->w_h);
	printf("w_w: %d\n",pstCameraConf->w_w);
	*/
	if(strncmp(pstCameraConf->decode, "NULL",strlen("NULL")) == 0 || strncmp(pstCameraConf->path,"NULL",strlen("NULL")) == 0)
	{
		printf("%s: error\n",__FUNCTION__);
		return -1;
	}
	return 0;
}
