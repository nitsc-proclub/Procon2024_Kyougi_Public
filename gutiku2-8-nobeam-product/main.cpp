#include "solver.cpp"
#include "../include/source/kyougi_app.cpp"

#include <iostream>
#include <boost/asio.hpp>
#include <string>
#include <fstream>
#include <cstdlib>

using boost::asio::ip::tcp;

class Solver_Interface {
public:
    Solver_Interface(string host,const unsigned short receive_port,const unsigned short send_port,std::unique_ptr<ISolver> solver) :
        host_(std::move(host)),
        receive_port_(receive_port),
        send_port_(send_port),
        receive_buffer_(),
        solver_(std::move(solver))
        {
            solver_->init();
        }

    void start() {
        receive_problem(); //受信ループ 問題を受信したら終了

        std::cout << "start solving\n";
        solver_->solve();//解答 解き終わったら終了
        std::cout << "finish solving\n";

        send_answer(solver_->get_answer());//解答を送信
        save_answer(solver_->get_answer());
    }

    void receive_problem() {
        boost::system::error_code error;

        // TCPソケット作成とバインド
        tcp::acceptor acceptor(io_context_, tcp::endpoint(tcp::v4(), receive_port_));
        tcp::socket socket(io_context_);

        std::cout << "start wait accept...\n";

        // クライアントからの接続を待機
        acceptor.accept(socket);

        std::cout << "start receive problem...\n";

        // 問題文の受信
        std::string problem;
        while (true) {
            std::size_t len = socket.read_some(boost::asio::buffer(receive_buffer_), error);

            if (error == boost::asio::error::eof) {
                std::cout << "End of transmission detected.\n";
                solver_->set_problem(problem);
                socket.close();
                break;
            }
            problem += string(receive_buffer_.data(), len);
        }
    }

    void send_answer(const std::string& answer) {
        std::cout << "start sending\n";
        size_t total_bytes_sent = 0;
        const size_t message_size = answer.size();

        // TCPソケット作成し、サーバに接続
        tcp::socket socket(io_context_);
        socket.connect(tcp::endpoint(boost::asio::ip::address::from_string(host_), send_port_));

        // メッセージの送信（"PROBLEM" の部分を送信）
        boost::asio::write(socket, boost::asio::buffer(answer), boost::asio::transfer_all());

        std::cout << "finish sending!\n";
    }


    static void save_answer(const std::string& answer){
        //jsonファイルに出力
        std::string filename = "answer.json"; //ファイル名
        std::ofstream outfile(filename);

        // ファイルが正常に開けたか確認
        if (outfile.is_open()) {
            outfile << answer;
            outfile.close();
            std::cout << "save:" << filename << std::endl;
        } else {
            std::cerr << "error:" << filename << std::endl;
        }
    };

private:
    const int max_packet_size_ = 1024;
    boost::asio::io_context io_context_;

    string host_;
    const unsigned short send_port_;
    const unsigned short receive_port_;
    
    std::array<char, 1024> receive_buffer_;
    std::unique_ptr<ISolver> solver_;
};

int main(int argc, char *argv[]){ //host receive_port send_port
    if(argc<4){
        std::cout << "Please arguments host receive_port send_port\n";
        return 1;
    }
    string host = argv[1];
    int receive_port = atoi(argv[2]);
    int send_port = atoi(argv[3]);

    // 受信ポートを指定
    Solver_Interface receiver(host,receive_port,send_port,(std::move(std::make_unique<Solver>())));
    receiver.start();
    return 0;
}