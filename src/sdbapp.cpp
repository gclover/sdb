

extern "C" {

#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>

#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <event2/event.h>

}

#include <Poco/Thread.h>
#include <Poco/Runnable.h>
#include <Poco/Util/ServerApplication.h>
#include <Poco/Util/Option.h>
#include <Poco/Util/OptionSet.h>
#include <Poco/Util/HelpFormatter.h>
#include <iostream>


using Poco::Thread;
using Poco::Runnable;
using Poco::Util::ServerApplication;
using Poco::Util::Application;
using Poco::Util::Option;
using Poco::Util::OptionSet;
using Poco::Util::HelpFormatter;

class SdbServer : public Runnable {
public:
	SdbServer(int port)
	: port_(port) {
		eventbase_ =  event_base_new();
		if (!eventbase_) {
			std::cerr << "Could not initialize libevent!" << std::endl;
			return;
		}
	}

	~SdbServer() {
		evconnlistener_free(listener_);
		event_base_free(eventbase_);
	}

	void run() {

	
		struct sockaddr_in sin = this->create_addr(port_);

		listener_ = evconnlistener_new_bind(eventbase_, &SdbServer::onListener, (void *)eventbase_,
	    						LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_FREE, -1,
	    						(struct sockaddr*)&sin, sizeof(sin));
		if (!listener_) {
			std::cerr << "Could not create a listener!" << std::endl;
			return;
		}

		event_base_dispatch(eventbase_);
		std::cout << "run exit" << std::endl;
	}

	void stop() {
		struct timeval delay = { 0, 0 };
		event_base_loopexit(eventbase_, &delay);
		std::cout << "stop exit" << std::endl;
	}

private:

	static void onListener(struct evconnlistener *listener, evutil_socket_t fd, 
				struct sockaddr *sa, int socklen, void *user_data) {
		struct event_base *base = (struct event_base*)user_data;
		struct bufferevent *bev;

		const char MESSAGE[] = "Hello, World!\n";
		bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
		if (!bev) {
			std::cerr << "Error constructing bufferevent!" << std::endl;
			event_base_loopbreak(base);
			return;
		}
		bufferevent_setcb(bev, NULL, &SdbServer::onWrite, &SdbServer::onEvent, NULL);
		bufferevent_enable(bev, EV_WRITE);
		bufferevent_disable(bev, EV_READ);

//		std::cout << "write message" << std::endl;
		bufferevent_write(bev, MESSAGE, strlen(MESSAGE));
	}

	static void onWrite(struct bufferevent *bev, void *user_data) {
		struct evbuffer *output = bufferevent_get_output(bev);
		if (evbuffer_get_length(output) == 0) {
//			std::cerr << "flushed answer" << std::endl;
			bufferevent_free(bev);
		}
	}

	static void onEvent(struct bufferevent *bev, short events, void *user_data) {
		if (events & BEV_EVENT_EOF) 
			std::cout << "Connection closed." << std::endl;
		else if (events & BEV_EVENT_ERROR) 
			std::cout << "Got an error on the connection" << std::endl;
		/* None of the other events can happen here, since we haven't enabled
		 * timeouts */
		bufferevent_free(bev);
	}

	struct sockaddr_in create_addr(int port) {
		struct sockaddr_in sin;
		memset(&sin, 0, sizeof(sin));
        	sin.sin_family = AF_INET;
        	sin.sin_port = htons(port_);
		return sin;
	}

	struct evconnlistener *listener_;
	struct event_base *eventbase_ ;
	int port_;
};



class SdbApplication : public ServerApplication {
public:
	SdbApplication()
	: helpRequested_(false) {}
	
	~SdbApplication() {}

protected:
	void initialize(Application& self) {
		loadConfiguration(); 
		ServerApplication::initialize(self);
	}
		
	void uninitialize() {
		ServerApplication::uninitialize();
	}

	void defineOptions(OptionSet& options) {
		ServerApplication::defineOptions(options);
		
		options.addOption(
			Option("help", "h", "display help information on command line arguments")
				.required(false)
				.repeatable(false));
	}

	void handleOption(const std::string& name, const std::string& value) {
		ServerApplication::handleOption(name, value);

		if (name == "help")
			helpRequested_ = true;
	}

	void displayHelp() {
		HelpFormatter helpFormatter(options());
		helpFormatter.setCommand(commandName());
		helpFormatter.setUsage("OPTIONS");
		helpFormatter.setHeader("Sdb server.");
		helpFormatter.format(std::cout);
	}

	int main(const std::vector<std::string>& args) {
		if (helpRequested_) {
			displayHelp();
		} else {
			int port = (int)config().getInt("sdbserver.port", 9980);

			SdbServer srv(port);
			Thread srvThread; 
			srvThread.start(srv);
			srvThread.join();
			waitForTerminationRequest();
			srv.stop();
			srvThread.join();
		}
		return Application::EXIT_OK;
	}
	
private:
	bool helpRequested_;
};


int main(int argc, char** argv) {
	SdbApplication app;
	return app.run(argc, argv);
}


