#include "HttpParser.h"
#include <string.h>
CHttpParser::CHttpParser()
{
	m_complete = false;
	memset(&m_parser, 0, sizeof(m_parser));
	m_parser.data = this;//data�洢���ӻ��׽��ֶ��󣨿��Դ���HttpParser���󣩣����ڻص������Ǿ�̬�������׼��Ա������ֻ�н�thisָ����Ϊ��������ſ���ʹ�����ڳ�Ա����
	http_parser_init(&m_parser, HTTP_REQUEST);//�����ֻ���յ����Կͻ��˵�����(����ǿͻ��˻��յ�����˵���Ӧ)
	memset(&m_settings, 0, sizeof(m_settings));
	m_settings.on_message_begin = &CHttpParser::OnMessageBegin;
	m_settings.on_url = &CHttpParser::OnUrl;
	m_settings.on_status = &CHttpParser::OnStatus;
	m_settings.on_header_field = &CHttpParser::OnHeaderField;
	m_settings.on_header_value = &CHttpParser::OnHeaderValue;
	m_settings.on_headers_complete = &CHttpParser::OnHeadersComplete;
	m_settings.on_body = &CHttpParser::OnBody;
	m_settings.on_message_complete = &CHttpParser::OnMessageComplete;
	/*m_settings.on_chunk_header = NULL;
	m_settings.on_chunk_complete = NULL;*///��Ϊǰ��memset�ÿ��ˣ��������ﲻ����
}

CHttpParser::~CHttpParser()
{}

CHttpParser::CHttpParser(const CHttpParser & http)
{
	memcpy(&m_parser, &http.m_parser, sizeof(m_parser));
	m_parser.data = this;
	memcpy(&m_settings, &http.m_settings, sizeof(m_settings));
	m_status = http.m_status;
	m_url = http.m_url;
	m_body = http.m_body;
	m_complete = http.m_complete;
	m_lastField = http.m_lastField;
}

CHttpParser& CHttpParser::operator=(const CHttpParser& http)
{
	if (this != &http) {
		memcpy(&m_parser, &http.m_parser, sizeof(m_parser));
		m_parser.data = this;
		memcpy(&m_settings, &http.m_settings, sizeof(m_settings));
		m_status = http.m_status;//Buffer��������string�̳��࣬�ȺŸ�ֵ����ǳ��ֵ��ַ
		m_url = http.m_url;
		m_body = http.m_body;
		m_complete = http.m_complete;
		m_lastField = http.m_lastField;
	}
	return *this;
}

size_t CHttpParser::Parser(const Buffer& data)
{
	m_complete = false;
	size_t ret = http_parser_execute( 
		&m_parser, &m_settings, data, data.size());//�����˶���
	if (m_complete == false) {
		m_parser.http_errno = 0x7f;//127����
		return 0;
	}
	return ret;
}

int CHttpParser::OnMessageBegin(http_parser* parser)
{
	return ((CHttpParser*)parser->data)->OnMessageBegin();//((CHttpParser*)parser->data)Ϊ���ص�ָ����󣬼�ʹ�þ�̬�������ö���ĳ�Ա����
}

int CHttpParser::OnUrl(http_parser* parser, const char* at, size_t length)
{
	return ((CHttpParser*)parser->data)->OnUrl(at,length);
}

int CHttpParser::OnStatus(http_parser* parser, const char* at, size_t length)
{
	return ((CHttpParser*)parser->data)->OnStatus(at,length);
}

int CHttpParser::OnHeaderField(http_parser* parser, const char* at, size_t length)
{
	return ((CHttpParser*)parser->data)->OnHeaderField(at,length);
}

int CHttpParser::OnHeaderValue(http_parser* parser, const char* at, size_t length)
{
	return ((CHttpParser*)parser->data)->OnHeaderValue(at,length);
}

int CHttpParser::OnHeadersComplete(http_parser* parser)
{
	return ((CHttpParser*)parser->data)->OnHeadersComplete();
}

int CHttpParser::OnBody(http_parser* parser, const char* at, size_t length)
{
	return ((CHttpParser*)parser->data)->OnBody(at,length);
}

int CHttpParser::OnMessageComplete(http_parser* parser)
{
	return ((CHttpParser*)parser->data)->OnMessageComplete();
}

