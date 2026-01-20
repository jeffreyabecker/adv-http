#pragma once
#include "./Defines.h"
#include <memory>
#include <map>
#include <any>
#include <type_traits>

namespace HTTPServer
{

    template <typename Self>
    class ServiceManager
    {
    protected:
        mutable std::map<String, std::any> items_;

    public:
        std::map<String, std::any> &items() const
        {
            return items_;
        }

        bool hasService(const String &serviceName) const
        {
            return items_.find(serviceName) != items_.end();
        }
        Self &use(std::function<void(Self &)> setupFunc)
        {
            setupFunc(static_cast<Self &>(*this));
            return static_cast<Self &>(*this);
        }

        template <typename T>
        T *getService(const String &serviceName) const
        {
            auto it = items_.find(serviceName);
            if (it != items_.end())
            {
                if (auto ptr = std::any_cast<T>(&it->second))
                {
                    return ptr;
                }
            }
            return nullptr;
        }
    };

    template <typename TServiceManager, typename TSelf>
    class ServiceManagerService
    {
    protected:

        void add(TServiceManager &server, const String &serviceName, std::any serviceInstance)
        {
            server.items()[serviceName] = serviceInstance;
        }
        template <typename TServiceInstance>
        void addOnce(TServiceManager &server, const String &serviceName, TServiceInstance serviceInstance)
        {
            if (!server.hasService(serviceName))
            {
                add(server, serviceName, std::forward<TServiceInstance>(serviceInstance));
            }
        }
        template <typename TServiceInstance>
        void add(TServiceManager &server, const String &serviceName, TServiceInstance serviceInstance)
        {
            server.items()[serviceName] = std::make_any<TServiceInstance>(serviceInstance);
        }
        // template <typename TServiceInstance>
        // virtual void addOnce(TServiceManager &server, const String &serviceName, TServiceInstance &&serviceInstance)
        // {
        //     if (!server.hasService(serviceName))
        //     {
        //         server.items()[serviceName] = std::make_any<TServiceInstance>(std::forward<TServiceInstance>(serviceInstance));
        //     }
        // }

        

    public:
        ServiceManagerService() {}
        virtual ~ServiceManagerService() = default;
        // void operator()(TServiceManager &server)
        // {
        //     if (!server.hasService(TSelf::NAME))
        //     {
        //         init(server);
        //     }
        // }
    };

}