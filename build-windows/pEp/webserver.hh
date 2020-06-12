#pragma once

#include <string>
#include <unordered_map>
#include <boost/asio/ip/tcp.hpp>
#include <boost/regex.hpp>
#include <boost/beast/http.hpp>


namespace pEp {
    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace net = boost::asio;
    using tcp = boost::asio::ip::tcp;

    // class Webserver
    //
    // when an URL handler is present it is called for each matching URL
    // otherwise this server is searching for static files in doc_root
    // only registered file types and no subdirectories are served for 
    // static files
    //
    // to deliver 404 return nullptr from handler
    //
    // this server is supporting GET for static files and POST for handlers

    class Webserver {
        public:

            typedef boost::regex url_t;
            typedef http::request< http::string_body > request;
            typedef http::response< http::string_body > response;
            typedef std::function< response(boost::cmatch, const request&) > handler_t;

        private: 
            struct Handling {
                boost::regex regex;
                handler_t handler;
            };

            net::io_context _ioc;
            tcp::acceptor _acceptor;
            std::string _doc_root;
            std::unordered_map< std::string, Handling > _urls;
            handler_t _generic_handler;
            bool _running;
            std::mutex _mtx;

        public:

            // if doc_root is empty, don't deliver arbitrary files.
            Webserver(net::ip::address addr, unsigned short port, const std::string& doc_root = "");

            Webserver(const Webserver&) = delete;
            Webserver& operator=(const Webserver&) = delete;
            virtual ~Webserver() = default;

            void add_url_handler   (const std::string& url_regex, handler_t handler);
            void remove_url_handler(const std::string& url_regex);

            // the generic handler will be called if the URL does not match any registered handlers
            void add_generic_url_handler   (handler_t handler);
            void remove_generic_url_handler();

            void run();
            void shutdown();

            static beast::string_view mime_type(beast::string_view path);

            static
            response create_status_response(const request& req, http::status status);

        protected:
            void deliver_status(tcp::socket *socket, const request& req, http::status status);
            void deliver_file  (tcp::socket *socket, const request& req);
            handler_t find_handler(const request& req, boost::cmatch& m);

            // called at the beginning of a connection thread. Do nothing by default.
            virtual void thread_init() {}

            // called at the beginning of a connection thread. Do nothing by default.
            virtual void thread_done() {}

            // is called by run(), in a separate thread.
            void do_session(tcp::socket *socket);
    };
};

