#include "../src/HttpClient.h"
#include "ppc-tools/src/common/TransTools.h"
#include <boost/asio.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <iostream>
#include <vector>

using tcp = boost::asio::ip::tcp;
namespace http = boost::beast::http;
namespace beast = boost::beast;
namespace net = boost::asio;
using namespace ppc::http;
using namespace ppc::tools;

#define PORT 1234
#define PATH "/test"

// Function to handle incoming HTTP requests
void handle_request(http::request<http::string_body>& req, http::response<http::string_body>& res)
{
    if (req.method() == http::verb::post && req.target() == PATH)
    {
        res.result(http::status::ok);
        res.set(http::field::content_type, "text/plain");

        // Access the binary data received in the request body
        std::string& bodyData = req.body();

        std::cout << "Server receives body: " << bodyData.data() << std::endl;

        // Save the binary data to a local file
        std::ofstream outfile("http_out", std::ios::binary);
        if (outfile.is_open())
        {
            outfile.write(reinterpret_cast<const char*>(bodyData.data()), bodyData.size());
            outfile.close();
            res.body() = "Binary data received and saved successfully.";
        }
        else
        {
            res.result(http::status::internal_server_error);
            res.body() = "Failed to save binary data.";
        }
    }
    else
    {
        res.result(http::status::not_found);
        res.set(http::field::content_type, "text/plain");
        res.body() = "Not Found";
    }
}

void newServer()
{
    try
    {
        // Create a Boost ASIO io_context
        net::io_context ioc;

        // Create a TCP acceptor to listen on port
        tcp::acceptor acceptor(ioc, {tcp::v4(), PORT});

        while (true)
        {
            // Wait for an incoming connection
            tcp::socket socket(ioc);
            acceptor.accept(socket);

            // Read the request
            beast::flat_buffer buffer;
            http::request<http::string_body> request;
            http::read(socket, buffer, request);

            // Prepare the response
            http::response<http::string_body> response;
            handle_request(request, response);

            // Send the response
            http::write(socket, response);
            socket.shutdown(tcp::socket::shutdown_send);
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Server Error: " << e.what() << std::endl;
    }
}

void testClient()
{
    try
    {
        net::io_context io_context;
        std::thread io_thread([&io_context]() { io_context.run(); });

        HttpClient client(io_context, "127.0.0.1", PORT);

        std::map<std::string, std::string> headers = {{"Content-Type", "application/octet-stream"}};

        bcos::bytes body = {'a', 'b', 'c', 'd'};
        auto response = client.post(PATH, headers, body);
        std::cout << "Client receives body: " << std::string(response.begin(), response.end())
                  << std::endl;

        body = {'1', '2', '3', '4'};
        response.clear();
        response = client.post(PATH, headers, body);
        std::cout << "Client receives body: " << std::string(response.begin(), response.end())
                  << std::endl;

        uint32_t a = 37;
        auto body1 = std::make_shared<bcos::bytes>();
        encodeUnsignedNum(body1, a);
        response.clear();
        response = client.post(PATH, headers, *body1);
        std::cout << "Client receives body: " << std::string(response.begin(), response.end())
                  << std::endl;

        io_thread.join();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Client Error: " << e.what() << std::endl;
    }
}

int main(int argc, char const* argv[])
{
    std::thread myThread(newServer);
    std::cout << "Server started!" << std::endl;

    testClient();

    // Wait for the new thread to finish (optional)
    myThread.join();
    return 0;
}
