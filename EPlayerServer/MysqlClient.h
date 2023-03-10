#pragma once
#include "Public.h"
#include "DatabaseHelper.h"
#include <mysql/mysql.h>

class CMysqlClient :public CDatabaseClient
{
public:
	CMysqlClient() {
		bzero(&m_db, sizeof(m_db));
		m_bInit = false;
	}
	virtual ~CMysqlClient() {
		Close();
	}

	CMysqlClient(const CMysqlClient&) = delete;
	CMysqlClient& operator=(const CMysqlClient&) = delete;

	//���麯����ʹ�ø����Ϊ�����࣬�����������ʵ��������������д�������
	//����
	virtual int Connect(const KeyValue& args);//�ü�ֵ�Է�ʽ���������������������ݿ⣨��ֹ�������ݿ�Ҫ�������һ�µ�����Ҫ������ص�����/�鷳��
	//ִ��
	virtual int Exec(const Buffer& sql);
	//�������ִ��
	virtual int Exec(const Buffer& sql, Result& result, const _Table_& table);//��ѯ�Ͳ�ѯ���(�������Ӧ���Ƕ�����ŷ���ORM��Ҫ��)
	//������
	virtual int StartTransaction();
	//�����ύ
	virtual int CommitTransaction();
	//����ع�
	virtual int RollbackTransaction();
	//�ر�����
	virtual int Close();
	//�ж��Ƿ�������״̬ true��ʾ�����У�false��ʾδ����
	virtual bool isConnected();
private:
	MYSQL m_db;
	bool m_bInit;//Ĭ��false ��ʾδ��ʼ�� ��ʼ�������true����ʾ������

	//class ExecParam {
	//public:
	//	ExecParam(CMysqlClient* obj, Result& result, const _Table_& table)
	//		:obj(obj), result(result), table(table) {}
	//	CMysqlClient* obj;
	//	Result& result;
	//	const _Table_& table;
	//};
};

class _mysql_table_ :public _Table_
{
public:
	_mysql_table_() :_Table_() {}
	virtual ~_mysql_table_() {}

	_mysql_table_(const _mysql_table_& table);
	_mysql_table_& operator=(const _mysql_table_& table);

	//�������ڴ����ñ��SQL���
	virtual Buffer Create();//��Ϊ��Ҫ�����ַ����������Ǻ����ڲ������ľֲ��ַ��������ں���ִ����Ϻ��Զ�ִ�������������Ҳ������⣩
	virtual Buffer Drop();
	//��
	virtual Buffer Insert(const _Table_& values);
	//ɾ
	virtual Buffer Delete(const _Table_& values, const Buffer& condition = "");
	//��
	virtual Buffer Modify(const _Table_& values, const Buffer& condition = "");//value����������ģ�
	//��
	virtual Buffer Query(const Buffer& condition = "");//����ѯ������
	//����һ�����ڱ�Ķ���(�ñ�ĸ���)�����ڲ�ѯʹ��
	virtual PTable Copy() const;
	virtual void ClearFieldCondition();
	//��ȡ���ȫ��
	virtual operator const Buffer() const;
};


class _mysql_field_ :public _Field_
{
public:

	_mysql_field_();
	_mysql_field_(
		int ntype,
		const Buffer& name,
		const Buffer& type,
		const Buffer& size,
		unsigned attr,
		const Buffer& default_,
		const Buffer& check
	);
	_mysql_field_(const _mysql_field_& field);
	virtual ~_mysql_field_();
	//�����ַ���ת�ɶ�Ӧ��ֵ
	virtual Buffer Create();
	//�и�ֵ
	virtual void LoadFromStr(const Buffer& str);
	//����תΪ��ʽ�ַ�����ʽ������=��ֵ��
	virtual Buffer toEqualExp() const;
	//����תΪSql�����ַ�����ʽ(��ֵ)
	virtual Buffer toSqlStr() const;
	//�������ض���,��ȡ����
	virtual operator const Buffer() const;
private:
	Buffer Str2Hex(const Buffer& data) const;
};