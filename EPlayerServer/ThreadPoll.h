#pragma once
#include "Epoll.h"
#include "Thread.h"
#include "Function.h"
#include "Socket.h"
#include <vector>

class CThreadPoll {
public:
	CThreadPoll() {
		m_server = NULL;//�׽���ָ���ÿ�
		timespec tp = { 0,0 };
		clock_gettime(CLOCK_REALTIME, &tp);
		char* buf = NULL;
		asprintf(&buf, "%d.%d.sock", tp.tv_sec % 100000 , tp.tv_nsec % 1000000);
		if (buf != NULL) {
			m_path = buf;
			free(buf);
		}//������Ļ�����start�ӿ����ж�m_path�������
		usleep(1);//��ֹ�����̳߳����ñ����׽����ļ��غ�
	};
	~CThreadPoll() {
		Close();
	}
	CThreadPoll(const CThreadPoll&) = delete;//��ֹ����
	CThreadPoll& operator=(const CThreadPoll&) = delete;//��ֹ��ֵ

	int Start(unsigned count) {
		if (m_server != NULL) return -1;//�Ѿ���ʼ����
		if (m_path.size() == 0) return -2;//���캯��ʧ��
		m_server = new CSocket();
		if (m_server == NULL) return -3;
		int ret = m_server->Init(CSocketParam(m_path, SOCK_ISSERVER));
		if (ret != 0) return -4;
		ret = m_epoll.Create(count);
		if (ret != 0) return -5;
		ret = m_epoll.Add(*m_server, EpollData((void*)m_server));
		if (ret != 0) return -6;
		m_threads.resize(count);
		for (unsigned i = 0; i < count; i++) {
			m_threads[i] = new CThread(&CThreadPoll::TaskDispatch, this);
			if (m_threads[i] == NULL) return -7;
			ret = m_threads[i]->Start();//�����߳�
			if (ret != 0) return -8;//����ʧ��
		}
		return 0;
	}
	void Close() {//�̳߳ؽ����ͳ�ʼ��ʧ��ʱ������
		m_epoll.Close();
		if (m_server) {
			CSocketBase* p = m_server;
			m_server = NULL;
			delete p;
		}
		for (auto thread : m_threads) {
			if (thread != NULL) delete thread;
		}
		m_threads.clear();
		unlink(m_path);
	}
	template<typename _FUNCTION_, typename... _ARGS_>
	int AddTask(_FUNCTION_ func, _ARGS_... args) {
		static thread_local CSocket client;//thread_localΪC++11��������ԣ�����ÿ���̶߳���һ������ÿ���̷߳��䲻ͬ�Ŀͻ��ˣ�
		int ret = 0;
		if (client == -1) {
			ret = client.Init(CSocketParam(m_path, 0));
			if (ret != 0) return -1;
			ret = client.Link();
			if (ret != 0) return -2;
		}
		CFunctionBase* base = new CFunction<_FUNCTION_, _ARGS_...>(func, args...);//ģ�庯���ѹ���
		if (base == NULL) return -3;
		Buffer data(sizeof(base));
		memcpy(data, &base, sizeof(base));//��ָ���ַ��ֵ��data��������ָ��ָ��Ķ������ڵ�ַ��
		ret = client.Send(data);
		if (ret != 0) {
			delete base;
			return -4;
		}
		return 0;
	}
	size_t Size() const { return m_threads.size(); };
private:
	int TaskDispatch() {
		while (m_epoll != -1) {
			EPEvents events;
			int ret = 0;
			ssize_t esize = m_epoll.WaitEvents(events);
			if (esize > 0) {
				for (ssize_t i = 0; i < esize; i++) {
					if (events[i].events & EPOLLIN) {
						CSocketBase* pClient = NULL;
						if (events[i].data.ptr == m_server) {//�������׽����յ����룬�пͻ���������
							ret = m_server->Link(&pClient);
							if (ret != 0) continue;
							m_epoll.Add(*pClient, EpollData((void*)pClient));
							if (ret != 0) {
								delete pClient;
								continue;
							}
						}
						else {//�ͻ����׽����յ����루�յ����ݣ�
							pClient = (CSocketBase*)events[i].data.ptr;
							if (pClient != NULL) {
								CFunctionBase* base = NULL;
								Buffer data(sizeof(base));
								ret = pClient->Recv(data);
								if (ret <= 0) {//��ȡʧ��
									m_epoll.Del(*pClient);
									delete pClient;
									continue;
								}
								memcpy(&base,(char*)data, sizeof(base));
								if (base != NULL) {
									(*base)();//�����������ĵ���
									delete base;//����ִ�н������ͷ�
								}
							}
						}
					}
				}
			}
		}
		return 0;
	}
	
	CEpoll m_epoll;
	std::vector<CThread*> m_threads;//Ϊʲô���߳�ָ������������̶߳�������?��Ҫ�ŵ�������Ķ���һ��Ҫ��Ĭ�Ϲ��캯���͸��ƹ��캯�����߳�û�и��ƹ��캯������˲���ֱ�ӷ��������ڣ����ֻ�ܷ���ָ�롣
	CSocketBase* m_server;
	Buffer m_path;//�������ӣ��ᴴ��һ��β�ļ�����Сһֱ��0���������׽��ֵ��ں˶������ļ��е�һ������
};