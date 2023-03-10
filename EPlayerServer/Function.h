#ifndef FUNCTIOND
#define FUNCTIOND

#include <unistd.h>//ע��������Ҫ�������Ŀ����Ŀ¼���ϵ�ǰĿ¼"."�����ҵ����ͷ�ļ�
#include <sys/types.h>//���Ͷ���
#include <functional>
//����һ������:�麯�����Ժ�ģ�庯�����Բ���ͬʱ����,
//����һ��ģ����������麯��
class CSocketBase;
class Buffer;
class CFunctionBase //ͨ����������ֹCProcess��m_func����ȾΪģ�庯��
{
public:
    virtual ~CFunctionBase() {}
    virtual int operator()() { return -1; };
    virtual int operator()(CSocketBase*) { return -1; };//����������ֻ��ҵ���߼�������,����Ϊֱ��return-1,ʹ�ú�������಻ʵ������Ҳ����
    virtual int operator()(CSocketBase*, const Buffer&) { return -1; };//����������ֻ��ҵ���߼�������,����Ϊֱ��return-1,ʹ�ú�������಻ʵ������Ҳ����
};

template<typename _FUNCTION_, typename... _ARGS_>
class CFunction : public CFunctionBase
{
public:
    CFunction(_FUNCTION_ func, _ARGS_... args)//�����funcһ�������洫���ĺ������ڵ�ַ,���������func���붼�Ǵ���&func(ȡ��ַ),��Ϊ��֪�����������Ӧ�Ķ���,���븴�Ƶ����Ա����û�е���(��֪����Ӧ�ĸ�����,������)
        :m_binder(std::forward<_FUNCTION_>(func), std::forward<_ARGS_>(args)...)//forward��ʾԭ��ת����()...��ʾ����չ�� std::forward<T>(u)������������T �� u����TΪ��ֵ��������ʱ��u����ת��ΪT���͵���ֵ������u����ת��ΪT������ֵ����˶���std::forward��Ϊ����ʹ����ֵ���ò����ĺ���ģ���н������������ת�����⡣
        //bind�����Ա����ʱ����һ��������ʾ����ĳ�Ա������ָ��(������ʽ��ָ��&,��Ϊ���������Ὣ����ĳ�Ա������ʽת���ɺ���ָ��)���ڶ���������ʾ����ĵ�ַ(ʹ�ö����Ա������ָ��ʱ������Ҫ֪����ָ�������ĸ�������˵ڶ�������Ϊ����ĵ�ַ &base)��
    {}//����m_binderǰ���ð�Ŵ����ʼ��
    virtual ~CFunction() {}
    virtual int operator()() {
        return m_binder();//����ִ��ʱ(*m_func)(); <=>  ִ����m_binder()���ɵĿɵ��ö���;
    }// return  std::_Bindres_helper<int, _FUNCTION_, _ARGS_...>::type
  //  virtual int operator()(CSocketBase* pClient) {
  //      return m_binder(pClient);
  //  }// std::_Bindres_helper<int, _FUNCTION_, _ARGS_...>::type
  //  virtual int operator()(CSocketBase* pClient, const Buffer& data) {
		//return m_binder(pClient, data);
  //  }
    //std::bind���ڸ�һ���ɵ��ö���󶨲������ɵ��ö�������������󣨷º�����������ָ�롢�������á���Ա����ָ������ݳ�Աָ�롣
    typename std::_Bindres_helper<int, _FUNCTION_, _ARGS_...>::type m_binder;//�൱�ڷ���
};

#endif