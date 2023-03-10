#ifndef CProcessD
#define CProcessD
#include "Function.h"
#include <memory.h>//memset
#include <sys/socket.h>//socket
#include <sys/stat.h>
#include <fcntl.h>//func control
#include <stdlib.h>
#include <cerrno>
#include <signal.h>

class CProcess
{
public:
    CProcess() {//��¼������ĺ����Ͳ����������õ�ʱ�����ȷ�ϡ��ڹ����ʱ����ȷ��,��˻ᱨ�Ҳ��������б�Ĵ���
        //CFunction* m_func = nullptr;
        //ʹ�ø��࣬���಻��ģ��ģ������ڹ����ʱ��ȷ�ϣ����ᱨ�������Ͱ�ģ�������ں���SetEntryFunction���ˣ�������CProcess���ģ����
        m_func = NULL;
        memset(pipes, 0, sizeof(pipes));
    }
    ~CProcess() {
        if (m_func != NULL) {
            delete m_func;
            m_func = NULL;
        }
    }
    template<typename _FUNCTION_, typename... _ARGS_>//�������ͣ��ɱ��������
    int setEntryFunction(_FUNCTION_ func, _ARGS_... args) {//������ں���
        m_func = new CFunction<_FUNCTION_, _ARGS_...>(func, args...);
        return 0;
    }
    int CreateSubProcess() {//�����ӽ���
        if (m_func == NULL) return -1;
        //Ҫ��fork֮ǰ�����׽��֣�����fork���޷�����ͨѶ
        int ret = socketpair(AF_LOCAL, SOCK_STREAM, 0, pipes);//AF_LOCAL:linux���еı����׽���,�ᴴ����һ�����ӣ�SOCK_STREAM��TCPЭ��
        if (ret == -1) return -2;
        pid_t pid = fork();//����pid>0�����̣�==0�ӽ��̣�==-1����ʧ��
        if (pid == -1) return -3;
        if (pid == 0) {//�ӽ���
            close(pipes[1]);//�ӽ��̹ر�д�ܵ���ֻ��Ҫ���������̴��룬�ӽ��̽��գ�
            pipes[1] = 0;
            //������ӽ��̴�������Ҫ��������������һ���ܵ��������������̶���һ���ط�д����һֱ������״̬��
            ret = (*m_func)();
            exit(0);//������֮��ֱ���˳������践�أ���ֹ����ִ�������̴��룩
        }
        //������
        close(pipes[0]);//�رն��ܵ�
        pipes[0] = 0;
        m_pid = pid;
        return 0;
    }
    ////��Ҫ�����ļ�����������
    int sendFD(int fd) {//���������
        //fd:File Deskriptor
        struct msghdr msg;
        iovec iov[2];
        char buf[2][10] = { "houzss","NB" };
        bzero(&msg, sizeof(msg));
        iov[0].iov_base = buf[0];//��Ϣ����Ҫ����Ҫ�����ļ�������fd
        iov[0].iov_len = sizeof(buf[0]);//6���ַ�+1��������'\0'
        iov[1].iov_base = buf[1];
        iov[1].iov_len = sizeof(buf[1]);//6���ַ�+1��������'\0'
        msg.msg_iov = iov;//�������ݣ�ָ�룩
        msg.msg_iovlen = 2;//�������ݸ���
        msg.msg_name = NULL;
        msg.msg_namelen = 0;
        //��������ݲ�����Ҫ���ݵģ��������ݣ���
        cmsghdr* cmsg = (cmsghdr*)calloc(1, CMSG_LEN(sizeof(int)));
        //cmsghdr* cmsg = new cmsghdr;
        //bzero(cmsg, sizeof(cmsghdr));
        if (cmsg == NULL) return -1;
        cmsg->cmsg_len = CMSG_LEN(sizeof(int));//�꣬�ļ�����������
        cmsg->cmsg_level = SOL_SOCKET;//�汾̫�£����ù�
        cmsg->cmsg_type = SCM_RIGHTS;//Ȩ��
        *(int*)CMSG_DATA(cmsg) = fd;//control
        msg.msg_control = cmsg;
        msg.msg_controllen = cmsg->cmsg_len;

        ssize_t ret = sendmsg(pipes[1], &msg, 0);//д�ܵ����ܵ�ֻ�ܴ�1д��
        //char buf1[100] = "test for straight write";
        //ssize_t ret = write(pipes[1], buf1, sizeof(buf1));
        //if (ret != 0) printf("errno:%d msg:%s\n", errno, strerror(errno));
        if (ret == -1) {//
            return -2;
        }
        free(cmsg);
        return 0;
    }
    int recvFD(int& fd)//�ӽ���
        //��������ֱ���޸�fd��������Ǹ������ı��� 
    {
        msghdr msg;
        iovec iov[2];
        char buf[][10] = { "","" };
        bzero(&msg, sizeof(msg));
        iov[0].iov_base = buf[0];
        iov[0].iov_len = sizeof(buf[0]);
        iov[1].iov_base = buf[1];
        iov[1].iov_len = sizeof(buf[1]);
        msg.msg_iov = iov;
        msg.msg_iovlen = 2;
        msg.msg_name = NULL;
        msg.msg_namelen = 0;//������һ����
        cmsghdr* cmsg = (cmsghdr*)calloc(1, CMSG_LEN(sizeof(int)));//����Ҫ���յ�����
        if (cmsg == NULL) return -1;
        cmsg->cmsg_len = CMSG_LEN(sizeof(int));
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_RIGHTS;//Ȩ��
        msg.msg_control = cmsg;
        msg.msg_controllen = CMSG_LEN(sizeof(int));
        ssize_t ret = recvmsg(pipes[0], &msg, 0);//���ܵ����ܵ�ֻ�ܴ�0����
        //char buf1[100] = "";
        //ssize_t ret = read(pipes[0], buf1, sizeof(buf1));
        //if (ret != 0) printf("errno:%d msg:%s\n", errno, strerror(errno));
        //printf("%s(%d):<%s> buf = %s\n", __FILE__, __LINE__, __FUNCTION__, buf1);//������־�ӽ���ǰ��ӡһ��
        if (ret == -1) {
            free(cmsg);
            return -2;
        }
        fd = *(int*)CMSG_DATA(cmsg);//�ɹ������cmsg�ж����ݣ��ļ���������
        free(cmsg);//��ֹ�ڴ�й©
        return 0;
    }

