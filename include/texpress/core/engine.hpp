#pragma once

#include <algorithm>
#include <memory>
#include <type_traits>
#include <vector>

#include <texpress/core/system.hpp>


namespace texpress
{
    class  engine
    {
    public:
        engine() = default;
        engine(const engine& that) = delete;
        engine(engine&& temp) = delete;
        virtual ~engine() = default;
        engine& operator=(const engine& that) = delete;
        engine& operator=(engine&& temp) = delete;

        template<typename system_type, typename... system_arguments>
        system_type* add_system(system_arguments&&... arguments)
        {
            static_assert(std::is_base_of<texpress::system, system_type>::value, "The type does not inherit from system.");
            systems_.push_back(std::make_unique<system_type>(arguments...));
            const auto system = systems_.back().get();
            return static_cast<system_type*>(system);
        }
        template<typename system_type>
        system_type* system()
        {
            static_assert(std::is_base_of<texpress::system, system_type>::value, "The type does not inherit from system.");
            auto iterator = std::find_if(systems_.begin(), systems_.end(), system_type_predicate<system_type>);
            if (iterator == systems_.end())
                return nullptr;
            return static_cast<system_type*>(iterator->get());
        }
        template<typename system_type>
        void         remove_system()
        {
            static_assert(std::is_base_of<texpress::system, system_type>::value, "The type does not inherit from system.");
            auto iterator = std::remove_if(systems_.begin(), systems_.end(), system_type_predicate<system_type>);
            if (iterator == systems_.end())
                return;
            systems_.erase(iterator, systems_.end());
        }

    protected:
        template<typename system_type>
        static bool  system_type_predicate(const std::unique_ptr<texpress::system>& iteratee)
        {
            return typeid(system_type) == typeid(*iteratee.get());
        }

        std::vector<std::unique_ptr<texpress::system>> systems_;
    };
}
