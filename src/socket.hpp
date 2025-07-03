#pragma once

#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>

#include <functional>
#include <optional>
#include <string>
#include <thread>

class socket_manager {
   public:
    using client = websocketpp::client<websocketpp::config::asio_client>;

    socket_manager();
    ~socket_manager();

    void connect(std::string const& address);
    bool connecting();
    bool connected();
    void disconnect();

    void set_message_callback(std::function<void(std::string const&)> callback);
    void set_close_callback(std::function<void()> callback);
    void send(std::string const& message);

   private:
    client socket_client;
    std::optional<websocketpp::connection_hdl> connecting_handle;
    std::optional<websocketpp::connection_hdl> handle;
    std::thread run_thread;

    std::function<void(std::string const&)> message_callback;
    std::function<void()> close_callback;

    bool connecting_to(websocketpp::connection_hdl hdl);

    void on_open(websocketpp::connection_hdl hdl);
    void on_fail(websocketpp::connection_hdl hdl);
    void on_message(websocketpp::connection_hdl hdl, client::message_ptr msg);
    void on_close(websocketpp::connection_hdl hdl);
};
