#pragma once
#include "Socket.h"
#include "Epoll.h"
#include "ThreadPoll.h"
#include "Process.h"

template<typename _FUNCTION_, typename... _ARGS_>
class CConnectedFunction : public CFunctionBase {
public:
	CConnectedFunction(_FUNCTION_ func, _ARGS_... args)//�����funcһ�������洫���ĺ������ڵ�ַ,���������func���붼�Ǵ���&func(ȡ��ַ),��Ϊ��֪�����������Ӧ�Ķ���,���븴�Ƶ����Ա����û�е���(��֪����Ӧ�ĸ�����,������)
		:m_binder(std::forward<_FUNCTION_>(func), std::forward<_ARGS_>(args)...)//forward��ʾԭ��ת����()...��ʾ����չ�� std::forward<T>(u)������������T �� u����TΪ��ֵ��������ʱ��u����ת��ΪT���͵���ֵ������u����ת��ΪT������ֵ����˶���std::forward��Ϊ����ʹ����ֵ���ò����ĺ���ģ���н������������ת�����⡣
		//bind�����Ա����ʱ����һ��������ʾ����ĳ�Ա������ָ��(������ʽ��ָ��&,��Ϊ���������Ὣ����ĳ�Ա������ʽת���ɺ���ָ��)���ڶ���������ʾ����ĵ�ַ(ʹ�ö����Ա������ָ��ʱ������Ҫ֪����ָ�������ĸ�������˵ڶ�������Ϊ����ĵ�ַ &base)��
	{}//����m_binderǰ���ð�Ŵ����ʼ��
	virtual ~CConnectedFunction() {}
	virtual int operator()(CSocketBase* pClient) {
		return m_binder(pClient);
	}
	//std::bind���ڸ�һ���ɵ��ö���󶨲������ɵ��ö�������������󣨷º�����������ָ�롢�������á���Ա����ָ������ݳ�Աָ�롣
	typename std::_Bindres_helper<int, _FUNCTION_, _ARGS_...>::type m_binder;//�൱�ڷ�
};

template<typename _FUNCTION_, typename... _ARGS_>
class CReceivedFunction : public CFunctionBase {
public:
	CReceivedFunction(_FUNCTION_ func, _ARGS_... args)//�����funcһ�������洫���ĺ������ڵ�ַ,���������func���붼�Ǵ���&func(ȡ��ַ),��Ϊ��֪�����������Ӧ�Ķ���,���븴�Ƶ����Ա����û�е���(��֪����Ӧ�ĸ�����,������)
		:m_binder(std::forward<_FUNCTION_>(func), std::forward<_ARGS_>(args)...)//forward��ʾԭ��ת����()...��ʾ����չ�� std::forward<T>(u)������������T �� u����TΪ��ֵ��������ʱ��u����ת��ΪT���͵���ֵ������u����ת��ΪT������ֵ����˶���std::forward��Ϊ����ʹ����ֵ���ò����ĺ���ģ���н������������ת�����⡣
		//bind�����Ա����ʱ����һ��������ʾ����ĳ�Ա������ָ��(������ʽ��ָ��&,��Ϊ���������Ὣ����ĳ�Ա������ʽת���ɺ���ָ��)���ڶ���������ʾ����ĵ�ַ(ʹ�ö����Ա������ָ��ʱ������Ҫ֪����ָ�������ĸ�������˵ڶ�������Ϊ����ĵ�ַ &base)��
	{}//����m_binderǰ���ð�Ŵ����ʼ��
	virtual ~CReceivedFunction() {}
	virtual int operator()(CSocketBase* pClient, const Buffer& data) {
		return m_binder(pClient, data);
	}
	//std::bind���ڸ�һ���ɵ��ö���󶨲������ɵ��ö�������������󣨷º�����������ָ�롢�������á���Ա����ָ������ݳ�Աָ�롣
	typename std::_Bindres_helper<int, _FUNCTION_, _ARGS_...>::type m_binder;//�൱�ڷ���
};


class CBusiness {//���ڴ���ҵ�����(������)
public:
	CBusiness() : 
		m_connectedcallback(NULL), 
		m_recvcallback(NULL){}
	virtual int BusinessProcess(CProcess* proc) = 0;//���麯��
	template<typename _FUNCTION_, typename... _ARGS_>
	int setConnectedCallback(_FUNCTION_ func, _ARGS_... args)
	{
		//m_connectedcallback = new CFunction<_FUNCTION_, _ARGS_...>(func, args...);
		m_connectedcallback = new CConnectedFunction<_FUNCTION_, _ARGS_...>(func, args...);
		if (m_connectedcallback == NULL) return -1;//����ʧ��
		return 0;//���óɹ�
	}
	template<typename _FUNCTION_, typename... _ARGS_>
	int setRecvCallback(_FUNCTION_ func, _ARGS_... args)
	{
		//m_recvcallback = new CFunction<_FUNCTION_, _ARGS_...>(func, args...);
		m_recvcallback = new CReceivedFunction<_FUNCTION_, _ARGS_...>(func, args...);
		if (m_recvcallback == NULL) return -1;//����ʧ��
		return 0;//���óɹ�
	}
protected:
	CFunctionBase* m_connectedcallback;//���ӳɹ��ص�����
	CFunctionBase* m_recvcallback;//���ճɹ��ص�����
};

class CServer//1.���ӽ���(�ͻ��˴������)  
{
public:
	CServer();
	~CServer();
	CServer(const CServer&) = delete;//�����˽���\�̵߳�,���ܱ����ƿ���
	CServer& operator=(const CServer&) = delete;
	int Init(CBusiness* business, const Buffer& ip = "0.0.0.0", short port = 9999, unsigned thread_count = 2);
	int Run();
	int Close();

	
private:
	CThreadPoll m_tpoll;//�̳߳�
	CSocketBase* m_server;
	CEpoll m_epoll;//epoll��(��Ҫ�������߳̽���ͻ���)
	CProcess m_process;//���ڿ�����
	CBusiness* m_business;//ҵ��ģ��, ��Ҫ�ֶ�delete
	int ThreadFunc();//���йؼ��߼�
};

