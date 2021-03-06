/*
 * Promise2
 *
 * Copyright (c) 2015 "0of" Magnus
 * Licensed under the MIT license.
 * https://github.com/0of/Promise2/blob/master/LICENSE
 */
#ifndef NESTING_PROMISE_INTERNALS_H
#define NESTING_PROMISE_INTERNALS_H

#include "PromiseInternalsBase.h"

namespace Promise2 {
	namespace Details {
		// nesting promise PromiseNodeInternal
    template<typename ReturnType, typename ArgType, typename IsTask = std::false_type>
    class NestingPromiseNodeInternal : public PromiseNodeInternalBase<ReturnType, ArgType, std::false_type> {
      using Base = PromiseNodeInternalBase<ReturnType, ArgType, std::false_type>;

    private:
      std::function<Promise<ReturnType>(ArgType)> _onFulfill;

    public:
      NestingPromiseNodeInternal(std::function<Promise<ReturnType>(ArgType)>&& onFulfill, 
                  OnRejectFunction<ReturnType>&& onReject,
                  const std::shared_ptr<ThreadContext>& context)
        : Base(std::move(onReject), context)
        , _onFulfill{ std::move(onFulfill) }
      {}

    public:
      virtual void run() override {
       	std::call_once(Base::_called, [this]() {
          Promise<ReturnType> wrapped;

          try {
            wrapped = std::move(_onFulfill(std::forward<ReturnType>(Base::PreviousRetrievable::get())));
          } catch (...) {
            wrapped = std::move(Promise<ReturnType>::Rejected(std::current_exception()));
          }

          wrapped.internal()->chainNext(Base::_forward);
      	});
      }
    };  

    template<typename ReturnType, typename IsTask>
    class NestingPromiseNodeInternal<ReturnType, void, IsTask> : public PromiseNodeInternalBase<ReturnType, void, IsTask> {
      using Base = PromiseNodeInternalBase<ReturnType, void, IsTask>;

    private:
      std::function<Promise<ReturnType>()> _onFulfill;

    public:
      NestingPromiseNodeInternal(std::function<Promise<ReturnType>()>&& onFulfill, 
                  OnRejectFunction<ReturnType>&& onReject,
                  const std::shared_ptr<ThreadContext>& context)
        : Base(std::move(onReject), context)
        , _onFulfill{ std::move(onFulfill) }
      {}

    public:
      virtual void run() override {
       	std::call_once(Base::_called, [this]() {
          Promise<ReturnType> wrapped;

          try {
            Base::get();
            wrapped = std::move(_onFulfill());
          } catch (...) {
            wrapped = std::move(Promise<ReturnType>::Rejected(std::current_exception()));
          }

          wrapped.internal()->chainNext(Base::_forward);
      	});
      }
    };  
	} // Details
}

#endif // NESTING_PROMISE_INTERNALS_H