    int sendSocket(int fd, const sockaddr_in* addrin) {//�����˶Ե�ַ�ķ���(�����׽��������)
        //fd:File Deskriptor
        struct msghdr msg;
        iovec iov;
        char buf[20] = "";
        bzero(&msg, sizeof(msg));
        memcpy(buf, addrin, sizeof(sockaddr_in));
        iov.iov_base = buf;
        iov.iov_len = sizeof(buf);
        //����ֱ����addrin����iovҲ����
        //iov.iov_base = (void*)addrin;//��Ϣ����Ҫ����Ҫ�����ļ�������fd
        //iov.iov_len = sizeof(sockaddr_in);
        msg.msg_iov = &iov;//�������ݣ�ָ�룩
        msg.msg_iovlen = 1;//�������ݸ���
        msg.msg_name = NULL;
        msg.msg_namelen = 0;
        //��������ݲ�����Ҫ���ݵģ��������ݣ���
        cmsghdr* cmsg = (cmsghdr*)calloc(1, CMSG_LEN(sizeof(int)));
        //cmsghdr* cmsg = new cmsghdr;
        //bzero(cmsg, sizeof(cmsghdr));
        if (cmsg == NULL) return -1;
        cmsg->cmsg_len = CMSG_LEN(sizeof(int));//�꣬�ļ�����������
        cmsg->cmsg_level = SOL_SOCKET;//�汾̫�£����ù�
        cmsg->cmsg_type = SCM_RIGHTS;//Ȩ��
        *(int*)CMSG_DATA(cmsg) = fd;//control
        msg.msg_control = cmsg;
        msg.msg_controllen = cmsg->cmsg_len;

        ssize_t ret = sendmsg(pipes[1], &msg, 0);//д�ܵ����ܵ�ֻ�ܴ�1д��
        //char buf1[100] = "test for straight write";
        //ssize_t ret = write(pipes[1], buf1, sizeof(buf1));
        //if (ret != 0) printf("errno:%d msg:%s\n", errno, strerror(errno));
        if (ret == -1) {//
            printf("******errno %d msg %s\n", errno, strerror(errno));
            return -2;
        }
        free(cmsg);
        return 0;
    }
    int recvSocket(int& fd, sockaddr_in* addrin)//�ӽ���
        //��������ֱ���޸�fd��������Ǹ������ı��� 
    {
        msghdr msg;
        iovec iov;
        char buf[20] = "";
        bzero(&msg, sizeof(msg));
        iov.iov_base = buf;
        iov.iov_len = sizeof(buf);
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;
        msg.msg_name = NULL;
        msg.msg_namelen = 0;//������һ����
        cmsghdr* cmsg = (cmsghdr*)calloc(1, CMSG_LEN(sizeof(int)));//����Ҫ���յ�����
        if (cmsg == NULL) return -1;
        cmsg->cmsg_len = CMSG_LEN(sizeof(int));
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_RIGHTS;//Ȩ��
        msg.msg_control = cmsg;
        msg.msg_controllen = CMSG_LEN(sizeof(int));
        ssize_t ret = recvmsg(pipes[0], &msg, 0);//���ܵ����ܵ�ֻ�ܴ�0����
        //char buf1[100] = "";
        //ssize_t ret = read(pipes[0], buf1, sizeof(buf1));
        //if (ret != 0) printf("errno:%d msg:%s\n", errno, strerror(errno));
        //printf("%s(%d):<%s> buf = %s\n", __FILE__, __LINE__, __FUNCTION__, buf1);//������־�ӽ���ǰ��ӡһ��
        if (ret == -1) {
            free(cmsg);
            return -2;
        }
        memcpy(addrin, buf, sizeof(sockaddr_in));
        fd = *(int*)CMSG_DATA(cmsg);//�ɹ������cmsg�ж����ݣ��ļ���������
        if(msg.msg_iov[0].iov_base!=addrin)//���������Լ��ӵ�
            addrin = (sockaddr_in*)msg.msg_iov[0].iov_base;
        free(cmsg);//��ֹ�ڴ�й©
        return 0;
    }

