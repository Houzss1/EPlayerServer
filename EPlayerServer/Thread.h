#pragma once
#include <unistd.h>//linuxͨ����������
#include <pthread.h>
#include <fcntl.h>//Ȩ�����͡��ļ��������͵ĺ궨��
#include "Function.h"
#include <cstdio>
#include <signal.h>
#include <cerrno>
#include <map>
class CThread
{
public:
	CThread() {
		m_function = NULL;
		m_thread = 0;//unsigned long
		m_bpaused = false;
	}
	template<typename _FUNCTION_, typename... _ARGS_>
	CThread(_FUNCTION_ func, _ARGS_... args) 
		:m_function(new CFunction<_FUNCTION_, _ARGS_...>(func, args...))
	{
		m_thread = 0;
		m_bpaused = false;
	}
	~CThread() {}
	//��ֹ���ƿ���
	CThread(const CThread&) = delete;
	CThread operator=(const CThread&) = delete;

	template<typename _FUNCTION_, typename... _ARGS_>
	int SetThreadFunc(_FUNCTION_ func, _ARGS_... args)
	{
		m_function = new CFunction<_FUNCTION_, _ARGS_...>(func, args...);
		if (m_function == NULL) return -1;
		return 0;
	}
	int Start()
	{
		pthread_attr_t attr;
		int ret = 0;
		ret = pthread_attr_init(&attr);
		if (ret != 0) return -1;
		//linux�߳�ִ�к�windows��ͬ��pthread������״̬joinable״̬��unjoinable״̬������߳���joinable״̬�����̺߳����Լ������˳�ʱ��pthread_exitʱ�������ͷ��߳���ռ�ö�ջ���߳����������ܼ�8K�ࣩ��ֻ�е��������pthread_join֮����Щ��Դ�Żᱻ�ͷš�����unjoinable״̬���̣߳���Щ��Դ���̺߳����˳�ʱ��pthread_exitʱ�Զ��ᱻ�ͷš�
		//unjoinable���Կ�����pthread_createʱָ���������̴߳��������߳���pthread_detach�Լ�, �磺pthread_detach(pthread_self())����״̬��Ϊunjoinable״̬��ȷ����Դ���ͷš����߽��߳���Ϊ joinable,Ȼ����ʱ����pthread_join.
		ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);//������ʱ�������߳�״̬��ΪJoinable����Ӧ��ҪStop����ʱ�߳���δ����Ҫjoin�ȴ�����������
		if (ret != 0) return -2;
		//ret = pthread_attr_setscope(&attr, PTHREAD_SCOPE_PROCESS);//Ĭ�ϣ��ɲ����ã����þ�����Χ�ڽ����ڲ�������ȫ��/����ϵͳ���н��̣�����ȫ�ֽ��̵������߳̾����������ף�
		//if (ret != 0) return -3;
		ret = pthread_create(&m_thread,&attr,&CThread::ThreadEntry, this);//���ﴫ���thisΪCThread������thisָ��,����m_function��Ӧ���ָ��(���ָ���Ѿ������뺯�������ڲ���)
		//pthread_create���̺߳��������Ǿ�̬������ԭ��: void *(*__start_routine) (void *) <=> void *ThreadFunc(void *args); ������ͨ���Ա�������麯��������ʵ���϶��ǰ����˵������ǵĶ���� this ָ�룬���������������Ż����̺߳�����Ϊ��void *��������+ ThreadFunc(this, void *args);
		//��͵����˸ú����ĸ�ʽ�ǲ����� pthread_create() ���̺߳�����Ҫ��ġ��ʣ�������Ա������Ϊ�̺߳���ʱ�������Ǿ�̬�ġ�
		//�Ѿ��Ļ���ȥ��̬��ȥ�Ǿ�̬��ԭ��Ϊ��ʹ��һЩ��Ա���Ժ�һЩ��Ա���������һֱҪʹ��ָ��ͱȽϵ������£���̫���ʣ�
		//��Ϊ�����ڹ��г�Ա����ͨ����������˽�о�̬��Ա���������У�������Ϊ��̬��Ա����û��thisָ�룬ֻ�ܴ���this��Ϊ������Ȼ��ͨ��thiz=this,thiz->nonStaticFunc()ָ�������һ������ʵ�ִӾ�̬���Ǿ�̬��
		if (ret != 0) return -4;
		m_mapThread[m_thread] = this;
		ret = pthread_attr_destroy(&attr);
		if (ret != 0) return -5;
		return 0;
	}
	int Pause() {
		if (m_thread != 0) return -1;//�߳���Ч
		if (m_bpaused == true) {
			m_bpaused = false;
			return 0;
		}
		m_bpaused = true;//��Ϊ��ֵ�����ȽϿ죬����Ӧ�ȸ�ֵ���������߳̿�ʼ�ȴ���ͣ�ź�ʱҲ�����̿������bool�����Ѿ��л��ɱ�ʾ�̱߳���ͣ��
		int ret = pthread_kill(m_thread, SIGUSR1);//���̷߳�����ͣ�ź�
		if (ret != 0) {
			m_bpaused = false;
			return -2;
		}
		return 0;
	}
	int Stop() {
		if (m_thread != 0) {//���̴߳���
			pthread_t thread = m_thread;//�ȴ��߳���������
			m_thread = 0;
			timespec ts;
			ts.tv_sec = 0;//��
			ts.tv_nsec = 100 * 1000000;//����(��������Ϊ100����)
			int ret = pthread_timedjoin_np(thread, NULL, &ts);//pthread_join()�������̺߳������̣߳����߳������ȴ����߳̽�����Ȼ��������߳���Դ��
			if (ret == ETIMEDOUT) {//��ʱ
				pthread_detach(thread);//���̸߳�Ϊunjoinable״̬�������Ͳ���Ҫ��pthread_join()һ�����̡߳���ƨ�ɡ����ر��̼߳����ͷ��߳���Դ��
				pthread_kill(thread, SIGUSR2);//���̷߳����˳��źţ���Ϊ��һ���ѽ��̸߳�Ϊunjoinable�����Ի�ֱ���ͷ���Դ����ջ����������
			}
		}
		return 0;
	}
	bool isValid() const { return m_thread != 0; }
