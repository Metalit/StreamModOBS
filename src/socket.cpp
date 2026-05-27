#include "socket.hpp"
#include "main.hpp"

#define CATCH_ASIO                                     \
      catch (std::exception const& e) {                \
        log_error("%s", e.what());            \
    } catch (websocketpp::lib::error_code e) {         \
        log_error("%s", e.message().data());  \
    } catch (...) {                                    \
        log_error("unknown websocket error"); \
    }

socket_manager::socket_manager() {
    log_entry();
    socket_client.init_asio();
    socket_client.start_perpetual();

    socket_client.set_open_handler(bind_mem(&socket_manager::on_open, this));
    socket_client.set_fail_handler(bind_mem(&socket_manager::on_fail, this));
    socket_client.set_message_handler(bind_mem(&socket_manager::on_message, this));
    socket_client.set_close_handler(bind_mem(&socket_manager::on_close, this));

    run_thread = std::thread(bind_mem(&client::run, &socket_client));
}

socket_manager::~socket_manager() {
    log_entry();
    socket_client.stop_perpetual();
    disconnect();
    // force closes connections, in case something was in progress
    socket_client.stop();
    run_thread.join();
}

void socket_manager::connect(std::string const& address) {
    disconnect();
    try {
        log_info("websocket connecting: ws://%s", address.c_str());
        websocketpp::lib::error_code error_code;
        client::connection_ptr connection = socket_client.get_connection("ws://" + address, error_code);
        if (error_code)
            throw error_code;
        connecting_handle = connection->get_handle();
        socket_client.connect(connection);
    }
    CATCH_ASIO;
}

bool socket_manager::connecting() {
    return connecting_handle.has_value();
}

bool socket_manager::connected() {
    return handle.has_value();
}

void socket_manager::disconnect() {
    try {
        log_info("websocket disconnecting: %s", handle.has_value() ? "true" : "false");
        if (handle)
            socket_client.close(*handle, websocketpp::close::status::going_away, "");
        connecting_handle.reset();
        handle.reset();
    }
    CATCH_ASIO;
}

void socket_manager::set_message_callback(std::function<void(std::string const&)> callback) {
    message_callback = std::move(callback);
}

void socket_manager::set_close_callback(std::function<void()> callback) {
    close_callback = std::move(callback);
}

void socket_manager::send(std::string const& message) {
    if (handle)
        socket_client.send(*handle, message, websocketpp::frame::opcode::value::BINARY);
}

bool socket_manager::connecting_to(websocketpp::connection_hdl hdl) {
    if (!connecting_handle)
        return false;
    // basically just owner_equals
    return !hdl.owner_before(*connecting_handle) && !connecting_handle->owner_before(hdl);
}

void socket_manager::on_open(websocketpp::connection_hdl hdl) {
    if (!connecting_to(hdl)) {
        try {
            socket_client.close(hdl, websocketpp::close::status::going_away, "");
        }
        CATCH_ASIO;
        return;
    }
    log_info("websocket opened");
    connecting_handle.reset();
    if (handle)
        disconnect();
    handle = hdl;
}

void socket_manager::on_fail(websocketpp::connection_hdl hdl) {
    if (!connecting_to(hdl))
        return;
    log_warning("websocket failed");
    connecting_handle.reset();
}

void socket_manager::on_message(websocketpp::connection_hdl hdl, client::message_ptr msg) {
    if (message_callback)
        message_callback(msg->get_payload());
}

void socket_manager::on_close(websocketpp::connection_hdl hdl) {
    log_info("websocket closed");
    if (close_callback)
        close_callback();
    handle.reset();
}
