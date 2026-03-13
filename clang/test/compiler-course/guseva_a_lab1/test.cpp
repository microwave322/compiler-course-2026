// RUN: %clang_cc1 -load %llvmshlibdir/OverrideVisitor_Guseva_Alena_FIIT2_ClangAST%pluginext -plugin OverrideCheckPlugin %s -fsyntax-only 2>&1 | FileCheck %s

class Base1 {
public:
    virtual void foo();
};

class Derived1 : public Base1 {
public:
// CHECK: warning: virtual method is not marked 'override'
    void foo(); 
};

class Base2 {
public:
    virtual void foo();
};

class Derived2 : public Base2 {
public:
// CHECK-NOT: warning: virtual method is not marked 'override'
    void foo() override;
};

class Base3 {
public:
    void foo();
};

class Derived3 : public Base3 {
public:
// CHECK-NOT: warning: virtual method is not marked 'override'
    void foo();
};

class Base4 {
public:
    virtual void foo();
};

class Mid : public Base4 {};

class Derived4 : public Mid {
public:
// CHECK: warning: virtual method is not marked 'override'
    void foo();
};

class Base5 {
public:
    virtual void foo();
    virtual void bar();
};

class Derived5 : public Base5 {
public:
// CHECK: warning: virtual method is not marked 'override'
    void foo() { }
// CHECK-NOT: warning: virtual method is not marked 'override'
    void bar() override { }
};
