/*******************************************************************************
 * 文件名称： ftp_client.c
 * 内容摘要： 本文件提供ftp功能实现
 
 *******************************************************************************/

#include <arpa/inet.h>
#include "pub_div.h"
#include "tcfs_log.h"
#include "dba_define.h"
#include "dba_struct.h"
#include "dba_func.h"
#include "dba_ftp_client.h"

/*******************************************************************************/

/**********************************************************************
* 函数名称：dba_ftp_create_conctrl_connect
* 功能描述：create conctrl-connect
************************************************************************/
DWORD dba_ftp_create_conctrl_connect(INT *ctrl_socket,
                                     const char *ftp_server_ip,
                                     const WORD ftp_port)
{
    struct sockaddr_in serv_addr;
    char   recv_line[FTP_RCV_LEN] = {0};

    DBA_NULLPOINTER_CHK_RTVALUE(ctrl_socket);
    DBA_NULLPOINTER_CHK_RTVALUE(ftp_server_ip);

    *ctrl_socket = socket(AF_INET,SOCK_STREAM,0);
    if(*ctrl_socket < 0)
    {
        DBA_PRN_ERROR("create conctrl socket failed.");
        return DBA_RC_FTP_CREATE_CTRL_SOCKET_FAIL;
    }

    //设置接收和发送超时
    struct timeval timeout =
        {
            5,0
        }
        ;//设置接收时延10s
    setsockopt(*ctrl_socket,SOL_SOCKET,SO_SNDTIMEO,(const char*)&timeout,sizeof(timeout));
    setsockopt(*ctrl_socket,SOL_SOCKET,SO_RCVTIMEO,(const char*)&timeout,sizeof(timeout));

    bzero(&serv_addr,sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(ftp_port);
    serv_addr.sin_addr.s_addr = inet_addr(ftp_server_ip);

    /*lint -e740 */
    if( connect(*ctrl_socket,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) <0 )
    {
        DBA_PRN_ERROR("connect control port fail.");
        return DBA_RC_FTP_CONNECT_CTRL_PORT_FAIL;
    }
    /*lint +e740 */

    if( recv(*ctrl_socket,recv_line,FTP_RCV_LEN,0) < 0 )
    {
        DBA_PRN_ERROR("recv fail.");
        return DBA_RC_FTP_RECV_CTRL_PORT_FAIL;
    }
    else if( strncmp(recv_line,"220",3) == 0 )
    {
        DBA_PRN_DEBUG("ftp server is ready.");
    }
    else
    {
        DBA_PRN_ERROR("ftp server is not ready(%s).",recv_line);
        return DBA_RC_FTP_SERVER_NOT_READY;
    }

    return DBA_RC_OK;
}

/**********************************************************************
* 函数名称：dba_ftp_user
* 功能描述：向ftp server表明身份
************************************************************************/
DWORD dba_ftp_user(INT ctrl_socket,const char *user_name)
{
    char recv_line[FTP_RCV_LEN] = {0};
    char send_line[FTP_SEND_LEN] = {0};

    DBA_NULLPOINTER_CHK_RTVALUE(user_name);

    sprintf(send_line,"USER %s\r\n",user_name);

    if(send(ctrl_socket,send_line,strlen(send_line),0) < 0)
    {
        DBA_PRN_ERROR("send user(%s) fail.",send_line);
        return DBA_RC_FTP_USER_SEND_FAIL;
    }

    recv(ctrl_socket,recv_line,FTP_RCV_LEN,0);
    if(strncmp(recv_line,"331",3) == 0)
    {
        DBA_PRN_DEBUG("send user success,please specify passwd.");
    }
    else
    {
        DBA_PRN_ERROR("recv user(%s) fail(%s).",send_line,recv_line);
        return DBA_RC_FTP_USER_RECV_FAIL;
    }

    return DBA_RC_OK;
}
/**********************************************************************
* 函数名称：dba_ftp_pass
* 功能描述：向ftp server认证密码
************************************************************************/
DWORD dba_ftp_pass(INT ctrl_socket,const char *passwd)
{
    char recv_line[FTP_RCV_LEN] = {0};
    char send_line[FTP_SEND_LEN] = {0};

    DBA_NULLPOINTER_CHK_RTVALUE(passwd);

    sprintf(send_line,"PASS %s\r\n",passwd);

    if(send(ctrl_socket,send_line,strlen(send_line),0) < 0)
    {
        DBA_PRN_ERROR("send pass(%s) fail.",send_line);
        return DBA_RC_FTP_PASS_SEND_FAIL;
    }

    recv(ctrl_socket,recv_line,FTP_RCV_LEN,0);
    if(strncmp(recv_line,"230",3) == 0)
    {
        DBA_PRN_DEBUG("login success.");
    }
    else
    {
        DBA_PRN_ERROR("recv pass(%s) fail(%s).",send_line,recv_line);
        return DBA_RC_FTP_PASS_RECV_FAIL;
    }

    return DBA_RC_OK;
}

/**********************************************************************
* 函数名称：dba_ftp_pasv
* 功能描述：请求服务器等待数据连接
* 输入参数：
* 输出参数：
* 返 回 值：
* 其它说明：
* 修改日期     版本号     修改人      修改内容
* 
************************************************************************/
DWORD dba_ftp_pasv(INT ctrl_socket,DWORD *data_socket_port)
{
    INT  addr[6];
    char recv_line[FTP_RCV_LEN] = {0};
    char send_line[FTP_SEND_LEN] = {0};

    DBA_NULLPOINTER_CHK_RTVALUE(data_socket_port);

    strncpy(send_line,"PASV\r\n",FTP_SEND_LEN-1);

    if(send(ctrl_socket,send_line,strlen(send_line),0) < 0)
    {
        DBA_PRN_ERROR("send pasv fail.");
        return DBA_RC_FTP_PASV_SEND_FAIL;
    }

    recv(ctrl_socket,recv_line,FTP_RCV_LEN,0);
    if(strncmp(recv_line,"227",3) == 0)
    {
        DBA_PRN_DEBUG("enter pasv mode success(%s).",recv_line);
    }
    else
    {
        DBA_PRN_ERROR("recv pasv fail(%s).",recv_line);
        return DBA_RC_FTP_PASV_RECV_FAIL;
    }

    sscanf(recv_line, "%*[^(](%d,%d,%d,%d,%d,%d)",
           &addr[0],&addr[1],&addr[2],&addr[3],&addr[4],&addr[5]);
    *data_socket_port = (DWORD)(addr[4]*256 + addr[5]);

    return DBA_RC_OK;
}

/**********************************************************************
* 函数名称：dba_ftp_size
* 功能描述：获取文件大小
************************************************************************/
DWORD dba_ftp_size(INT ctrl_socket,const char *remote_file_path,DWORD *file_size)
{
    char recv_line[FTP_RCV_LEN] = {0};
    char send_line[FTP_SEND_LEN] = {0};
    WORD i = 0;
    char *ptr = NULL;
    char str_file_size[32] = {0};

    DBA_NULLPOINTER_CHK_RTVALUE(remote_file_path);
    DBA_NULLPOINTER_CHK_RTVALUE(file_size);

    sprintf(send_line,"SIZE %s\r\n",remote_file_path);

    if(send(ctrl_socket,send_line,strlen(send_line),0) < 0)
    {
        DBA_PRN_ERROR("send size fail.");
        return DBA_RC_FTP_SIZE_SEND_FAIL;
    }

    recv(ctrl_socket,recv_line,FTP_RCV_LEN,0);
    if(strncmp(recv_line,"213",3) == 0)
    {
        ptr = recv_line + 4;
        while(((*ptr)!='\r') && (i < 32))
        {
            str_file_size[i++]=*ptr;
            ptr++;
        }

        sscanf(str_file_size, "%ud", file_size);
        DBA_PRN_DEBUG("recv size success.");
    }
    else
    {
        DBA_PRN_ERROR("recv size fail(%s).",recv_line);
        return DBA_RC_FTP_SIZE_RECV_FAIL;
    }

    return DBA_RC_OK;
}

/**********************************************************************
* 函数名称：dba_ftp_type
* 功能描述: 设置数据传送类型
************************************************************************/
DWORD dba_ftp_type(INT ctrl_socket,const char *type)
{
    char recv_line[FTP_RCV_LEN] = {0};
    char send_line[FTP_SEND_LEN] = {0};

    DBA_NULLPOINTER_CHK_RTVALUE(type);

    sprintf(send_line,"TYPE %s\r\n",type);

    if(send(ctrl_socket,send_line,strlen(send_line),0) < 0)
    {
        DBA_PRN_ERROR("send type fail.");
        return DBA_RC_FTP_TYPE_SEND_FAIL;
    }

    recv(ctrl_socket,recv_line,FTP_RCV_LEN,0);
    if(strncmp(recv_line,"200",3) == 0)
    {
        DBA_PRN_DEBUG("setting binary data transmission success.");
    }
    else
    {
        DBA_PRN_ERROR("recv type fail(%s).",recv_line);
        return DBA_RC_FTP_TYPE_RECV_FAIL;
    }

    return DBA_RC_OK;
}
/**********************************************************************
* 函数名称：dba_ftp_create_data_connect
* 功能描述：创建数据连接
************************************************************************/
DWORD dba_ftp_create_data_connect(INT *data_socket,DWORD port)
{
    struct sockaddr_in serv_addr;
    char   ftp_server_ip[IPLEN] = {0};
    WORD  ftp_port = 0;

    DBA_NULLPOINTER_CHK_RTVALUE(data_socket);

    dba_get_ftp_ipandport(ftp_server_ip,&ftp_port);

    *data_socket = socket(AF_INET,SOCK_STREAM,0);
    if(*data_socket < 0)
    {
        DBA_PRN_ERROR("create data socket failed.");
        return DBA_RC_FTP_CREATE_DATA_SOCKET_FAIL;
    }

    //设置接收和发送超时
    struct timeval timeout =
        {
            5,0
        }
        ;//设置接收时延10s
    setsockopt(*data_socket,SOL_SOCKET,SO_SNDTIMEO,(const char*)&timeout,sizeof(timeout));
    setsockopt(*data_socket,SOL_SOCKET,SO_RCVTIMEO,(const char*)&timeout,sizeof(timeout));

    bzero(&serv_addr,sizeof(serv_addr));

    serv_addr.sin_family=AF_INET;
    serv_addr.sin_addr.s_addr=inet_addr(ftp_server_ip);
    serv_addr.sin_port=htons((WORD)port);

    /*lint -e740 */
    if( connect(*data_socket,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0 )
    {
        DBA_PRN_ERROR("connect data port fail.");
        return DBA_RC_FTP_CONNECT_DATA_PORT_FAIL;
    }
    /*lint +e740 */

    return DBA_RC_OK;
}
/**********************************************************************
* 函数名称：dba_ftp_login
* 功能描述：ftp client login
************************************************************************/
DWORD dba_ftp_login(INT *ctrl_socket)
{
    char user_name[NAMELEN]   = {0};
    char passwd[PASSWDLEN]    = {0};
    char ftp_server_ip[IPLEN] = {0};
    WORD ftp_port = SERV_PORT;

    DBA_NULLPOINTER_CHK_RTVALUE(ctrl_socket);

    dba_get_ftp_info(ftp_server_ip,&ftp_port,user_name,passwd);

    DBA_NOK_CHK_RTCODE(dba_ftp_create_conctrl_connect(ctrl_socket,ftp_server_ip,ftp_port));
    //user
    DBA_NOK_CHK_RTCODE(dba_ftp_user(*ctrl_socket,user_name));
    //passwd
    DBA_NOK_CHK_RTCODE(dba_ftp_pass(*ctrl_socket,passwd));

    return DBA_RC_OK;
}
/**********************************************************************
* 函数名称：dba_ftp_nlst
* 功能描述: 发送nlst命令
************************************************************************/
DWORD dba_ftp_nlst(INT ctrl_socket,const char *remote_file_path)
{
    char recv_line[FTP_RCV_LEN] = {0};
    char send_line[FTP_SEND_LEN] = {0};

    DBA_NULLPOINTER_CHK_RTVALUE(remote_file_path);

    sprintf(send_line,"NLST %s\r\n",remote_file_path);

    if(send(ctrl_socket,send_line,strlen(send_line),0) < 0)
    {
        DBA_PRN_ERROR("send nlst(%s) fail.",send_line);
        return DBA_RC_FTP_NLST_SEND_FAIL;
    }

    recv(ctrl_socket,recv_line,FTP_RCV_LEN,0);
    if(strncmp(recv_line,"150",3) == 0)
    {
        DBA_PRN_DEBUG("send nlst(%s) success(%s).",send_line,recv_line);
        if(NULL != strstr(recv_line,"226"))
        {
            DBA_PRN_DEBUG("nlst path(%s) is empty.",remote_file_path);
            return DBA_RC_FTP_FILE_NOT_EXIST;
        }
    }
    else
    {
        DBA_PRN_ERROR("recv nlst(%s) fail(%s).",send_line,recv_line);
        return DBA_RC_FTP_NLST_RECV_FAIL;
    }

    return DBA_RC_OK;
}
/**********************************************************************
* 函数名称：dba_ftp_nlst
* 功能描述: ftp nlst (实现的不完全，只能接受缓存511个字节)
************************************************************************/
DWORD dba_ftp_nlst_reciev_data(INT ctrl_socket,INT data_socket,const char* remote_file_path)
{
    DWORD ret               = DBA_RC_FTP_FILE_EXIST;
    long  recv_bytes        = -1;
    char  recv_line[FTP_RCV_LEN] = {0};

    DBA_NULLPOINTER_CHK_RTVALUE(remote_file_path);

    recv_bytes = recv(data_socket,recv_line,FTP_RCV_LEN,0);
    if(recv_bytes == 0)
    {
        DBA_PRN_DEBUG("nlst path(%s) is empty.",remote_file_path);
        ret = DBA_RC_FTP_FILE_NOT_EXIST;
    }
    else if(recv_bytes < 0)
    {
        DBA_PRN_ERROR("recv nlst data fail(%s).",recv_line);
        ret = DBA_RC_FTP_NLST_RECV_FAIL;
    }
    else
    {
        DBA_PRN_DEBUG("nlst path(%s) is not empty.",remote_file_path);
        ret = DBA_RC_FTP_FILE_EXIST;
    }

    dba_close_ftp_socket(data_socket);

    memset(recv_line,0,sizeof(recv_line));
    recv(ctrl_socket,recv_line,FTP_RCV_LEN,0);
    if(strncmp(recv_line,"226",3) == 0)
    {
        DBA_PRN_DEBUG("Directory send OK.");
    }
    else
    {
        DBA_PRN_ERROR("Directory send fail(%s).",recv_line);
        return DBA_RC_FTP_NLST_RECV_FAIL;
    }

    return ret;
}

/**********************************************************************
* 函数名称：dba_ftp_nlst_file_by_pasv
* 功能描述: ftp nlst(PASV)
************************************************************************/
DWORD dba_ftp_nlst_file_by_pasv(INT ctrl_socket,const char *remote_file_path)
{
    DWORD ret = DBA_RC_OK;
    DWORD ftp_server_port = 0;
    INT   data_socket     = INVALID_SOCKET;

    DBA_NULLPOINTER_CHK_RTVALUE(remote_file_path);

    //set mode(pasv)
    DBA_NOK_CHK_RTCODE(dba_ftp_pasv(ctrl_socket,&ftp_server_port));
    //set type(binary)
    DBA_NOK_CHK_RTCODE(dba_ftp_type(ctrl_socket,"I"));
    //create data socket
    ret = dba_ftp_create_data_connect(&data_socket,ftp_server_port);
    if(DBA_RC_OK != ret)
    {
        dba_close_ftp_socket(data_socket);
        return ret;
    }

    //nlst
    ret = dba_ftp_nlst(ctrl_socket,remote_file_path);
    if(DBA_RC_OK != ret)
    {
        dba_close_ftp_socket(data_socket);
        return ret;
    }

    //recv nlst data
    ret = dba_ftp_nlst_reciev_data(ctrl_socket,data_socket,remote_file_path);
    dba_close_ftp_socket(data_socket);

    return ret;
}
/**********************************************************************
* 函数名称：dba_ftp_stor
* 功能描述: 发送stor命令
************************************************************************/
DWORD dba_ftp_stor(INT ctrl_socket,const CHAR *remote_file_path)
{
    CHAR recv_line[FTP_RCV_LEN] = {0};
    CHAR send_line[FTP_SEND_LEN] = {0};

    DBA_NULLPOINTER_CHK_RTVALUE(remote_file_path);

    sprintf(send_line,"STOR %s\r\n",remote_file_path);

    if(send(ctrl_socket,send_line,strlen(send_line),0) < 0)
    {
        DBA_PRN_ERROR("send stor(%s) fail.",send_line);
        return DBA_RC_FTP_STOR_SEND_FAIL;
    }

    recv(ctrl_socket,recv_line,FTP_RCV_LEN,0);
    if(strncmp(recv_line,"150",3) == 0)
    {
        DBA_PRN_DEBUG("send stor success(%s).",recv_line);
    }
    else
    {
        DBA_PRN_ERROR("recv stor(%s) fail(%s).",send_line,recv_line);
        return DBA_RC_FTP_STOR_RECV_FAIL;
    }

    return DBA_RC_OK;
}
/**********************************************************************
* 函数名称：dba_ftp_send_data
* 功能描述: ftp client upload file
************************************************************************/
DWORD dba_ftp_send_data(INT ctrl_socket,INT data_socket,const CHAR* local_file_path)
{
    FILE   *fd               = NULL;
    CHAR   recv_line[FTP_RCV_LEN] = {0};
    DWORD  size              = 0;
    CHAR   buffer[65535]     = {0};

    DBA_NULLPOINTER_CHK_RTVALUE(local_file_path);

    fd = fopen(local_file_path,"r");
    if(NULL == fd)
    {
        DBA_PRN_ERROR("open local file fail.");
        return DBA_RC_FTP_OPEN_LOCAL_FILE_FAIL;
    }

    while(!feof(fd))
    {
        memset(buffer,0,sizeof(buffer));
        size = (DWORD)fread(buffer,1,sizeof(buffer),fd);
        if(ferror(fd))
        {
            fclose(fd);
            DBA_PRN_ERROR("read local file fail.");
            return DBA_RC_FTP_READ_LOCAL_FILE_FAIL;
        }
        else
        {
            send(data_socket,buffer,size,0);
        }
    }

    fclose(fd);
    dba_close_ftp_socket(data_socket);

    recv(ctrl_socket,recv_line,FTP_RCV_LEN,0);
    if(strncmp(recv_line,"226",3)==0)
    {
        DBA_PRN_DEBUG("send data success(%s).",recv_line);
    }
    else
    {
        DBA_PRN_ERROR("send data fail or file is empty(%s).",recv_line);
        return DBA_RC_FTP_PUT_RECV_FAIL;
    }

    return DBA_RC_OK;
}

/**********************************************************************
* 函数名称：dba_ftp_up_file_by_pasv
* 功能描述：ftp 上传文件(PASV)
************************************************************************/
DWORD dba_ftp_up_file_by_pasv(INT ctrl_socket,
                              const char *local_file_path,
                              const char *remote_file_path)
{
    DWORD ret = DBA_RC_OK;
    DWORD ftp_server_port = 0;
    INT  data_socket      = INVALID_SOCKET;

    DBA_NULLPOINTER_CHK_RTVALUE(local_file_path);
    DBA_NULLPOINTER_CHK_RTVALUE(remote_file_path);

    //set type(binary)
    DBA_NOK_CHK_RTCODE(dba_ftp_type(ctrl_socket,"I"));
    //set mode(pasv)
    DBA_NOK_CHK_RTCODE(dba_ftp_pasv(ctrl_socket,&ftp_server_port));
    //create data socket
    ret = dba_ftp_create_data_connect(&data_socket,ftp_server_port);
    if(DBA_RC_OK != ret)
    {
        dba_close_ftp_socket(data_socket);
        return ret;
    }

    //stor
    ret = dba_ftp_stor(ctrl_socket,remote_file_path);
    if(DBA_RC_OK != ret)
    {
        dba_close_ftp_socket(data_socket);
        return ret;
    }

    //send data
    ret = dba_ftp_send_data(ctrl_socket,data_socket,local_file_path);
    if(DBA_RC_OK != ret)
    {
        dba_close_ftp_socket(data_socket);
        return ret;
    }

    return DBA_RC_OK;
}
/**********************************************************************
* 函数名称：dba_ftp_quit
* 功能描述：ftp退出
************************************************************************/
DWORD dba_ftp_quit(INT ctrl_socket)
{
    char recv_line[FTP_RCV_LEN] = {0};
    char send_line[FTP_SEND_LEN] = {0};

    strncpy(send_line,"QUIT\r\n",FTP_SEND_LEN-1);

    if(send(ctrl_socket,send_line,strlen(send_line),0) < 0)
    {
        DBA_PRN_ERROR("send quit fail.");
        return DBA_RC_FTP_QUIT_SEND_FAIL;
    }

    recv(ctrl_socket,recv_line,FTP_RCV_LEN,0);
    if(strncmp(recv_line,"221",3) == 0)
    {
        DBA_PRN_DEBUG("send quit success(%s).",recv_line);
    }
    else
    {
        DBA_PRN_ERROR("recv quit fail(%s).",recv_line);
        return DBA_RC_FTP_QUIT_RECV_FAIL;
    }

    return DBA_RC_OK;
}
/**********************************************************************
* 函数名称：dba_ftp_mkd
* 功能描述：ftp 创建目录
************************************************************************/
DWORD dba_ftp_mkd(INT ctrl_socket,const char *path)
{
    char recv_line[FTP_RCV_LEN] = {0};
    char send_line[FTP_SEND_LEN] = {0};

    DBA_NULLPOINTER_CHK_RTVALUE(path);

    sprintf(send_line,"MKD %s\r\n",path);

    if(send(ctrl_socket,send_line,strlen(send_line),0) < 0)
    {
        DBA_PRN_ERROR("send mkd(%s) fail.",send_line);
        return DBA_RC_FTP_MKD_SEND_FAIL;
    }

    recv(ctrl_socket,recv_line,FTP_RCV_LEN,0);
    if(strncmp(recv_line,"257",3) == 0)
    {
        DBA_PRN_DEBUG("make directory success.");
    }
    else
    {
        DBA_PRN_ERROR("recv mkd(%s) fail(%s).",send_line,recv_line);
        return DBA_RC_FTP_MKD_RECV_FAIL;
    }

    return DBA_RC_OK;
}
/**********************************************************************
* 函数名称：dba_ftp_cwd
* 功能描述：ftp 切换目录
************************************************************************/
DWORD dba_ftp_cwd(INT ctrl_socket,const char *path)
{
    char recv_line[FTP_RCV_LEN] = {0};
    char send_line[FTP_SEND_LEN] = {0};

    DBA_NULLPOINTER_CHK_RTVALUE(path);

    sprintf(send_line,"CWD %s\r\n",path);

    if(send(ctrl_socket,send_line,strlen(send_line),0) < 0)
    {
        DBA_PRN_ERROR("send cwd(%s) fail.",send_line);
        return DBA_RC_FTP_CWD_SEND_FAIL;
    }

    recv(ctrl_socket,recv_line,FTP_RCV_LEN,0);
    if(strncmp(recv_line,"250",3) == 0)
    {
        DBA_PRN_DEBUG("directory changed success.");
    }
    else
    {
        DBA_PRN_ERROR("recv cwd(%s) fail(%s).",send_line,recv_line);
        return DBA_RC_FTP_CWD_RECV_FAIL;
    }

    return DBA_RC_OK;
}
/**********************************************************************
* 函数名称：dba_ftp_pwd
* 功能描述：ftp获取当前工作目录
************************************************************************/
DWORD dba_ftp_pwd(INT ctrl_socket,char *ftp_cur_dir)
{
    DBA_NULLPOINTER_CHK_RTVALUE(ftp_cur_dir);

    char send_line[FTP_SEND_LEN] = {0};
    
    strncpy(send_line,"PWD\r\n",FTP_SEND_LEN-1);
    if(send(ctrl_socket,send_line,strlen(send_line),0) < 0)
    {
        DBA_PRN_ERROR("send pwd fail.");
        return DBA_RC_FTP_PWD_SEND_FAIL;
    }
    CHAR recv_line[FTP_RCV_LEN] = {0};
    recv(ctrl_socket,recv_line,FTP_RCV_LEN,0);
    if(strncmp(recv_line,"257",3) == 0)
    {
        WORD i=0;
        CHAR cur_dir[PATHLEN]= {0};
        CHAR * ptr = recv_line+5;
        while(*(ptr)!='"')
        {
            cur_dir[i++]=*(ptr);
            ptr++;
        }
        cur_dir[i]='\0';
        DBA_PRN_DEBUG("ftp current directory is:\n%s\n.",cur_dir);
        snprintf(ftp_cur_dir,PATHLEN,"%s",cur_dir);
    }
    else
    {
        DBA_PRN_ERROR("recv pwd fail(%s).",recv_line);
        return DBA_RC_FTP_PWD_RECV_FAIL;
    }

    return DBA_RC_OK;
}
/**********************************************************************
* 函数名称：dba_ftp_retr
* 功能描述: 发送retr命令
************************************************************************/
DWORD dba_ftp_retr(INT ctrl_socket,const CHAR *remote_file_path)
{
    CHAR recv_line[FTP_RCV_LEN] = {0};
    CHAR send_line[FTP_SEND_LEN] = {0};

    DBA_NULLPOINTER_CHK_RTVALUE(remote_file_path);

    sprintf(send_line,"RETR %s\r\n",remote_file_path);

    if(send(ctrl_socket,send_line,strlen(send_line),0) < 0)
    {
        DBA_PRN_ERROR("send retr(%s) fail.",send_line);
        return DBA_RC_FTP_RETR_SEND_FAIL;
    }

    recv(ctrl_socket,recv_line,FTP_RCV_LEN,0);
    if(strncmp(recv_line,"300",3) <= 0)
    {
        DBA_PRN_DEBUG("send RETR success(%s).",recv_line);
    }
    else
    {
        DBA_PRN_ERROR("recv RETR(%s) fail(%s).",send_line,recv_line);
        return DBA_RC_FTP_RETR_RECV_FAIL;
    }

    return DBA_RC_OK;
}
/**********************************************************************
* 函数名称：dba_ftp_recv_data
* 功能描述: ftp client download file
************************************************************************/
DWORD dba_ftp_recv_data(INT ctrl_socket,INT data_socket,const char* local_file_path)
{
    FILE   *fd               = NULL;
    char   recv_line[FTP_RCV_LEN] = {0};
    long long  file_size     = 0;
    char   buffer[4096]      = {0};
    DWORD  recv_bytes        = 0;
    DWORD  write_bytes       = 0;

    DBA_NULLPOINTER_CHK_RTVALUE(local_file_path);

    fd = fopen(local_file_path,"w+");
    if(NULL == fd)
    {
        DBA_PRN_ERROR("open local file fail.");
        return DBA_RC_FTP_OPEN_LOCAL_FILE_FAIL;
    }

    while( (recv_bytes = (DWORD)recv(data_socket,buffer,sizeof(buffer),0)) > 0)
    {
        write_bytes = (DWORD)fwrite(buffer,1,recv_bytes,fd);
        if(write_bytes != recv_bytes)
        {
            dba_close_ftp_socket(data_socket);
            fclose(fd);
            DBA_PRN_ERROR("write local file fail.");
            return DBA_RC_FTP_WRITE_LOCAL_FILE_FAIL;
        }
        file_size += write_bytes;
    }
    fclose(fd);
    dba_close_ftp_socket(data_socket);

    recv(ctrl_socket,recv_line,FTP_RCV_LEN,0);
    if(strncmp(recv_line,"226",3) == 0)
    {
        DBA_PRN_DEBUG("recv data success(%s),file size is=%ld.",recv_line,file_size);
    }
    else
    {
        DBA_PRN_ERROR("recv data fail(%s).",recv_line);
        return DBA_RC_FTP_GET_RECV_FAIL;
    }

    return DBA_RC_OK;
}
/**********************************************************************
* 函数名称：dba_ftp_get_file
* 功能描述：ftp 下载文件
************************************************************************/
DWORD dba_ftp_get_file(INT ctrl_socket,const CHAR *remote_file_path,const CHAR *local_file_path)
{
    DWORD ret = DBA_RC_OK;
    DWORD ftp_server_port = 0;
    INT  data_socket      = INVALID_SOCKET;

    DBA_NULLPOINTER_CHK_RTVALUE(local_file_path);
    DBA_NULLPOINTER_CHK_RTVALUE(remote_file_path);

    //set type(binary)
    DBA_NOK_CHK_RTCODE(dba_ftp_type(ctrl_socket,"I"));
    //set mode(pasv)
    DBA_NOK_CHK_RTCODE(dba_ftp_pasv(ctrl_socket,&ftp_server_port));
    //create data socket
    ret = dba_ftp_create_data_connect(&data_socket,ftp_server_port);
    if(DBA_RC_OK != ret)
    {
        dba_close_ftp_socket(data_socket);
        return ret;
    }
    //retr
    ret = dba_ftp_retr(ctrl_socket,remote_file_path);
    if(DBA_RC_OK != ret)
    {
        dba_close_ftp_socket(data_socket);
        return ret;
    }

    //recv data
    ret = dba_ftp_recv_data(ctrl_socket,data_socket,local_file_path);
    if(DBA_RC_OK != ret)
    {
        dba_close_ftp_socket(data_socket);
        return ret;
    }

    return DBA_RC_OK;
}
/**********************************************************************
* 函数名称：dba_mk_model_dir
* 功能描述：创建model目录
************************************************************************/
DWORD dba_mk_model_dir(INT ctrl_socket,const char *dir_name)
{
    DBA_NULLPOINTER_CHK_RTVALUE(dir_name);

    DBA_NOK_CHK_RTCODE(dba_ftp_pwd(ctrl_socket,g_tFtpCurPath));
    DBA_NOK_CHK_RTCODE(dba_ftp_cwd(ctrl_socket,g_tFtpCurPath));

    DWORD ret = DBA_RC_OK;
    char   remote_uspp_path[PATHLEN] = {0};
    //create uspp
    if(1 != strlen(g_tFtpCurPath))
    {
        snprintf(remote_uspp_path,PATHLEN,"%s/%s",g_tFtpCurPath,"uspp");
    }
    else
    {
        snprintf(remote_uspp_path,PATHLEN,"%s%s",g_tFtpCurPath,"uspp");
    }
    ret = dba_ftp_cwd(ctrl_socket,remote_uspp_path);
    if(ret != DBA_RC_OK)
    {
        DBA_PRN_DEBUG("(%s) is not existed.",remote_uspp_path);
        DBA_NOK_CHK_RTCODE(dba_ftp_mkd(ctrl_socket,"uspp"));
        DBA_NOK_CHK_RTCODE(dba_ftp_cwd(ctrl_socket,remote_uspp_path));
    }
    else
    {
        DBA_PRN_DEBUG("(%s) is existed.",remote_uspp_path);
    }
    //create ms-name
    CHAR   remote_work_path[PATHLEN] = {0};
    snprintf(remote_work_path,PATHLEN,"%s/%s",remote_uspp_path,dir_name);

    ret = dba_ftp_cwd(ctrl_socket,remote_work_path);
    if(ret != DBA_RC_OK)
    {
        DBA_PRN_DEBUG("(%s) is not existed.",remote_work_path);
        DBA_NOK_CHK_RTCODE(dba_ftp_mkd(ctrl_socket,dir_name));
        DBA_NOK_CHK_RTCODE(dba_ftp_cwd(ctrl_socket,remote_work_path));
    }
    else
    {
        DBA_PRN_ERROR("(%s) is existed.",remote_work_path);
    }

    snprintf(g_tFtpModelPath,PATHLEN,"%s",remote_work_path);

    return DBA_RC_OK;
}
/**********************************************************************
* 函数名称：dba_ftp_port
* 功能描述: 设置数据端口
************************************************************************/
DWORD dba_ftp_port(INT ctrl_socket,INT *data_socket)
{
    char recv_line[FTP_RCV_LEN] = {0};
    char send_line[FTP_SEND_LEN] = {0};

    DBA_NULLPOINTER_CHK_RTVALUE(data_socket);

    *data_socket=socket(AF_INET,SOCK_STREAM,0);
    if(*data_socket<0)
    {
        DBA_PRN_ERROR("create data port failed.\n");
        return DBA_RC_FTP_CREATE_DATA_SOCKET_FAIL;
    }
    //设置接收和发送超时
    struct timeval timeout = {5,0}        ;//设置接收时延5s
    setsockopt(*data_socket,SOL_SOCKET,SO_SNDTIMEO,(const char*)&timeout,sizeof(timeout));
    setsockopt(*data_socket,SOL_SOCKET,SO_RCVTIMEO,(const char*)&timeout,sizeof(timeout));

    struct sockaddr_in serv_addr;
    serv_addr.sin_family=AF_INET;
    /*
    char *ip = getenv("ftpdataip");
    DBA_NULLPOINTER_CHK_RTVALUE(ip);
    serv_addr.sin_addr.s_addr = inet_addr(ip);
    */
    serv_addr.sin_port =htons(49555);
    /*lint -e740 */
    
    unsigned int flag=1;
    unsigned int flag_len = sizeof(flag);
    if (setsockopt(*data_socket,SOL_SOCKET,SO_REUSEADDR,(const char*)&flag,flag_len) < 0)
    {
        DBA_PRN_ERROR("failed to set socket opt(port reuse).\n");
        return DBA_RC_FTP_SET_OPT_FAIL;
    }
    
    unsigned int len=sizeof(serv_addr);
    if (bind(*data_socket, (struct sockaddr *) &serv_addr, len) < 0)
    {
        DBA_PRN_ERROR("failed to bind a port.\n");
        return DBA_RC_FTP_BIND_PORT_FAIL;
    }
    if (listen(*data_socket, 64) < 0)
    {
        DBA_PRN_ERROR("failed to listen on port(%d).\n",ntohs(serv_addr.sin_port));
        return DBA_RC_FTP_CREATE_LST_FAIL;
    }
    socklen_t sock_len = sizeof(serv_addr);
    if(getsockname(*data_socket,(struct sockaddr *)&serv_addr,&sock_len) != 0)
    {
        DBA_PRN_ERROR("failed to getsockname.\n");
        return DBA_RC_FTP_GET_LOCAL_IP_FAIL;
    }
    /*lint +e740 */
    unsigned char *portp = NULL;
    portp = (unsigned char *) &serv_addr.sin_port;
    /*
    unsigned char *adp = NULL;
    adp = (unsigned char *) &serv_addr.sin_addr;
    
    snprintf(send_line, FTP_SEND_LEN, "PORT %d,%d,%d,%d,%d,%d\r\n",
             adp[0] & 0xff, adp[1] & 0xff, adp[2] & 0xff, adp[3] & 0xff,
             portp[0] & 0xff, portp[1] & 0xff);
    */
    
    char *ip = getenv("ftpdataip");
    DBA_NULLPOINTER_CHK_RTVALUE(ip);
    snprintf(send_line, FTP_SEND_LEN, "PORT %s,%d,%d\r\n",
             ip,portp[0] & 0xff,portp[1] & 0xff);
    
    if(send(ctrl_socket,send_line,strlen(send_line),0) < 0)
    {
        DBA_PRN_ERROR("send port(%s) fail.",send_line);
        return DBA_RC_FTP_PORT_SEND_FAIL;
    }
    recv(ctrl_socket,recv_line,FTP_RCV_LEN,0);
    if(strncmp(recv_line,"200",3)==0)
    {
        DBA_PRN_DEBUG("%s",recv_line);
    }
    else
    {
        DBA_PRN_ERROR("recv port(%s) fail(%s).",send_line,recv_line);
        return DBA_RC_FTP_PORT_RECV_FAIL;
    }

    return DBA_RC_OK;
}

/**********************************************************************
* 函数名称：dba_ftp_accpet_data_port
* 功能描述：ftp port模式接收服务端连接
************************************************************************/
DWORD dba_ftp_accpet_data_port(INT data_socket,INT *srv_data_socket)
{
    DBA_NULLPOINTER_CHK_RTVALUE(srv_data_socket);
    /*lint -e740 */
    struct sockaddr_in serv_addr;
    socklen_t len = (socklen_t)sizeof(serv_addr);
    *srv_data_socket = accept(data_socket,(struct sockaddr *)&serv_addr, &len);
    if( *srv_data_socket < 0 )
    {
        DBA_PRN_ERROR("accept the srv connect fail.");
        return DBA_RC_FTP_ACCEPT_SRV_CNT_FAIL;
    }
    /*lint +e740 */

    return DBA_RC_OK;
}
/**********************************************************************
* 函数名称：dba_ftp_up_file_by_port
* 功能描述：ftp 用port模式上传文件
************************************************************************/
DWORD dba_ftp_up_file_by_port(INT ctrl_socket,
                              const char *local_file_path,
                              const char *remote_file_path)
{
    DWORD ret = DBA_RC_OK;
    INT  data_socket      = INVALID_SOCKET;

    DBA_NULLPOINTER_CHK_RTVALUE(local_file_path);
    DBA_NULLPOINTER_CHK_RTVALUE(remote_file_path);
    //set type(binary)
    DBA_NOK_CHK_RTCODE(dba_ftp_type(ctrl_socket,"I"));
    //set mode(port)
    ret = dba_ftp_port(ctrl_socket,&data_socket);
    if(DBA_RC_OK != ret)
    {
        dba_close_ftp_socket(data_socket);
        return ret;
    }
    //stor
    ret = dba_ftp_stor(ctrl_socket,remote_file_path);
    if(DBA_RC_OK != ret)
    {
        dba_close_ftp_socket(data_socket);
        return ret;
    }
    //accept srv connect
    INT  srv_data_socket = INVALID_SOCKET;
    ret = dba_ftp_accpet_data_port(data_socket,&srv_data_socket);
    if(DBA_RC_OK != ret)
    {
        dba_close_ftp_socket(data_socket);
        return ret;
    }
    //send data
    ret = dba_ftp_send_data(ctrl_socket,srv_data_socket,local_file_path);
    if(DBA_RC_OK != ret)
    {
        dba_close_ftp_socket(data_socket);
        return ret;
    }

    return DBA_RC_OK;
}
/**********************************************************************
* 函数名称：dba_ftp_nlst_file_by_port
* 功能描述: ftp 用port模式名称列表
************************************************************************/
DWORD dba_ftp_nlst_file_by_port(INT ctrl_socket,const char *remote_file_path)
{
    DWORD ret = DBA_RC_OK;
    INT   data_socket     = INVALID_SOCKET;

    DBA_NULLPOINTER_CHK_RTVALUE(remote_file_path);
    //set type(binary)
    DBA_NOK_CHK_RTCODE(dba_ftp_type(ctrl_socket,"I"));
    //set mode(port)
    ret = dba_ftp_port(ctrl_socket,&data_socket);
    if(DBA_RC_OK != ret)
    {
        dba_close_ftp_socket(data_socket);
        return ret;
    }
    //nlst
    ret = dba_ftp_nlst(ctrl_socket,remote_file_path);
    if(DBA_RC_OK != ret)
    {
        dba_close_ftp_socket(data_socket);
        return ret;
    }
    //accept srv connect
    INT  srv_data_socket = INVALID_SOCKET;
    ret = dba_ftp_accpet_data_port(data_socket,&srv_data_socket);
    if(DBA_RC_OK != ret)
    {
        dba_close_ftp_socket(data_socket);
        return ret;
    }
    //recv nlst data
    ret = dba_ftp_nlst_reciev_data(ctrl_socket,srv_data_socket,remote_file_path);
    dba_close_ftp_socket(data_socket);

    return ret;
}


