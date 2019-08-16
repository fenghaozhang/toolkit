#!/usr/bin/env python

# There is no build rule to automatically generate bind_calback.py.
# If there are any changes, you need to mannually run:
#
# python bind_callback.py > bind_callback.h

def write_prologue():
    print """// WARNING: This file is generated by bind_callback.py.  DO NOT edit here.
// GLOBAL_NOLINT

#ifndef _SRC_BASE_BIND_CALLBACK_H
#define _SRC_BASE_BIND_CALLBACK_H

#include <google/protobuf/stubs/common.h>  // for Closure
#include "src/base/call_traits.h"

namespace bind_callback_details
{

const uint64_t BIND_CALLBACK_MAGIC        = 0x4c4c4143444e4942ULL;  // "BINDCALL"
const uint64_t BIND_CALLBACK_FREED_OBJECT = 0x4c4c414345455246ULL;  // "FREECALL"

#ifndef NDEBUG
void CheckMagic(uint64_t magic);
#endif  // NDEBUG

}  // namespace bind_callback_details

// Here defines a family of BindCallback templates.
//
// These templates implement interface google::protobuf::Closure.  Unlike
// NewCallback, BindMethod does no memory allocation or free at all, which
// is useful in latency-sensitive scenarios.  A user owns the full  life-
// cycle.  It is recommended to use it with a memory pool.
//
// Example 1: Bind to member methods of class.
//
//     class Clazz
//     {
//     pubilc:
//          void M0();
//          void M1(int);
//          void M2(int, int);
//     };
//
//     Clazz obj;
//
//     BindCallbackM0<Clazz> m0;
//     m0.Bind(&obj, &Clazz::M0);
//     m0.Run();  // calls obj.M0()
//
//     BindCallbackM1<Clazz, int> m1;
//     m1.Bind(&obj, &Clazz::M1, 10);
//     m1.Run();  // calls obj.M1(10);
//
//     BindCallbackM2<Clazz, int, int> m2;
//     m2.Bind(&obj, &Clazz::M2, 10, 20);
//     m2.Run();  // calls obj.M2(10, 20);
//
// Example 2: Bind to global functions.
//
//     void F0();
//     void F1(int);
//     void F2(int, int);
//
//     BindCallbackF0 f0;
//     f0.Bind(&F0);
//     b0.Run();  // calls F0()
//
//     BindCallbackF1<int> f1;
//     f1.Bind(&F1, 10);
//     f1.Run();  // calls F1(10)
//
//     BindCallbackF2<int, int> f2;
//     f2.Bind(&F2, 10, 20);
//     f2.Run();  // calls F2(10, 20)
//
// Example 3: Callback with arguments
//
//     class Clazz
//     {
//     pubilc:
//          void M1R2(int arg0, const std::string& result0, int result1);
//     };
//
//     Clazz obj;
//     BindCallbackM1R2<Clazz,
//         int /* arg0 */,
//         std::string /* result0 */,
//         int /* result1 */> callback;
//     callback.Bind(&obj, &Clazz::M1R2, 100 /* arg0 */);
//     callback.SetResult0(200);
//     callback.SetResult1("hello");
//     callback.Run();  // calls Clazz::M1R2(100, 200, "hello")
"""
    return

def write_template_params(params):
    print 'template <%s>' % ', '.join(['typename %s' % r for r in params])
    return

def write_callback_p(r):
    write_template_params(['Result%d' % i for i in xrange(r)])
    class_name = 'BindCallbackR%d' % r
    print 'class %s : public google::protobuf::Closure' % class_name
    print '{'
    print 'public:'
    print '    %s() : %s {}' % (class_name, ', '.join(['mResult%d(Result%d())' % (i, i) for i in xrange(r)]))
    for i in xrange(r):
        print '    typename call_traits<Result%d>::param_type GetResult%d() const { return mResult%d; }' % (i, i, i)
        print '    void SetResult%d(typename call_traits<Result%d>::param_type r) { mResult%d = r; }' % (i, i, i)
        print '    Result%d* MutableResult%d() { return &mResult%d; }' % (i, i, i)
        print
    print 'protected:'
    for i in xrange(r):
        print '    Result%d mResult%d;' % (i, i)
    print '};'
    print
    return

