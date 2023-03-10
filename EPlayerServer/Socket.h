#ifndef SOCKETD
#define SOCKETD
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>//�����׽���
#include <netinet/in.h>//�����׽��ֲ�������
#include <arpa/inet.h>//����API�ӿ�
#include <cerrno>
#include <fcntl.h>
#include "Public.h"

enum SockAttr {//ȡֵһ����2^n�η�,�Ա�ʾ��Щ����������һ��������ͬʱ���ڣ�����111B=7D��ʾUDP���������ͷ�����
	SOCK_ISSERVER = 1,//�Ƿ��Ƿ�������1��ʾ�ǣ�0��ʾΪ�ͻ��ˣ�
	SOCK_ISNONBLOCK = 2,//�Ƿ��������1��ʾ��������0��ʾ������
	SOCK_ISUDP = 4,//�Ƿ���UDP 1��ʾUDP 0��ʾTCP
	SOCK_ISIP = 8,//�Ƿ��������׽��� 1��ʾ�����׽��֣�IPЭ�飩��0��ʾ�����׽���
	SOCK_ISREUSE = 16,//�Ƿ������˵�ַ
};

class CSocketParam
{
public:
	CSocketParam(){
		bzero(&addr_in, sizeof(addr_in));//���Ϊ0
		bzero(&addr_un, sizeof(addr_un));
		port = -1;
		attr = 0;//Ĭ���ǿͻ��ˡ�������TCP�������׽���
	}
	CSocketParam(sockaddr_in* addrin, int attr) {
		this->attr = attr;
		memcpy(&addr_in, addrin, sizeof(addr_in));
	}
	CSocketParam(const Buffer& ip, short port, int attr) {
		this->ip = ip;
		this->port = port;
		this->attr = attr;
		addr_in.sin_family = AF_INET;//AF:Address Family PF:Protocol Family��Э���飩������windows��AF_INET��PF_INET��ȫһ�� ����TCP/IPЭ����
		addr_in.sin_port = htons(port);//host to net (short)�����ֽ���תΪ�����ֽ���
		//htonl�����unsigned long��ʾ
		addr_in.sin_addr.s_addr = inet_addr(ip);	
	}
	CSocketParam(const Buffer& path, int attr) {
		ip = path;
		addr_un.sun_family = AF_UNIX;
		strcpy(addr_un.sun_path, path);
		this->attr = attr;
	}
	~CSocketParam(){}
	CSocketParam(const CSocketParam& param) {
		ip = param.ip;
		port = param.port;
		attr = param.attr;
		memcpy(&addr_in, &param.addr_in, sizeof(addr_in));
		memcpy(&addr_un, &param.addr_un, sizeof(addr_un));
	}
	CSocketParam& operator=(const CSocketParam& param) {
		if (this != &param) {
			ip = param.ip;
			port = param.port;
			attr = param.attr;
			memcpy(&addr_in, &param.addr_in, sizeof(addr_in));
			memcpy(&addr_un, &param.addr_un, sizeof(addr_un));
		}
		return *this;
	}
	sockaddr* addrin() { return (sockaddr*)&addr_in; }//����ϵͳAPIʱʹ��
	sockaddr* addrun() { return (sockaddr*)&addr_un; }

	//��ַ
	sockaddr_in addr_in;//������Ա����Ϊ����˽�еģ�Ҳ��������Ϊm_addr_in
	sockaddr_un addr_un;//�����׽��ֵ�ַ
	//��������
	Buffer ip;//ip
	short port;//�˿�
	int attr;//״̬���ο�SockAttr
};
class CSocketBase
{
public:
	CSocketBase() {
		m_socket = -1;
		m_status = 0;//��ʼ��δ���
	}
	virtual ~CSocketBase() {//������������������������������
		Close();
	}
	//�������������Ա�֤��delete����ָ���ʱ�����˳������������������delete������������������麯�����Ͳ����д����ԣ�������ܾͲ�������ɾ������
	
