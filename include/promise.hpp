#pragma once
#include <functional>
#include <memory>
#include <utility>
#include <vector>

#include "promise_traits.hpp"

class IPromiseState
{
public:
    virtual ~IPromiseState() = default;
};

enum class State
{
    Pending,
    Fullfilled
};
using Handler = std::pair<std::shared_ptr<IPromiseState>, std::function<void()>>;

template <class T>
class PromiseState : public IPromiseState
{
public:
    using ValueType = std::conditional_t<std::is_void_v<T>, undefined, T>;

    State _state;
    ValueType _value;
    std::vector<Handler> _handlers;

public:
    PromiseState() : _value{}, _state{State::Pending}, _handlers{} {}

    void resolve(ValueType &&value = {})
    {
        if (_state == State::Pending)
        {

            _value = value;
            _state = State::Fullfilled;
            for (const auto &handler : _handlers)
            {
                handler.second();
            }
            _handlers.clear();
        }
    }

    friend class Promise<T>;
};

template <class T>
class Promise
{
private:
    using ValueType = typename PromiseState<T>::ValueType;
    std::shared_ptr<PromiseState<T>> _state_ptr;

public:
    template <class Executor>
    Promise(Executor executor) : _state_ptr{std::make_shared<PromiseState<T>>()}
    {
        executor([sp = _state_ptr.get()](ValueType &&val) {
            sp->resolve(std::forward<ValueType>(val));
        });
    }

    Promise(const Promise &other) = delete;
    Promise &operator=(const Promise &other) = delete;

    Promise(Promise &&other) = default;
    Promise &operator=(Promise &&other) = default;

    std::shared_ptr<PromiseState<T>> get_state_pointer() const { return _state_ptr; }

    template <class OnFullfilled>
    auto then(OnFullfilled onFullfilled)
    {
        using PValueType = std::invoke_result_t<OnFullfilled, ValueType>;

        if constexpr (is_promise_v<PValueType>)
        {
            using UnwrappedPValueType = unwrap_promise_t<PValueType>;
            if (_state_ptr->_state == State::Fullfilled)
            {
                return onFullfilled(_state_ptr->_value);
            }
            else
            {
                Handler h;
                Promise<UnwrappedPValueType> promise{[sp = _state_ptr.get(), &h, &onFullfilled](const auto &resolve) {
                    h.second = [sp, onFullfilled = std::move(onFullfilled), resolve] {
                        onFullfilled(sp->_value).then([&resolve](UnwrappedPValueType &&value) {
                            resolve(std::forward<UnwrappedPValueType>(value));
                        });
                    };
                }};
                h.first = promise._state_ptr;
                _state_ptr->_handlers.push_back(std::move(h));
                return promise;
            }
        }
        else
        {

            if (_state_ptr->_state == State::Fullfilled)
            {
                return Promise<PValueType>([this, &onFullfilled](const auto &resolve) {
                    resolve(return_undefined_if_void(onFullfilled, std::move(_state_ptr->_value)));
                });
            }
            else
            {
                Handler h;
                Promise<PValueType> p([sp = _state_ptr.get(), &h, &onFullfilled](const auto &resolve) {
                    h.second = [sp, onFullfilled = std::move(onFullfilled), resolve] {
                        resolve(return_undefined_if_void(onFullfilled, std::move(sp->_value)));
                    };
                });
                h.first = p._state_ptr;
                _state_ptr->_handlers.push_back(std::move(h));
                return p;
            }
        }
    }

    template <class>
    friend class Promise;
};