#pragma once
#include "config.hpp"
#include "common/log.hpp"
#include "router.h"

namespace moon
{
    class log;
    class server;
    class router;
    class worker;

    class service
    {
    public:
        friend class worker;

        service() = default;

        service(const service&) = delete;

        service& operator=(const service&) = delete;

        virtual ~service(){}

        uint32_t id() const
        {
            return id_;
        }

        const std::string & name() const
        {
            return name_;
        }

        void set_name(const std::string & name)
        {
            name_ = name;
        }

        void set_server_context(server * s, router * r, worker * w)
        {
            server_ = s;
            router_ = r;
            worker_ = w;
        }

        server* get_server() const
        {
            return server_;
        }

        router* get_router() const
        {
            return router_;
        }

        worker* get_worker() const
        {
            return worker_;
        }

        bool unique() const
        {
            return unique_;
        }

        log* logger() const
        {
            return log_;
        }

        void logger(log * l)
        {
            log_ = l;
        }

        bool is_start() const
        {
            return start_;
        }

        bool ok() const
        {
            return ok_;
        }

        void ok(bool v)
        {
            ok_ = v;
        }

        bool profile() const
        {
            return profile_;
        }

        void profile(bool v)
        {
            profile_ = v;
        }

        int64_t cpu_time()
        {
            return cpu_time_;
        }

        template<typename Message>
        void handle_message(Message&& m)
        {
            int64_t cpu_start = 0;
            if (profile_)
            {
                cpu_start = time::microsecond();
            }

            try
            {
                dispatch(m.get());
                //redirect message
                if (m->receiver() != id() && m->receiver() != 0)
                {
                    MOON_ASSERT(!m->broadcast(), "can not redirect broadcast message");
                    if constexpr (std::is_rvalue_reference_v<decltype(m)>)
                    {
                        router_->send_message(std::forward<message_ptr_t>(m));
                    }
                }
            }
            catch (std::exception& e)
            {
                CONSOLE_ERROR(logger(), "service::handle_message exception: %s", e.what());
            }

            if (profile_)
            {
                cpu_time_ = time::microsecond() - cpu_start;
            }
        }

        void quit()
        {
            router_->remove_service(id_, 0, 0);
        }
    public:
        virtual bool init(std::string_view) = 0;

        virtual void start()
        {
            start_ = true;
        }

        virtual void dispatch(message* msg) = 0;

        virtual void on_timer(uint32_t, bool) {};

        virtual void exit()
        {
            quit();
        }

        virtual void destroy()
        {
            ok_ = false;
        }
    protected:
        void set_unique(bool v)
        {
            unique_ = v;
        }

        void set_id(uint32_t v)
        {
            id_ = v;
        }
    protected:
        bool start_ = false;
        bool ok_ = false;
        bool unique_ = false;
        bool profile_ = false;
        uint32_t id_ = 0;
        log* log_ = nullptr;
        server* server_ = nullptr;
        router* router_ = nullptr;
        worker* worker_ = nullptr;
        int64_t cpu_time_ = 0;
        std::string   name_;
    };
}

