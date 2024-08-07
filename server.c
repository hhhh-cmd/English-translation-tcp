/*******server.c*********************************************************************************/
#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<string.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<pthread.h>
#include<sqlite3.h>
#include<time.h>
#include<sys/stat.h>
#include<fcntl.h>
int server_start(int listenfd);             //服务器初始化函数
int server_accept(int listenfd);            //服务器连接客户端函数
void *thread_func(void *connfd);            //线程的处理函数
int server_register(int connfd);            //服务器注册函数
int server_login(int connfd);               //服务器登陆函数 
int server_search(int connfd,int l);        //服务器搜索单词函数
int server_history(int connfd,int l);       //服务器历史纪录传给客户端函数
int server_history_write(char *word);       //服务器历史记录写入文件函数
 
int main()
{
    int ret;
    int listenfd;
    listenfd = server_start(listenfd);
    if(-1 == ret )
    {
        printf("server_start failed\n");
        return -1;
    }
 
    while(1)
    {
        //建立连接    
        int connfd;
        connfd = server_accept(listenfd);
        if(-1 == connfd)
        {
            printf("server_accept failed\n");
            return -1;
        }
 
        //采用多线程的方式
        pthread_t thread;
        ret = pthread_create(&thread,NULL,thread_func,(void *)&connfd);
        if (0 != ret)
        {
            perror("pthread_creat");
            close(connfd);
            continue;
        }
        printf("pthread_creat success! connfd\n");
 
        //给线程收尸体，用pthread_join会阻塞
        if(0 != pthread_detach(thread))
        {
            perror("pthread_detach");
            close(connfd);
            continue;
        }
    }
    return 0;
}
 
