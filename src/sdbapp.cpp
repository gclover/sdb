

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

#include "sdbserver.h"

namespace sdb {

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
			srv.start();
			waitForTerminationRequest();
			srv.stop();
		}
		return Application::EXIT_OK;
	}
	
private:
	bool helpRequested_;
};

}

int main(int argc, char** argv) {
	sdb::SdbApplication app;
	return app.run(argc, argv);
}


