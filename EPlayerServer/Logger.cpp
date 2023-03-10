#include "Logger.h"

LogInfo::LogInfo(const char* file, int line, const char* func, 
	pid_t pid, pthread_t tid, int level, 
	const char* fmt, ...)
{
	const char sLevel[][8] = {
		"INFO","DEBUG","WARNING","ERROR","FATAL"
	};
	char* buf = NULL;
	bAuto = false;
	int count = asprintf(&buf, "%s(%d):[%s][%s]<%d-%d>(%s) ", file, line, sLevel[level], (char*)CLoggerServer::GetTimeStr(), pid, tid, func);
	if (count > 0) {
		m_buf = buf;
		free(buf);
	}
	else return;//��֮����ֱ���˳�

	va_list ap;//�ɱ��������
	va_start(ap, fmt);
	count = vasprintf(&buf, fmt, ap);//vasprintf �������䳤��������������� buf �У����ɹ��򷵻�������ݵĳ��ȣ���ʧ���򷵻� -1.
	if (count > 0) {
		m_buf += buf;
		free(buf);//��ֹ�ڴ�й©
	}
	m_buf += "\n";
	va_end(ap);
}

LogInfo::LogInfo(const char* file, int line, const char* func, 
	pid_t pid, pthread_t tid, int level)
{//�Լ��������� ��ʽ��־
	const char sLevel[][8] = {
		"INFO","DEBUG","WARNING","ERROR","FATAL"
	};
	char* buf = NULL;
	bAuto = true;
	int count = asprintf(&buf, "%s(%d):[%s][%s]<%d-%d>(%s) ", file, line, sLevel[level], (char*)CLoggerServer::GetTimeStr(), pid, tid, func);
	if (count > 0) {
		m_buf = buf;
		free(buf);
	}
}

LogInfo::LogInfo(const char* file, int line, const char* func,
	pid_t pid, pthread_t tid, int level, 
	void* pData, size_t nSize)
{
	const char sLevel[][8] = {
		"INFO","DEBUG","WARNING","ERROR","FATAL"
	};
	char* buf = NULL;
	bAuto = false;
	int count = asprintf(&buf, "%s(%d):[%s][%s]<%d-%d>(%s)\n", file, line, sLevel[level], (char*)CLoggerServer::GetTimeStr(), pid, tid, func);
	if (count > 0) {
		m_buf = buf;
		free(buf);
	}
	else return;//��֮����ֱ���Ƴ�

	Buffer out;
	size_t i = 0;
	char* Data = (char*)pData;
	for (; i < nSize; i++) {
		char buf[16] = "";
		snprintf(buf, sizeof(buf), "%02X ", Data[i] & 0xFF);
		m_buf += buf;
		if (0 == ((i + 1) % 16)) {//ÿ16���ַ�һ��
			m_buf += "\t; ";
			char buf[17] = "";
			memcpy(buf, Data + i - 15, 16);
			for (int j = 0; j < 16; j++)
				if ((buf[j] < 32) && (buf[j]>=0)) buf[j] = '.';//������ʾ��������һ������ռ3��16λ��
			m_buf += buf;
			//for (size_t j = i - 15; j <= i; j++) {//�������
			//	if (((Data[j] & 0xFF) > 31) && ((Data[j] & 0xFF) < 0x7F)) {//31Ϊ����λ,��ASCII�����ʾ��Χ��
			//		m_buf += Data[i];
			//	}
			//	else {
			//		m_buf += '.';
			//	}
			//}
			m_buf += "\n";
		}
	}
	//����β��
	size_t k = i % 16;
	if (k != 0) {//��β�͵���16���������δ����
		for (size_t j = 0; j < 16 - k; j++) m_buf += "   ";
		m_buf += "\t; ";
		for (size_t j = i - k; j <= i; j++) {
			if (((Data[j] & 0xFF) > 31) && ((Data[j] & 0xFF) < 0x7F)) {//31Ϊ����λ,��ASCII�����ʾ��Χ��
				m_buf += Data[i];
			}
			else {
				m_buf += '.';
			}
		}
		m_buf += "\n";
	}

}

LogInfo::~LogInfo()
{
	if (bAuto) {//Ϊ�����ʾ��Ҫ�Լ�����
		m_buf += "\n";
		CLoggerServer::Trace(*this);//���Լ�����ȥ
	}
}