    static int SwitchDeamon()//ת�����ػ� Linux Daemon���ػ����̣��������ں�̨��һ��������̡��������ڿ����ն˲��������Ե�ִ��ĳ�������ȴ�����ĳЩ�������¼���������Ҫ�û�����������ж����ṩĳ�ַ��񣬲��Ƕ�����ϵͳ���Ƕ�ĳ���û������ṩ����
    {
        printf("%s(%d):<%s> pid = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid());//�꣺__FILE__ʼ��ָ������ļ�����__LINE__ʼ��ָ���кţ�__FUNCTION__ʼ��ָ�������
        pid_t ret = fork();//�����ӽ���
        if (ret == -1) {
            return -1;
        }
        else if (ret > 0) {//�����̽���
            exit(0);
        }
        //�ӽ�����������
        //�б�Ҫ�Ƚ���һ��Linux�еĽ���������նˣ���¼�Ự�ͽ�����֮��Ĺ�ϵ����������һ�������飬������ţ�GID�����ǽ����鳤�Ľ��̺ţ�PID������¼�Ự���԰�����������顣��Щ�����鹲��һ�������նˡ���������ն�ͨ���Ǵ������̵ĵ�¼�նˡ�
        //�����նˣ���¼�Ự�ͽ�����ͨ���ǴӸ����̼̳������ġ����ǵ�Ŀ�ľ���Ҫ�������ǣ�ʹ֮�������ǵ�Ӱ�졣
        ret = setsid();//�����»Ự�����ն˿��ƣ��������ӽ��̳�Ϊ�µĻỰ�鳤���µĽ����鳤������ԭ���ĵ�¼�Ự�ͽ���������
        if (ret == -1) return -2;//ʧ���򷵻�
        ret = fork();//���������
        if (ret == -1) return -3;
        else if (ret > 0) exit(0);//�ӽ��̽���������̳�Ϊ�¶����̣����޸����̣���û�лỰ�鳤���ܿ��ƣ�

        //������������£������ػ�״̬��
        for (int i = 0; i < 3; i++) {
            close(i);//�ص�����Ҫ���ļ������� [0,1,2]==>��׼���롢��׼�������׼����
        }
        umask(0);//�����ļ�����:�ػ����̴Ӹ����̼̳������ļ�������ʽ������ܻ�ܾ�����ĳЩ���Ȩ�ޣ��ļ�Ȩ��������ָ���ε��ļ�Ȩ���еĶ�Ӧλ��
        signal(SIGCHLD, SIG_IGN);//�����ӽ�����Ϣ�ź�(���԰�ĳ���źŵĴ����趨ΪSIG_IGN��������)
        //�ӽ��̵�����״̬�����仯�ͻᷢ��SIGCHILD�źţ��������˼�ǣ��ӽ��̱Ƚ�������ĸ���Լ������仯��Ҫ����ĸ˵һ�¡�
        return 0;
    }
private:
    CFunctionBase* m_func;
    pid_t m_pid;//��¼����pid��
    int pipes[2];//�����ܵ������ڻ�ĳ�socket�����շ���
};
#endif
