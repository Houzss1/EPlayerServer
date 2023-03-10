#pragma once
#include "Public.h"
#include "DatabaseHelper.h"
#include "sqlite3/sqlite3.h"

class CSqlite3Client:public CDatabaseClient 
{
public:
	CSqlite3Client() { 
		m_db = NULL; 
		m_stmt = NULL; 
	}
	virtual ~CSqlite3Client() {
		Close();
	}

	CSqlite3Client(const CSqlite3Client&) = delete;
	CSqlite3Client& operator=(const CSqlite3Client&) = delete;

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
	sqlite3_stmt* m_stmt;
	sqlite3* m_db;

	static int ExecCallback(void* arg, int count, char** names, char** values);
	int ExecCallback(Result& result, const _Table_& table, int count, char** names, char** values);
	class ExecParam {
	public:
		ExecParam(CSqlite3Client* obj, Result& result, const _Table_& table) 
			:obj(obj),result(result),table(table){}
		CSqlite3Client* obj;
		Result& result;
		const _Table_& table;
	};
};

class _sqlite3_table_ :public _Table_
{
public:
	_sqlite3_table_() :_Table_() {}
	virtual ~_sqlite3_table_() {}

	_sqlite3_table_(const _sqlite3_table_& table);
	_sqlite3_table_& operator=(const _sqlite3_table_& table);

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
	virtual Buffer Query(const Buffer& condition = "");
	//����һ�����ڱ�Ķ���(�ñ�ĸ���)�����ڲ�ѯʹ��
	virtual PTable Copy() const;
	virtual void ClearFieldCondition();
	//��ȡ���ȫ��
	virtual operator const Buffer() const;
};


class _sqlite3_field_ :public _Field_
{
public:

	_sqlite3_field_();
	_sqlite3_field_(
		int ntype,
		const Buffer& name,
		const Buffer& type,
		const Buffer& size,
		unsigned attr,
		const Buffer& default_,
		const Buffer& check
	);
	_sqlite3_field_(const _sqlite3_field_& field);
	virtual ~_sqlite3_field_();
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