int CHttpParser::OnMessageBegin()
{
	return 0;
}

int CHttpParser::OnUrl(const char* at, size_t length)
{
	m_url = Buffer(at, length);
	return 0;
}

int CHttpParser::OnStatus(const char* at, size_t length)
{
	m_status = Buffer(at, length);
	return 0;
}

int CHttpParser::OnHeaderField(const char* at, size_t length)
{
	m_lastField = Buffer(at, length);
	return 0;
}

int CHttpParser::OnHeaderValue(const char* at, size_t length)
{
	m_HeaderValues[m_lastField] = Buffer(at, length);
	return 0;
}

int CHttpParser::OnHeadersComplete()
{
	return 0;
}

int CHttpParser::OnBody(const char* at, size_t length)
{
	m_body = Buffer(at, length);
	return 0;
}

int CHttpParser::OnMessageComplete()
{
	m_complete = true;
	return 0;
}

UrlParser::UrlParser(const Buffer& url)
{
	m_url = url;
	m_protocol = "";
	m_host = "";
	m_uri = "";
	m_port = 80;
	m_KeyValues.clear();
}

int UrlParser::Parser()
{
	//������������Э�顢�����Ͷ˿ڡ�URI
	//����Э�鲿��
	const char* pos = m_url;//"https://cn.bing.com:80/"
	const char* target = strstr(pos, "://");
	if (target == NULL) return -1;
	m_protocol = Buffer(pos, target);//"https"

	//���������Ͷ˿�
	pos = target+3;//posλ�����ƣ����ҵ�һ��/  pos="cn.bing.com:80/"
	target = strchr(pos, '/');
	if (target == NULL) {
		if ((m_protocol.size() + 3) >= m_url.size())	return -2;//URL��ȫ
		m_host = pos;//����û�з�б�ܣ�ֱ��Э��://����ȫ����host
		return 0;
	}
	Buffer value = Buffer(pos, target);//value ="cn.bing.com:80"
	if (value.size() == 0) return -3;//host�����ǿ�
	target = strchr(value, ':');
	if (target != NULL) {
		m_host = Buffer(value, target);//m_host = "cn.bing.com"
		m_port = atoi(Buffer(target + 1, (char*)value + value.size()));//m_port = aoit((char*)"80")
	}
	else {//�Ҳ����˿ں�
		m_host = value;
	}
	//����URI
	pos = strchr(pos, '/');// pos="/search?EID=MBSC&form=BGGCDF&pc=U710&q=URL+URI"
	target = strchr(pos, '?');//"https://cn.bing.com/search?EID=MBSC&form=BGGCDF&pc=U710&q=URL+URI"���ҵ�?֮��Ĳ���
	if (target == NULL) {//û�ҵ�����ʾʣ�µ�ȫ��m_uri
		m_uri = pos+1;//ʡ��"/"
		return 0;
	}
	else {
		m_uri = Buffer(pos+1, target);//uri="search"
		//����key��value
		pos = target + 1;//"key1=value1&..."
		const char* t = NULL;//ָ���������ָ�룬���ܶ�ָ��ָ�����ֵ�����޸ģ������Ըı�ָ�����
		do {
			target = strchr(pos, '&');
			if (target == NULL) {//"a=b"��""
				t = strchr(pos, '=');
				if (t == NULL) return -4;//?��û������
				m_KeyValues[Buffer(pos, t)] = Buffer(t + 1);
				break;//�˳�do-whileѭ��
			}
			else {//"a=b&c=d"
				Buffer kv(pos, target);//kv��ʾ "a=b"
				t = strchr(kv, '=');
				if (t == NULL) return -5;
				m_KeyValues[Buffer(kv, t)] = Buffer(t + 1, (char*)kv + kv.size());
				pos = target + 1;
			}
		} while (pos != NULL);
	}

	return 0;
}

Buffer UrlParser::operator[](const Buffer& name)
{
	auto it = m_KeyValues.find(name);
	if (it == m_KeyValues.end())
		return Buffer();
	return it->second;
}

void UrlParser::SetUrl(const Buffer& url)
{
	m_url = url;
	m_protocol = "";
	m_host = "";
	m_uri = "";
	m_port = 80;
	m_KeyValues.clear();
}