private:
	CFunctionBase* m_function;
	pthread_t m_thread;
	bool m_bpaused;//true��ʾ��ͣ false��ʾ������
	static std::map<pthread_t, CThread*> m_mapThread;//���Ǿ�̬�ģ���Ȼ��Sigaction�޷�ʹ�ã��������ⲿ��ʼ��
	static void* ThreadEntry(void* arg)//_stdcall��׼���� ��̬��Ա���������� �������ڶ������ǹ��г�Ա����������ͨ��������ֱ�ӷ���
	{
		CThread* thiz = (CThread*)arg;//��̬����û��thisָ�룬ֻ��ȡ����
		struct sigaction act = { 0 };//���Ǻ������ǽṹ��
		sigemptyset(&act.sa_mask);//������룬��ֹ������
		act.sa_flags = SA_SIGINFO;//ʹ�ûص�����������
		act.sa_sigaction = &CThread::Sigaction;
		sigaction(SIGUSR1, &act, NULL);//������ͣ
		sigaction(SIGUSR2, &act, NULL);//���ڽ�������Stop����������ɱ���źţ�SIGUSRΪ�û�DIY�����ʾ������ź�����ֻ��1��2��

		thiz->EnterThread();

		if (thiz->m_thread) thiz->m_thread = 0;//��Ϊ0������
		pthread_t thread = pthread_self();//���ǲ�����thiz->m_thread���п���thiz->m_threadΪ�գ���stop���������ˣ�
		auto it = m_mapThread.find(thread);
		if (it != m_mapThread.end())
			m_mapThread[thread] = NULL;
		pthread_detach(thread);//��Ϊunjoinable�����߳̽������к��������٣������ͷ�ռ����Դ��
		pthread_exit(NULL);//�˳��߳�
	}
	static void Sigaction(int signo,siginfo_t* info,void* context)//signo��signal_number��info������ȥ�Ķϵ���Ϣ context��������
	{
		if (signo == SIGUSR1) {//�߳���ͣ
			pthread_t thread = pthread_self();
			auto it = m_mapThread.find(thread);
			if (it != m_mapThread.end()) {//map�д���
				if (it->second!=NULL) {
					it->second->m_bpaused = true;
					while (it->second->m_bpaused) {
						if (it->second->m_thread == 0) pthread_exit(NULL);//����ֹͣ
						usleep(1000);//ÿ����1ms��ѯ״̬�Ƿ�ok
					}
				}
			}
		}
		else if (signo == SIGUSR2) {//�߳��˳�
			pthread_exit(NULL);
		}
	}
	void EnterThread()//__thiscall��ָ����Ϊ��ʽ������������ã���
	{
		if (m_function != NULL) {
			int ret = (*m_function)();//����m_function()���������Ѵ���ԭ������ĺ�������
			//1.������������˽�зǾ�̬��Ա����,��ôһ����Ҫthisָ���ҵ���Ӧ�����Ӧ�ĳ�Ա����
			//2.��������Ҫ��������˽�г�Ա,�϶��ᴫ��thisָ��,���ɵ���ԭ�����ڵ�˽�г�Ա����/����)
			if (ret != 0) {
				printf("%s(%d):[%s] ret = %d\n", __FILE__, __LINE__, __FUNCTION__,ret);
			}
		}
	}
};