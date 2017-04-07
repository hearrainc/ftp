
/* *****************************************************************************************
*    文 件 名：ftp_client.h
*    说    明：ftp_client.c文件中函数声明
*******************************************************************************************/


#ifndef  __FTP_CLIENT_H
#define  __FTP_CLIENT_H

/******************************************************************************************/
//public
DWORD dba_ftp_login(INT *ctrl_socket);
DWORD dba_ftp_size(INT ctrl_socket,const char *remote_file_path,DWORD *file_size);
DWORD dba_ftp_quit(INT ctrl_socket);
DWORD dba_ftp_mkd(INT ctrl_socket,const char *path);
DWORD dba_ftp_cwd(INT ctrl_socket,const char *path);
DWORD dba_ftp_pwd(INT ctrl_socket,char *ftp_cur_dir);
DWORD dba_ftp_get_file(INT ctrl_socket,const char *remote_file_path,const char *local_file_path);
DWORD dba_mk_model_dir(INT ctrl_socket,const char *dir_name);

//PASV Mode
DWORD dba_ftp_nlst_file_by_pasv(INT ctrl_socket,const char *remote_file_path);
DWORD dba_ftp_up_file_by_pasv(INT ctrl_socket,
                              const char *local_file_path,
                              const char *remote_file_path);

//PORT Mode
DWORD dba_ftp_nlst_file_by_port(INT ctrl_socket,const char *remote_file_path);
DWORD dba_ftp_up_file_by_port(INT ctrl_socket,
                              const char *local_file_path,
                              const char *remote_file_path);
//private
DWORD dba_ftp_create_conctrl_connect(INT *ctrl_socket,
                                     const char *ftp_server_ip,
                                     const WORD ftp_port);
DWORD dba_ftp_user(INT ctrl_socket,const char *user_name);
DWORD dba_ftp_pass(INT ctrl_socket,const char *passwd);
DWORD dba_ftp_pasv(INT ctrl_socket,DWORD *data_socket_port);
DWORD dba_ftp_type(INT ctrl_socket,const char *type);
DWORD dba_ftp_create_data_connect(INT *data_socket,DWORD port);
DWORD dba_ftp_nlst(INT ctrl_socket,const char *remote_file_path);
DWORD dba_ftp_nlst_reciev_data(INT ctrl_socket,INT data_socket,const char* remote_file_path);
DWORD dba_ftp_stor(INT ctrl_socket,const char *remote_file_path);
DWORD dba_ftp_send_data(INT ctrl_socket,INT data_socket,const char* local_file_path);

DWORD dba_ftp_accpet_data_port(INT data_socket,INT *srv_data_socket);
DWORD dba_ftp_port(INT ctrl_socket,INT *data_socket);

#endif




