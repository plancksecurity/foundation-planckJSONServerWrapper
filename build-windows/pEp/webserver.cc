// this file is derived from a boost::beast sample

#include <boost/beast/core.hpp>
#include <boost/filesystem.hpp>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

namespace fs = boost::filesystem;

#include "webserver.hh"

namespace pEp {
    Webserver::Webserver(net::ip::address addr, unsigned short port, const std::string& doc_root)
    : _ioc{1}
    , _acceptor{_ioc, {addr, port}, false}
    , _doc_root{doc_root}
    , _generic_handler{}
    , _port{port}
    , _running{false}
    { }

beast::string_view Webserver::mime_type(beast::string_view path)
{
    using beast::iequals;
    auto const ext = [&path]
    {
        auto const pos = path.rfind(".");
        if(pos == beast::string_view::npos)
            return beast::string_view{};
        return path.substr(pos);
    }();
    if(iequals(ext, ".htm"))  return "text/html; charset=utf-8";
    if(iequals(ext, ".html")) return "text/html; charset=utf-8";
    if(iequals(ext, ".css"))  return "text/css; charset=utf-8";
    if(iequals(ext, ".txt"))  return "text/plain; charset=utf-8";
    if(iequals(ext, ".js"))   return "application/javascript; charset=utf-8";
    if(iequals(ext, ".json")) return "application/json; charset=utf-8";
    if(iequals(ext, ".xml"))  return "application/xml; charset=utf-8";
    if(iequals(ext, ".png"))  return "image/png";
    if(iequals(ext, ".jpeg")) return "image/jpeg";
    if(iequals(ext, ".jpg"))  return "image/jpeg";
    if(iequals(ext, ".gif"))  return "image/gif";
    if(iequals(ext, ".ico"))  return "image/vnd.microsoft.icon";
    if(iequals(ext, ".tiff")) return "image/tiff";
    if(iequals(ext, ".tif"))  return "image/tiff";
    if(iequals(ext, ".svg"))  return "image/svg+xml; charset=utf-8";
    if(iequals(ext, ".svgz")) return "image/svg+xml";
    return "application/octet-stream";
}


void Webserver::add_url_handler(const std::string& url_regex, handler_t handler)
{
    std::lock_guard< std::mutex > lock(_mtx);
    _urls.emplace(url_regex, Handling{boost::regex(url_regex), handler});
}


void Webserver::remove_url_handler(const std::string& url_regex)
{
    std::lock_guard< std::mutex > lock(_mtx);
    _urls.erase(url_regex);
}


void Webserver::add_generic_url_handler(handler_t handler)
{
    _generic_handler = handler;
}


void Webserver::remove_generic_url_handler()
{
    _generic_handler = nullptr;
}


Webserver::response Webserver::create_status_response(const request& req, http::status status)
{
    http::response< http::string_body > res{status, req.version()};
    res.set(http::field::content_type, "text/html; charset=utf-8");
    res.keep_alive(req.keep_alive());
    std::stringstream s;
    s << "<html><body>" << int(status) << " " << status << "</body></html>";
    res.body() = s.str();
    res.prepare_payload();
    if (status != http::status::internal_server_error)
        res.keep_alive(req.keep_alive());

    return res;
}

Webserver* Webserver::probing_port_range(net::ip::address addr, unsigned short
        start, unsigned short end, unsigned short& port, const std::string&
        doc_root)
{
    pEp::Webserver *web = nullptr;

    for (port = start; port <= end; ++port) {
        try {
            web = new pEp::Webserver{addr, port, doc_root};
            break;
        }
        catch (boost::system::system_error& err) {
        }
    }

    return web;
}

void Webserver::deliver_status(tcp::socket *socket, const request& req, http::status status)
{
    const response res { create_status_response(req, status) };
    beast::error_code ec;
    http::write(*socket, res, ec);
}


void Webserver::deliver_file(tcp::socket *socket, const request& req)
{
    static boost::regex file{"/([\\w\\d]{1,100}\\.[\\w\\d]{1,4})"};
    boost::cmatch m;
    std::string d{req.target().data(), req.target().length()};
    if (boost::regex_match(d.c_str(), m, file)) {
        fs::path p{_doc_root};
        p /= std::string(m[1]);

        beast::error_code ec;
        http::file_body::value_type body;
        body.open(p.string().c_str(), beast::file_mode::scan, ec);
        if (ec == beast::errc::no_such_file_or_directory) {
            deliver_status(socket, req, http::status::not_found);
        }
        else if (ec) {
            deliver_status(socket, req, http::status::internal_server_error);
        }
        else {
            auto const size = body.size();
            http::response<http::file_body> res{
                    std::piecewise_construct,
                    std::make_tuple(std::move(body)),
                    std::make_tuple(http::status::ok, req.version())};
            res.set(http::field::content_type, mime_type(p.string().c_str()));
            res.content_length(size);
            res.keep_alive(req.keep_alive());
            http::write(*socket, res, ec);
        }
    }
    else {
        deliver_status(socket, req, http::status::not_found);
    }
}


Webserver::handler_t Webserver::find_handler(const request& req, boost::cmatch& m)
{
    std::lock_guard< std::mutex > lock(_mtx);

    for (auto it=_urls.begin(); it!=_urls.end(); ++it) {
        std::string d{req.target().data(), req.target().length()};
        if (boost::regex_match(d.c_str(), m, it->second.regex))
            return it->second.handler;
    }

    return _generic_handler; // might be empty std::function!
}


void Webserver::do_session(tcp::socket *socket)
{
    beast::error_code ec;
    beast::flat_buffer buffer;

    while (_running)
    {
        http::request_parser<http::string_body> parser;
        parser.body_limit((std::numeric_limits<std::int32_t>::max)());

        http::read(*socket, buffer, parser, ec);
        if (ec) {
            std::cerr << ec.message() << "\n";
            goto the_end;
        }

        http::request<http::string_body> req = parser.get();

        const auto method = req.method();
        switch (method)
        {
            case http::verb::post: // fall through
            case http::verb::get:
            {
                boost::cmatch m;
                Webserver::handler_t handler = find_handler(req, m);

                if (handler) {
                    try{
                        const Webserver::response res = handler(m, req);
                        http::write(*socket, res, ec);
                    }
                    catch(...){
                        deliver_status(socket, req, http::status::internal_server_error);
                    }
                }
                else {
                    if(method == http::verb::get && !_doc_root.empty())
                    {
                        deliver_file(socket, req);
                    }else{
                        deliver_status(socket, req, http::status::not_found);
                    }
                }
                break;
            }

            default:
                deliver_status(socket, req, http::status::method_not_allowed);
        };

        if (ec)
            break;
    }

the_end:
    socket->shutdown(tcp::socket::shutdown_send, ec);
    delete socket;
}


void Webserver::runner(Webserver *me)
{
    while (me->_running)
    {
        tcp::socket* socket = new tcp::socket{me->_ioc};
        me->_acceptor.accept(*socket);

        std::function< void() > tf = [=]()
            {
                me->thread_init();
                me->do_session(socket);
                me->thread_done();
            };

        std::thread{tf}.detach();
    }
}

void Webserver::run()
{
    _running = true;
    _runner = std::thread(runner, this);
}

void Webserver::shutdown()
{
    _running = false;
}

};

