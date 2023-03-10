#pragma once
#include "Public.h"
#include <map>
#include <list>
#include <memory>
#include <vector>
class _Table_;
using PTable = std::shared_ptr<_Table_>;//C++ ����ָ��ײ��ǲ������ü����ķ�ʽʵ�ֵġ��򵥵���⣬����ָ����������ڴ�ռ��ͬʱ����Ϊ���䱸һ������ֵ����ʼֵΪ 1����ÿ�����¶���ʹ�ô˶��ڴ�ʱ��������ֵ +1����֮��ÿ��ʹ�ô˶��ڴ�Ķ����ͷ�ʱ��������ֵ�� 1�����ѿռ��Ӧ������ֵΪ 0 ʱ�������������ж���ʹ�������öѿռ�ͻᱻ�ͷŵ���
using KeyValue = std::map<Buffer, Buffer>;//����
using Result = std::list<PTable>;

class _Field_;
using PField = std::shared_ptr<_Field_>;
using FieldArray = std::vector<PField>;
using FieldMap = std::map<Buffer, PField>;
//shared_ptr:��ֹ�ڴ���Դ������������һ����������������

class CDatabaseClient
{
public:
	CDatabaseClient() {}
	virtual ~CDatabaseClient(){}

	CDatabaseClient(const CDatabaseClient&) = delete;
	CDatabaseClient& operator=(const CDatabaseClient&) = delete;

	//���麯����ʹ�ø����Ϊ�����࣬�����������ʵ��������������д�������
	//����
	virtual int Connect(const KeyValue& args) = 0;//�ü�ֵ�Է�ʽ���������������������ݿ⣨��ֹ�������ݿ�Ҫ�������һ�µ�����Ҫ������ص�����/�鷳��
	//ִ��
	virtual int Exec(const Buffer& sql) = 0;
	//�������ִ��
	virtual int Exec(const Buffer& sql, Result& result, const _Table_& table) = 0;//��ѯ�Ͳ�ѯ���(�������Ӧ���Ƕ�����ŷ���ORM��Ҫ��)
	//������
	virtual int StartTransaction() = 0;
	//�����ύ
	virtual int CommitTransaction() = 0;
	//����ع�
	virtual int RollbackTransaction() = 0;
	//�ر�����
	virtual int Close() = 0;
	//�ж��Ƿ�������״̬
	virtual bool isConnected() = 0;
};

//��Ļ���ʵ��
class _Table_ 
{
public:
	_Table_(){}
	virtual ~_Table_() {}
	//�������ڴ����ñ��SQL���
	virtual Buffer Create() = 0;//��Ϊ��Ҫ�����ַ����������Ǻ����ڲ������ľֲ��ַ��������ں���ִ����Ϻ��Զ�ִ�������������Ҳ������⣩
	
	virtual Buffer Drop() = 0;
	//��
	virtual Buffer Insert(const _Table_& values) = 0;
	//ɾ
	virtual Buffer Delete(const _Table_& values, const Buffer& condition = "") = 0;
	//��
	virtual Buffer Modify(const _Table_& values, const Buffer& condition = "") = 0;//TODO:�����Ż�
	//��
	virtual Buffer Query(const Buffer& condition = "") = 0;
	//����һ�����ڱ�Ķ���(�ñ�ĸ���)�����ڲ�ѯʹ��
	virtual PTable Copy() const= 0;
	//��������е�ʹ��״̬
	virtual void ClearFieldCondition() = 0;
	//��ȡ���ȫ��
	virtual operator const Buffer() const = 0;//ע�ⷵ��Database+'.'+Name

	//��������DB����
	Buffer Database;
	//������
	Buffer Name;
	//�еĶ��壨����
	FieldArray FieldDefine;
	//�е�ӳ������ڼ�ֵƥ�䣬���ѯ�Ƿ����ĳ�У�
	FieldMap Fields;
};

enum {
	SQL_INSERT = 1,//������
	SQL_MODIFY = 2,//�޸���
	SQL_CONDITION = 4,//��ѯ������
};

enum {
	NONE = 0,
	NOT_NULL = 1,//�ǿ�
	DEFAULT = 2,//Ĭ��ֵ
	UNIQUE = 4,//Ψһ
	PRIMARY_KEY = 8,//����
	CHECK = 16,//Լ��
	AUTOINCREMENT = 32//�Զ�����
};

using SqlType = enum {
	TYPE_NULL = 0,
	TYPE_BOOL = 1,
	TYPE_INT = 2,
	TYPE_DATETIME = 4,
	TYPE_REAL = 8,//С������
	TYPE_VARCHAR = 16,
	TYPE_TEXT = 32,
	TYPE_BLOB = 64
};

class _Field_
{
public:
	_Field_() {}
	virtual ~_Field_() {}

	_Field_(const _Field_& field) {
		Name = field.Name;
		Type = field.Type;
		Size = field.Size;
		Attr = field.Attr;
		Default = field.Default;
		Check = field.Check;
	}
	virtual _Field_& operator=(const _Field_& field) {//Ϊʲô�������������ö����Ƕ����������಻�����ɶ���������ö���ֻ�ܷ����������������ã�
		if (this != &field) {
			Name = field.Name;
			Type = field.Type;
			Size = field.Size;
			Attr = field.Attr;
			Default = field.Default;
			Check = field.Check;
		}
		return *this;
	}
	//�����ַ���ת�ɶ�Ӧ��ֵ
	virtual Buffer Create() = 0;
	//���ɵ��ںŵı��ʽ����Ҫ����where���ʹ�ã�
	virtual void LoadFromStr(const Buffer& str) = 0;
	//����תΪSql�����ַ�����ʽ
	virtual Buffer toEqualExp() const = 0;
	//��ȡ�е�ȫ�����ڲ�ͬ���ݿ�ȫ����ʽ��һ����
	virtual Buffer toSqlStr() const = 0;
	//�������ض���
	virtual operator const Buffer() const = 0;

	//����
	Buffer Name;
	Buffer Type;
	Buffer Size;
	unsigned Attr;//���ԣ�Ψһ�� ����
	Buffer Default;//Ĭ��ֵ����ͬ���ݿ�����в��죬mysql��������Ĭ��ֵ��
	Buffer Check;//Լ����������ͬ���ݿ����Ҳ�в��죩
	//��������
	unsigned Condition;//������ʲô��
	union {
		bool Bool;
		int Int;
		double Double;
		Buffer* Str;
	}Value;//�洢�еĲ�ѯ���
	int nType;//����Value������洢������
};

//#name��ʾ��nameתΪ�ַ���
#define DECLARE_TABLE_CLASS(name, base) class name:public base{ \
public: \
virtual PTable Copy() const{return PTable(new name(*this));} \
name():base(){Name=#name;

#define DECLARE_FIELD(baseclass,ntype,name,type,size, attr,default_,check) \
{ \
PField field(new baseclass(ntype, name, type, size, attr, default_, check)); \
FieldDefine.push_back(field); \
Fields[name] = field;}

#define DECLARE_TABLE_CLASS_END() }};