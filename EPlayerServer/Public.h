#pragma once
#include <string>
#include <string.h>
class Buffer :public std::string
{//string�����࣬���˷�װ������
public:
	Buffer() :std::string() {}//Ĭ�Ϲ��캯������������ĺ����������˸��ࣨ����std::string����Ĭ�Ϲ��캯��
	Buffer(size_t size) :std::string() { resize(size); }//�̳и���string�ĳ�Ա���� }//�����˻���Ĺ��캯�����⻹�ı��˴�С
	Buffer(const std::string& str) :std::string(str) {}//�ṩ�Զ�ת��string��Buffer����Ĺ��캯��
	Buffer(const char* str) :std::string(str) {}//�ṩ�Զ�ת��char*��Buffer����Ĺ��캯��
	Buffer(const char* str, size_t length) :std::string() {
		resize(length);//�̳и���string�ĳ�Ա����
		memcpy((char*)c_str(), str, length);
	}
	Buffer(const char* begin, const char* end) :std::string() {
		long int len = end - begin;//����ҿ������Գ��Ȳ��ü�1
		if (len > 0) {
			resize(len);
			memcpy((char*)c_str(), begin, len);
		}
	}
	//ע�⣺���幹�캯����ͬʱ����Ҫ���¶���Ĭ�Ϲ��캯��
	operator char* () { return (char*)c_str(); }//����ǿ��ת��
	operator unsigned char* () { return (unsigned char*)c_str(); }
	operator char* () const { return (char*)c_str(); }//������������Ϊ������������ǿ��ת������ʽ����this��
	operator const char* () const { return c_str(); }//���������͵�ǿ��ת�������س�������ָ��
	operator void* () { return (void*)c_str(); }
	operator const void* () const { return (void*)c_str(); }
};