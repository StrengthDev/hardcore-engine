
#pragma once

template<typename... Args>
class UserPredicate {
public:
    using FnPtr = bool (*)(Args... args, void* user_data);

    UserPredicate(FnPtr predicate, void* user_data) : predicate(predicate), user_data(user_data) {}

    bool operator()(Args... args) const noexcept {
        if (this->predicate) {
            return predicate(args..., this->user_data);
        }

        return true;
    }

private:
    FnPtr predicate;
    void* user_data;
};