	//��ʼ�� ���������׽��ִ�����bind�󶨡�listen������ �ͻ��ˣ��׽��ִ�����
	virtual int Init(const CSocketParam& param) = 0;//���麯����ǿ�ȼ̳��ࣨ��ͻ��ˣ�����ʵ�ָú���������ʵ�֣����������ڳ������޷�ʵ�������޷���������������
	//����ֻ�а����г�����ʵ���ˣ�����ʵ����
	//���� ��������accept�� �ͻ��ˣ�connect�� ������UDP��������Ժ��ԣ���Ϊudp��link������send��recv�У� ����ȻUDP���Բ�����������߿��Ը�0����֤������ʹ��ʲôЭ�����̶���ͳһ�ġ���֤��дҵ���ʱ���ñ�֤�ϲ�����ô����
	virtual int Link(CSocketBase** pClient = NULL) = 0;//���麯��  �����������������acceptĳ���ͻ��ˣ���Ҫ����һ������ָ��ָ����󣩣��ͻ��˲����������connect���ò�����ʹ��Ĭ�ϲ���Ϊ�գ�
	//�����࣬�޷�����ʵ���������Բ���ʹ��&
	//** ����ָ�룬һ��ָ��ֵ���ڲ��ı���޷����������ں��������ڼ���������Ǹ������ģ�������ֱ��ʹ���ⲿ��ָ�룩
	//*& ��ʾ��ָ�����͵����ã�������Ϊ�βε�ָ���ַ����뺯��������ֱ��ʹ���ⲿָ������������Ǹ�ֵ������������µ�ָ�루*����������Ϊ���ñ������ܸ�ֵnull������ֻ���ö���ָ��
	//��������
	virtual int Send(const Buffer& data) = 0;//���麯��
	//��������
	virtual int Recv(Buffer& data) = 0;//���麯��
	//�ر�����
	virtual int Close() {
		m_status = 3;//�Ѿ��ر�
		if (m_socket != -1) {//������δ�ر�ʱ
			int fd = m_socket;
			m_socket = -1;
			if ((m_param.attr & SOCK_ISSERVER) && //������
				((m_param.attr & SOCK_ISIP) == 0))//ʹ�ñ����׽���(�������׽���)
				unlink(m_param.ip);//ֻ��ʹ�ñ����׽��ֵķ�������Ӧ���ͷţ��ͻ��˻�û��ȫ�������ͷŻᵼ������:��ipΪ�����׽���ʱ������·��������sock�ļ����Ҳ���
			close(fd);
		}
		return 0;
	}
	virtual operator int() { return m_socket; }
	virtual operator int() const{ return m_socket; }
	virtual operator const sockaddr_in* () const { return &m_param.addr_in; }
	virtual operator sockaddr_in* () { return &m_param.addr_in; }
protected://��������Ϊprivate����Ϊ�̳е�����Ҳ��Ҫʹ�ã�������Ա�������ࣨ�����ࣩ���ǿɷ��ʵģ�
	//�׽�����������Ĭ��ֵ-1
	int m_socket;
	//״̬����Ϊ��ʱ�����ǲ��ܸ����׽���������ȷ��״̬�����ڽ��պͷ���ǰδȷ���Ƿ����ӻ��ʼ���ͽ��в����������һϵ�����⣩: 0��Ĭ��ֵ����ʼ��δ��� 1��ʼ������� 2������� 3�ѹر�
	int m_status;
	//��ʼ������
	CSocketParam m_param;
};

//����ͱ����׽��ֶ���һ
class CSocket :public CSocketBase//���м̳У�����˽�м̳У���ô��������ࣨ���ࣩ�޷����ʸ�������г�Ա
{
public:
	CSocket() :CSocketBase() {}
	CSocket(int sock) :CSocketBase(){
		m_socket = sock;
	}
	virtual ~CSocket() {//������������������������������
		Close();
	}
	//�������������Ա�֤��delete����ָ���ʱ�����˳������������������delete������������������麯�����Ͳ����д����ԣ�������ܾͲ�������ɾ������

	//��ʼ�� ���������׽��ִ�����bind�󶨡�listen������ �ͻ��ˣ��׽��ִ�����
	virtual int Init(const CSocketParam& param) {
		if (m_status != 0) return -1;//���Ѿ���ʼ�����׽����ѹرգ������رգ����Ͳ�Ӧ�ö��γ�ʼ��
		m_param = param;
		int type = (m_param.attr & SOCK_ISUDP) ? SOCK_DGRAM : SOCK_STREAM;//SOCK_DGRAM������ʽ��UDP��
		if (m_socket == -1) {//δ��ʼ��ʱ�ͳ�ʼ��
			if (param.attr & SOCK_ISIP)
				m_socket = socket(PF_INET, type, 0);//PF_INET����IPЭ��
			else
				m_socket = socket(PF_LOCAL, type, 0);//PF_LOCAL����Э�飬SOCK_STREAM TCP
		}
		else//�׽����Ѵ����ǿգ�accept�����׽��֣���������Ϊm_socket!=-1��ֱ��Ϊm_status��Ϊ2
			m_status = 2;//����CEPlayerServer.h(virtual int BusinessProcess(CProcess* proc)��50���У��ͻ��˴�����̵����߳��յ�������socket��Ҫ�����ͻ��˵�ҵ������,������һ���׽��ֺ�ֻ����Initû��Linkֱ�Ӵ���
		if (m_socket == -1) return -2;
		int ret = 0;
		if (m_param.attr & SOCK_ISREUSE) {//����
			int option = 1;
			ret = setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
			if (ret == -1) return -7;
		}
		if (m_param.attr & SOCK_ISSERVER) {//�Ƿ���������Ҫbind��listen
			if (param.attr & SOCK_ISIP)//����
				ret = bind(m_socket, m_param.addrin(), sizeof(sockaddr_in));//TODO:�����������
			else//����
				ret = bind(m_socket, m_param.addrun(), sizeof(sockaddr_un));
			if (ret == -1) return -3;//��Ϊ0���ڷ�����Ҳ��Ӧ����״̬�������ܳ���EAGAIN
			ret = listen(m_socket, 32);
			if (ret == -1) return -4;
		}//�ͻ��˲��ø�ɶ�����Բ���else

		if (m_param.attr & SOCK_ISNONBLOCK) {//����Ҫ����Ϊ��������ʽ
			int option = fcntl(m_socket, F_GETFL);//��ȡ�׽���
			if (option == -1) return -5;//��ȡʧ��
			option |= O_NONBLOCK;//��״̬�������Ƿ������ڶ�����ĳλ�����֣���������Ҫ��|��λ�����㣬��ֹ��������Ҳ�ܵ��ı�
			ret = fcntl(m_socket, F_SETFL, option);
			if(ret == -1) return -6;
		}
		
		if (m_status == 0)
			m_status = 1;//��ʼ�����
		return 0;
	}
	
