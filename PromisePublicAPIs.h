/*
* Promise2
*
* Copyright (c) 2015 "0of" Magnus
* Licensed under the MIT license.
* https://github.com/0of/Promise2/blob/master/LICENSE
*/

#ifndef PROMISE_PUBLIC_APIS_H
#define PROMISE_PUBLIC_APIS_H

#include <functional>
#include <memory>
#include <exception>
#include <stdexcept>
 
#include "PromiseConfig.h"
#include "declfn.h"

namespace Promise2 {
  // declarations
  namespace Details {
    template<typename T> class PromiseNode;
    template<typename T> class Forward;
    template<typename T> using DeferPromiseCore = std::unique_ptr<Forward<T>>;
  } // Details

  template<typename T> class Promise;
  template<typename T> using SharedPromiseNode = std::shared_ptr<Details::PromiseNode<T>>;
  // !

  //
  // @class ThreadContext
  //
  class ThreadContext {
  public:
    virtual ~ThreadContext() = default;

  public:
    virtual void scheduleToRun(std::function<void()>&& task) = 0;
  };

#if DEFERRED_PROMISE
  //
  // @class PromiseDefer
  //
  template<typename T>
  class PromiseDefer {
  private:
    Details::DeferPromiseCore<T> _core;

  public:
    PromiseDefer(Details::DeferPromiseCore<T>&& core);

    PromiseDefer(PromiseDefer<T>&&) = default;
    ~PromiseDefer() = default;

  public:
    template<typename X> void setResult(X&& r);

    void setException(std::exception_ptr e);

  private:
    PromiseDefer(const PromiseDefer<T>&) = delete;
    PromiseDefer& operator = (const PromiseDefer<T>&) = delete;
  };

  template<>
  class PromiseDefer<void> {
  private:
    Details::DeferPromiseCore<void> _core;

  public:
    PromiseDefer(Details::DeferPromiseCore<void>&& core);

    PromiseDefer(PromiseDefer<void>&&) = default;
    ~PromiseDefer() = default;

  public:
    void setResult();

    void setException(std::exception_ptr e);

  private:
    PromiseDefer(const PromiseDefer<void>&) = delete;
    PromiseDefer<void>& operator = (const PromiseDefer<void>&) = delete;
  };
#endif // DEFERRED_PROMISE

  template<typename T>
  class PromiseSpawner {
  public:
    // constructor with task and running context
    static Promise<T> New(std::function<T(void)>&& task, ThreadContext* &&context);

#if DEFERRED_PROMISE
    // constructor with deferred task
    static Promise<T> New(std::function<void(PromiseDefer<T>&&)>&& task, ThreadContext* &&context);
#endif // DEFERRED_PROMISE

#if NESTING_PROMISE
    // constructor with nesting promise task
    static Promise<T> New(std::function<Promise<T>()>&& task, ThreadContext* &&context);
#endif // NESTING_PROMISE
  };

  template<typename T>
  class PromiseThenable {
  public:
    template<typename NextT>
    static Promise<NextT> Then(SharedPromiseNode<T>& node,
                               std::function<NextT(T)>&& onFulfill, 
                               std::function<void(std::exception_ptr)>&& onReject, 
                               ThreadContext* &&context);

#if DEFERRED_PROMISE
    template<typename NextT>
    static Promise<NextT> ThenDeferred(SharedPromiseNode<T>& node,
                               std::function<void(PromiseDefer<NextT>&&, T)>&& onFulfill, 
                               std::function<void(std::exception_ptr)>&& onReject, 
                               ThreadContext* &&context);
#endif // DEFERRED_PROMISE

#if NESTING_PROMISE
    template<typename NextT>
    static Promise<NextT> ThenNesting(SharedPromiseNode<T>& node, 
                               std::function<Promise<NextT>(T)>&& onFulfill,
                               std::function<void(std::exception_ptr)>&& onReject, 
                               ThreadContext* &&context);
#endif // NESTING_PROMISE
  };