def write_callback_mp(m, r):
    write_template_params(['Class'] + ['Arg%d' % i for i in xrange(m)] + ['Result%d' % i for i in xrange(r)])
    if r == 0:
        class_name = 'BindCallbackM%d' % m
        ext_class_name = 'google::protobuf::Closure'
    else:
        class_name = 'BindCallbackM%dR%d' % (m, r)
        ext_class_name = 'BindCallbackR%d<%s>' % (r, ', '.join(['Result%d' % i for i in xrange(r)]))
    print 'class %s : public %s' % (class_name, ext_class_name)
    print '{'
    print 'public:'
    print '    typedef void (Class::*Method)(%s);' % ', '.join(
            ['typename call_traits<Arg%d>::param_type' % i for i in xrange(m)] +
            ['typename call_traits<Result%d>::param_type' % i for i in xrange(r)])
    print
    if m == 0:
        print '    %s() : mMagic(bind_callback_details::BIND_CALLBACK_MAGIC), mObject(NULL), mMethod(NULL) {}' % class_name
    else:
        print '    %s() : mMagic(bind_callback_details::BIND_CALLBACK_MAGIC), mObject(NULL), mMethod(NULL), %s {}' % (class_name, ', '.join(['mArg%d(Arg%d())' % (i, i) for i in xrange(m)]))
    print
    print '    ~%s()' % class_name
    print '    {'
    print '#ifndef NDEBUG'
    print '         bind_callback_details::CheckMagic(mMagic);'
    print '#endif  // NDEBUG'
    print '         mMagic = bind_callback_details::BIND_CALLBACK_FREED_OBJECT;'
    print '    }'
    print
    if m == 0:
        print '    void Bind(Class* object, Method method)'
    else:
        print '    void Bind(Class* object, Method method, %s)' % ', '.join(
                ['typename call_traits<Arg%d>::param_type arg%d' % (i, i) for i in xrange(m)])
    print '    {'
    print '#ifndef NDEBUG'
    print '        bind_callback_details::CheckMagic(mMagic);'
    print '#endif  // NDEBUG'
    print '        mObject = object;'
    print '        mMethod = method;'
    for i in xrange(m):
        print '        mArg%d = arg%d;' % (i, i)
    print '    }'
    print
    print '    /* override */ void Run()'
    print '    {'
    print '#ifndef NDEBUG'
    print '        bind_callback_details::CheckMagic(mMagic);'
    print '#endif  // NDEBUG'
    print '        (mObject->*mMethod)(%s);' % ', '.join(
            ['mArg%d' % i for i in xrange(m)] + ['this->mResult%d' % i for i in xrange(r)])
    print '    }'
    print
    print 'private:'
    print '    uint64_t mMagic;'
    print '    Class*   mObject;'
    print '    Method   mMethod;'
    for i in xrange(m):
        print '    Arg%d mArg%d;' % (i, i)
    print '};'
    print
    return

def write_callback_fp(f, r):
    if f > 0 or r > 0:
        write_template_params(['Arg%d' % i for i in xrange(f)] + ['Result%d' % i for i in xrange(r)])
    if r == 0:
        class_name = 'BindCallbackF%d' % f
        ext_class_name = 'google::protobuf::Closure'
    else:
        class_name = 'BindCallbackF%dR%d' % (f, r)
        ext_class_name = 'BindCallbackR%d<%s>' % (r, ', '.join(['Result%d' % i for i in xrange(r)]))
    print 'class %s : public %s' % (class_name, ext_class_name)
    print '{'
    print 'public:'
    print '    typedef void (*Function)(%s);' % ', '.join(
            ['typename call_traits<Arg%d>::param_type' % i for i in xrange(f)] +
            ['typename call_traits<Result%d>::param_type' % i for i in xrange(r)])
    print
    if f == 0:
        print '    %s() : mFunction(NULL) {}' % class_name
    else:
        print '    %s() : mFunction(NULL), %s {}' % (class_name, ', '.join(['mArg%d(Arg%d())' % (i, i) for i in xrange(f)]))
    print
    if f == 0:
        print '    void Bind(Function function)'
    else:
        print '    void Bind(Function function, %s)' % ', '.join(
                ['typename call_traits<Arg%d>::param_type arg%d' % (i, i) for i in xrange(f)])
    print '    {'
    print '        mFunction = function;'
    for i in xrange(f):
        print '        mArg%d = arg%d;' % (i, i)
    print '    }'
    print
    print '    /* override */ void Run()'
    print '    {'
    print '        mFunction(%s);' % ', '.join(
            ['mArg%d' % i for i in xrange(f)] + ['this->mResult%d' % i for i in xrange(r)])
    print '    }'
    print
    print 'private:'
    print '    Function mFunction;'
    for i in xrange(f):
        print '    Arg%d mArg%d;' % (i, i)
    print '};'
    print
    return

def write_epilogue():
    print
    print '#endif  // _SRC_BASE_BIND_CALLBACK_H'
    return

def main():
    write_prologue()
    for r in xrange(1, 4):
        write_callback_p(r)
    for m in xrange(0, 4):
        for r in xrange(0, 4):
            write_callback_mp(m, r)
    for f in xrange(0, 4):
        for r in xrange(0, 4):
            write_callback_fp(f, r)
    write_epilogue()

if __name__ == '__main__':
    main()

# vim: set ts=4 sw=2 et