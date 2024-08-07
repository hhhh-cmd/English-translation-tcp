/********client.c******************************************************************************/
#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<string.h>
#include<arpa/inet.h>
#include<unistd.h>
int client_start(int sockfd);             //客户端初始化函数
void client_meau(void);                   //客户端菜单函数
int client_register(int sockfd);          //客户端注册函数
int client_login(int sockfd);             //客户端登陆函数
int client_search(int sockfd,int l);      //客户端搜索单词函数
int client_history(int sockfd,int l);     //客户端查询历史记录函数
int client_name(int sockfd);              //客户端登陆和注册输入名字函数
int client_mima(int sockfd);              //客户端登陆和注册输入密码函数
 
int main()
{
    int ret;
    int sockfd;
    int chause;
    int l = 2;
    char buf[256];
    sockfd = client_start(sockfd);
    if(-1 == sockfd)
    {
        printf("client_start failed\n");
        return -1;
    }
 
    while(1)
    {
        client_meau();
        scanf("%d",&chause);
        memset(buf,0,sizeof(buf));
        ret = write(sockfd,&chause,sizeof(chause));//传chause给服务器
        if(-1 == ret)
        {
            perror("client->write");
            continue;
        }
 
        switch(chause)
        {
            case 1:
                client_register(sockfd);
                break;
            case 2:
                l = client_login(sockfd);//用l保存登陆的状态
                break;
            case 3:
                client_search(sockfd,l);
                break;
            case 4:
                client_history(sockfd,l);
                break;
            case 5:
                return 0;
            default:
                printf("请选择正确的选项\n");
                continue;
        }
    }
    close(sockfd);
    return 0;
}
 
/*************************************************************/
/***********客户端********************************************/
int client_start(int sockfd)
{
    int ret;
    /*1.socket*/
    sockfd = socket(AF_INET,SOCK_STREAM,0);
    if(-1 == sockfd)
    {
        perror("server->socket");
        return -1;
    }
    printf("sockfd = %d\n",sockfd);
 
    /*2.connect*/
    struct sockaddr_in addr;
    memset(&addr,0,sizeof(struct sockaddr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8888);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    ret = connect(sockfd,(const struct sockaddr *)&addr,sizeof(struct sockaddr_in));
    if(-1 == ret)
    {
        perror("server->bind");
        return -1;
    }
    printf("connect success\n");
    return sockfd;
}
 
/*************************************************************/
/**********主菜单******************************************/
void client_meau(void)
{
    printf("**********请您选择要进行的操作****************\n");
    printf("----------------------------------------------\n");
    printf("|           -.-*英英词典*-.-                 |\n");
    printf("|                * __ *                      |\n");
    printf("|                                            |\n");
    printf("|           1.用户注册                       |\n");
    printf("|           2.用户登陆                       |\n");
    printf("|           3.单词在线翻译                   |\n");
    printf("|           4.历史记录查询                   |\n");
    printf("|           5.退出                           |\n");
    printf("|                                            |\n");
    printf("----------------------------------------------\n");
    printf("\n");
    printf("input:");
}
 
/*************************************************************/
/***********注册*****************************************/
int client_register(int sockfd)
{
    int ret;
    int k = 0;// 保存注册的状态
    ret = client_name(sockfd);
    if(-1 == ret)
    {
        printf("name failed\n");
        return -1;
    }
 
    ret = client_mima(sockfd);
    if(-1 == ret)
    {
        printf("mima failed\n");
        return -1;
    }
 
    ret = read(sockfd,&k,sizeof(k));
    if(-1 == ret)
    {
        printf("client_register->read k\n");
        return -1;
    }
    if(k == 1)
    {
        printf("注册成功\n");
    }
    else
    {
        printf("注册失败\n");
    }
    return 0;
}
 
int client_name(int sockfd)//name
{
    char name;
    int ret;
    char buf[16];
    memset(buf,0,sizeof(buf));
    fprintf(stdout,"请输入帐号:\n");
    scanf("%s",buf);
    //buf[strlen(buf)-1] = '\0';
    if(0 == strlen(buf))
    {
        printf("name is null\n");
        return -1;
    }
    ret = write(sockfd,buf,sizeof(buf));
    if(-1 == ret)
    {
        perror("client_register->write.name");
        printf("failed\n");
        return -1;
    }
    return 0;
}
 
int client_mima(int sockfd)//mima
{
    char mima;
    int ret;
    char buf[16];
    fprintf(stdout,"请输入密码\n");
    scanf("%s",buf);
    //buf[strlen(buf)-1] = '\0';
    if(0 == strlen(buf))
    {
        printf("mima is null\n");
        return -1;
    }
    ret = write(sockfd,buf,sizeof(buf));
    if(-1 == ret)
    {
        perror("client_register->write.mima");
        printf("failed\n");
        return -1;
    }
    return 0;
}
 
 
 
 
/*************************************************************/
/************登陆****************************************/
int client_login(int sockfd)
{
    int ret;
    char buf[16];
    int b = 0;
    memset(buf,0,sizeof(buf));
    client_name(sockfd);
    if(-1 == ret)
    {
        printf("name failed\n");
        return -1;
    }
    client_mima(sockfd);
    if(-1 == ret)
    {
        printf("mima failed\n");
        return -1;
    }
 
    ret = read(sockfd,&b,sizeof(b));
    if(-1 == ret)
    {
        perror("client_register->write.mima");
        printf("failed\n");
        return -1;
    }
 
    if(1 == b)
    {
        printf("login success\n");
        return 0;
    }
    else
    {
        printf("login fail\n");
        return -1;
    }
}
 
/*************************************************************/
/***********搜索************************************/
int client_search(int sockfd,int l)
{
    char word[64];
    int ret;
    char r = '1';
    char buf[1024];
    memset(word,0,sizeof(word));
    memset(buf,0,sizeof(buf));
    if(0 != l)
    {
        printf("please login\n");
        return -1;
    }
 
    printf("请输入要搜索的单词:\n");
    scanf("%s",word);
    ret = write(sockfd,word,sizeof(word));
    if(-1 == ret)
    {
        perror("client_search->write");
        return -1;
    }
 
    ret = read(sockfd,buf,sizeof(buf));
    if(-1 == ret)
    {
        perror("client_search->read");
        return -1;
    }
 
    if('0' == buf[0])
    {
        printf("no the word\n");
    }
    else
    {
        printf("查询内容:%s\n",buf);
    }
    return 0;
}
 
/*************************************************************/
/*************历史查询*************************************/
int client_history(int sockfd,int l)
{
    char buf[128];
    int ret;
    int i = 0;
 
    if(0 != l)
    {
        printf("please login\n");
        return -1;
    }
 
    ret = read(sockfd,&i,sizeof(i));
    if(-1 == ret)
    {
        perror("client_history->line");
        return -1;
    }
 
    printf("一共有%3d个历史数据\n",i);
    while(i--)
    {
        memset(buf,0,sizeof(buf));
        ret = read(sockfd,buf,sizeof(buf));
        if(-1 == ret)
        {
            perror("client_history->read");
            return -1;
        }
        printf("%s",buf);
    }
    printf("\n");
    return 0;
}
 
 
 