  template<>
  class PromiseThenable<void> {
  public:
    template<typename NextT>
    static Promise<NextT> Then(SharedPromiseNode<void>& node,
                               std::function<NextT(void)>&& onFulfill, 
                               std::function<void(std::exception_ptr)>&& onReject, 
                               ThreadContext* &&context);

#if DEFERRED_PROMISE
    template<typename NextT>
    static Promise<NextT> ThenDeferred(SharedPromiseNode<void>& node,
                               std::function<void(PromiseDefer<NextT>&&)>&& onFulfill, 
                               std::function<void(std::exception_ptr)>&& onReject, 
                               ThreadContext* &&context);
#endif // DEFERRED_PROMISE

#if NESTING_PROMISE
    template<typename NextT>
    static Promise<NextT> ThenNesting(SharedPromiseNode<void>& node, 
                               std::function<Promise<NextT>(void)>&& onFulfill,
                               std::function<void(std::exception_ptr)>&& onReject, 
                               ThreadContext* &&context);
#endif // NESTING_PROMISE

  };

#define THEN_INLINE_IMP(type) \
    { if (!isValid()) throw std::logic_error("invalid promise"); \
      return Thenable::Then##type(_node, std::move(onFulfill), std::move(onReject), std::move(context)); }

  //
  // @class Promise
  //
  template<typename T>
  class Promise : public PromiseSpawner<T> {
    template<typename Type> friend class PromiseThenable;

  private:    
    using Thenable = PromiseThenable<T>;

    friend class PromiseSpawner<T>;

  public:
    template<typename ArgType>
    static Promise<T> Resolved(ArgType&& arg);
    static Promise<T> Rejected(std::exception_ptr e);

  private:
    std::shared_ptr<Details::PromiseNode<T>> _node;

  public:
    // empty constructor
    Promise() = default;

    Promise(Promise<T>&& promise) = default;

    Promise<T>& operator = (Promise<T>&& promise) = default;

  public:
    template<typename OnFulfill>
    auto then(OnFulfill&& onFulfill,
                        std::function<void(std::exception_ptr)>&& onReject, 
                        ThreadContext* &&context) {
      auto onFulfillFn = declfn(onFulfill){ std::move(onFulfill) };

      static_assert(!std::is_same<decltype(onFulfillFn), std::false_type>::value, "you need to provide a callable");
      return then(std::move(onFulfillFn), std::move(onReject), std::move(context));
    }

  public:
    template<typename NextT>
    Promise<NextT> then(std::function<NextT(T)>&& onFulfill, 
                        std::function<void(std::exception_ptr)>&& onReject, 
                        ThreadContext* &&context) THEN_INLINE_IMP()

#if DEFERRED_PROMISE
    template<typename NextT>
    Promise<NextT> then(std::function<void(PromiseDefer<NextT>&&, T)>&& onFulfill,
                        std::function<void(std::exception_ptr)>&& onReject, 
                        ThreadContext* &&context) THEN_INLINE_IMP(Deferred)
#endif // DEFERRED_PROMISE

#if NESTING_PROMISE
    template<typename NextT>
    Promise<NextT> then(std::function<Promise<NextT>(T)>&& onFulfill,
                        std::function<void(std::exception_ptr)>&& onReject, 
                        ThreadContext* &&context) THEN_INLINE_IMP(Nesting)
#endif // NESTING_PROMISE
    
  public:
    bool isValid() const {
      return !!_node;
    }

  public:
    bool isFulfilled() const; 
    bool isRejected() const;

  private:
    Promise(const Promise<T>& ) = delete;
    Promise& operator = (const Promise<T>& ) = delete;
  };

    template<>
    class Promise<void> : public PromiseSpawner<void> {
    template<typename Type> friend class PromiseThenable;

  private:
    friend class PromiseSpawner<void>;

    using Thenable = PromiseThenable<void>;
    using SelfType = Promise<void>;

  public:
    static SelfType Resolved();
    static SelfType Rejected(std::exception_ptr e);

  private:
    std::shared_ptr<Details::PromiseNode<void>> _node; 

  public:
    // empty constructor
    Promise() = default;

    Promise(SelfType&& promise) = default;

    SelfType& operator = (SelfType&& promise) = default;

  public:
    template<typename OnFulfill>
    auto then(OnFulfill&& onFulfill,
              std::function<void(std::exception_ptr)>&& onReject, 
              ThreadContext* &&context) {
      auto onFulfillFn = declfn(onFulfill){ std::move(onFulfill) };

      static_assert(!std::is_same<decltype(onFulfillFn), std::false_type>::value, "you need to provide a callable");
      return then(std::move(onFulfillFn), std::move(onReject), std::move(context));
    }

    template<typename NextT>
    Promise<NextT> then(std::function<NextT(void)>&& onFulfill, 
                        std::function<void(std::exception_ptr)>&& onReject, 
                        ThreadContext* &&context) THEN_INLINE_IMP()

#if DEFERRED_PROMISE
    template<typename NextT>
    Promise<NextT> then(std::function<void(PromiseDefer<NextT>&&)>&& onFulfill,
                        std::function<void(std::exception_ptr)>&& onReject, 
                        ThreadContext* &&context) THEN_INLINE_IMP(Deferred)
#endif // DEFERRED_PROMISE

#if NESTING_PROMISE
    template<typename NextT>
    Promise<NextT> then(std::function<Promise<NextT>()>&& onFulfill,
                        std::function<void(std::exception_ptr)>&& onReject, 
                        ThreadContext* &&context) THEN_INLINE_IMP(Nesting)
#endif // NESTING_PROMISE
    
  public:
    bool isValid() const {
      return !!_node;
    }

    bool isFulfilled() const;
    bool isRejected() const;

  private:
    Promise(const SelfType& ) = delete;
    Promise& operator = (const SelfType& ) = delete;
  };
}
 
#endif // PROMISE_PUBLIC_APIS_H
 