/********************************************************************/
/*******************服务器初始化**************************************/
int server_start(int listenfd)
{
    /*1.socket*/
    int ret;
    listenfd = socket(AF_INET,SOCK_STREAM,0);
    if(-1 == listenfd)
    {
        perror("server->socket");
        return -1;
    }
    printf("created listenfd = %d success\n",listenfd);
 
    /*2.bind*/
    struct sockaddr_in addr;
    memset(&addr,0,sizeof(struct sockaddr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8888);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    ret = bind(listenfd,(const struct sockaddr *)&addr,sizeof(struct sockaddr_in));
    if(-1 == ret)
    {
        perror("server->bind");
        return -1;
    }
    printf("port: %d \n",htons(addr.sin_port));
 
    /*3.listen*/
    ret = listen(listenfd,0);
    if(-1 == ret)
    {
        perror("server->listen");
        return -1;
    }
    printf("listen success\n");
 
    return listenfd;
}
 
/********************************************************************/
/******************服务器连接************************************/
int server_accept(int listenfd)
{
    /*4.accept*/
    socklen_t addrlen;
    struct sockaddr_in cltaddr;
    addrlen = sizeof(socklen_t);
    int connfd;
    connfd = accept(listenfd,(struct sockaddr *)&cltaddr,&addrlen);
    if(-1 == connfd)
    {
        perror("server->accept");
        return -1;
    }
    printf("connfd = %d\n",connfd);
    return connfd;
}
 
/********************************************************************/
/************线程的处理函数*****************************************/
void *thread_func(void *arg)
{
    int connfd = *(int *)arg;
    int ret;
    int l;
    int chause;
    char buf[256];
    char word[16];
    while(1)
    {
        ret = read(connfd,&chause,sizeof(chause));//接收chause
        //printf("chause = %d\n",chause);
        if(-1 == ret)
        {
            perror("server->read");
            close(connfd);
            pthread_exit(0);
        }
 
        switch(chause)
        {
            case 1:
                server_register(connfd);
                break;
            case 2:
                l = server_login(connfd);
                break;
            case 3:
                server_search(connfd,l);
                break;
            case 4:
                server_history(connfd,l);
                break;
            case 5:
                printf("this client quit\n");
                close(connfd);
                pthread_exit(0);
            default:
                continue;
        }
    }
}
 
/********************************************************************/
/********注册***********************************/
int server_register(int connfd)
{
    char buf[256];
    char buf2[256];
    int ret;
    char name[16];
    char mima[16];
    char sql[256];
    sqlite3 *db;
    char *errmsg;
    int k = 1;//用来保存注册是否成功的状态   1：成功   0：失败   这个数据会发送给客户端
    memset(sql,0,sizeof(sql));
    memset(buf,0,sizeof(buf));
    memset(buf2,0,sizeof(buf2));
 
    ret = read(connfd,buf,sizeof(buf));//name
    strncpy(name,buf,16);
    if(-1 == ret)
    {
        perror("server_register->read.name");
        return -1;
    }
 
    ret = read(connfd,buf2,sizeof(buf2));//mima
    strncpy(mima,buf2,16);
    if(-1 == ret)
    {
        perror("server_register->read.mima");
        return -1;
    }
 
    /*打开数据库文件*/
    ret = sqlite3_open("my.db",&db);
    if(SQLITE_OK != ret)
    {
        fprintf(stderr,"open:%s\n",sqlite3_errmsg(db));
        return -1;
    }
#if 1
    /*创建表之前应该判断下有没有user这个表，如果有就不创建了*/
    sprintf(sql,"create table user(name char primary key,mima Interger)");
    ret = sqlite3_exec(db,sql,NULL,NULL,&errmsg);
    if(SQLITE_OK != ret)
    {
        fprintf(stderr,"create:%s\n",errmsg);
        return -1;
    }
#endif
    memset(sql,0,sizeof(sql));
     
    /*插入数据*/
    sprintf(sql,"insert into user values('%s','%s')",name,mima);
    ret = sqlite3_exec(db,sql,NULL,NULL,&errmsg);
    if(SQLITE_OK != ret)
    {
        k = 0;
        fprintf(stderr,"insert:%s\n",errmsg);
        ret = write(connfd,&k,sizeof(k));
        if(-1 == ret)
        {
            perror("server_login->write k");
            return -1;
        }
        return -1;
    }
    ret = write(connfd,&k,sizeof(k));
    if(-1 == ret)
    {
        perror("server_login->write k");
        return -1;
    }
 
    printf("注册成功\n");
    return 0;
}
 
/********************************************************************/
/*********登陆 返回0表示成功********************************/
int server_login(int connfd)
{
    int ret;
    char buf[16];
    char buf2[16];
    char sql[256];
    char sql1[256];
    char name[16];
    char mima[16];
    char *errmsg;
    sqlite3 *db;
    memset(buf,0,sizeof(buf));
    memset(buf2,0,sizeof(buf2));
    memset(sql,0,sizeof(sql));
    memset(sql1,0,sizeof(sql1));
    ret = read(connfd,buf,sizeof(buf));//name
    strncpy(name,buf,16);
    if(-1 == ret)
    {
        perror("server_register->read.name");
        return -1;
    }
 
    ret = read(connfd,buf2,sizeof(buf2));//mima
    strncpy(mima,buf2,16);
    if(-1 == ret)
    {
        perror("server_register->read.mima");
        return -1;
    }
 
    /*打开数据库文件*/
    ret = sqlite3_open("my.db",&db);
    if(SQLITE_OK != ret)
    {
        fprintf(stderr,"open:%s\n",sqlite3_errmsg(db));
        return -1;
    }
    /*查询数据*/
    char **result;
    int row;
    int column;
    int b = 1;      //是否登录成功，1：成功  0：失败   会发送给客户端
    sprintf(sql,"select * from user where name='%s' and mima='%s'",name,mima);
    printf("name = %s,mima=%s\n",name,mima);
    ret = sqlite3_get_table(db,sql,&result,&row,&column,&errmsg);
    if(SQLITE_OK != ret)
    {
        fprintf(stderr,"select:%s\n",errmsg);
        return -1;
    }
    if((1 == row))
    {
        printf("login success\n");
        ret = write(connfd,&b,sizeof(b));
        if(-1 == ret)
        {
            perror("server_login->write");
            return -1;
        }
        return 0;
    }
    else
    {
        b = 0;
        ret = write(connfd,&b,sizeof(b));
        if(-1 == ret)
        {
            perror("server_login->write");
            return -1;
        }
        printf("login fail\n");
        return -1;
    }
}
 
/********************************************************************/
/**********搜索*********************************/
int server_search(int connfd,int l)
{
    int ret;
    char r = '0';
    char buf[1024];
    char b[256];
    char word[64];
    memset(buf,0,sizeof(buf));
    memset(b,0,sizeof(b));
    memset(word,0,sizeof(word));
    if(0 != l)
    {
        printf("please login\n");
        return 0;
    }
 
    ret = read(connfd,word,sizeof(word));
    if(-1 == ret)
    {
        perror("server_search->read");
        return -1;
    }
 
    ret = server_history_write(word);///写入历史纪录文档 
    if(-1 == ret)
    {
        printf("history write fail\n");
    }
 
    FILE *fp;
    fp = fopen("dict.txt","r");
    if(fp == NULL){
        perror("fopen error");
        return -1;
    }

    while(1)
    {
        int i = 0;
        int k = 1;
        memset(buf,0,sizeof(buf));
        if (NULL == fgets(buf,sizeof(buf),fp))
        {
            break;
        }
        while(word[i])
        {
            if(word[i] != buf[i])
            {
                k = 0;
                break;
            }
            i++;
        }
        if((k == 1) && (buf[i++] ==' '))
        {
            ret = write(connfd,buf,strlen(buf));
            r = '1';//表示找到这个单词
            if(-1 == ret)
            {
                perror("server_search->write");
                return -1;
            }
            break;
        }
    }
    //r = 0;//表示没找到这个单词
    if(r == '0')
    {
        ret = write(connfd,&r,sizeof(r));
        if(-1 == ret)
        {
            perror("server_serach->noword");
            return -1;
        }
    }
    return 0;
}
 
/********************************************************************/
/***********历史记录写入文件********************************/
int server_history_write(char *word)
{
    FILE *fp;
    size_t ret;
    char buf[64];
    memset(buf,0,sizeof(buf));
    time_t t;
    time(&t);
    struct tm *tp = localtime(&t);
    int i = 0;
    fp = fopen("history.txt","a+");
    if(NULL == fp)
    {
        perror("fopen");
        return -1;
    }
    fprintf(fp,"%2d-%2d  %2d:%2d:%2d :  ",tp->tm_mon +1,tp->tm_mday,tp->tm_hour,tp->tm_min,tp->tm_sec);
    
    while('\0' != word[i])//word  len
    {
        i++; 
    }
 
    ret = fwrite(word,i,1,fp);
    if(-1 == ret)
    {
        perror("server_history->write time");
        return -1;
    }
 
    ret = fwrite("\n",1,1,fp);
    if(-1 == ret)
    {
        perror("server_history->write huanhang");
        return -1;
    }
    fclose(fp);
    return 0;
}
 
/********************************************************************/
/****************历史纪录读出来，传给服务器 *******************/
int server_history(int connfd,int l)
{
    int fd;
    char buf[128];
    memset(buf,0,sizeof(buf));
    int ret;
    FILE *fp;
    int i = 0;
    if(0 != l)
    {
        return -1;
    }
    fp = fopen("history.txt","r");
    if(NULL == fp)
    {
        perror("fopen");
        return -1;
    }
    while(NULL != fgets(buf,sizeof(buf),fp))// 获得行号
    {
        i++;
    }
    fclose(fp);
 
    ret = write(connfd,&i,sizeof(i));//发送行号
    if(-1 == ret)
    {
        perror("server_history->line");
        return -1;
    }
 
    fp = fopen("history.txt","r");
    while(NULL != fgets(buf,sizeof(buf),fp))// 发送具体内容
    {
        ret = write(connfd,buf,sizeof(buf));
        if(-1 == ret)
        {
            perror("server_history->write");
            return -1;
        }
    }
    fclose(fp);
    return 0;
}
 
 
 
