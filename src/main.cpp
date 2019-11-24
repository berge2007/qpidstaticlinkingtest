#include <proton/connection.hpp>
#include <proton/container.hpp>
#include <proton/delivery.hpp>
#include <proton/message.hpp>
#include <proton/messaging_handler.hpp>
#include <proton/receiver_options.hpp>
#include <proton/source_options.hpp>
#include <proton/tracker.hpp>
#include <iostream>
#include <vector>

#if __cplusplus < 201103L
#define OVERRIDE
#else
#define OVERRIDE override
#endif


#include <string>
#include <sstream>
#include <ostream>
#include <vector>
#include <stdexcept>

namespace example {
/** bad_option is thrown for option parsing errors */
struct bad_option : public std::runtime_error {
    bad_option(const std::string& s) : std::runtime_error(s) {}
};

/** Simple command-line option parser for example programs */
class options {
  public:

    options(int argc, char const * const * argv) : argc_(argc), argv_(argv), prog_(argv[0]), help_() {
        size_t slash = prog_.find_last_of("/\\");
        if (slash != std::string::npos)
            prog_ = prog_.substr(slash+1); // Extract prog name from path
        add_flag(help_, 'h', "help", "Print the help message");
    }

    ~options() {
        for (opts::iterator i = opts_.begin(); i != opts_.end(); ++i)
            delete *i;
    }

    /** Updates value when parse() is called if option is present with a value. */
    template<class T>
    void add_value(T& value, char short_name, const std::string& long_name, const std::string& description, const std::string var) {
        opts_.push_back(new option_value<T>(value, short_name, long_name, description, var));
    }

    /** Sets flag when parse() is called if option is present. */
    void add_flag(bool& flag, char short_name, const std::string& long_name, const std::string& description) {
        opts_.push_back(new option_flag(flag, short_name, long_name, description));
    }

    /** Parse the command line, return the index of the first non-option argument.
     *@throws bad_option if there is a parsing error or unknown option.
     */
    int parse() {
        int arg = 1;
        for (; arg < argc_ && argv_[arg][0] == '-'; ++arg) {
            opts::iterator i = opts_.begin();
            while (i != opts_.end() && !(*i)->parse(argc_, argv_, arg))
                ++i;
            if (i == opts_.end())
                throw bad_option(std::string("unknown option ") + argv_[arg]);
        }
        if (help_) throw bad_option("");
        return arg;
    }

    /** Print a usage message */
  friend std::ostream& operator<<(std::ostream& os, const options& op) {
      os << std::endl << "usage: " << op.prog_ << " [options]" << std::endl;
      os << std::endl << "options:" << std::endl;
      for (opts::const_iterator i = op.opts_.begin(); i < op.opts_.end(); ++i)
          os << **i << std::endl;
      return os;
  }

 private:
    class option {
      public:
        option(char s, const std::string& l, const std::string& d, const std::string v) :
            short_(std::string("-") + s), long_("--" + l), desc_(d), var_(v) {}
        virtual ~option() {}

        virtual bool parse(int argc, char const * const * argv, int &i) = 0;
        virtual void print_default(std::ostream&) const {}

      friend std::ostream& operator<<(std::ostream& os, const option& op) {
          os << "  " << op.short_;
          if (!op.var_.empty()) os << " " << op.var_;
          os << ", " << op.long_;
          if (!op.var_.empty()) os << "=" << op.var_;
          os << std::endl << "        " << op.desc_;
          op.print_default(os);
          return os;
      }

      protected:
        std::string short_, long_, desc_, var_;
    };

    template <class T>
    class option_value : public option {
      public:
        option_value(T& value, char s, const std::string& l, const std::string& d, const std::string& v) :
            option(s, l, d, v), value_(value) {}

        bool parse(int argc, char const * const * argv, int &i) {
            std::string arg(argv[i]);
            if (arg == short_ || arg == long_) {
                if (i < argc-1) {
                    set_value(arg, argv[++i]);
                    return true;
                } else {
                    throw bad_option("missing value for " + arg);
                }
            }
            if (arg.compare(0, long_.size(), long_) == 0 && arg[long_.size()] == '=' ) {
                set_value(long_, arg.substr(long_.size()+1));
                return true;
            }
            return false;
        }

        virtual void print_default(std::ostream& os) const { os << " (default " << value_ << ")"; }

        void set_value(const std::string& opt, const std::string& s) {
            std::istringstream is(s);
            is >> value_;
            if (is.fail() || is.bad())
                throw bad_option("bad value for " + opt + ": " + s);
        }

      private:
        T& value_;
    };

    class option_flag: public option {
      public:
        option_flag(bool& flag, const char s, const std::string& l, const std::string& d) :
            option(s, l, d, ""), flag_(flag)
        { flag_ = false; }

        bool parse(int /*argc*/, char const * const * argv, int &i) {
            if (argv[i] == short_ || argv[i] == long_) {
                flag_ = true;
                return true;
            } else {
                return false;
            }
        }

      private:
        bool &flag_;
    };

    typedef std::vector<option*> opts;

    int argc_;
    char const * const * argv_;
    std::string prog_;
    opts opts_;
    bool help_;
};
}

using proton::receiver_options;
using proton::source_options;

class client : public proton::messaging_handler {
  private:
    std::string url;
    std::vector<std::string> requests;
    proton::sender sender;
    proton::receiver receiver;
  public:
    client(const std::string &u, const std::vector<std::string>& r) : url(u), requests(r) {}
    void on_container_start(proton::container &c) OVERRIDE {
        sender = c.open_sender(url);
        // Create a receiver requesting a dynamically created queue
        // for the message source.
        receiver_options opts = receiver_options().source(source_options().dynamic(true));
        receiver = sender.connection().open_receiver("", opts);
    }
    void send_request() {
        proton::message req;
        req.body(requests.front());
        req.reply_to(receiver.source().address());
        sender.send(req);
    }
    void on_receiver_open(proton::receiver &) OVERRIDE {
        send_request();
    }
    void on_message(proton::delivery &d, proton::message &response) OVERRIDE {
        if (requests.empty()) return; // Spurious extra message!
        std::cout << requests.front() << " => " << response.body() << std::endl;
        requests.erase(requests.begin());
        if (!requests.empty()) {
            send_request();
        } else {
            d.connection().close();
        }
    }
};
int main(int argc, char **argv) {
    std::string url("127.0.0.1:5672/examples");
    example::options opts(argc, argv);
    opts.add_value(url, 'a', "address", "connect and send to URL", "URL");
    try {
        opts.parse();
        std::vector<std::string> requests;
        requests.push_back("Twas brillig, and the slithy toves");
        requests.push_back("Did gire and gymble in the wabe.");
        requests.push_back("All mimsy were the borogroves,");
        requests.push_back("And the mome raths outgrabe.");
        client c(url, requests);
        proton::container(c).run();
        return 0;
    } catch (const example::bad_option& e) {
        std::cout << opts << std::endl << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    return 1;
}

