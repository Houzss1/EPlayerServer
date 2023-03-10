#pragma once
#include "Thread.h"
#include "Epoll.h"
#include "Socket.h"
#include <map>
#include <sys/timeb.h>
#include <stdarg.h>
#include <sstream>
#include <sys/stat.h>

enum LogLevel {
	LOG_INFO,//��Ϣ
	LOG_DEBUG,//����
	LOG_WARNING,//����
	LOG_ERROR,//����
	LOG_FATAL//��������
};

class LogInfo {
public:
	LogInfo(
		const char* file, int line, const char* func,
		pid_t pid, pthread_t tid, int level,
		const char* fmt, ...);//const char* fmt,...Ϊ�ɱ������fmtΪ�ɱ������ʽ��...��������������
	LogInfo(
		const char* file, int line, const char* func,
		pid_t pid, pthread_t tid, int level);
	LogInfo(
		const char* file, int line, const char* func,
		pid_t pid, pthread_t tid, int level,
		void* pData, size_t nSize);
	
	~LogInfo();
	operator Buffer() const{
		return m_buf;
	}
	template<typename T>
	LogInfo& operator<<(const T& data) {//����ֵ�����Լ���������ʹ��<<�����
		std::stringstream stream;
		stream << data;
		//printf("%s(%d):[%s][%s]\n", __FILE__, __LINE__, __FUNCTION__, stream.str().c_str());
		m_buf += stream.str().c_str();
		//printf("%s(%d):[%s][%s]\n", __FILE__, __LINE__, __FUNCTION__, (char*)m_buf);
		return *this;
	}
private:
	bool bAuto;//Ĭ����false ��ʽ��־����Ϊtrue
	Buffer m_buf;
};

class CLoggerServer//��Ӧ�ñ�����
{
public://����ӿ�
	CLoggerServer()
		:m_thread(&CLoggerServer::ThreadFunc, this)
	{
		m_server = NULL;
		char curpath[256] = "";
		getcwd(curpath, sizeof(curpath));
		m_path = curpath;
		m_path += "/log/" + GetTimeStr() + ".log";//��־�ļ���
		printf("%s(%d):[%s]path=%s\n", __FILE__, __LINE__, __FUNCTION__, (char*)m_path);
	}
	~CLoggerServer() {
		Close();
	}
	
	CLoggerServer(const CLoggerServer&) = delete;//���ܸ���
	CLoggerServer& operator=(const CLoggerServer&) = delete;//���ܸ�ֵ