	//���� ��������accept�� �ͻ��ˣ�connect�� ������UDP��������Ժ��ԣ���Ϊudp��link������send��recv�У� ����ȻUDP���Բ�����������߿��Ը�0����֤������ʹ��ʲôЭ�����̶���ͳһ�ġ���֤��дҵ���ʱ���ñ�֤�ϲ�����ô����
	virtual int Link(CSocketBase** pClient = NULL) {
		if ((m_status <= 0) || (m_socket == -1)) return -1;//��ʼ��δ���
		int ret = 0;
		if (m_param.attr & SOCK_ISSERVER) {//�����
			if (pClient == NULL) return -2;//����ָ���ַ��ӦΪ�գ������ܴ������ںͿͻ������ӵ�socketͨ��
			CSocketParam param;//���ڽ���
			socklen_t len = 0;
			int fd = -1;
			if (m_param.attr & SOCK_ISIP) {//����
				param.attr |= SOCK_ISIP;
				len = sizeof(sockaddr_in);
				fd = accept(m_socket, param.addrin(), &len);//����һ������ͨ�ŵ��µ�socket�ġ�����������
			}
			else {
				len = sizeof(sockaddr_un);
				fd = accept(m_socket, param.addrun(), &len);//����һ������ͨ�ŵ��µ�socket�ġ�����������
			}
			if (fd == -1) return -3;
			*pClient = new CSocket(fd);//����µ�socket���ڿͻ���������֮���ͨ�š�
			if (*pClient == NULL) return -4;
			ret = (*pClient)->Init(param);
			if (ret != 0) {//��ʼ��ʧ��
				delete (*pClient);
				*pClient = NULL;
				return -5;
			}
		}
		else {//�ͻ���
			if (m_param.attr & SOCK_ISIP)
				ret = connect(m_socket, m_param.addrin(), sizeof(sockaddr_in));
			else
				ret = connect(m_socket, m_param.addrun(), sizeof(sockaddr_un));
			if (ret != 0) return -6;
		}
		m_status = 2;//�������
		return 0;
	}
	//��������
	virtual int Send(const Buffer& data) {
		if ((m_status < 2) || (m_socket == -1)) return -1;
		ssize_t index = 0;
		while (index < (ssize_t)data.size()) {//û���������
			ssize_t len = write(m_socket, (char*)data + index, data.size() - index);
			if (len == 0) return -2;//�ر���
			if (len < 0) return -3;
			index += len;
		}
		//printf("%s(%d):[%s]data.size()=%d index=%d\n", __FILE__, __LINE__, __FUNCTION__, data.size(), index);
		return 0;
	}
	//�������� ����0����ʾ���ճɹ���С��0����ʾʧ�ܣ�����0����ʾû�յ����ݣ���û�д���
	virtual int Recv(Buffer& data) {
		if ((m_status < 2) || (m_socket == -1)) return -1;
		data.resize(1024 * 1024);//��Ϊһ��ʼ���������Ϊ�գ�sizeΪ0��������Ҫ���ô�С
		ssize_t len = read(m_socket, (void*)data, data.size());
		if (len > 0) {//���ճɹ�
			data.resize(len);
			return (int)len;//�յ�����
		}
		data.clear();
		if (len < 0) {
			if (errno == EINTR || (errno == EAGAIN)) {//������ʽ
				data.clear();
				return 0;//δ�յ�����
			}
			return -2;//���ʹ���
		}
		return -3;//������len==0(EOF)���׽��ֱ��ر�  //TODO:���������⣬�����˶δ���
	}
	//�ر�����
	virtual int Close() {
		return CSocketBase::Close();
	}
};
#endif