	int Start(){
		if (m_server != NULL) return -1;//��startǰ�Ѿ�������ֱ�ӱ����˳�
		if (access("log", W_OK | R_OK) != 0) {//�鿴������ļ����Ƿ��ж���д��Ȩ�ޣ���������0˵���ļ��в�����
			mkdir("log", S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);//��ǰ�û�������ǰ�û�д���û�������û���д�������û���
		}
		m_file = fopen(m_path, "w+");//w+:�򿪵��ļ��Ѵ���ʱ��������ԭ�ļ�
		if (m_file == NULL) return -2;//�ļ���ʧ��
		int ret = m_epoll.Create(1);//����Epoll
		if (ret != 0) return -3;
		m_server = new CSocket();
		if (m_server == NULL) {
			Close();
			return -4;//��ʼ���׽��ֶ���ʧ��
		}
		ret = m_server->Init(CSocketParam("./log/server.sock", (int)SOCK_ISSERVER| SOCK_ISREUSE));
		if (ret != 0) {
			Close();
			//printf("%s(%d):<%s> pid = %d ret:%d errno:%d msg:%s\n", __FILE__, __LINE__, __FUNCTION__, getpid(), ret, errno, strerror(errno));
			return -5;//���������׽���ʧ��
		}
		ret = m_epoll.Add(*m_server, EpollData((void*)m_server), EPOLLIN | EPOLLERR);//����־���������ļ����������ӵ�Epoll���У�����Epoll�ض���־��������IO��������ʹ����źŵļ�����
		if (ret != 0) {
			Close();
			return -6;
		}
		ret = m_thread.Start();
		if (ret != 0) {
			printf("%s(%d):<%s> pid = %d ret:%d errno:%d msg:%s\n", __FILE__, __LINE__, __FUNCTION__, getpid(), ret, errno, strerror(errno));
			Close();
			return -7;//�߳̿�ʼ����ʧ��
		}
		return 0;
	}
	int Close(){
		if (m_server != NULL) {
			CSocketBase* p = m_server;
			m_server = NULL;
			delete p;
		}
		m_epoll.Close();
		m_thread.Stop();
		return 0;
	}
	//����������־���̵Ľ��̺��߳�ʹ�õ�
	static void Trace(const LogInfo& info) {//��־���������֣��ڴ浼����ʽ����׼���������ʽ��print��ʽ
		int ret = 0;
		static thread_local CSocket client;//1����̬�ı����׽��ֶ��󣨲�������ÿ�κ����������³�ʼ��/�����ı䣩2�������̣߳�ÿ���̵߳�����������������µ��ù��캯�������ÿ���̶߳���Ӧһ�������׽��ֶ���[�߳�֮�䲻����]��
		if (client == -1) {
			ret = client.Init(CSocketParam("./log/server.sock", 0));
			if (ret != 0) {
#ifdef _DEBUG
				printf("%s(%d):[%s]ret=%d\n", __FILE__, __LINE__, __FUNCTION__, ret);
#endif
				return;
			}
			printf("%s(%d):[%s]ret=%d client=%d\n", __FILE__, __LINE__, __FUNCTION__, ret, (int)client);
			ret = client.Link();
			printf("%s(%d):[%s]ret=%d client=%d\n", __FILE__, __LINE__, __FUNCTION__, ret, (int)client);
		}
		ret = client.Send(info);
		printf("%s(%d):[%s]ret=%d client=%d\n", __FILE__, __LINE__, __FUNCTION__, ret, (int)client);
	}
	static Buffer GetTimeStr() {//��ȡ��ʽ��ʱ��
		Buffer result(128);
		timeb tmb;
		ftime(&tmb);//�õ����뵥λ��ʱ��
		tm* pTm = localtime(&tmb.time);
		int nSize = snprintf(result, result.size(),
			"%04d-%02d-%02d_%02d-%02d-%02d.%03d",
			pTm->tm_year + 1900, pTm->tm_mon + 1, pTm->tm_mday,
			pTm->tm_hour, pTm->tm_min, pTm->tm_sec,
			tmb.millitm
		);
		result.resize(nSize);
		return result;
	}
private:
	CThread m_thread;
	CEpoll m_epoll;
	CSocketBase* m_server;
	Buffer m_path;
	FILE* m_file;
	int ThreadFunc() {//�̺߳���Ӧ��˽��
		printf("%s(%d):[%s] m_thread.isValid(): %d m_epoll:%d server:%p\n", __FILE__, __LINE__, __FUNCTION__, m_thread.isValid(), (int)m_epoll, m_server);
		EPEvents events;
		std::map<int, CSocketBase*> MapClients;
		while (m_thread.isValid() && (m_epoll != -1) && (m_server != NULL)) {
			ssize_t ret = m_epoll.WaitEvents(events, 1000);
			printf("%s(%d):[%s]ret=%d\n", __FILE__, __LINE__, __FUNCTION__, ret);
			if (ret < 0) break;//��ֹ�̣߳�����
			if (ret > 0) {//==0�����ȴ����ɣ���ʾû�����κ����ݣ�
				ssize_t i = 0;
				printf("%s(%d):[%s]ret=%d\n", __FILE__, __LINE__, __FUNCTION__, ret);
				for (; i < ret; i++) {
					if (events[i].events & EPOLLERR) {
						break;//�������˾�Ҫ��ֹ
					}
					else if (events[i].events & EPOLLIN) {
						//printf("%s(%d):[%s]ptr=%p\n", __FILE__, __LINE__, __FUNCTION__, events[i].data.ptr);
						//printf("%s(%d):[%s]m_server=%p\n", __FILE__, __LINE__, __FUNCTION__, m_server);
						if (events[i].data.ptr == m_server) {//�������յ���������(�յ��ɷ�����׽��ַ���������Ϣ)ֻ��һ��������пͻ���Ҫ������,��ʱ�����Ҫaccept
							CSocketBase* pClient = NULL;
							int r = m_server->Link(&pClient);//ָ��ĵ�ַ�����ǵ�ַ���϶��ǿգ�
							//printf("%s(%d):[%s]r=%d\n", __FILE__, __LINE__, __FUNCTION__, r);
							if (r < 0) {//�׽��������⣬û���ܳɹ�
								continue;//���ô�����Ϊret>0���ͻ���delete����
							}
							r = m_epoll.Add(*pClient, EpollData((void*)pClient), EPOLLIN | EPOLLERR);//����Կͻ��˴����� �ͻ��˴������ ���м���������Epoll�أ��������뱨��
							//ע�⣬�����*pClient�����EpollData�Ĺ��캯��ʹ�� union������m_data�洢ָ������󷵻�����EpollData����
							//printf("%s(%d):[%s]r=%d\n", __FILE__, __LINE__, __FUNCTION__, r);
							if (r < 0) {
								delete pClient;
								continue;
							}//�ɹ��󱣴�
							auto it = MapClients.find(*pClient);
							if (it != MapClients.end()) {//���Ѿ�����
								if(it->second)	delete it->second;//ɾ�����滻���µ�
							}
							MapClients[*pClient] = pClient;//ע�⣬�����*pClient���Զ�ת��Ϊ������µ�m_socket
							//printf("%s(%d):[%s]r=%d\n", __FILE__, __LINE__, __FUNCTION__, r);
						}
						else {//���߳����ڿͻ��ˣ���Ҫ��(�յ��ɿͻ����׽��ַ���������־��Ϣ����Ҫ���ղ�д����־)
							printf("%s(%d):[%s]ptr=%p\n", __FILE__, __LINE__, __FUNCTION__, events[i].data.ptr);
							CSocketBase* pClient = (CSocketBase*)events[i].data.ptr;
							if (pClient != NULL) {//��Ӧ�ͻ��˿϶��Ѿ���������Link(connect)���ӹ����϶��Ѿ����������if���̣����ʲ���ҪInit��Link��Epoll.add
								Buffer data(1024 * 1024);
								int r2 = pClient->Recv(data);
								printf("%s(%d):[%s]r2=%d\n", __FILE__, __LINE__, __FUNCTION__, r2);
								if (r2 <= 0) {//����ʧ��
									printf("%s(%d):[%s]r2=%d\n", __FILE__, __LINE__, __FUNCTION__, r2);
									MapClients[*pClient] = NULL;
									//printf("%s(%d):[%s]r2=%d\n", __FILE__, __LINE__, __FUNCTION__, r2);
									delete pClient;
									//printf("%s(%d):[%s]r2=%d\n", __FILE__, __LINE__, __FUNCTION__, r2);
								}
								else {//���ճɹ�
									//printf("%s(%d):[%s]data=%s\n", __FILE__, __LINE__, __FUNCTION__, (char*)data);
									WriteLog(data);//д����־
								}
								//printf("%s(%d):[%s]r2=%d\n", __FILE__, __LINE__, __FUNCTION__, r2);
							}
						}
					}
				}
				if (i != ret) {//��ǰ�����������߳���Ҫbreak��
					break;
				}
			}
		}
		for (auto it = MapClients.begin(); it != MapClients.end(); it++) {
			if (it->second) {//!= NULL
				delete it->second;//��ֹ�ڴ�й©
			}
		}
		MapClients.clear();
		return 0;
	}
	void WriteLog(const Buffer& data) {
		if (m_file != NULL) {
			FILE* pFile = m_file;
			fwrite((char*)data, 1, data.size(), pFile);//һ��дһ���ֽڣ�дdata.size()��
			fflush(pFile);//��ջ����������ѻ�����Ӧд���ļ�������ִ��д�������
#ifdef _DEBUG
			printf("%s",(char*)data);
#endif
		}
	}
};
#ifndef TRACE
#define TRACEI(...) CLoggerServer::Trace(LogInfo(__FILE__,__LINE__,__FUNCTION__,getpid(),pthread_self(),LOG_INFO,__VA_ARGS__))//ǰ���...��������__VA_ARGS__�ɱ������
#define TRACED(...) CLoggerServer::Trace(LogInfo(__FILE__,__LINE__,__FUNCTION__,getpid(),pthread_self(),LOG_DEBUG,__VA_ARGS__))//ǰ���...��������__VA_ARGS__�ɱ������
#define TRACEW(...) CLoggerServer::Trace(LogInfo(__FILE__,__LINE__,__FUNCTION__,getpid(),pthread_self(),LOG_WARNING,__VA_ARGS__))//ǰ���...��������__VA_ARGS__�ɱ������
#define TRACEE(...) CLoggerServer::Trace(LogInfo(__FILE__,__LINE__,__FUNCTION__,getpid(),pthread_self(),LOG_ERROR,__VA_ARGS__))//ǰ���...��������__VA_ARGS__�ɱ������
#define TRACEF(...) CLoggerServer::Trace(LogInfo(__FILE__,__LINE__,__FUNCTION__,getpid(),pthread_self(),LOG_FATAL,__VA_ARGS__))//ǰ���...��������__VA_ARGS__�ɱ������

//LOGI<<"hello"<<"how are you";
#define LOGI LogInfo(__FILE__,__LINE__,__FUNCTION__,getpid(),pthread_self(),LOG_INFO)
#define LOGD LogInfo(__FILE__,__LINE__,__FUNCTION__,getpid(),pthread_self(),LOG_DEBUG)
#define LOGW LogInfo(__FILE__,__LINE__,__FUNCTION__,getpid(),pthread_self(),LOG_WARNING)
#define LOGE LogInfo(__FILE__,__LINE__,__FUNCTION__,getpid(),pthread_self(),LOG_ERROR)
#define LOGF LogInfo(__FILE__,__LINE__,__FUNCTION__,getpid(),pthread_self(),LOG_FATAL)
//�ڴ浼������ 00 01 02 03... ; ...a...
#define DUMPI(data, size) CLoggerServer::Trace(LogInfo(__FILE__, __LINE__, __FUNCTION__, getpid(), pthread_self(), LOG_INFO, data, size))//data:�������ڵ�ַ��size���ߴ�
#define DUMPD(data, size) CLoggerServer::Trace(LogInfo(__FILE__, __LINE__, __FUNCTION__, getpid(), pthread_self(), LOG_DEBUG, data, size))//data:�������ڵ�ַ��size���ߴ�
#define DUMPW(data, size) CLoggerServer::Trace(LogInfo(__FILE__, __LINE__, __FUNCTION__, getpid(), pthread_self(), LOG_WARNING, data, size))//data:�������ڵ�ַ��size���ߴ�
#define DUMPE(data, size) CLoggerServer::Trace(LogInfo(__FILE__, __LINE__, __FUNCTION__, getpid(), pthread_self(), LOG_ERROR, data, size))//data:�������ڵ�ַ��size���ߴ�
#define DUMPF(data, size) CLoggerServer::Trace(LogInfo(__FILE__, __LINE__, __FUNCTION__, getpid(), pthread_self(), LOG_FATAL, data, size))//data:�������ڵ�ַ��size���ߴ�
#endif